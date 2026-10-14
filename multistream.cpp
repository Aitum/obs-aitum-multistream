#include "config-utils.hpp"
#include "multistream.hpp"
#include "obs-module.h"
#include "version.h"
#include <obs-frontend-api.h>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QGroupBox>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QUrl>
#include <util/config-file.h>
#include <util/platform.h>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>

extern "C" {
#include "file-updater.h"
}

OBS_DECLARE_MODULE()
OBS_MODULE_AUTHOR("Aitum");
OBS_MODULE_USE_DEFAULT_LOCALE("aitum-multistream", "en-US")

static MultistreamDock *multistream_dock = nullptr;

update_info_t *version_update_info = nullptr;

bool version_info_downloaded(void *param, struct file_download_data *file)
{
	UNUSED_PARAMETER(param);
	if (!file || !file->buffer.num) {
		return true;
	}

	// Copy the updater's buffer before dispatching to the UI thread.
	auto info = QString::fromUtf8((const char *)file->buffer.array);
	QMetaObject::invokeMethod(
		QCoreApplication::instance(),
		[info] {
			if (multistream_dock)
				QMetaObject::invokeMethod(multistream_dock, "ApiInfo", Qt::DirectConnection,
							  Q_ARG(QString, info));
		},
		Qt::QueuedConnection);

	if (version_update_info) {
		update_info_destroy(version_update_info);
		version_update_info = nullptr;
	}
	return true;
}

bool obs_module_load(void)
{
	blog(LOG_INFO, "[Aitum-Multistream] loaded version %s", PROJECT_VERSION);

	const auto main_window = static_cast<QMainWindow *>(obs_frontend_get_main_window());
	multistream_dock = new MultistreamDock(main_window);
	obs_frontend_add_dock_by_id("AitumMultistreamDock", obs_module_text("AitumMultistream"), multistream_dock);

	std::string url = "https://api.aitum.tv/plugin/multi";
	const char *pguid = config_get_string(obs_frontend_get_app_config(), "General", "InstallGUID");
	if (pguid) {
		url += "?uuid=";
		url += pguid;
	}

	version_update_info =
		update_info_create_single("[Aitum Multistream]", "OBS", url.c_str(), version_info_downloaded, nullptr);
	return true;
}

namespace {

struct DockCall {
	std::mutex mutex;
	std::condition_variable cv;
	bool finished = false;
	bool ok = false;
	// A timed-out call must not access the caller's response.
	bool cancelled = false;
};

static const std::chrono::milliseconds dock_call_timeout(2000);

// Resolve the dock on the UI thread; worker threads must not retain its pointer.
// obs-websocket consumes the response when its callback returns, so wait here.
// Unlike BlockingQueuedConnection, this can cancel calls not picked up at shutdown.
// The timeout does not bound work already running. UI-thread callers time out.
template<typename Fn> static bool run_on_dock(Fn fn)
{
	auto call = std::make_shared<DockCall>();
	QMetaObject::invokeMethod(
		QCoreApplication::instance(),
		[call, fn] {
			std::lock_guard<std::mutex> lock(call->mutex);
			if (call->cancelled)
				return;
			if (multistream_dock) {
				fn(multistream_dock);
				call->ok = true;
			}
			call->finished = true;
			call->cv.notify_all();
		},
		Qt::QueuedConnection);

	std::unique_lock<std::mutex> lock(call->mutex);
	if (!call->cv.wait_for(lock, dock_call_timeout, [&call] { return call->finished; })) {
		call->cancelled = true;
		return false;
	}
	return call->ok;
}

} // namespace

void MultistreamDock::vendor_emit_output_state(obs_output_t *output, bool active)
{
	if (!vendor)
		return;
	auto event = obs_data_create();
	// Keep upstream's event payload: the libobs output name, not the dock label.
	obs_data_set_string(event, "name", obs_output_get_name(output));
	obs_websocket_vendor_emit_event(vendor, active ? "output_started" : "output_stopped", event);
	obs_data_release(event);
}

void MultistreamDock::vendor_request_version(obs_data_t *request_data, obs_data_t *response_data, void *)
{
	UNUSED_PARAMETER(request_data);
	obs_data_set_string(response_data, "version", PROJECT_VERSION);
	obs_data_set_bool(response_data, "success", true);
}

void MultistreamDock::vendor_request_get_outputs(obs_data_t *request_data, obs_data_t *response_data, void *)
{
	UNUSED_PARAMETER(request_data);
	if (!run_on_dock([response_data](MultistreamDock *dock) { dock->FillOutputs(response_data); })) {
		obs_data_set_bool(response_data, "success", false);
		obs_data_set_string(response_data, "error", "dock_unavailable");
	}
}

void MultistreamDock::vendor_request_output_action(obs_data_t *request_data, obs_data_t *response_data, VendorOutputAction action)
{
	const char *name = obs_data_get_string(request_data, "name");
	if (!name || !*name) {
		obs_data_set_bool(response_data, "success", false);
		obs_data_set_string(response_data, "error", "Missing name");
		return;
	}
	QString qname = QString::fromUtf8(name);
	// The queued lambda may outlive this callback after a timeout.
	auto error = std::make_shared<const char *>(nullptr);
	bool answered = run_on_dock([qname, action, error](MultistreamDock *dock) {
		switch (action) {
		case VendorOutputAction::Start:
			*error = dock->RemoteStartOutput(qname);
			break;
		case VendorOutputAction::Stop:
			*error = dock->RemoteStopOutput(qname);
			break;
		}
	});
	if (!answered) {
		obs_data_set_bool(response_data, "success", false);
		obs_data_set_string(response_data, "error", "dock_unavailable");
		return;
	}
	obs_data_set_bool(response_data, "success", *error == nullptr);
	if (*error)
		obs_data_set_string(response_data, "error", *error);
}

void MultistreamDock::vendor_request_start_output(obs_data_t *rd, obs_data_t *res, void *)
{
	vendor_request_output_action(rd, res, VendorOutputAction::Start);
}
void MultistreamDock::vendor_request_stop_output(obs_data_t *rd, obs_data_t *res, void *)
{
	vendor_request_output_action(rd, res, VendorOutputAction::Stop);
}

void MultistreamDock::LoadWebsocket()
{
	if (vendor) {
		return;
	}
	vendor = obs_websocket_register_vendor("aitum-multistream");
	if (!vendor) {
		blog(LOG_ERROR, "[Aitum Multistream] Failed to register websocket vendor");
		return;
	}
	// Null userdata avoids retaining a dock pointer across teardown.
	obs_websocket_vendor_register_request(vendor, "version", vendor_request_version, nullptr);
	obs_websocket_vendor_register_request(vendor, "get_outputs", vendor_request_get_outputs, nullptr);
	obs_websocket_vendor_register_request(vendor, "start_output", vendor_request_start_output, nullptr);
	obs_websocket_vendor_register_request(vendor, "stop_output", vendor_request_stop_output, nullptr);
	blog(LOG_INFO, "[Aitum Multistream] registered obs-websocket vendor 'aitum-multistream'");
}

void obs_module_post_load()
{
	if (multistream_dock) {
		multistream_dock->LoadVerticalOutputs(true);
		multistream_dock->LoadWebsocket();
	}
}

void obs_module_unload()
{
	if (version_update_info) {
		update_info_destroy(version_update_info);
		version_update_info = nullptr;
	}
	// Qt may have deleted the dock already; its destructor clears the global.
	delete multistream_dock;
}

const char *obs_module_name(void)
{
	return obs_module_text("AitumMultistream");
}

// Hold a reference until the graphics-thread task completes.
static void StopOutput(obs_output_t *output)
{
	output = obs_output_get_ref(output);
	if (!output)
		return;
	obs_queue_task(
		OBS_TASK_GRAPHICS,
		[](void *param) {
			auto o = (obs_output_t *)param;
			obs_output_stop(o);
			obs_output_release(o);
		},
		output, false);
}

// Return only scheme://host[:port]; ingest URLs can contain credentials.
static QString SafeServerUrl(const char *server)
{
	if (!server || !*server)
		return QString();
	QString raw = QString::fromUtf8(server);
	QUrl url(raw);
	// Parse bare host/path values as authorities.
	if (url.host().isEmpty())
		url = QUrl(QStringLiteral("//") + raw);
	auto host = url.host();
	if (host.isEmpty())
		return QString();
	// QUrl::host() strips IPv6 brackets; restore them before appending a port.
	if (host.contains(QLatin1Char(':')))
		host = QStringLiteral("[") + host + QStringLiteral("]");
	QString safe = url.scheme().isEmpty() ? QString() : url.scheme() + QStringLiteral("://");
	safe += host;
	if (url.port() > 0)
		safe += QStringLiteral(":") + QString::number(url.port());
	return safe;
}

