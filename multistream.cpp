#include "config-utils.hpp"
#include "multistream.hpp"
#include "obs-module.h"
#include "version.h"
#include <obs-frontend-api.h>
#include <QDesktopServices>
#include <QGroupBox>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <util/config-file.h>
#include <util/platform.h>

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
	if (!file || !file->buffer.num)
		return true;

	QMetaObject::invokeMethod(multistream_dock, "ApiInfo", Q_ARG(QString, QString::fromUtf8((const char *)file->buffer.array)));

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

void obs_module_post_load()
{
	if (multistream_dock)
		multistream_dock->LoadVerticalOutputs(true);
}

void obs_module_unload()
{
	if (version_update_info) {
		update_info_destroy(version_update_info);
		version_update_info = nullptr;
	}
	if (multistream_dock) {
		delete multistream_dock;
	}
}

const char *obs_module_name(void)
{
	return obs_module_text("AitumMultistream");
}

// Asks an output to stop, from the graphics thread as obs expects.
//
// Call this on the UI thread with an output still held by `outputs`. The extra
// reference is for the trip: obs_queue_task runs the task later, and in between
// the output's own "stop" signal can drop the plugin's reference and destroy
// it, leaving the task to run against freed memory.
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

void RemoveWidget(QWidget *widget);

void RemoveLayoutItem(QLayoutItem *item)
{
	if (!item)
		return;
	RemoveWidget(item->widget());
	if (item->layout()) {
		while (QLayoutItem *item2 = item->layout()->takeAt(0))
			RemoveLayoutItem(item2);
	}
	delete item;
}