void RemoveWidget(QWidget *widget);

void RemoveLayoutItem(QLayoutItem *item)
{
	if (!item) {
		return;
	}
	RemoveWidget(item->widget());
	if (item->layout()) {
		while (QLayoutItem *item2 = item->layout()->takeAt(0)) {
			RemoveLayoutItem(item2);
		}
	}
	delete item;
}

void RemoveWidget(QWidget *widget)
{
	if (!widget) {
		return;
	}
	if (widget->layout()) {
		auto l = widget->layout();
		QLayoutItem *item;
		while (l->count() > 0 && (item = l->takeAt(0))) {
			RemoveLayoutItem(item);
		}
		delete l;
	}
	delete widget;
}

// Output button styling
void MultistreamDock::outputButtonStyle(QPushButton *button)
{
	button->setMinimumHeight(24);

	std::string baseStyles = "min-width: 30px; padding: 2px 10px; border-width: 2px;";

	button->setStyleSheet(QString::fromUtf8(baseStyles + (button->isChecked() ? "background: rgb(0,210,153);" : "")));

	button->setIcon(button->isChecked() ? streamActiveIcon : streamInactiveIcon);
}

// Common styling things here
auto canvasGroupStyle = QString("padding: 0px 0px 0px 0px;");                          // Main Canvas, Vertical Canvas
auto canvasGroupHeaderStyle = QString("padding: 0px 0px 0px 0px; font-weight: bold;"); // header of each group
auto outputTitleStyle = QString("QLabel{}");                                           // "Built -in stream"
auto outputGroupStyle = QString("background-color: %1; padding: 0px;")
				.arg(QPalette().color(QPalette::ColorRole::Mid).name(QColor::HexRgb)); // wrapper around above

auto outputPlatformIconSize = 36;

// For showing warning for no vertical integration
void showVerticalWarning(QVBoxLayout *verticalLayout)
{
	auto verticalWarning = new QWidget;
	verticalWarning->setContentsMargins(0, 0, 0, 0);

	auto verticalWarningLayout = new QVBoxLayout;
	verticalWarningLayout->setContentsMargins(0, 0, 0, 0);

	auto label = new QLabel(QString::fromUtf8(obs_module_text("NoVerticalWarning")));
	label->setStyleSheet(QString("padding: 0px;"));
	label->setWordWrap(true);
	label->setTextFormat(Qt::RichText);
	label->setOpenExternalLinks(true);
	verticalWarningLayout->addWidget(label);
	verticalWarning->setLayout(verticalWarningLayout);

	verticalLayout->addWidget(verticalWarning);
}

config_t *get_user_config(void)
{
	return obs_frontend_get_user_config();
}