void RemoveWidget(QWidget *widget)
{
	if (!widget)
		return;
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
				if (button == QMessageBox::No)
					stop = false;
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
		if (!configDialog)
			configDialog = new OBSBasicSettings((QMainWindow *)obs_frontend_get_main_window());
		auto settings = obs_data_create();
		if (current_config)
			obs_data_apply(settings, current_config);
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
		if (exiting)
			return;
		if (obs_get_video() != mainVideo) {
			oldVideo.push_back(mainVideo);
			mainVideo = obs_get_video();
			for (auto it = outputs.begin(); it != outputs.end(); it++) {
				auto venc = obs_output_get_video_encoder(std::get<obs_output_t *>(*it));
				if (venc && !obs_encoder_active(venc))
					obs_encoder_set_video(venc, mainVideo);
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
			if (name.empty())
				continue;
			for (auto it = outputs.begin(); it != outputs.end(); it++) {
				if (std::get<std::string>(*it) != name)
					continue;

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
			if (name.empty())
				continue;
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
	videoCheckTimer.stop();
	for (auto it = outputs.begin(); it != outputs.end(); it++) {
		auto old = std::get<obs_output_t *>(*it);
		signal_handler_t *signal = obs_output_get_signal_handler(old);
		signal_handler_disconnect(signal, "start", stream_output_start, this);
		signal_handler_disconnect(signal, "stop", stream_output_stop, this);
		auto service = obs_output_get_service(old);
		if (obs_output_active(old)) {
			obs_output_force_stop(old);
		}
		if (!exiting)
			obs_output_release(old);
		obs_service_release(service);
	}
	outputs.clear();
	obs_data_array_release(vertical_outputs);
	obs_data_release(current_config);
	obs_frontend_remove_event_callback(frontend_event, this);
	multistream_dock = nullptr;
}

void MultistreamDock::frontend_event(enum obs_frontend_event event, void *private_data)
{
	auto md = (MultistreamDock *)private_data;
	if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
		md->finished_loading = true;
		md->LoadSettingsFile();
		if (!md->newer_version_available.isEmpty())
			md->AskUpdate();
	}else if (event == OBS_FRONTEND_EVENT_PROFILE_CHANGED) {
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

		// Only STARTED, never STARTING. At STARTING the main output has not
		// been started yet, so obs_output_active() on it is still false and
		// every output that borrows a main encoder would be refused. STARTING
		// is also emitted for main starts that then fail straight away, with
		// no stop event afterwards to undo it.
		if (event == OBS_FRONTEND_EVENT_STREAMING_STARTED)
			md->StartOutputsWithMain();
	} else if (event == OBS_FRONTEND_EVENT_STREAMING_STOPPING || event == OBS_FRONTEND_EVENT_STREAMING_STOPPED) {
		md->mainStreamButton->setChecked(false);
		md->outputButtonStyle(md->mainStreamButton);

		// Only STOPPING: it is the earliest notice, and with OBS's stream
		// delay enabled STOPPED is held back by the whole delay, which would
		// leave these outputs running for minutes after the operator pressed
		// stop.
		// Both events, not just STOPPING. libobs only raises "stopping" from
		// obs_output_stop/obs_output_force_stop, so a main stream that drops on
		// its own -- lost connection, server hang-up -- produces STOPPED with
		// no STOPPING at all, and these outputs would carry on streaming after
		// the main one had died. Running twice is harmless: anything stopped at
		// STOPPING has already left the list by then.
		//
		// Note nothing clears pending_auto_stop here. A stop remembered for an
		// output that is still shaking hands has to outlive the main stream
		// going down -- that is the whole case it exists for.
		md->StopOutputsWithMain();
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
		if (!t)
			continue;
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
		if (std::get<std::string>(*it) != nameChars)
			continue;
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
					if (button == QMessageBox::No)
						start = false;
				}
				if (!start || !proc_handler_call(ph, "aitum_vertical_start_stream_output", &cd))
					streamButton->setChecked(false);
			} else {
				bool stop = true;
				bool warnBeforeStreamStop = config_get_bool(config, "BasicWindow", "WarnBeforeStoppingStream");
				if (warnBeforeStreamStop && isVisible()) {
					auto button = QMessageBox::question(
						this, QString::fromUtf8(obs_frontend_get_locale_string("ConfirmStop.Title")),
						QString::fromUtf8(obs_frontend_get_locale_string("ConfirmStop.Text")),
						QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
					if (button == QMessageBox::No)
						stop = false;
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
				if (!StartOutput(output_data, streamButton))
					streamButton->setChecked(false);
			} else {
				bool stop = true;
				bool warnBeforeStreamStop =
					config_get_bool(get_user_config(), "BasicWindow", "WarnBeforeStoppingStream");
				if (warnBeforeStreamStop && isVisible()) {
					auto button = QMessageBox::question(
						this, QString::fromUtf8(obs_frontend_get_locale_string("ConfirmStop.Title")),
						QString::fromUtf8(obs_frontend_get_locale_string("ConfirmStop.Text")),
						QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
					if (button == QMessageBox::No)
						stop = false;
				}
				if (stop) {
					blog(LOG_INFO, "[Aitum Multistream] stop stream clicked '%s'",
					     obs_data_get_string(output_data, "name"));
					const char *name2 = obs_data_get_string(output_data, "name");
					for (auto it = outputs.begin(); it != outputs.end(); it++) {
						if (std::get<std::string>(*it) != name2)
							continue;

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

	if (vertical)
		verticalCanvasOutputLayout->addWidget(streamGroup);
	else
		mainCanvasOutputLayout->addWidget(streamGroup);
}

static void ensure_directory(char *path)
{
#ifdef _WIN32
	char *backslash = strrchr(path, '\\');
	if (backslash)
		*backslash = '/';
#endif

	char *slash = strrchr(path, '/');
	if (slash) {
		*slash = 0;
		os_mkdirs(path);
		*slash = '/';
	}

#ifdef _WIN32
	if (backslash)
		*backslash = '\\';
#endif
}

void MultistreamDock::SaveSettings()
{
	char *path = obs_module_config_path("config.json");
	if (!path)
		return;
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
			if (!t)
				continue;
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
	if (current_config)
		obs_data_apply(pd, current_config);
	obs_data_release(pd);

	if (obs_data_save_json_safe(config, path, "tmp", "bak")) {
		blog(LOG_INFO, "[Aitum Multistream] Saved settings");
	} else {
		blog(LOG_ERROR, "[Aitum Multistream] Failed saving settings");
	}
	obs_data_release(config);
	bfree(path);
}

// ---- Following the main output -------------------------------------------
// Both of these run on the Qt UI thread, from frontend_event.

// Starts every output configured with "start with main output" that is not
// already running or connecting.
void MultistreamDock::StartOutputsWithMain()
{
	if (!current_config)
		return;
	pending_auto_stop.clear();
	auto outputs2 = obs_data_get_array(current_config, "outputs");
	if (!outputs2)
		return;
	auto count = obs_data_array_count(outputs2);
	for (size_t i = 0; i < count; i++) {
		auto item = obs_data_array_item(outputs2, i);
		if (!item)
			continue;
		auto name = obs_data_get_string(item, "name");
		if (!obs_data_get_bool(item, "start_w_main") || !name || !*name) {
			obs_data_release(item);
			continue;
		}
		// An entry in `outputs` means an output object already exists for this
		// name, so it is running or still connecting. Leave it alone: starting
		// it again would tear down what the user already had going. Note that
		// obs_output_active() is not usable as that test, because it stays
		// false for the whole connect phase.
		bool busy = false;
		for (auto it = outputs.begin(); it != outputs.end(); it++) {
			if (std::get<std::string>(*it) == name) {
				busy = true;
				break;
			}
		}
		if (busy) {
			blog(LOG_INFO, "[Aitum Multistream] '%s' is already running, not starting it with the main output",
			     name);
			obs_data_release(item);
			continue;
		}
		QPushButton *streamButton = nullptr;
		for (int j = 1; j < mainCanvasOutputLayout->count(); j++) {
			auto layoutItem = mainCanvasOutputLayout->itemAt(j);
			if (layoutItem && layoutItem->widget() &&
			    layoutItem->widget()->objectName() == QString::fromUtf8(name)) {
				streamButton = layoutItem->widget()->findChild<QPushButton *>(QStringLiteral("canvasStream"));
				break;
			}
		}
		if (!streamButton) {
			blog(LOG_WARNING, "[Aitum Multistream] no dock row for '%s', not starting it with the main output",
			     name);
			obs_data_release(item);
			continue;
		}
		blog(LOG_INFO, "[Aitum Multistream] starting stream '%s' with the main output", name);
		bool started = StartOutput(item, streamButton, true);
		streamButton->setChecked(started);
		outputButtonStyle(streamButton);
		obs_data_release(item);
	}
	obs_data_array_release(outputs2);
}

// Stops every output configured with "stop with main output".
void MultistreamDock::StopOutputsWithMain()
{
	if (!current_config)
		return;
	auto outputs2 = obs_data_get_array(current_config, "outputs");
	if (!outputs2)
		return;
	auto count = obs_data_array_count(outputs2);
	for (size_t i = 0; i < count; i++) {
		auto item = obs_data_array_item(outputs2, i);
		if (!item)
			continue;
		auto name = obs_data_get_string(item, "name");
		if (!obs_data_get_bool(item, "stop_w_main") || !name || !*name) {
			obs_data_release(item);
			continue;
		}
		for (auto it = outputs.begin(); it != outputs.end(); it++) {
			if (std::get<std::string>(*it) != name)
				continue;
			auto output = std::get<obs_output_t *>(*it);
			if (obs_output_active(output) || obs_output_reconnecting(output)) {
				blog(LOG_INFO, "[Aitum Multistream] stopping stream '%s' with the main output", name);
				StopOutput(output);
			} else {
				// Still connecting. obs_output_stop() does nothing for an
				// output that is neither active nor reconnecting, so asking
				// now would be silently dropped and the output would go live
				// after the main stream had already gone down. Remember it and
				// stop it as soon as it reports that it started.
				blog(LOG_INFO,
				     "[Aitum Multistream] '%s' is still connecting, it will be stopped as soon as it starts",
				     name);
				pending_auto_stop.insert(name);
			}
			break;
		}
		obs_data_release(item);
	}
	obs_data_array_release(outputs2);
}

bool MultistreamDock::StartOutput(obs_data_t *settings, QPushButton *streamButton, bool automatic)
{
	if (!settings)
		return false;

	bool warnBeforeStreamStart = config_get_bool(get_user_config(), "BasicWindow", "WarnBeforeStartingStream");
	if (!automatic && warnBeforeStreamStart && isVisible()) {
		auto button = QMessageBox::question(this, QString::fromUtf8(obs_frontend_get_locale_string("ConfirmStart.Title")),
						    QString::fromUtf8(obs_frontend_get_locale_string("ConfirmStart.Text")),
						    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
		if (button == QMessageBox::No)
			return false;
	}

	// Every failure below is reported through this. An automatic start shows
	// nothing: there may be nobody at the machine to dismiss it, and one modal
	// per configured output would wedge OBS entirely. The log line is written
	// either way, since on an automatic start it is the only feedback there is.
	auto reportFailure = [this, automatic](const char *locale_key) {
		if (automatic)
			return;
		auto text = QString::fromUtf8(obs_module_text(locale_key));
		QMessageBox::warning(this, text, text);
	};

	const char *name = obs_data_get_string(settings, "name");
	// Starting this name supersedes any stop remembered for it while it was
	// connecting, whoever asked for the start. Without this the deferred stop
	// would fire against whatever output holds the name later and kill it.
	pending_auto_stop.erase(name);
	// Replacing an output that is still in the list. The order here matters and
	// the previous order could free the output twice.
	//
	// stream_output_stop() also releases the output and erases its entry, and
	// obs_output_force_stop() raises "stop" synchronously on this very thread,
	// so the old code re-entered its own cleanup half way through and then
	// carried on with an iterator into a vector that had been modified.
	//
	// So: unhook the signals first, so the handler cannot run at all; take the
	// entry out of the vector before anything that can re-enter; and only then
	// release. The disconnects are unconditional -- gating them on
	// obs_output_active() misses the whole RTMP connect phase, where the output
	// is not yet active but its signals are already live.
	obs_output_t *previous = nullptr;
	for (auto it = outputs.begin(); it != outputs.end(); it++) {
		if (std::get<std::string>(*it) != name)
			continue;
		previous = std::get<obs_output_t *>(*it);
		signal_handler_t *previous_signal = obs_output_get_signal_handler(previous);
		signal_handler_disconnect(previous_signal, "start", stream_output_start, this);
		signal_handler_disconnect(previous_signal, "stop", stream_output_stop, this);
		outputs.erase(it);
		break;
	}
	if (previous) {
		auto service = obs_output_get_service(previous);
		if (obs_output_active(previous))
			obs_output_force_stop(previous);
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
				reportFailure("MainOutputNotActive");
				return false;
			}
			auto vei = (int)obs_data_get_int(settings, "video_encoder_index");
			venc = obs_output_get_video_encoder2(main_output, vei);
			obs_output_release(main_output);
			if (!venc) {
				blog(LOG_WARNING,
				     "[Aitum Multistream] failed to start stream '%s' because encoder index %d was not found",
				     obs_data_get_string(settings, "name"), vei);
				reportFailure("MainOutputEncoderIndexNotFound");
				return false;
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
			if (divisor > 1)
				obs_encoder_set_frame_rate_divisor(venc, (uint32_t)divisor);

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
				reportFailure("MainOutputNotActive");
				return false;
			}
			auto aei = (int)obs_data_get_int(settings, "audio_encoder_index");
			aenc = obs_output_get_audio_encoder(main_output, aei);
			obs_output_release(main_output);
			if (!aenc) {
				blog(LOG_WARNING,
				     "[Aitum Multistream] failed to start stream '%s' because encoder index %d was not found",
				     obs_data_get_string(settings, "name"), aei);
				reportFailure("MainOutputEncoderIndexNotFound");
				return false;
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
			reportFailure("MainOutputNotActive");
			return false;
		}

		aenc = obs_output_get_audio_encoder(main_output, 0);
		obs_output_release(main_output);
	}
	if (!aenc || !venc) {
		return false;
	}
	auto server = obs_data_get_string(settings, "stream_server");
	if (!server || !strlen(server)) {
		server = obs_data_get_string(settings, "server");
		if (server && strlen(server))
			obs_data_set_string(settings, "stream_server", server);
	}
	bool whip = strstr(server, "whip") != nullptr;
	auto s = obs_data_create();
	obs_data_set_string(s, "server", server);
	auto key = obs_data_get_string(settings, "stream_key");
	if (!key || !strlen(key)) {
		key = obs_data_get_string(settings, "key");
		if (key && strlen(key))
			obs_data_set_string(settings, "stream_key", key);
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

	outputs.push_back({obs_data_get_string(settings, "name"), output, streamButton});

	if (!obs_output_start(output)) {
		// A start that fails here never raises "stop", so nothing else will
		// clean up after it. Left in the list the name would look permanently
		// busy, and an output configured to start with the main output would
		// never be started again.
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
		return false;
	}

	return true;
}

void MultistreamDock::stream_output_start(void *data, calldata_t *calldata)
{
	auto md = (MultistreamDock *)data;
	auto output = obs_output_get_ref((obs_output_t *)calldata_ptr(calldata, "output"));
	if (!output)
		return;
	// Raised on the output's own thread. Everything below belongs to the UI
	// thread -- the outputs list, the buttons, pending_auto_stop -- so hand the
	// whole job over rather than doing any of it here. The reference keeps the
	// output alive for the trip.
	QMetaObject::invokeMethod(
		md,
		[md, output] {
			for (auto it = md->outputs.begin(); it != md->outputs.end(); it++) {
				if (std::get<obs_output_t *>(*it) != output)
					continue;
				auto button = std::get<QPushButton *>(*it);
				if (button && !button->isChecked()) {
					button->setChecked(true);
					md->outputButtonStyle(button);
				}
				// If the main output went down while this one was still
				// connecting, the stop asked for then did nothing, because
				// obs_output_stop() ignores an output that is neither active
				// nor reconnecting. It has started now, so it can be stopped.
				auto name = std::get<std::string>(*it);
				if (md->pending_auto_stop.erase(name)) {
					blog(LOG_INFO,
					     "[Aitum Multistream] stopping '%s' now it has connected, main already stopped",
					     name.c_str());
					StopOutput(output);
				}
				break;
			}
			obs_output_release(output);
		},
		Qt::QueuedConnection);
}

void MultistreamDock::stream_output_stop(void *data, calldata_t *calldata)
{
	auto md = (MultistreamDock *)data;
	auto output = obs_output_get_ref((obs_output_t *)calldata_ptr(calldata, "output"));
	if (!output)
		return;
	// Same as above: raised on the output's thread, all of the work is the UI
	// thread's. Doing the release and the erase here is what used to collide
	// with StartOutput tearing the same entry down.
	QMetaObject::invokeMethod(
		md,
		[md, output] {
			for (auto it = md->outputs.begin(); it != md->outputs.end(); it++) {
				if (std::get<obs_output_t *>(*it) != output)
					continue;
				auto button = std::get<QPushButton *>(*it);
				if (button && button->isChecked()) {
					button->setChecked(false);
					md->outputButtonStyle(button);
				}
				md->pending_auto_stop.erase(std::get<std::string>(*it));
				if (!md->exiting)
					obs_output_release(output);
				md->outputs.erase(it);
				break;
			}
			obs_output_release(output);
		},
		Qt::QueuedConnection);
}

void MultistreamDock::ApiInfo(QString info)
{
	auto d = obs_data_create_from_json(info.toUtf8().constData());
	if (!d)
		return;
	auto data_obj = obs_data_get_obj(d, "data");
	obs_data_release(d);
	if (!data_obj)
		return;
	auto version = obs_data_get_string(data_obj, "version");
	int major;
	int minor;
	int patch;
	if (sscanf(version, "%d.%d.%d", &major, &minor, &patch) == 3) {
		auto sv = MAKE_SEMANTIC_VERSION(major, minor, patch);
		if (sv > MAKE_SEMANTIC_VERSION(PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_VERSION_PATCH)) {
			newer_version_available = QString::fromUtf8(version);
			configButton->setStyleSheet(QString::fromUtf8("background: rgb(192,128,0);"));
			if (finished_loading)
				AskUpdate();
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

	if (vertical_outputs)
		obs_data_array_release(vertical_outputs);
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

void MultistreamDock::storeMainStreamEncoders()
{
	if (!current_config)
		return;
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
			if (divisor > 0)
				mainEncoderDescription +=
					QString::fromUtf8(" ") + QString::number(fps / divisor, 'g', 4) + QString::fromUtf8("fps");

			auto settings = obs_encoder_get_settings(encoder);
			auto bitrate = settings ? obs_data_get_int(settings, "bitrate") : 0;
			if (bitrate > 0)
				mainEncoderDescription +=
					QString::fromUtf8(" ") + QString::number(bitrate) + QString::fromUtf8("Kbps");
			obs_data_release(settings);

			obs_data_set_string(current_config, settingName.toUtf8().constData(),
					    mainEncoderDescription.toUtf8().constData());

		} else if (found && obs_data_has_user_value(current_config, settingName.toUtf8().constData())) {
			obs_data_unset_user_value(current_config, settingName.toUtf8().constData());
		}
	}
	obs_output_release(output);
}

void MultistreamDock::AskUpdate() {
	auto parts = newer_version_available.split(".");
	if (parts.count() < 3)
		return;
	int major = parts.value(0).toInt();
	int minor = parts.value(1).toInt();
	int patch = parts.value(2).toInt();
	auto sv = MAKE_SEMANTIC_VERSION(major, minor, patch);


	char *path = obs_module_config_path("config.json");
	if (!path)
		return;
	
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
		QDesktopServices::openUrl(QUrl(QString::fromUtf8("https://aitum.tv/download/multi/")));
	} else if (mb.clickedButton() == skip) {
		if (!config)
			config = obs_data_create();
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
	if (!pix.isNull())
		QLabel::setPixmap(scaledPixmap());
}