MultistreamDock::MultistreamDock(QWidget *parent) : QFrame(parent)
{
	// Main layout
	mainLayout = new QVBoxLayout;
	mainLayout->setContentsMargins(0, 0, 0, 0);
	setLayout(mainLayout);

	auto t = new QWidget;
	auto tl = new QVBoxLayout;
	tl->setSpacing(8); // between canvas groups
	tl->setContentsMargins(0, 0, 0, 0);
	t->setStyleSheet(QString("padding: 0px; margin:0px;"));
	t->setLayout(tl);

	// Group for built in canvas
	auto mainCanvasGroup = new QGroupBox;
	mainCanvasGroup->setStyleSheet(canvasGroupStyle);

	mainCanvasLayout = new QVBoxLayout;
	mainCanvasLayout->setSpacing(4); // between outputs on main canvas

	// Layout for header row
	auto mainCanvasTitleRowLayout = new QHBoxLayout;

	auto mainCanvasLabel = new QLabel(QString::fromUtf8(obs_module_text("MainCanvas")));
	mainCanvasLabel->setStyleSheet(canvasGroupHeaderStyle);
	mainCanvasTitleRowLayout->addWidget(mainCanvasLabel);

	mainCanvasLayout->addLayout(mainCanvasTitleRowLayout);

	// We store the actual outputs here
	mainCanvasOutputLayout = new QVBoxLayout;
	mainCanvasOutputLayout->setSpacing(4); // between outputs on main canvas

	auto mainStreamGroup = new QGroupBox;
	mainStreamGroup->setStyleSheet(outputGroupStyle);

	auto mainStreamLayout = new QVBoxLayout;

	auto l2 = new QHBoxLayout;

	// Label for built in stream
	auto bisHeaderLabel = new QLabel(QString::fromUtf8(obs_module_text("BuiltinStream")));
	bisHeaderLabel->setStyleSheet(outputTitleStyle);

	// blank because we're pulling settings through from bis later
	mainPlatformIconLabel = new QLabel;
	auto platformIcon = ConfigUtils::getPlatformIconFromEndpoint(QString::fromUtf8(""));
	mainPlatformIconLabel->setPixmap(platformIcon.pixmap(outputPlatformIconSize, outputPlatformIconSize));

	l2->addWidget(mainPlatformIconLabel);
	l2->addWidget(bisHeaderLabel, 1);

	mainStreamButton = new QPushButton;
	mainStreamButton->setObjectName(QStringLiteral("canvasStream"));
	mainStreamButton->setIcon(streamInactiveIcon);
	mainStreamButton->setCheckable(true);
	mainStreamButton->setChecked(false);

	outputButtonStyle(mainStreamButton);

	connect(mainStreamButton, &QPushButton::clicked, [this] {
		const auto config = get_user_config();
		if (obs_frontend_streaming_active()) {
			bool stop = true;
			bool warnBeforeStreamStop = config_get_bool(config, "BasicWindow", "WarnBeforeStoppingStream");
			if (warnBeforeStreamStop && isVisible()) {
				auto button = QMessageBox::question(
					this, QString::fromUtf8(obs_frontend_get_locale_string("ConfirmStop.Title")),
					QString::fromUtf8(obs_frontend_get_locale_string("ConfirmStop.Text")),
					QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
				if (button == QMessageBox::No) {
					stop = false;
				}
			}
			if (stop) {
				obs_frontend_streaming_stop();
				mainStreamButton->setChecked(false);
			} else {
				mainStreamButton->setChecked(true);
			}
		} else {
			bool warnBeforeStreamStart = config_get_bool(config, "BasicWindow", "WarnBeforeStartingStream");
			if (warnBeforeStreamStart && isVisible()) {
				auto button = QMessageBox::question(
					this, QString::fromUtf8(obs_frontend_get_locale_string("ConfirmStart.Title")),
					QString::fromUtf8(obs_frontend_get_locale_string("ConfirmStart.Text")),
					QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
				if (button == QMessageBox::No) {
					mainStreamButton->setChecked(false);
				} else {
					obs_frontend_streaming_start();
				}
			} else {
				obs_frontend_streaming_start();
			}
			mainStreamButton->setChecked(true);
		}
		outputButtonStyle(mainStreamButton);
	});
	//streamButton->setSizePolicy(sp2);
	mainStreamButton->setToolTip(QString::fromUtf8(obs_module_text("Stream")));
	l2->addWidget(mainStreamButton);
	mainStreamLayout->addLayout(l2);

	mainStreamGroup->setLayout(mainStreamLayout);

	mainCanvasOutputLayout->addWidget(mainStreamGroup);

	mainCanvasLayout->addLayout(mainCanvasOutputLayout);
	mainCanvasGroup->setLayout(mainCanvasLayout);

	tl->addWidget(mainCanvasGroup);

	// VERTICAL
	auto verticalCanvasGroup = new QGroupBox;
	verticalCanvasGroup->setStyleSheet(canvasGroupStyle);

	verticalCanvasLayout = new QVBoxLayout;
	verticalCanvasGroup->setLayout(verticalCanvasLayout);
	tl->addWidget(verticalCanvasGroup);

	tl->addStretch(1);

	// Layout for header row
	auto verticalCanvasTitleRowLayout = new QHBoxLayout;

	auto verticalCanvasLabel = new QLabel(QString::fromUtf8(obs_module_text("VerticalCanvas")));
	verticalCanvasLabel->setStyleSheet(canvasGroupHeaderStyle);
	verticalCanvasTitleRowLayout->addWidget(verticalCanvasLabel);

	verticalCanvasLayout->addLayout(verticalCanvasTitleRowLayout);

	// We store the actual outputs here
	verticalCanvasOutputLayout = new QVBoxLayout;
	verticalCanvasOutputLayout->setSpacing(4); // between outputs on vertical canvas

	verticalCanvasLayout->addLayout(verticalCanvasOutputLayout); // Add output layout to parent

	//tl->addWidget(verticalCanvasGroup);
	QScrollArea *scrollArea = new QScrollArea;
	scrollArea->setWidget(t);
	scrollArea->setWidgetResizable(true);
	scrollArea->setLineWidth(0);
	scrollArea->setFrameShape(QFrame::NoFrame);
	scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	mainLayout->addWidget(scrollArea, 1);

	// Bottom Button Row
	auto buttonRow = new QHBoxLayout;
	buttonRow->setContentsMargins(8, 6, 8, 4);
	buttonRow->setSpacing(8);

	// Config Button
	configButton = new QPushButton;
	configButton->setMinimumHeight(30);
	configButton->setProperty("themeID", "configIconSmall");
	configButton->setProperty("class", "icon-gear");
	configButton->setFlat(true);
	configButton->setAutoDefault(false);
	//configButton->setSizePolicy(sp2);
	configButton->setToolTip(QString::fromUtf8(obs_module_text("AitumMultistreamSettings")));
	QPushButton::connect(configButton, &QPushButton::clicked, [this] {
		if (!configDialog) {
			configDialog = new OBSBasicSettings((QMainWindow *)obs_frontend_get_main_window());
		}
		auto settings = obs_data_create();
		if (current_config) {
			obs_data_apply(settings, current_config);
		}
		configDialog->LoadSettings(settings);
		configDialog->LoadVerticalSettings(true);
		configDialog->LoadOutputStats(&oldVideo);
		configDialog->SetNewerVersion(newer_version_available);
		configDialog->setResult(QDialog::Rejected);
		if (configDialog->exec() == QDialog::Accepted) {
			if (current_config) {
				obs_data_apply(current_config, settings);
				obs_data_release(settings);
				SaveSettings();
				LoadSettings();
				configDialog->SaveVerticalSettings();
				LoadVerticalOutputs(false);
			} else {
				current_config = settings;
			}
		} else {
			obs_data_release(settings);
		}
	});

	buttonRow->addWidget(configButton);

	// Contribute Button
	auto contributeButton = new QPushButton;
	contributeButton->setMinimumHeight(30);
	contributeButton->setIcon(ConfigUtils::generateEmojiQIcon("❤️"));
	contributeButton->setToolTip(QString::fromUtf8(obs_module_text("AitumMultistreamDonate")));
	QPushButton::connect(contributeButton, &QPushButton::clicked,
			     [] { QDesktopServices::openUrl(QUrl("https://aitum.tv/contribute")); });
	buttonRow->addWidget(contributeButton);

	// Aitum Button
	auto aitumButton = new QPushButton;
	aitumButton->setMinimumHeight(30);
	//aitumButton->setSizePolicy(sp2);
	aitumButton->setIcon(QIcon(":/aitum/media/aitum.png"));
	aitumButton->setToolTip(QString::fromUtf8("https://aitum.tv"));
	QPushButton::connect(aitumButton, &QPushButton::clicked, [] { QDesktopServices::openUrl(QUrl("https://aitum.tv")); });
	buttonRow->addWidget(aitumButton);

	mainLayout->addLayout(buttonRow);

	obs_frontend_add_event_callback(frontend_event, this);

	mainVideo = obs_get_video();
	connect(&videoCheckTimer, &QTimer::timeout, [this] {
		if (exiting) {
			return;
		}
		if (obs_get_video() != mainVideo) {
			oldVideo.push_back(mainVideo);
			mainVideo = obs_get_video();
			for (auto it = outputs.begin(); it != outputs.end(); it++) {
				auto venc = obs_output_get_video_encoder(std::get<obs_output_t *>(*it));
				if (venc && !obs_encoder_active(venc)) {
					obs_encoder_set_video(venc, mainVideo);
				}
			}
		}

		auto service = obs_frontend_get_streaming_service();
		auto url = QString::fromUtf8(service ? obs_service_get_connect_info(service, OBS_SERVICE_CONNECT_INFO_SERVER_URL)
						     : "");
		if (url != mainPlatformUrl) {
			mainPlatformUrl = url;
			mainPlatformIconLabel->setPixmap(ConfigUtils::getPlatformIconFromEndpoint(url).pixmap(
				outputPlatformIconSize, outputPlatformIconSize));
		}

		int idx = 0;
		while (auto item = mainCanvasOutputLayout->itemAt(idx++)) {
			auto streamGroup = item->widget();
			if (idx == 1) {
				auto active = obs_frontend_streaming_active();
				foreach(QObject * c, streamGroup->children())
				{
					std::string cn = c->metaObject()->className();
					if (cn == "QPushButton") {
						auto pb = (QPushButton *)c;
						if (pb->isChecked() != active) {
							pb->setChecked(active);
							outputButtonStyle(pb);
						}
					}
				}
				continue;
			}
			std::string name = streamGroup->objectName().toUtf8().constData();
			if (name.empty()) {
				continue;
			}
			for (auto it = outputs.begin(); it != outputs.end(); it++) {
				if (std::get<std::string>(*it) != name) {
					continue;
				}

				auto active = obs_output_active(std::get<obs_output_t *>(*it));
				foreach(QObject * c, streamGroup->children())
				{
					std::string cn = c->metaObject()->className();
					if (cn == "QPushButton") {
						auto pb = (QPushButton *)c;
						if (pb->isChecked() != active) {
							pb->setChecked(active);
							outputButtonStyle(pb);
						}
					}
				}
			}
		}
		auto ph = obs_get_proc_handler();
		struct calldata cd;
		calldata_init(&cd);
		idx = 0;
		while (auto item = verticalCanvasOutputLayout->itemAt(idx++)) {
			auto streamGroup = item->widget();
			std::string name = streamGroup->objectName().toUtf8().constData();
			if (name.empty()) {
				continue;
			}
			obs_output_t *output = nullptr;
			calldata_set_string(&cd, "name", name.c_str());
			if (proc_handler_call(ph, "aitum_vertical_get_stream_output", &cd)) {
				output = (obs_output_t *)calldata_ptr(&cd, "output");
			}
			bool active = obs_output_active(output);
			obs_output_release(output);
			foreach(QObject * c, streamGroup->children())
			{
				std::string cn = c->metaObject()->className();
				if (cn == "QPushButton") {
					auto pb = (QPushButton *)c;
					if (pb->isChecked() != active) {
						pb->setChecked(active);
						outputButtonStyle(pb);
					}
				}
			}
		}
		calldata_free(&cd);
	});
	videoCheckTimer.start(500);
	LoadSettingsFile();
}

MultistreamDock::~MultistreamDock()
{
	// Reject queued requests before teardown and prevent a second delete at unload.
	multistream_dock = nullptr;
	// obs-websocket owns vendor cleanup. Unregistering through its cached proc
	// handler is unsafe if that module has already unloaded.
	vendor = nullptr;
	videoCheckTimer.stop();
	// Disconnect first: force_stop can synchronously invoke our stop handler.
	for (auto it = outputs.begin(); it != outputs.end(); it++) {
		signal_handler_t *signal = obs_output_get_signal_handler(std::get<obs_output_t *>(*it));
		signal_handler_disconnect(signal, "start", stream_output_start, this);
		signal_handler_disconnect(signal, "stop", stream_output_stop, this);
	}
	for (auto it = outputs.begin(); it != outputs.end(); it++) {
		auto old = std::get<obs_output_t *>(*it);
		auto service = obs_output_get_service(old);
		if (obs_output_active(old)) {
			obs_output_force_stop(old);
		}
		if (!exiting) {
			obs_output_release(old);
		}
		obs_service_release(service);
	}
	outputs.clear();
	obs_data_array_release(vertical_outputs);
	obs_data_release(current_config);
	obs_frontend_remove_event_callback(frontend_event, this);
}

void MultistreamDock::frontend_event(enum obs_frontend_event event, void *private_data)
{
	auto md = (MultistreamDock *)private_data;
	if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
		md->finished_loading = true;
		md->LoadSettingsFile();
		// Vertical Canvas may initialize its docks after our post-load hook.
		md->LoadVerticalOutputs(false);
		if (!md->newer_version_available.isEmpty()) {
			md->AskUpdate();
		}
	} else if (event == OBS_FRONTEND_EVENT_PROFILE_CHANGED) {
		md->LoadSettingsFile();
	} else if (event == OBS_FRONTEND_EVENT_PROFILE_CHANGING || event == OBS_FRONTEND_EVENT_PROFILE_RENAMED) {
		md->SaveSettings();
	} else if (event == OBS_FRONTEND_EVENT_EXIT) {
		md->SaveSettings();
		md->exiting = true;
	} else if (event == OBS_FRONTEND_EVENT_STREAMING_STARTING || event == OBS_FRONTEND_EVENT_STREAMING_STARTED) {
		md->mainStreamButton->setChecked(true);
		md->outputButtonStyle(md->mainStreamButton);
		md->mainStreamButton->setIcon(md->streamActiveIcon);
		md->storeMainStreamEncoders();
	} else if (event == OBS_FRONTEND_EVENT_STREAMING_STOPPING || event == OBS_FRONTEND_EVENT_STREAMING_STOPPED) {
		md->mainStreamButton->setChecked(false);
		md->outputButtonStyle(md->mainStreamButton);
	}
}

void MultistreamDock::LoadSettingsFile()
{
	char *profile = obs_frontend_get_current_profile();
	if (current_config && strcmp(obs_data_get_string(current_config, "name"), profile) == 0) {
		bfree(profile);
		return;
	}
	obs_data_release(current_config);
	current_config = nullptr;
	char *path = obs_module_config_path("config.json");
	if (!path) {
		bfree(profile);
		return;
	}
	obs_data_t *config = obs_data_create_from_json_file_safe(path, "bak");
	bfree(path);
	if (!config) {
		config = obs_data_create();
		blog(LOG_WARNING, "[Aitum Multistream] No configuration file loaded");
	} else {
		blog(LOG_INFO, "[Aitum Multistream] Loaded configuration file");
	}
	partnerBlockTime = obs_data_get_int(config, "partner_block");

	auto profiles = obs_data_get_array(config, "profiles");
	auto pc = obs_data_array_count(profiles);
	obs_data_t *pd = nullptr;
	for (size_t i = 0; i < pc; i++) {
		obs_data_t *t = obs_data_array_item(profiles, i);
		if (!t) {
			continue;
		}
		auto name = obs_data_get_string(t, "name");
		if (strcmp(profile, name) == 0) {
			pd = t;
			break;
		}
		obs_data_release(t);
	}
	obs_data_array_release(profiles);

	obs_data_release(config);
	if (!pd) {
		current_config = obs_data_create();
		obs_data_set_string(current_config, "name", profile);
		bfree(profile);
		blog(LOG_INFO, "[Aitum Multistream] profile not found");
		LoadSettings();
		return;
	}
	bfree(profile);
	current_config = pd;
	LoadSettings();
}

void MultistreamDock::LoadSettings()
{

	auto outputs2 = obs_data_get_array(current_config, "outputs");
	// Clear pointers before deleting rows. Renamed or removed outputs won't be rebound.
	for (auto it = outputs.begin(); it != outputs.end(); it++)
		std::get<QPushButton *>(*it) = nullptr;
	int idx = 1;
	while (auto item = mainCanvasOutputLayout->itemAt(idx)) {
		auto streamGroup = item->widget();
		mainCanvasOutputLayout->removeWidget(streamGroup);
		RemoveWidget(streamGroup);
	}

	obs_data_array_enum(
		outputs2,
		[](obs_data_t *data2, void *param) {
			auto d = (MultistreamDock *)param;
			d->LoadOutput(data2, false);
		},
		this);
	obs_data_array_release(outputs2);
}

void MultistreamDock::LoadOutput(obs_data_t *output_data, bool vertical)
{
	auto nameChars = obs_data_get_string(output_data, "name");
	auto name = QString::fromUtf8(nameChars);
	if (vertical) {
		for (int i = 0; i < verticalCanvasOutputLayout->count(); i++) {
			auto item = verticalCanvasOutputLayout->itemAt(i);
			auto oName = item->widget()->objectName();
			if (oName == name) {
				return;
			}
		}
	} else {
		for (int i = 1; i < mainCanvasOutputLayout->count(); i++) {
			auto item = mainCanvasOutputLayout->itemAt(i);
			auto oName = item->widget()->objectName();
			if (oName == name) {
				return;
			}
		}
	}
	auto streamButton = new QPushButton;
	for (auto it = outputs.begin(); it != outputs.end(); it++) {
		if (std::get<std::string>(*it) != nameChars) {
			continue;
		}
		if (obs_data_get_bool(output_data, "advanced")) {
			auto output = std::get<obs_output_t *>(*it);
			auto video_encoder = obs_output_get_video_encoder(output);
			if (video_encoder &&
			    strcmp(obs_encoder_get_id(video_encoder), obs_data_get_string(output_data, "video_encoder")) == 0) {
				auto ves = obs_data_get_obj(output_data, "video_encoder_settings");
				obs_encoder_update(video_encoder, ves);
				obs_data_release(ves);
			}
		}
		std::get<QPushButton *>(*it) = streamButton;
	}
	auto streamGroup = new QGroupBox;
	streamGroup->setStyleSheet(outputGroupStyle);
	streamGroup->setObjectName(name);
	auto streamLayout = new QVBoxLayout;

	auto l2 = new QHBoxLayout;

	auto endpoint = QString::fromUtf8(obs_data_get_string(output_data, "stream_server"));
	auto platformIconLabel = new QLabel;
	auto platformIcon = ConfigUtils::getPlatformIconFromEndpoint(endpoint);

	platformIconLabel->setPixmap(platformIcon.pixmap(outputPlatformIconSize, outputPlatformIconSize));

	l2->addWidget(platformIconLabel);

	l2->addWidget(new QLabel(name), 1);

	streamButton->setMinimumHeight(30);
	streamButton->setObjectName(QStringLiteral("canvasStream"));
	streamButton->setIcon(streamInactiveIcon);
	streamButton->setCheckable(true);
	streamButton->setChecked(false);
	outputButtonStyle(streamButton);

	if (vertical) {
		std::string output_name = obs_data_get_string(output_data, "name");
		connect(streamButton, &QPushButton::clicked, [this, streamButton, output_name] {
			auto ph = obs_get_proc_handler();
			struct calldata cd;
			calldata_init(&cd);
			calldata_set_string(&cd, "name", output_name.c_str());
			auto config = get_user_config();
			if (streamButton->isChecked()) {
				bool start = true;
				bool warnBeforeStreamStart = config_get_bool(config, "BasicWindow", "WarnBeforeStartingStream");
				if (warnBeforeStreamStart && isVisible()) {
					auto button = QMessageBox::question(
						this, QString::fromUtf8(obs_frontend_get_locale_string("ConfirmStart.Title")),
						QString::fromUtf8(obs_frontend_get_locale_string("ConfirmStart.Text")),
						QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
					if (button == QMessageBox::No) {
						start = false;
					}
				}
				if (!start || !proc_handler_call(ph, "aitum_vertical_start_stream_output", &cd)) {
					streamButton->setChecked(false);
				}
			} else {
				bool stop = true;
				bool warnBeforeStreamStop = config_get_bool(config, "BasicWindow", "WarnBeforeStoppingStream");
				if (warnBeforeStreamStop && isVisible()) {
					auto button = QMessageBox::question(
						this, QString::fromUtf8(obs_frontend_get_locale_string("ConfirmStop.Title")),
						QString::fromUtf8(obs_frontend_get_locale_string("ConfirmStop.Text")),
						QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
					if (button == QMessageBox::No) {
						stop = false;
					}
				}
				if (stop) {
					proc_handler_call(ph, "aitum_vertical_stop_stream_output", &cd);
				} else {
					streamButton->setChecked(true);
				}
			}
			calldata_free(&cd);

			outputButtonStyle(streamButton);
		});
	} else {
		connect(streamButton, &QPushButton::clicked, [this, streamButton, output_data] {
			if (streamButton->isChecked()) {
				blog(LOG_INFO, "[Aitum Multistream] start stream clicked '%s'",
				     obs_data_get_string(output_data, "name"));
				if (!StartOutput(output_data, streamButton)) {
					streamButton->setChecked(false);
				}
			} else {
				bool stop = true;
				bool warnBeforeStreamStop =
					config_get_bool(get_user_config(), "BasicWindow", "WarnBeforeStoppingStream");
				if (warnBeforeStreamStop && isVisible()) {
					auto button = QMessageBox::question(
						this, QString::fromUtf8(obs_frontend_get_locale_string("ConfirmStop.Title")),
						QString::fromUtf8(obs_frontend_get_locale_string("ConfirmStop.Text")),
						QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
					if (button == QMessageBox::No) {
						stop = false;
					}
				}
				if (stop) {
					blog(LOG_INFO, "[Aitum Multistream] stop stream clicked '%s'",
					     obs_data_get_string(output_data, "name"));
					const char *name2 = obs_data_get_string(output_data, "name");
					for (auto it = outputs.begin(); it != outputs.end(); it++) {
						if (std::get<std::string>(*it) != name2) {
							continue;
						}

						StopOutput(std::get<obs_output_t *>(*it));
					}
				} else {
					streamButton->setChecked(true);
				}
			}
			outputButtonStyle(streamButton);
		});
	}
	//streamButton->setSizePolicy(sp2);
	streamButton->setToolTip(QString::fromUtf8(obs_module_text("Stream")));
	l2->addWidget(streamButton);
	streamLayout->addLayout(l2);

	streamGroup->setLayout(streamLayout);

	if (vertical) {
		verticalCanvasOutputLayout->addWidget(streamGroup);
	} else {
		mainCanvasOutputLayout->addWidget(streamGroup);
	}
}

static void ensure_directory(char *path)
{
#ifdef _WIN32
	char *backslash = strrchr(path, '\\');
	if (backslash) {
		*backslash = '/';
	}
#endif

	char *slash = strrchr(path, '/');
	if (slash) {
		*slash = 0;
		os_mkdirs(path);
		*slash = '/';
	}

#ifdef _WIN32
	if (backslash) {
		*backslash = '\\';
	}
#endif
}

void MultistreamDock::SaveSettings()
{
	char *path = obs_module_config_path("config.json");
	if (!path) {
		return;
	}
	obs_data_t *config = obs_data_create_from_json_file_safe(path, "bak");
	if (!config) {
		ensure_directory(path);
		config = obs_data_create();
		blog(LOG_WARNING, "[Aitum Multistream] New configuration file");
	}
	obs_data_set_int(config, "partner_block", partnerBlockTime);
	auto profiles = obs_data_get_array(config, "profiles");
	if (!profiles) {
		profiles = obs_data_array_create();
		obs_data_set_array(config, "profiles", profiles);
	}
	obs_data_t *pd = nullptr;
	if (current_config) {
		auto old_name = obs_data_get_string(current_config, "name");
		auto pc = obs_data_array_count(profiles);
		for (size_t i = 0; i < pc; i++) {
			obs_data_t *t = obs_data_array_item(profiles, i);
			if (!t) {
				continue;
			}
			auto name = obs_data_get_string(t, "name");
			if (strcmp(old_name, name) == 0) {
				pd = t;
				break;
			}
			obs_data_release(t);
		}
	}
	if (!pd) {
		pd = obs_data_create();
		obs_data_array_push_back(profiles, pd);
	}
	obs_data_array_release(profiles);
	char *profile = obs_frontend_get_current_profile();
	obs_data_set_string(pd, "name", profile);
	bfree(profile);
	if (current_config) {
		obs_data_apply(pd, current_config);
	}
	obs_data_release(pd);

	if (obs_data_save_json_safe(config, path, "tmp", "bak")) {
		blog(LOG_INFO, "[Aitum Multistream] Saved settings");
	} else {
		blog(LOG_ERROR, "[Aitum Multistream] Failed saving settings");
	}
	obs_data_release(config);
	bfree(path);
}

bool MultistreamDock::StartOutput(obs_data_t *settings, QPushButton *streamButton)
{
	if (!settings)
		return false;

	bool warnBeforeStreamStart = config_get_bool(get_user_config(), "BasicWindow", "WarnBeforeStartingStream");
	if (warnBeforeStreamStart && isVisible()) {
		auto button = QMessageBox::question(this, QString::fromUtf8(obs_frontend_get_locale_string("ConfirmStart.Title")),
						    QString::fromUtf8(obs_frontend_get_locale_string("ConfirmStart.Text")),
						    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
		if (button == QMessageBox::No)
			return false;
	}

	auto result = StartOutputInternal(settings, streamButton);
	if (!result.ok && result.locale_key) {
		auto text = QString::fromUtf8(obs_module_text(result.locale_key));
		QMessageBox::warning(this, text, text);
	}
	return result.ok;
}

// No dialogs: the caller chooses UI or websocket error reporting.
MultistreamDock::StartOutputResult MultistreamDock::StartOutputInternal(obs_data_t *settings, QPushButton *streamButton)
{
	if (!settings)
		return {false, "no_settings", nullptr};

	const char *name = obs_data_get_string(settings, "name");
	obs_output_t *previous = nullptr;
	for (auto it = outputs.begin(); it != outputs.end(); it++) {
		if (std::get<std::string>(*it) != name)
			continue;
		previous = std::get<obs_output_t *>(*it);
		outputs.erase(it);
		break;
	}
	if (previous) {
		auto service = obs_output_get_service(previous);
		bool was_active = obs_output_active(previous);
		// The entry is gone, so disconnect and emit the stop event here.
		signal_handler_t *signal = obs_output_get_signal_handler(previous);
		signal_handler_disconnect(signal, "start", stream_output_start, this);
		signal_handler_disconnect(signal, "stop", stream_output_stop, this);
		if (was_active)
			obs_output_force_stop(previous);
		if (was_active)
			vendor_emit_output_state(previous, false);
		obs_output_release(previous);
		obs_service_release(service);
	}
	obs_encoder_t *venc = nullptr;
	obs_encoder_t *aenc = nullptr;
	auto advanced = obs_data_get_bool(settings, "advanced");
	if (advanced) {
		auto venc_name = obs_data_get_string(settings, "video_encoder");
		if (!venc_name || venc_name[0] == '\0') {
			//use main encoder
			auto main_output = obs_frontend_get_streaming_output();
			if (!obs_output_active(main_output)) {
				obs_output_release(main_output);
				blog(LOG_WARNING, "[Aitum Multistream] failed to start stream '%s' because main was not started",
				     obs_data_get_string(settings, "name"));
				return {false, "main_output_not_active", "MainOutputNotActive"};
			}
			auto vei = (int)obs_data_get_int(settings, "video_encoder_index");
			venc = obs_output_get_video_encoder2(main_output, vei);
			obs_output_release(main_output);
			if (!venc) {
				blog(LOG_WARNING,
				     "[Aitum Multistream] failed to start stream '%s' because encoder index %d was not found",
				     obs_data_get_string(settings, "name"), vei);
				return {false, "encoder_index_not_found", "MainOutputEncoderIndexNotFound"};
			}
		} else {
			obs_data_t *s = nullptr;
			auto ves = obs_data_get_obj(settings, "video_encoder_settings");
			if (ves) {
				s = obs_data_create();
				obs_data_apply(s, ves);
				obs_data_release(ves);
			}
			std::string video_encoder_name = "aitum_multi_video_encoder_";
			video_encoder_name += name;
			venc = obs_video_encoder_create(venc_name, video_encoder_name.c_str(), s, nullptr);
			obs_data_release(s);
			obs_encoder_set_video(venc, obs_get_video());
			auto divisor = obs_data_get_int(settings, "frame_rate_divisor");
			if (divisor > 1) {
				obs_encoder_set_frame_rate_divisor(venc, (uint32_t)divisor);
			}

			bool scale = obs_data_get_bool(settings, "scale");
			if (scale) {
				obs_encoder_set_scaled_size(venc, (uint32_t)obs_data_get_int(settings, "width"),
							    (uint32_t)obs_data_get_int(settings, "height"));
				obs_encoder_set_gpu_scale_type(venc, (obs_scale_type)obs_data_get_int(settings, "scale_type"));
			}
		}
		auto aenc_name = obs_data_get_string(settings, "audio_encoder");
		if (!aenc_name || aenc_name[0] == '\0') {
			//use main encoder
			auto main_output = obs_frontend_get_streaming_output();
			if (!obs_output_active(main_output)) {
				obs_output_release(main_output);
				blog(LOG_WARNING, "[Aitum Multistream] failed to start stream '%s' because main was not started",
				     obs_data_get_string(settings, "name"));
				return {false, "main_output_not_active", "MainOutputNotActive"};
			}
			auto aei = (int)obs_data_get_int(settings, "audio_encoder_index");
			aenc = obs_output_get_audio_encoder(main_output, aei);
			obs_output_release(main_output);
			if (!aenc) {
				blog(LOG_WARNING,
				     "[Aitum Multistream] failed to start stream '%s' because encoder index %d was not found",
				     obs_data_get_string(settings, "name"), aei);
				return {false, "encoder_index_not_found", "MainOutputEncoderIndexNotFound"};
			}
		} else {
			obs_data_t *s = nullptr;
			auto aes = obs_data_get_obj(settings, "audio_encoder_settings");
			if (aes) {
				s = obs_data_create();
				obs_data_apply(s, aes);
				obs_data_release(aes);
			}
			std::string audio_encoder_name = "aitum_multi_audio_encoder_";
			audio_encoder_name += name;
			aenc = obs_audio_encoder_create(aenc_name, audio_encoder_name.c_str(), s,
							obs_data_get_int(settings, "audio_track"), nullptr);
			obs_data_release(s);
			obs_encoder_set_audio(aenc, obs_get_audio());
		}
	} else {
		auto main_output = obs_frontend_get_streaming_output();
		venc = main_output ? obs_output_get_video_encoder(main_output) : nullptr;
		if (!venc || !obs_output_active(main_output)) {
			obs_output_release(main_output);
			blog(LOG_WARNING, "[Aitum Multistream] failed to start stream '%s' because main was not started",
			     obs_data_get_string(settings, "name"));
			return {false, "main_output_not_active", "MainOutputNotActive"};
		}

		aenc = obs_output_get_audio_encoder(main_output, 0);
		obs_output_release(main_output);
	}
	if (!aenc || !venc) {
		return {false, "no_encoder", nullptr};
	}
	auto server = obs_data_get_string(settings, "stream_server");
	if (!server || !strlen(server)) {
		server = obs_data_get_string(settings, "server");
		if (server && strlen(server)) {
			obs_data_set_string(settings, "stream_server", server);
		}
	}
	bool whip = strstr(server, "whip") != nullptr;
	auto s = obs_data_create();
	obs_data_set_string(s, "server", server);
	auto key = obs_data_get_string(settings, "stream_key");
	if (!key || !strlen(key)) {
		key = obs_data_get_string(settings, "key");
		if (key && strlen(key)) {
			obs_data_set_string(settings, "stream_key", key);
		}
	}
	if (whip) {
		obs_data_set_string(s, "bearer_token", key);
	} else {
		obs_data_set_string(s, "key", key);
	}
	//use_auth
	//username
	//password
	std::string service_name = "aitum_multi_service_";
	service_name += name;
	auto service = obs_service_create(whip ? "whip_custom" : "rtmp_custom", service_name.c_str(), s, nullptr);
	obs_data_release(s);

	const char *type = obs_service_get_preferred_output_type(service);
	if (!type) {
		type = "rtmp_output";
		if (strncmp(server, "ftl", 3) == 0) {
			type = "ftl_output";
		} else if (strncmp(server, "rtmp", 4) != 0) {
			type = "ffmpeg_mpegts_muxer";
		}
	}
	std::string output_name = "aitum_multi_output_";
	output_name += name;
	auto output = obs_output_create(type, output_name.c_str(), nullptr, nullptr);
	obs_output_set_service(output, service);

	config_t *config = obs_frontend_get_profile_config();
	if (config) {
		obs_data_t *output_settings = obs_data_create();
		obs_data_set_string(output_settings, "bind_ip", config_get_string(config, "Output", "BindIP"));
		obs_data_set_string(output_settings, "ip_family", config_get_string(config, "Output", "IPFamily"));
		obs_output_update(output, output_settings);
		obs_data_release(output_settings);

		bool useDelay = config_get_bool(config, "Output", "DelayEnable");
		auto delaySec = (uint32_t)config_get_int(config, "Output", "DelaySec");
		bool preserveDelay = config_get_bool(config, "Output", "DelayPreserve");
		obs_output_set_delay(output, useDelay ? delaySec : 0, preserveDelay ? OBS_OUTPUT_DELAY_PRESERVE : 0);
	}

	signal_handler_t *signal = obs_output_get_signal_handler(output);
	signal_handler_disconnect(signal, "start", stream_output_start, this);
	signal_handler_disconnect(signal, "stop", stream_output_stop, this);
	signal_handler_connect(signal, "start", stream_output_start, this);
	signal_handler_connect(signal, "stop", stream_output_stop, this);

	//for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
	//auto venc = obs_output_get_video_encoder2(main_output, 0);
	//for (size_t i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++) {
	//obs_output_get_audio_encoder(main_output, 0);

	obs_output_set_video_encoder(output, venc);
	obs_output_set_audio_encoder(output, aenc, 0);

	// Register before starting: signals can fire before obs_output_start() returns.
	outputs.push_back({obs_data_get_string(settings, "name"), output, streamButton});

	if (!obs_output_start(output)) {
		// A synchronous start failure emits no stop signal; remove the busy entry here.
		auto last_error = obs_output_get_last_error(output);
		blog(LOG_WARNING, "[Aitum Multistream] failed to start stream '%s'%s%s", name, last_error ? ": " : "",
		     last_error ? last_error : "");
		for (auto it = outputs.begin(); it != outputs.end(); it++) {
			if (std::get<obs_output_t *>(*it) != output)
				continue;
			outputs.erase(it);
			break;
		}
		signal_handler_disconnect(signal, "start", stream_output_start, this);
		signal_handler_disconnect(signal, "stop", stream_output_stop, this);
		auto failed_service = obs_output_get_service(output);
		obs_output_release(output);
		obs_service_release(failed_service);
		return {false, "start_failed", nullptr};
	}

	return {true, nullptr, nullptr};
}

// Output signals may run off the UI thread. Retain the output for queued work.
// The deleter also runs when Qt discards the callback during dock destruction.
// This reference is separate from the one owned by the outputs entry.
using OutputTripRef = std::shared_ptr<obs_output_t>;

static OutputTripRef output_trip_ref(calldata_t *calldata)
{
	// calldata is only valid for the duration of the signal, so read it here.
	auto output = (obs_output_t *)calldata_ptr(calldata, "output");
	return OutputTripRef(output ? obs_output_get_ref(output) : nullptr, obs_output_release);
}

void MultistreamDock::stream_output_start(void *data, calldata_t *calldata)
{
	auto md = (MultistreamDock *)data;
	auto trip = output_trip_ref(calldata);
	if (!trip)
		return;
	QMetaObject::invokeMethod(
		md,
		[md, trip] {
			auto output = trip.get();
			for (auto it = md->outputs.begin(); it != md->outputs.end(); it++) {
				if (std::get<obs_output_t *>(*it) != output)
					continue;
				// Resolve the button after dispatch; settings reloads can replace it.
				auto button = std::get<QPushButton *>(*it);
				if (button && !button->isChecked()) {
					button->setChecked(true);
					md->outputButtonStyle(button);
				}
				md->vendor_emit_output_state(output, true);
				break;
			}
		},
		Qt::QueuedConnection);
}

void MultistreamDock::stream_output_stop(void *data, calldata_t *calldata)
{
	auto md = (MultistreamDock *)data;
	//const char *last_error = (const char *)calldata_ptr(calldata, "last_error");
	auto trip = output_trip_ref(calldata);
	if (!trip)
		return;
	QMetaObject::invokeMethod(
		md,
		[md, trip] {
			auto output = trip.get();
			for (auto it = md->outputs.begin(); it != md->outputs.end(); it++) {
				if (std::get<obs_output_t *>(*it) != output)
					continue;
				auto button = std::get<QPushButton *>(*it);
				if (button && button->isChecked()) {
					button->setChecked(false);
					md->outputButtonStyle(button);
				}
				md->outputs.erase(it);
				// Release the entry's reference unless libobs owns shutdown cleanup.
				if (!md->exiting)
					obs_output_release(output);
				md->vendor_emit_output_state(output, false);
				break;
			}
			// Missing entries were already released by replacement or teardown.
		},
		Qt::QueuedConnection);
}

void MultistreamDock::ApiInfo(QString info)
{
	auto d = obs_data_create_from_json(info.toUtf8().constData());
	if (!d) {
		return;
	}
	auto data_obj = obs_data_get_obj(d, "data");
	obs_data_release(d);
	if (!data_obj) {
		return;
	}
	auto version = obs_data_get_string(data_obj, "version");
	int major;
	int minor;
	int patch;
	if (sscanf(version, "%d.%d.%d", &major, &minor, &patch) == 3) {
		auto sv = MAKE_SEMANTIC_VERSION(major, minor, patch);
		if (sv > MAKE_SEMANTIC_VERSION(PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_VERSION_PATCH)) {
			newer_version_available = QString::fromUtf8(version);
			configButton->setStyleSheet(QString::fromUtf8("background: rgb(192,128,0);"));
			if (finished_loading) {
				AskUpdate();
			}
		}
	}
	time_t current_time = time(nullptr);
	if (current_time < partnerBlockTime || current_time - partnerBlockTime > 1209600) {
		obs_data_array_t *blocks = obs_data_get_array(data_obj, "partnerBlocks");
		size_t count = obs_data_array_count(blocks);
		size_t added_count = 0;
		for (size_t i = count; i > 0; i--) {
			obs_data_t *block = obs_data_array_item(blocks, i - 1);
			auto block_type = obs_data_get_string(block, "type");
			QBoxLayout *layout = nullptr;
			if (strcmp(block_type, "LINK") == 0) {
				auto button = new QPushButton(QString::fromUtf8(obs_data_get_string(block, "label")));
				button->setStyleSheet(QString::fromUtf8(obs_data_get_string(block, "qss")));
				auto url = QString::fromUtf8(obs_data_get_string(block, "data"));
				connect(button, &QPushButton::clicked, [url] { QDesktopServices::openUrl(QUrl(url)); });
				auto buttonRow = new QHBoxLayout;
				buttonRow->setContentsMargins(8, 0, 8, 0);
				buttonRow->setSpacing(8);
				buttonRow->addWidget(button);
				layout = buttonRow;
			} else if (strcmp(block_type, "IMAGE") == 0) {
				auto image_data = QString::fromUtf8(obs_data_get_string(block, "data"));
				if (image_data.startsWith("data:image/")) {
					auto pos = image_data.indexOf(";");
					auto format = image_data.mid(11, pos - 11);
					QImage image;
					if (image.loadFromData(QByteArray::fromBase64(image_data.mid(pos + 7).toUtf8().constData()),
							       format.toUtf8().constData())) {
						auto label = new AspectRatioPixmapLabel;
						label->setPixmap(QPixmap::fromImage(image));
						label->setAlignment(Qt::AlignCenter);
						label->setStyleSheet(QString::fromUtf8(obs_data_get_string(block, "qss")));
						auto labelRow = new QHBoxLayout;
						labelRow->addWidget(label, 1, Qt::AlignCenter);
						layout = labelRow;
					}
				}
			}
			if (layout) {
				added_count++;
				if (i == 1) {
					auto closeButton = new QPushButton("🞫");
					connect(closeButton, &QPushButton::clicked, [this, added_count] {
						for (size_t j = 0; j < added_count; j++) {
							auto item = mainLayout->takeAt(1);
							RemoveLayoutItem(item);
						}
						partnerBlockTime = time(nullptr);
						SaveSettings();
					});
					layout->addWidget(closeButton);
				}
				mainLayout->insertLayout(1, layout, 0);
			}
			obs_data_release(block);
		}
		obs_data_array_release(blocks);
	}
	obs_data_release(data_obj);
}

void MultistreamDock::LoadVerticalOutputs(bool firstLoad)
{
	auto ph = obs_get_proc_handler();
	struct calldata cd;
	calldata_init(&cd);
	if (!proc_handler_call(ph, "aitum_vertical_get_stream_settings", &cd)) {
		if (firstLoad) {                                         // only display warning on first load
			showVerticalWarning(verticalCanvasOutputLayout); // show warning
		}
		calldata_free(&cd);
		return;
	}

	if (vertical_outputs) {
		obs_data_array_release(vertical_outputs);
	}
	vertical_outputs = (obs_data_array_t *)calldata_ptr(&cd, "outputs");

	calldata_free(&cd);
	int idx = 0;
	while (auto item = verticalCanvasOutputLayout->itemAt(idx)) {
		auto streamGroup = item->widget();
		verticalCanvasOutputLayout->removeWidget(streamGroup);
		RemoveWidget(streamGroup);
	}

	obs_data_array_enum(
		vertical_outputs,
		[](obs_data_t *data2, void *param) {
			auto d = (MultistreamDock *)param;
			d->LoadOutput(data2, true);
		},
		this);
}

const char *MultistreamDock::RemoteStartOutput(const QString &name)
{
	if (!current_config)
		return RemoteVerticalOutput(name, true);
	auto nameUtf8 = name.toUtf8();

	// An existing entry includes connecting outputs, where obs_output_active() is still false.
	// Do not replace them on retries.
	for (auto it = outputs.begin(); it != outputs.end(); it++) {
		if (std::get<std::string>(*it) != nameUtf8.constData())
			continue;
		blog(LOG_INFO, "[Aitum Multistream] remote start ignored, '%s' is already starting or active",
		     nameUtf8.constData());
		return nullptr;
	}

	obs_data_t *found = nullptr;
	auto outputs2 = obs_data_get_array(current_config, "outputs");
	auto count = obs_data_array_count(outputs2);
	for (size_t i = 0; i < count; i++) {
		auto item = obs_data_array_item(outputs2, i);
		if (!item)
			continue;
		if (name == QString::fromUtf8(obs_data_get_string(item, "name"))) {
			found = item;
			break;
		}
		obs_data_release(item);
	}
	obs_data_array_release(outputs2);
	if (!found)
		return RemoteVerticalOutput(name, true);
	QPushButton *streamButton = nullptr;
	for (int i = 1; i < mainCanvasOutputLayout->count(); i++) {
		auto item = mainCanvasOutputLayout->itemAt(i);
		if (item && item->widget() && item->widget()->objectName() == name) {
			streamButton = item->widget()->findChild<QPushButton *>(QStringLiteral("canvasStream"));
			break;
		}
	}
	if (!streamButton) {
		blog(LOG_WARNING, "[Aitum Multistream] remote start: no dock row for '%s'", nameUtf8.constData());
		obs_data_release(found);
		return "no_dock_row";
	}
	blog(LOG_INFO, "[Aitum Multistream] remote start stream '%s'", nameUtf8.constData());
	auto result = StartOutputInternal(found, streamButton);
	obs_data_release(found);
	streamButton->setChecked(result.ok);
	outputButtonStyle(streamButton);
	if (!result.ok)
		blog(LOG_WARNING, "[Aitum Multistream] remote start of '%s' failed: %s", nameUtf8.constData(),
		     result.error ? result.error : "unknown");
	return result.ok ? nullptr : (result.error ? result.error : "start_failed");
}

const char *MultistreamDock::RemoteStopOutput(const QString &name)
{
	auto nameUtf8 = name.toUtf8();
	for (auto it = outputs.begin(); it != outputs.end(); it++) {
		if (std::get<std::string>(*it) != nameUtf8.constData())
			continue;
		blog(LOG_INFO, "[Aitum Multistream] remote stop stream '%s'", nameUtf8.constData());
		StopOutput(std::get<obs_output_t *>(*it));
		return nullptr;
	}
	// Stopping a configured but inactive output is a successful no-op upstream.
	auto configured = obs_data_get_array(current_config, "outputs");
	bool found = false;
	for (size_t i = 0; i < obs_data_array_count(configured); i++) {
		auto item = obs_data_array_item(configured, i);
		found = name == QString::fromUtf8(obs_data_get_string(item, "name"));
		obs_data_release(item);
		if (found)
			break;
	}
	obs_data_array_release(configured);
	return found ? nullptr : RemoteVerticalOutput(name, false);
}

const char *MultistreamDock::RemoteVerticalOutput(const QString &name, bool start)
{
	QPushButton *button = nullptr;
	for (int i = 0; i < verticalCanvasOutputLayout->count(); i++) {
		auto item = verticalCanvasOutputLayout->itemAt(i);
		if (item && item->widget() && item->widget()->objectName() == name) {
			button = item->widget()->findChild<QPushButton *>(QStringLiteral("canvasStream"));
			break;
		}
	}
	if (!button)
		return "Output not found";
	if (button->isChecked() == start)
		return nullptr;

	struct calldata cd;
	calldata_init(&cd);
	calldata_set_string(&cd, "name", name.toUtf8().constData());
	// The bridge returns void; success confirms dispatch, not the stream's final state.
	bool called = proc_handler_call(obs_get_proc_handler(),
					start ? "aitum_vertical_start_stream_output" : "aitum_vertical_stop_stream_output", &cd);
	calldata_free(&cd);
	if (!called)
		return "vertical_canvas_unavailable";
	button->setChecked(start);
	outputButtonStyle(button);
	return nullptr;
}

void MultistreamDock::FillOutputs(obs_data_t *response_data)
{
	auto arr = obs_data_array_create();
	if (current_config) {
		auto outputs2 = obs_data_get_array(current_config, "outputs");
		auto count = obs_data_array_count(outputs2);
		for (size_t i = 0; i < count; i++) {
			auto item = obs_data_array_item(outputs2, i);
			if (!item)
				continue;
			auto name = obs_data_get_string(item, "name");
			if (!name || !*name) {
				obs_data_release(item);
				continue;
			}
			auto server = obs_data_get_string(item, "stream_server");
			if (!server || !*server)
				server = obs_data_get_string(item, "server");
			auto advanced = obs_data_get_bool(item, "advanced");
			auto venc = obs_data_get_string(item, "video_encoder");
			auto aenc = obs_data_get_string(item, "audio_encoder");
			// Borrowing any main-stream encoder means the output can only
			// start while the main stream is active.
			bool requires_main = !advanced || !venc || !*venc || !aenc || !*aenc;
			bool active = false;
			for (auto it = outputs.begin(); it != outputs.end(); it++) {
				if (std::get<std::string>(*it) == name) {
					active = obs_output_active(std::get<obs_output_t *>(*it));
					break;
				}
			}
			auto entry = obs_data_create();
			obs_data_set_string(entry, "name", name);
			obs_data_set_bool(entry, "vertical", false);
			obs_data_set_string(entry, "stream_server", SafeServerUrl(server).toUtf8().constData());
			obs_data_set_bool(entry, "advanced", advanced);
			obs_data_set_bool(entry, "requires_main", requires_main);
			obs_data_set_bool(entry, "active", active);
			obs_data_array_push_back(arr, entry);
			obs_data_release(entry);
			obs_data_release(item);
		}
		obs_data_array_release(outputs2);
	}
	// Append vertical outputs to the same array, as in upstream's API.
	// GetStreamOutput adds a reference, so release after the active check.
	if (vertical_outputs) {
		auto ph = obs_get_proc_handler();
		auto count = obs_data_array_count(vertical_outputs);
		for (size_t i = 0; i < count; i++) {
			auto item = obs_data_array_item(vertical_outputs, i);
			if (!item)
				continue;
			auto name = obs_data_get_string(item, "name");
			if (!name || !*name) {
				obs_data_release(item);
				continue;
			}
			bool active = false;
			struct calldata cd;
			calldata_init(&cd);
			calldata_set_string(&cd, "name", name);
			if (proc_handler_call(ph, "aitum_vertical_get_stream_output", &cd)) {
				auto output = (obs_output_t *)calldata_ptr(&cd, "output");
				if (output) {
					active = obs_output_active(output);
					obs_output_release(output);
				}
			}
			calldata_free(&cd);
			auto entry = obs_data_create();
			obs_data_set_string(entry, "name", name);
			obs_data_set_bool(entry, "vertical", true);
			obs_data_set_bool(entry, "active", active);
			obs_data_array_push_back(arr, entry);
			obs_data_release(entry);
			obs_data_release(item);
		}
	}
	obs_data_set_array(response_data, "outputs", arr);
	obs_data_array_release(arr);

	obs_data_set_bool(response_data, "success", true);
	obs_data_set_bool(response_data, "main_active", obs_frontend_streaming_active());
}

void MultistreamDock::storeMainStreamEncoders()
{
	if (!current_config) {
		return;
	}
	struct obs_video_info ovi = {0};
	obs_get_video_info(&ovi);
	double fps = ovi.fps_den > 0 ? (double)ovi.fps_num / (double)ovi.fps_den : 0.0;
	auto output = obs_frontend_get_streaming_output();
	bool found = false;
	for (auto i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
		auto encoder = obs_output_get_video_encoder2(output, i);
		QString settingName = QString::fromUtf8("video_encoder_description") + QString::number(i);
		if (encoder) {
			found = true;
			auto mainEncoderDescription = QString::number(obs_encoder_get_width(encoder)) + "x" +
						      QString::number(obs_encoder_get_height(encoder));
			auto divisor = obs_encoder_get_frame_rate_divisor(encoder);
			if (divisor > 0) {
				mainEncoderDescription +=
					QString::fromUtf8(" ") + QString::number(fps / divisor, 'g', 4) + QString::fromUtf8("fps");
			}

			auto settings = obs_encoder_get_settings(encoder);
			auto bitrate = settings ? obs_data_get_int(settings, "bitrate") : 0;
			if (bitrate > 0) {
				mainEncoderDescription +=
					QString::fromUtf8(" ") + QString::number(bitrate) + QString::fromUtf8("Kbps");
			}
			obs_data_release(settings);

			obs_data_set_string(current_config, settingName.toUtf8().constData(),
					    mainEncoderDescription.toUtf8().constData());

		} else if (found && obs_data_has_user_value(current_config, settingName.toUtf8().constData())) {
			obs_data_unset_user_value(current_config, settingName.toUtf8().constData());
		}
	}
	obs_output_release(output);
}

void MultistreamDock::AskUpdate()
{
	auto parts = newer_version_available.split(".");
	if (parts.count() < 3) {
		return;
	}
	int major = parts.value(0).toInt();
	int minor = parts.value(1).toInt();
	int patch = parts.value(2).toInt();
	auto sv = MAKE_SEMANTIC_VERSION(major, minor, patch);

	char *path = obs_module_config_path("config.json");
	if (!path) {
		return;
	}

	obs_data_t *config = obs_data_create_from_json_file_safe(path, "bak");


	auto skip_version = config ? obs_data_get_int(config, "skip_version") : 0;
	if (sv == skip_version) {
		obs_data_release(config);
		bfree(path);
		return;
	}

	auto main_window = static_cast<QMainWindow *>(obs_frontend_get_main_window());

	QMessageBox mb(QMessageBox::Question, QString::fromUtf8(obs_frontend_get_locale_string("Updater.Title")),
		       QString::fromUtf8(obs_frontend_get_locale_string("Updater.Text")) + " " +
			       QString::fromUtf8(obs_module_text("AitumMultistream")) + " " + newer_version_available,
		       QMessageBox::StandardButtons(), main_window);
	auto update = mb.addButton(QString::fromUtf8(obs_frontend_get_locale_string("Updater.UpdateNow")), QMessageBox::YesRole);
	auto remind =
		mb.addButton(QString::fromUtf8(obs_frontend_get_locale_string("Updater.RemindMeLater")), QMessageBox::RejectRole);
	auto skip = mb.addButton(QString::fromUtf8(obs_frontend_get_locale_string("Updater.Skip")), QMessageBox::NoRole);
	mb.setDefaultButton(remind);
	mb.exec();
	if (mb.clickedButton() == update) {
		QDesktopServices::openUrl(QUrl(QString::fromUtf8("https://aitum.tv/download/multi")));
	} else if (mb.clickedButton() == skip) {
		if (!config) {
			config = obs_data_create();
		}
		obs_data_set_int(config, "skip_version", sv);
		if (obs_data_save_json_safe(config, path, "tmp", "bak")) {
			blog(LOG_INFO, "[Aitum Multistream] Saved settings");
		} else {
			blog(LOG_ERROR, "[Aitum Multistream] Failed saving settings");
		}
	}
	obs_data_release(config);
	bfree(path);
}

AspectRatioPixmapLabel::AspectRatioPixmapLabel(QWidget *parent) : QLabel(parent)
{
	setMinimumSize(1, 1);
	setScaledContents(false);
}

void AspectRatioPixmapLabel::setPixmap(const QPixmap &p)
{
	pix = p;
	QLabel::setPixmap(scaledPixmap());
}

int AspectRatioPixmapLabel::heightForWidth(int width) const
{
	return pix.isNull() ? height() : (pix.height() * width) / pix.width();
}

QSize AspectRatioPixmapLabel::sizeHint() const
{
	int w = width();
	return QSize(w, heightForWidth(w));
}

QPixmap AspectRatioPixmapLabel::scaledPixmap() const
{
	return pix.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void AspectRatioPixmapLabel::resizeEvent(QResizeEvent *e)
{
	UNUSED_PARAMETER(e);
	if (!pix.isNull()) {
		QLabel::setPixmap(scaledPixmap());
	}
}
