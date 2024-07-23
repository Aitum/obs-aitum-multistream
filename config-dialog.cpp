#include "config-dialog.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTextEdit>
#include <QRadioButton>
#include <QPlainTextEdit>
#include <QCompleter>
#include <QDesktopServices>
#include <QUrl>
#include <QIcon>
#include <QTabWidget>

#include "obs-module.h"
#include "version.h"
#include <util/dstr.h>

#include <sstream>
#include <util/platform.h>
#include "output-dialog.hpp"

template<typename T> std::string to_string_with_precision(const T a_value, const int n = 6)
{
	std::ostringstream out;
	out.precision(n);
	out << std::fixed << a_value;
	return std::move(out).str();
}

void RemoveWidget(QWidget *widget);
void RemoveLayoutItem(QLayoutItem *item);

// Platform icons deciphered from endpoints
QIcon OBSBasicSettings::getPlatformFromEndpoint(QString endpoint)
{

	if (endpoint.contains(QString::fromUtf8(".contribute.live-video.net")) ||
	    endpoint.contains(QString::fromUtf8(".twitch.tv"))) { // twitch
		return platformIconTwitch;
	} else if (endpoint.contains(QString::fromUtf8(".youtube.com"))) { // youtube
		return platformIconYouTube;
	} else if (endpoint.contains(QString::fromUtf8("fa723fc1b171.global-contribute.live-video.net"))) { // kick
		return platformIconKick;
	} else if (endpoint.contains(QString::fromUtf8(".tiktokcdn-"))) { // tiktok
		return platformIconTikTok;
	} else if (endpoint.contains(QString::fromUtf8(".pscp.tv"))) { // twitter
		return platformIconTwitter;
	} else if (endpoint.contains(QString::fromUtf8("livepush.trovo.live"))) { // trovo
		return platformIconTrovo;
	} else if (endpoint.contains(QString::fromUtf8(".facebook.com"))) { // facebook
		return platformIconFacebook;
	} else { // unknown
		return platformIconUnknown;
	}
}

OBSBasicSettings::OBSBasicSettings(QMainWindow *parent) : QDialog(parent)
{
	setMinimumWidth(983);
	setMinimumHeight(480);
	setWindowTitle(obs_module_text("AitumMultistreamSettings"));
	setSizeGripEnabled(true);

	const auto main_window = static_cast<QMainWindow *>(obs_frontend_get_main_window());

	listWidget = new QListWidget(this);

	listWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
	listWidget->setMaximumWidth(180);
	QListWidgetItem *listwidgetitem = new QListWidgetItem(listWidget);
	listwidgetitem->setIcon(QIcon(QString::fromUtf8(":/settings/images/settings/general.svg")));
	//listwidgetitem->setProperty("themeID", QVariant(QString::fromUtf8("configIconSmall")));
	//cogsIcon
	listwidgetitem->setText(QString::fromUtf8(obs_module_text("General")));

	listwidgetitem = new QListWidgetItem(listWidget);
	listwidgetitem->setIcon(QIcon(QString::fromUtf8(":/settings/images/settings/stream.svg")));
	listwidgetitem->setText(QString::fromUtf8(obs_module_text("MainCanvas")));

	listwidgetitem = new QListWidgetItem(listWidget);
	listwidgetitem->setIcon(QIcon(QString::fromUtf8(":/settings/images/settings/stream.svg")));
	listwidgetitem->setText(QString::fromUtf8(obs_module_text("VerticalCanvas")));

	listwidgetitem = new QListWidgetItem(listWidget);
	listwidgetitem->setIcon(main_window->property("defaultIcon").value<QIcon>());
	listwidgetitem->setText(QString::fromUtf8(obs_module_text("SetupTroubleshooter")));

	listwidgetitem = new QListWidgetItem(listWidget);
	listwidgetitem->setIcon(main_window->property("defaultIcon").value<QIcon>());
	listwidgetitem->setText(QString::fromUtf8(obs_module_text("Help")));

	listWidget->setCurrentRow(0);
	listWidget->setSpacing(1);

	auto settingsPages = new QStackedWidget;
	settingsPages->setContentsMargins(0, 0, 0, 0);
	settingsPages->setFrameShape(QFrame::NoFrame);
	settingsPages->setLineWidth(0);

	QWidget *generalPage = new QWidget;
	QScrollArea *scrollArea = new QScrollArea;
	scrollArea->setWidget(generalPage);
	scrollArea->setWidgetResizable(true);
	scrollArea->setLineWidth(0);
	scrollArea->setFrameShape(QFrame::NoFrame);
	settingsPages->addWidget(scrollArea);

	auto mainOutputsPage = new QGroupBox;
	mainOutputsPage->setProperty("customTitle", QVariant(true));
	mainOutputsPage->setStyleSheet(QString("QGroupBox[customTitle=\"true\"]{ padding-top: 4px;}"));
	mainOutputsPage->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

	scrollArea = new QScrollArea;
	scrollArea->setWidget(mainOutputsPage);
	scrollArea->setWidgetResizable(true);
	scrollArea->setLineWidth(0);
	scrollArea->setFrameShape(QFrame::NoFrame);
	settingsPages->addWidget(scrollArea);

	auto verticalOutputsPage = new QGroupBox;
	verticalOutputsPage->setProperty("customTitle", QVariant(true));
	verticalOutputsPage->setStyleSheet(QString("QGroupBox[customTitle=\"true\"]{ padding-top: 4px;}"));
	verticalOutputsPage->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

	scrollArea = new QScrollArea;
	scrollArea->setWidget(verticalOutputsPage);
	scrollArea->setWidgetResizable(true);
	scrollArea->setLineWidth(0);
	scrollArea->setFrameShape(QFrame::NoFrame);
	settingsPages->addWidget(scrollArea);

	troubleshooterText = new QTextEdit;
	troubleshooterText->setReadOnly(true);

	scrollArea = new QScrollArea;
	scrollArea->setWidget(troubleshooterText);
	scrollArea->setWidgetResizable(true);
	scrollArea->setLineWidth(0);
	scrollArea->setFrameShape(QFrame::NoFrame);
	settingsPages->addWidget(scrollArea);

	auto helpPage = new QWidget;
	scrollArea = new QScrollArea;
	scrollArea->setWidget(helpPage);
	scrollArea->setWidgetResizable(true);
	scrollArea->setLineWidth(0);
	scrollArea->setFrameShape(QFrame::NoFrame);
	settingsPages->addWidget(scrollArea);

	//mainOutputsPage

	mainOutputsLayout = new QFormLayout;
	mainOutputsLayout->setContentsMargins(9, 2, 9, 9);
	mainOutputsLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	mainOutputsLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);

	auto streaming_title_layout = new QHBoxLayout;
	auto streaming_title = new QLabel(QString::fromUtf8(obs_module_text("MainCanvas")));
	streaming_title->setStyleSheet(QString::fromUtf8("font-weight: bold;"));
	streaming_title_layout->addWidget(streaming_title, 0, Qt::AlignLeft);
	//auto guide_link = new QLabel(QString::fromUtf8("<a href=\"https://l.aitum.tv/vh-streaming-settings\">") + QString::fromUtf8(obs_module_text("ViewGuide")) + QString::fromUtf8("</a>"));
	//guide_link->setOpenExternalLinks(true);

	auto addButton = new QPushButton(QIcon(":/res/images/plus.svg"), QString::fromUtf8(obs_module_text("AddOutput")));
	addButton->setProperty("themeID", QVariant(QString::fromUtf8("addIconSmall")));

	connect(addButton, &QPushButton::clicked, [this] {
		auto outputDialog = new OutputDialog(this);

		outputDialog->setWindowModality(Qt::WindowModal);
		outputDialog->setModal(true);

		if (outputDialog->exec() == QDialog::Accepted) {
			// create a new output
			if (!settings)
				return;
			auto outputs = obs_data_get_array(settings, "outputs");
			if (!outputs) {
				outputs = obs_data_array_create();
				obs_data_set_array(settings, "outputs", outputs);
			}
			auto s = obs_data_create();

			// Set the info from the output dialog
			obs_data_set_string(s, "name", outputDialog->outputName.toUtf8().constData());
			obs_data_set_string(s, "stream_server", outputDialog->outputServer.toUtf8().constData());
			obs_data_set_string(s, "stream_key", outputDialog->outputKey.toUtf8().constData());

			obs_data_array_push_back(outputs, s);
			AddServer(mainOutputsLayout, s, outputs);
			obs_data_array_release(outputs);
			obs_data_release(s);
		}

		delete outputDialog;
	});

	//streaming_title_layout->addWidget(guide_link, 0, Qt::AlignRight);
	streaming_title_layout->addWidget(addButton, 0, Qt::AlignRight);

	mainOutputsLayout->addRow(streaming_title_layout);

	auto serverGroup = new QGroupBox;
	serverGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
	serverGroup->setStyleSheet(QString("QGroupBox{background-color: %1; padding-top: 4px;}")
					   .arg(palette().color(QPalette::ColorRole::Mid).name(QColor::HexRgb)));

	auto serverLayout = new QFormLayout;
	serverLayout->setContentsMargins(9, 2, 9, 9);
	serverLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	serverLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);

	auto mainTitle = new QLabel(QString::fromUtf8(obs_module_text("SettingsMainCanvasTitle")));
	mainTitle->setStyleSheet(QString::fromUtf8("font-weight: bold;"));
	serverLayout->addRow(mainTitle);

	auto mainDescription = new QLabel(QString::fromUtf8(obs_module_text("SettingsMainCanvasDescription")));
	//	mainTitle->setStyleSheet(QString::fromUtf8("font-weight: bold;"));
	serverLayout->addRow(mainDescription);

	serverGroup->setLayout(serverLayout);

	mainOutputsLayout->addRow(serverGroup);

	mainOutputsPage->setLayout(mainOutputsLayout);

	verticalOutputsLayout = new QFormLayout;
	verticalOutputsLayout->setContentsMargins(9, 2, 9, 9);
	verticalOutputsLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	verticalOutputsLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);

	streaming_title_layout = new QHBoxLayout;
	streaming_title = new QLabel(QString::fromUtf8(obs_module_text("VerticalCanvas")));
	streaming_title->setStyleSheet(QString::fromUtf8("font-weight: bold;"));
	streaming_title_layout->addWidget(streaming_title, 0, Qt::AlignLeft);
	//auto guide_link = new QLabel(QString::fromUtf8("<a href=\"https://l.aitum.tv/vh-streaming-settings\">") + QString::fromUtf8(obs_module_text("ViewGuide")) + QString::fromUtf8("</a>"));
	//guide_link->setOpenExternalLinks(true);
	//	addButton = new QPushButton(QIcon(":/res/images/plus.svg"), QString::fromUtf8(obs_module_text("AddOutput")));
	//	addButton->setProperty("themeID", QVariant(QString::fromUtf8("addIconSmall")));
	//	connect(addButton, &QPushButton::clicked, [this] {
	//		if (!vertical_outputs)
	//			return;
	//		auto s = obs_data_create();
	//		obs_data_set_string(s, "name", obs_module_text("Unnamed"));
	//		obs_data_array_push_back(vertical_outputs, s);
	//		AddServer(verticalOutputsLayout, s);
	//		obs_data_release(s);
	//	});

	verticalAddButton = new QPushButton(QIcon(":/res/images/plus.svg"), QString::fromUtf8(obs_module_text("AddOutput")));
	verticalAddButton->setProperty("themeID", QVariant(QString::fromUtf8("addIconSmall")));

	connect(verticalAddButton, &QPushButton::clicked, [this] {
		auto outputDialog = new OutputDialog(this);

		outputDialog->setWindowModality(Qt::WindowModal);
		outputDialog->setModal(true);

		if (outputDialog->exec() == QDialog::Accepted) {
			// create a new output
			if (!vertical_outputs)
				return;
			auto s = obs_data_create();
			obs_data_set_string(s, "name", outputDialog->outputName.toUtf8().constData());
			obs_data_set_string(s, "stream_server", outputDialog->outputServer.toUtf8().constData());
			obs_data_set_string(s, "stream_key", outputDialog->outputKey.toUtf8().constData());
			obs_data_array_push_back(vertical_outputs, s);
			AddServer(verticalOutputsLayout, s, vertical_outputs);
			obs_data_release(s);
		}

		delete outputDialog;
	});

	streaming_title_layout->addWidget(verticalAddButton, 0, Qt::AlignRight);

	verticalOutputsLayout->addRow(streaming_title_layout);

	verticalOutputsPage->setLayout(verticalOutputsLayout);

	const auto version =
		new QLabel(QString::fromUtf8(obs_module_text("Version")) + " " + QString::fromUtf8(PROJECT_VERSION) + " " +
			   QString::fromUtf8(obs_module_text("MadeBy")) + " <a href=\"https://aitum.tv\">Aitum</a>");
	version->setOpenExternalLinks(true);
	version->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

	newVersion = new QLabel;
	newVersion->setProperty("themeID", "warning");
	newVersion->setVisible(false);
	newVersion->setOpenExternalLinks(true);
	newVersion->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

	QPushButton *okButton = new QPushButton(QString::fromUtf8(obs_frontend_get_locale_string("OK")));
	connect(okButton, &QPushButton::clicked, [this] { accept(); });

	QPushButton *cancelButton = new QPushButton(QString::fromUtf8(obs_frontend_get_locale_string("Cancel")));
	connect(cancelButton, &QPushButton::clicked, [this] { reject(); });

	QHBoxLayout *bottomLayout = new QHBoxLayout;
	bottomLayout->addWidget(version, 1, Qt::AlignLeft);
	bottomLayout->addWidget(newVersion, 1, Qt::AlignLeft);
	bottomLayout->addWidget(cancelButton, 0, Qt::AlignRight);
	bottomLayout->addWidget(okButton, 0, Qt::AlignRight);

	QHBoxLayout *contentLayout = new QHBoxLayout;
	contentLayout->addWidget(listWidget);

	contentLayout->addWidget(settingsPages, 1);

	listWidget->connect(listWidget, &QListWidget::currentRowChanged, settingsPages, &QStackedWidget::setCurrentIndex);
	listWidget->setCurrentRow(1);

	QVBoxLayout *vlayout = new QVBoxLayout;
	vlayout->setContentsMargins(11, 11, 11, 11);
	vlayout->addLayout(contentLayout);
	vlayout->addLayout(bottomLayout);
	setLayout(vlayout);
}

OBSBasicSettings::~OBSBasicSettings()
{
	if (vertical_outputs)
		obs_data_array_release(vertical_outputs);
	for (auto it = encoder_properties.begin(); it != encoder_properties.end(); it++)
		obs_properties_destroy(it->second);
}

QIcon OBSBasicSettings::GetGeneralIcon() const
{
	return listWidget->item(0)->icon();
}

QIcon OBSBasicSettings::GetStreamIcon() const
{
	return listWidget->item(1)->icon();
}

QIcon OBSBasicSettings::GetOutputIcon() const
{
	return QIcon();
}

QIcon OBSBasicSettings::GetAudioIcon() const
{
	return QIcon();
}

QIcon OBSBasicSettings::GetVideoIcon() const
{
	return QIcon();
}

QIcon OBSBasicSettings::GetHotkeysIcon() const
{
	return QIcon();
}

QIcon OBSBasicSettings::GetAccessibilityIcon() const
{
	return QIcon();
}

QIcon OBSBasicSettings::GetAdvancedIcon() const
{
	return QIcon();
}

void OBSBasicSettings::SetGeneralIcon(const QIcon &icon)
{
	listWidget->item(0)->setIcon(icon);
}

void OBSBasicSettings::SetStreamIcon(const QIcon &icon)
{
	listWidget->item(1)->setIcon(icon);
	listWidget->item(2)->setIcon(icon);
}

void OBSBasicSettings::SetOutputIcon(const QIcon &icon)
{
	UNUSED_PARAMETER(icon);
	//listWidget->item(2)->setIcon(icon);
}

void OBSBasicSettings::SetAudioIcon(const QIcon &icon)
{
	UNUSED_PARAMETER(icon);
}

void OBSBasicSettings::SetVideoIcon(const QIcon &icon)
{
	UNUSED_PARAMETER(icon);
}

void OBSBasicSettings::SetHotkeysIcon(const QIcon &icon)
{
	UNUSED_PARAMETER(icon);
}

void OBSBasicSettings::SetAccessibilityIcon(const QIcon &icon)
{
	UNUSED_PARAMETER(icon);
}

void OBSBasicSettings::SetAdvancedIcon(const QIcon &icon)
{
	UNUSED_PARAMETER(icon);
}

void OBSBasicSettings::AddServer(QFormLayout *outputsLayout, obs_data_t *settings, obs_data_array_t *outputs)
{
	auto serverGroup = new QGroupBox;
	serverGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
	serverGroup->setProperty("altColor", QVariant(true));
	serverGroup->setProperty("customTitle", QVariant(true));
	serverGroup->setStyleSheet(
		QString("QGroupBox[altColor=\"true\"]{background-color: %1;} QGroupBox[customTitle=\"true\"]{padding-top: 4px;}")
			.arg(palette().color(QPalette::ColorRole::Mid).name(QColor::HexRgb)));

	auto serverLayout = new QFormLayout;
	serverLayout->setContentsMargins(9, 2, 9, 2);

	serverLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	serverLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);

	// Title
	auto server_title_layout = new QHBoxLayout;

	auto platformIconLabel = new QLabel;
	auto platformIcon = getPlatformFromEndpoint(QString::fromUtf8(obs_data_get_string(settings, "stream_server")));
	platformIconLabel->setPixmap(platformIcon.pixmap(30, 30));
	server_title_layout->addWidget(platformIconLabel, 0);

	auto streaming_title = new QLabel(QString::fromUtf8(obs_data_get_string(settings, "name")));
	streaming_title->setStyleSheet(QString::fromUtf8("font-weight: bold;"));
	server_title_layout->addWidget(streaming_title, 1, Qt::AlignLeft);

	// Config Button
	auto configButton = new QPushButton;
	configButton->setMinimumHeight(30);

	// Advanced settings
	const bool advanced = obs_data_get_bool(settings, "advanced");
	auto advancedGroup = new QGroupBox(QString::fromUtf8(obs_module_text("AdvancedGroupHeader")));
	advancedGroup->setContentsMargins(0, 4, 0, 0);
	
	advancedGroup->setStyleSheet("QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top right; padding: 12px 18px 0 0; }"
								 "QGroupBox { padding-top: 4px; padding-bottom: 0;}");
	advancedGroup->setVisible(advanced);

	auto advancedGroupLayout = new QVBoxLayout;
	advancedGroup->setLayout(advancedGroupLayout);
	
	// Tab widget
	// 1 = bg for active tab + pane, 2 = inactive tabs, 3 = tab text colour, 4 = border colour for pane
	auto tabStyles = QString("QTabWidget::pane {  background: %1; border-bottom-left-radius: 4px; border-bottom-right-radius: 4px; border-top-right-radius: 4px; margin-top: -1px; padding-top: 8px; border: 1px solid %4; } QTabWidget::tab-bar { margin-bottom: 0; padding-bottom: 0; border-color: %4; } QTabBar::tab { color: %3; padding: 10px; margin-bottom: 0; border: 1px solid %4; } QTabBar::tab:selected { background: %1; font-weight: bold; border-bottom: none; } QTabBar::tab:!selected { background: %2; }")
		.arg(palette().color(QPalette::ColorRole::Mid).name(QColor::HexRgb), palette().color(QPalette::ColorRole::Light).name(QColor::HexRgb), palette().color(QPalette::ColorRole::Text).name(QColor::HexRgb), palette().color(QPalette::ColorRole::Light).name(QColor::HexRgb));
	
	auto advancedTabWidget = new QTabWidget;
	advancedTabWidget->setContentsMargins(0, 0, 0, 0);
	advancedTabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	advancedTabWidget->setStyleSheet(tabStyles);
//	advancedTabWidget->setStyleSheet("QTabWidget::tab-bar { border: 1px solid gray; }"
//									 "QTabBar::tab { background: gray; color: white; padding: 10px; }"
//									 "QTabBar::tab:selected { background: lightgray; }"
//									 "QTabWidget::pane { border: none; background: pink; }");
	
//	auto pageStyle = QString("QWidget[page=\"true\"] { border: 1px solid %1; padding-top: 0; margin-top: 0; }")
//					.arg(QPalette().color(QPalette::ColorRole::Mid).name(QColor::HexRgb));
//	
	auto videoPage = new QWidget;
	videoPage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
//	videoPage->setStyleSheet(pageStyle);
//	videoPage->setProperty("page", true);
	auto videoPageLayout = new QFormLayout;
	videoPage->setLayout(videoPageLayout);
	
	auto audioPage = new QWidget;
	audioPage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
//	audioPage->setStyleSheet(pageStyle);
//	audioPage->setProperty("page", true);
	auto audioPageLayout = new QFormLayout;
	audioPage->setLayout(audioPageLayout);
	
	
	// VIDEO ENCODER
	auto videoEncoder = new QComboBox;
	videoEncoder->addItem(QString::fromUtf8(obs_module_text("MainEncoder")), QVariant(QString::fromUtf8("")));
	videoEncoder->setCurrentIndex(0);
	videoPageLayout->addRow(QString::fromUtf8(obs_module_text("VideoEncoder")), videoEncoder);

	auto videoEncoderIndex = new QComboBox;
	for (int i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
		videoEncoderIndex->addItem(QString::number(i + 1));
	}
	videoEncoderIndex->setCurrentIndex(obs_data_get_int(settings, "video_encoder_index"));
	connect(videoEncoderIndex, &QComboBox::currentIndexChanged, [videoEncoderIndex, settings] {
		if (videoEncoderIndex->currentIndex() >= 0)
			obs_data_set_int(settings, "video_encoder_index", videoEncoderIndex->currentIndex());
	});
	videoPageLayout->addRow(QString::fromUtf8(obs_module_text("VideoEncoderIndex")), videoEncoderIndex);

	auto videoEncoderGroup = new QGroupBox(QString::fromUtf8(obs_module_text("VideoEncoder")));
	videoEncoderGroup->setProperty("altColor", QVariant(true));
	auto videoEncoderGroupLayout = new QFormLayout();
	videoEncoderGroup->setLayout(videoEncoderGroupLayout);
	videoPageLayout->addRow(videoEncoderGroup);

	connect(videoEncoder, &QComboBox::currentIndexChanged,
		[this, serverGroup, advancedGroupLayout, videoPageLayout, videoEncoder, videoEncoderIndex, videoEncoderGroup,
		 videoEncoderGroupLayout, settings] {
			auto encoder_string = videoEncoder->currentData().toString().toUtf8();
			auto encoder = encoder_string.constData();
			obs_data_set_string(settings, "video_encoder", encoder);
			if (!encoder || encoder[0] == '\0') {
				videoPageLayout->setRowVisible(videoEncoderIndex, true);
				videoEncoderGroup->setVisible(false);
			} else {
				videoPageLayout->setRowVisible(videoEncoderIndex, false);
				videoEncoderGroup->setVisible(true);
				auto t = encoder_properties.find(serverGroup);
				if (t != encoder_properties.end()) {
					obs_properties_destroy(t->second);
					encoder_properties.erase(t);
				}
				for (int i = videoEncoderGroupLayout->rowCount() - 1; i >= 0; i--) {
					videoEncoderGroupLayout->removeRow(i);
				}
				//auto stream_encoder_settings = obs_encoder_defaults(encoder);
				auto ves = obs_data_get_obj(settings, "video_encoder_settings");
				if (!ves) {
					ves = obs_encoder_defaults(encoder);
					obs_data_set_obj(settings, "video_encoder_settings", ves);
				}
				auto stream_encoder_properties = obs_get_encoder_properties(encoder);
				encoder_properties[serverGroup] = stream_encoder_properties;

				obs_property_t *property = obs_properties_first(stream_encoder_properties);
				while (property) {
					AddProperty(stream_encoder_properties, property, ves, videoEncoderGroupLayout);
					obs_property_next(&property);
				}
				obs_data_release(ves);
				//obs_properties_destroy(stream_encoder_properties);
			}
		});

	const char *current_type = obs_data_get_string(settings, "video_encoder");
	const char *type;
	size_t idx = 0;
	while (obs_enum_encoder_types(idx++, &type)) {
		if (obs_get_encoder_type(type) != OBS_ENCODER_VIDEO)
			continue;
		uint32_t caps = obs_get_encoder_caps(type);
		if ((caps & (OBS_ENCODER_CAP_DEPRECATED | OBS_ENCODER_CAP_INTERNAL)) != 0)
			continue;
		const char *codec = obs_get_encoder_codec(type);
		if (astrcmpi(codec, "h264") != 0 && astrcmpi(codec, "hevc") != 0 && astrcmpi(codec, "av1") != 0)
			continue;
		videoEncoder->addItem(QString::fromUtf8(obs_encoder_get_display_name(type)), QVariant(QString::fromUtf8(type)));
		if (strcmp(type, current_type) == 0)
			videoEncoder->setCurrentIndex(videoEncoder->count() - 1);
	}
	videoEncoderGroup->setVisible(advanced && videoEncoder->currentIndex() > 0);

	auto audioEncoder = new QComboBox;
	audioEncoder->addItem(QString::fromUtf8(obs_module_text("MainEncoder")), QVariant(QString::fromUtf8("")));
	audioEncoder->setCurrentIndex(0);
	audioPageLayout->addRow(QString::fromUtf8(obs_module_text("AudioEncoder")), audioEncoder);

	//"audio_track"

	auto audioTrack = new QComboBox;
	for (int i = 0; i < 6; i++) {
		audioTrack->addItem(QString::number(i + 1));
	}
	audioTrack->setCurrentIndex(obs_data_get_int(settings, "audio_track"));
	connect(audioTrack, &QComboBox::currentIndexChanged, [audioTrack, settings] {
		if (audioTrack->currentIndex() >= 0)
			obs_data_set_int(settings, "audio_track", audioTrack->currentIndex());
	});
	audioPageLayout->addRow(QString::fromUtf8(obs_module_text("AudioTrack")), audioTrack);

	auto audioEncoderIndex = new QComboBox;
	for (int i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++) {
		audioEncoderIndex->addItem(QString::number(i + 1));
	}
	audioEncoderIndex->setCurrentIndex(obs_data_get_int(settings, "audio_encoder_index"));
	connect(audioEncoderIndex, &QComboBox::currentIndexChanged, [audioEncoderIndex, settings] {
		if (audioEncoderIndex->currentIndex() >= 0)
			obs_data_set_int(settings, "audio_encoder_index", audioEncoderIndex->currentIndex());
	});
	audioPageLayout->addRow(QString::fromUtf8(obs_module_text("AudioEncoderIndex")), audioEncoderIndex);

	auto audioEncoderGroup = new QGroupBox(QString::fromUtf8(obs_module_text("AudioEncoder")));
	audioEncoderGroup->setProperty("altColor", QVariant(true));
	auto audioEncoderGroupLayout = new QFormLayout();
	audioEncoderGroup->setLayout(audioEncoderGroupLayout);
	audioPageLayout->addRow(audioEncoderGroup);

	connect(audioEncoder, &QComboBox::currentIndexChanged,
		[this, serverGroup, advancedGroupLayout, audioPageLayout, audioEncoder, audioEncoderIndex, audioEncoderGroup,
		 audioEncoderGroupLayout, audioTrack, settings] {
			auto encoder_string = audioEncoder->currentData().toString().toUtf8();
			auto encoder = encoder_string.constData();
			obs_data_set_string(settings, "audio_encoder", encoder);
			if (!encoder || encoder[0] == '\0') {
				audioPageLayout->setRowVisible(audioEncoderIndex, true);
				audioPageLayout->setRowVisible(audioTrack, false);
				audioEncoderGroup->setVisible(false);
			} else {
				audioPageLayout->setRowVisible(audioEncoderIndex, false);
				audioPageLayout->setRowVisible(audioTrack, true);
				audioEncoderGroup->setVisible(true);
				auto t = encoder_properties.find(serverGroup);
				if (t != encoder_properties.end()) {
					obs_properties_destroy(t->second);
					encoder_properties.erase(t);
				}
				for (int i = audioEncoderGroupLayout->rowCount() - 1; i >= 0; i--) {
					audioEncoderGroupLayout->removeRow(i);
				}
				//auto stream_encoder_settings = obs_encoder_defaults(encoder);
				auto aes = obs_data_get_obj(settings, "audio_encoder_settings");
				if (!aes) {
					aes = obs_encoder_defaults(encoder);
					obs_data_set_obj(settings, "audio_encoder_settings", aes);
				}
				auto stream_encoder_properties = obs_get_encoder_properties(encoder);
				encoder_properties[serverGroup] = stream_encoder_properties;

				obs_property_t *property = obs_properties_first(stream_encoder_properties);
				while (property) {
					AddProperty(stream_encoder_properties, property, aes, audioEncoderGroupLayout);
					obs_property_next(&property);
				}
				obs_data_release(aes);
				//obs_properties_destroy(stream_encoder_properties);
			}
		});

	current_type = obs_data_get_string(settings, "audio_encoder");
	idx = 0;
	while (obs_enum_encoder_types(idx++, &type)) {
		if (obs_get_encoder_type(type) != OBS_ENCODER_AUDIO)
			continue;
		uint32_t caps = obs_get_encoder_caps(type);
		if ((caps & (OBS_ENCODER_CAP_DEPRECATED | OBS_ENCODER_CAP_INTERNAL)) != 0)
			continue;
		const char *codec = obs_get_encoder_codec(type);
		if (astrcmpi(codec, "aac") != 0 && astrcmpi(codec, "opus") != 0)
			continue;
		audioEncoder->addItem(QString::fromUtf8(obs_encoder_get_display_name(type)), QVariant(QString::fromUtf8(type)));
		if (strcmp(type, current_type) == 0)
			audioEncoder->setCurrentIndex(audioEncoder->count() - 1);
	}
	audioEncoderGroup->setVisible(audioEncoder->currentIndex() > 0);
	audioPageLayout->setRowVisible(audioTrack, audioEncoder->currentIndex() > 0);

	auto advancedButton = new QPushButton(QString::fromUtf8(obs_module_text("EditEncoderSettings")));
	advancedButton->setProperty("themeID", "configIconSmall");
	advancedButton->setCheckable(true);
	advancedButton->setChecked(advanced);
	connect(advancedButton, &QPushButton::clicked, [advancedButton, advancedGroup, settings] {
		const bool advanced = advancedButton->isChecked();
		advancedGroup->setVisible(advanced);
		obs_data_set_bool(settings, "advanced", advanced);
	});

	// Hook up
	advancedTabWidget->addTab(videoPage, QString::fromUtf8(obs_module_text("VideoEncoderSettings")));
	advancedTabWidget->addTab(audioPage, QString::fromUtf8(obs_module_text("AudioEncoderSettings")));
	advancedGroupLayout->addWidget(advancedTabWidget, 1);
	
	
	
	
	
	// Remove button
	auto removeButton =
		new QPushButton(QIcon(":/res/images/minus.svg"), QString::fromUtf8(obs_frontend_get_locale_string("Remove")));
	removeButton->setProperty("themeID", QVariant(QString::fromUtf8("removeIconSmall")));
	connect(removeButton, &QPushButton::clicked, [this, outputsLayout, serverGroup, settings, outputs] {
		outputsLayout->removeWidget(serverGroup);
		RemoveWidget(serverGroup);
		auto count = obs_data_array_count(outputs);
		for (size_t i = 0; i < count; i++) {
			auto item = obs_data_array_item(outputs, i);
			if (item == settings) {
				obs_data_array_erase(outputs, i);
				obs_data_release(item);
				break;
			}
			obs_data_release(item);
		}
	});

	// Edit button
	auto editButton = new QPushButton(QString::fromUtf8(obs_module_text("EditServerSettings")));
	editButton->setProperty("themeID", "configIconSmall");

	connect(editButton, &QPushButton::clicked, [this, settings] {
		auto outputDialog = new OutputDialog(this, obs_data_get_string(settings, "name"),
						     obs_data_get_string(settings, "stream_server"),
						     obs_data_get_string(settings, "stream_key"));

		outputDialog->setWindowModality(Qt::WindowModal);
		outputDialog->setModal(true);

		if (outputDialog->exec() == QDialog::Accepted) { // edit an output
			if (!settings)
				return;

			// Set the info from the output dialog
			obs_data_set_string(settings, "name", outputDialog->outputName.toUtf8().constData());
			obs_data_set_string(settings, "stream_server", outputDialog->outputServer.toUtf8().constData());
			obs_data_set_string(settings, "stream_key", outputDialog->outputKey.toUtf8().constData());

			// Reload
			LoadSettings(this->settings);
		}

		delete outputDialog;
	});

	// Buttons to layout
	server_title_layout->addWidget(editButton, 0, Qt::AlignRight);
	server_title_layout->addWidget(advancedButton, 0, Qt::AlignRight);
	server_title_layout->addWidget(removeButton, 0, Qt::AlignRight);

	serverLayout->addRow(server_title_layout);

	serverLayout->addRow(advancedGroup);

	serverGroup->setLayout(serverLayout);

	outputsLayout->addRow(serverGroup);
}

void OBSBasicSettings::LoadVerticalSettings()
{

	while (verticalOutputsLayout->rowCount() > 1) {
		auto i = verticalOutputsLayout->takeRow(1).fieldItem;
		RemoveLayoutItem(i);
		verticalOutputsLayout->removeRow(1);
	}
	auto ph = obs_get_proc_handler();
	struct calldata cd;
	calldata_init(&cd);
	if (!proc_handler_call(ph, "aitum_vertical_get_stream_settings", &cd)) {
		// Disable button if we don't have vertical
		verticalAddButton->setEnabled(false);
		calldata_free(&cd);
		return;
	}
	if (vertical_outputs)
		obs_data_array_release(vertical_outputs);
	vertical_outputs = (obs_data_array_t *)calldata_ptr(&cd, "outputs");
	obs_data_array_enum(
		vertical_outputs,
		[](obs_data_t *data, void *param) {
			auto d = (OBSBasicSettings *)param;
			d->AddServer(d->verticalOutputsLayout, data, d->vertical_outputs);
		},
		this);
	calldata_free(&cd);
}

void OBSBasicSettings::SaveVerticalSettings()
{
	if (!vertical_outputs)
		return;
	auto ph = obs_get_proc_handler();
	struct calldata cd;
	calldata_init(&cd);
	calldata_set_ptr(&cd, "outputs", vertical_outputs);
	proc_handler_call(ph, "aitum_vertical_set_stream_settings", &cd);
	calldata_free(&cd);
}

void OBSBasicSettings::LoadSettings(obs_data_t *settings)
{
	while (mainOutputsLayout->rowCount() > 2) {
		auto i = mainOutputsLayout->takeRow(2).fieldItem;
		RemoveLayoutItem(i);
		mainOutputsLayout->removeRow(2);
	}
	this->settings = settings;
	auto outputs = obs_data_get_array(settings, "outputs");
	obs_data_array_enum(
		outputs,
		[](obs_data_t *data, void *param) {
			auto d = (OBSBasicSettings *)param;
			auto outputs = obs_data_get_array(d->settings, "outputs");
			d->AddServer(d->mainOutputsLayout, data, outputs);
			obs_data_array_release(outputs);
		},
		this);
	obs_data_array_release(outputs);
}

void OBSBasicSettings::AddProperty(obs_properties_t *properties, obs_property_t *property, obs_data_t *settings,
				   QFormLayout *layout)
{
	obs_property_type type = obs_property_get_type(property);
	if (type == OBS_PROPERTY_BOOL) {
		auto widget = new QCheckBox(QString::fromUtf8(obs_property_description(property)));
		widget->setChecked(obs_data_get_bool(settings, obs_property_name(property)));
		layout->addWidget(widget);
		if (!obs_property_visible(property)) {
			widget->setVisible(false);
			int row = 0;
			layout->getWidgetPosition(widget, &row, nullptr);
			auto item = layout->itemAt(row, QFormLayout::LabelRole);
			if (item) {
				auto w = item->widget();
				if (w)
					w->setVisible(false);
			}
		}
		encoder_property_widgets.emplace(property, widget);
		connect(widget, &QCheckBox::stateChanged, [this, properties, property, settings, widget, layout] {
			obs_data_set_bool(settings, obs_property_name(property), widget->isChecked());
			if (obs_property_modified(property, settings)) {
				RefreshProperties(properties, layout);
			}
		});
	} else if (type == OBS_PROPERTY_INT) {
		auto widget = new QSpinBox();
		widget->setEnabled(obs_property_enabled(property));
		widget->setMinimum(obs_property_int_min(property));
		widget->setMaximum(obs_property_int_max(property));
		widget->setSingleStep(obs_property_int_step(property));
		widget->setValue((int)obs_data_get_int(settings, obs_property_name(property)));
		widget->setToolTip(QString::fromUtf8(obs_property_long_description(property)));
		widget->setSuffix(QString::fromUtf8(obs_property_int_suffix(property)));
		auto label = new QLabel(QString::fromUtf8(obs_property_description(property)));
		layout->addRow(label, widget);
		if (!obs_property_visible(property)) {
			widget->setVisible(false);
			label->setVisible(false);
		}
		encoder_property_widgets.emplace(property, widget);
		connect(widget, &QSpinBox::valueChanged, [this, properties, property, settings, widget, layout] {
			obs_data_set_int(settings, obs_property_name(property), widget->value());
			if (obs_property_modified(property, settings)) {
				RefreshProperties(properties, layout);
			}
		});
	} else if (type == OBS_PROPERTY_FLOAT) {
		auto widget = new QDoubleSpinBox();
		widget->setEnabled(obs_property_enabled(property));
		widget->setMinimum(obs_property_float_min(property));
		widget->setMaximum(obs_property_float_max(property));
		widget->setSingleStep(obs_property_float_step(property));
		widget->setValue(obs_data_get_double(settings, obs_property_name(property)));
		widget->setToolTip(QString::fromUtf8(obs_property_long_description(property)));
		widget->setSuffix(QString::fromUtf8(obs_property_float_suffix(property)));
		auto label = new QLabel(QString::fromUtf8(obs_property_description(property)));
		layout->addRow(label, widget);
		if (!obs_property_visible(property)) {
			widget->setVisible(false);
			label->setVisible(false);
		}
		encoder_property_widgets.emplace(property, widget);
		connect(widget, &QDoubleSpinBox::valueChanged, [this, properties, property, settings, widget, layout] {
			obs_data_set_double(settings, obs_property_name(property), widget->value());
			if (obs_property_modified(property, settings)) {
				RefreshProperties(properties, layout);
			}
		});
	} else if (type == OBS_PROPERTY_TEXT) {
		obs_text_type text_type = obs_property_text_type(property);
		if (text_type == OBS_TEXT_MULTILINE) {
			auto widget = new QPlainTextEdit;
			widget->document()->setDefaultStyleSheet("font { white-space: pre; }");
			widget->setTabStopDistance(40);
			widget->setPlainText(QString::fromUtf8(obs_data_get_string(settings, obs_property_name(property))));
			auto label = new QLabel(QString::fromUtf8(obs_property_description(property)));
			layout->addRow(label, widget);
			if (!obs_property_visible(property)) {
				widget->setVisible(false);
				label->setVisible(false);
			}
			encoder_property_widgets.emplace(property, widget);
			connect(widget, &QPlainTextEdit::textChanged, [this, properties, property, settings, widget, layout] {
				obs_data_set_string(settings, obs_property_name(property), widget->toPlainText().toUtf8());
				if (obs_property_modified(property, settings)) {
					RefreshProperties(properties, layout);
				}
			});
		} else {
			auto widget = new QLineEdit();
			widget->setText(QString::fromUtf8(obs_data_get_string(settings, obs_property_name(property))));
			if (text_type == OBS_TEXT_PASSWORD)
				widget->setEchoMode(QLineEdit::Password);
			auto label = new QLabel(QString::fromUtf8(obs_property_description(property)));
			layout->addRow(label, widget);
			if (!obs_property_visible(property)) {
				widget->setVisible(false);
				label->setVisible(false);
			}
			encoder_property_widgets.emplace(property, widget);
			if (text_type != OBS_TEXT_INFO) {
				connect(widget, &QLineEdit::textChanged, [this, properties, property, settings, widget, layout] {
					obs_data_set_string(settings, obs_property_name(property), widget->text().toUtf8());
					if (obs_property_modified(property, settings)) {
						RefreshProperties(properties, layout);
					}
				});
			}
		}
	} else if (type == OBS_PROPERTY_LIST) {
		auto widget = new QComboBox();
		widget->setMaxVisibleItems(40);
		widget->setToolTip(QString::fromUtf8(obs_property_long_description(property)));
		auto list_type = obs_property_list_type(property);
		obs_combo_format format = obs_property_list_format(property);

		size_t count = obs_property_list_item_count(property);
		for (size_t i = 0; i < count; i++) {
			QVariant var;
			if (format == OBS_COMBO_FORMAT_INT) {
				long long val = obs_property_list_item_int(property, i);
				var = QVariant::fromValue<long long>(val);

			} else if (format == OBS_COMBO_FORMAT_FLOAT) {
				double val = obs_property_list_item_float(property, i);
				var = QVariant::fromValue<double>(val);

			} else if (format == OBS_COMBO_FORMAT_STRING) {
				var = QByteArray(obs_property_list_item_string(property, i));
			}
			widget->addItem(QString::fromUtf8(obs_property_list_item_name(property, i)), var);
		}

		if (list_type == OBS_COMBO_TYPE_EDITABLE)
			widget->setEditable(true);

		auto name = obs_property_name(property);
		QVariant value;
		switch (format) {
		case OBS_COMBO_FORMAT_INT:
			value = QVariant::fromValue(obs_data_get_int(settings, name));
			break;
		case OBS_COMBO_FORMAT_FLOAT:
			value = QVariant::fromValue(obs_data_get_double(settings, name));
			break;
		case OBS_COMBO_FORMAT_STRING:
			value = QByteArray(obs_data_get_string(settings, name));
			break;
		default:;
		}

		if (format == OBS_COMBO_FORMAT_STRING && list_type == OBS_COMBO_TYPE_EDITABLE) {
			widget->lineEdit()->setText(value.toString());
		} else {
			auto idx = widget->findData(value);
			if (idx != -1)
				widget->setCurrentIndex(idx);
		}

		if (obs_data_has_autoselect_value(settings, name)) {
			switch (format) {
			case OBS_COMBO_FORMAT_INT:
				value = QVariant::fromValue(obs_data_get_autoselect_int(settings, name));
				break;
			case OBS_COMBO_FORMAT_FLOAT:
				value = QVariant::fromValue(obs_data_get_autoselect_double(settings, name));
				break;
			case OBS_COMBO_FORMAT_STRING:
				value = QByteArray(obs_data_get_autoselect_string(settings, name));
				break;
			default:;
			}
			int id = widget->findData(value);

			auto idx = widget->currentIndex();
			if (id != -1 && id != idx) {
				QString actual = widget->itemText(id);
				QString selected = widget->itemText(widget->currentIndex());
				QString combined = QString::fromUtf8(
					obs_frontend_get_locale_string("Basic.PropertiesWindow.AutoSelectFormat"));
				widget->setItemText(idx, combined.arg(selected).arg(actual));
			}
		}
		auto label = new QLabel(QString::fromUtf8(obs_property_description(property)));
		layout->addRow(label, widget);
		if (!obs_property_visible(property)) {
			widget->setVisible(false);
			label->setVisible(false);
		}
		encoder_property_widgets.emplace(property, widget);
		switch (format) {
		case OBS_COMBO_FORMAT_INT:
			connect(widget, &QComboBox::currentIndexChanged, [this, properties, property, settings, widget, layout] {
				obs_data_set_int(settings, obs_property_name(property), widget->currentData().toInt());
				if (obs_property_modified(property, settings)) {
					RefreshProperties(properties, layout);
				}
			});
			break;
		case OBS_COMBO_FORMAT_FLOAT:
			connect(widget, &QComboBox::currentIndexChanged, [this, properties, property, settings, widget, layout] {
				obs_data_set_double(settings, obs_property_name(property), widget->currentData().toDouble());
				if (obs_property_modified(property, settings)) {
					RefreshProperties(properties, layout);
				}
			});
			break;
		case OBS_COMBO_FORMAT_STRING:
			if (list_type == OBS_COMBO_TYPE_EDITABLE) {
				connect(widget, &QComboBox::currentTextChanged,
					[this, properties, property, settings, widget, layout] {
						obs_data_set_string(settings, obs_property_name(property),
								    widget->currentText().toUtf8().constData());
						if (obs_property_modified(property, settings)) {
							RefreshProperties(properties, layout);
						}
					});
			} else {
				connect(widget, &QComboBox::currentIndexChanged,
					[this, properties, property, settings, widget, layout] {
						obs_data_set_string(settings, obs_property_name(property),
								    widget->currentData().toString().toUtf8().constData());
						if (obs_property_modified(property, settings)) {
							RefreshProperties(properties, layout);
						}
					});
			}
			break;
		default:;
		}
	} else {
		// OBS_PROPERTY_PATH
		// OBS_PROPERTY_COLOR
		// OBS_PROPERTY_BUTTON
		// OBS_PROPERTY_FONT
		// OBS_PROPERTY_EDITABLE_LIST
		// OBS_PROPERTY_FRAME_RATE
		// OBS_PROPERTY_GROUP
		// OBS_PROPERTY_COLOR_ALPHA
	}
	obs_property_modified(property, settings);
}

void OBSBasicSettings::RefreshProperties(obs_properties_t *properties, QFormLayout *layout)
{

	obs_property_t *property = obs_properties_first(properties);
	while (property) {
		auto widget = encoder_property_widgets.at(property);
		auto visible = obs_property_visible(property);
		if (widget->isVisible() != visible) {
			widget->setVisible(visible);
			int row = 0;
			layout->getWidgetPosition(widget, &row, nullptr);
			auto item = layout->itemAt(row, QFormLayout::LabelRole);
			if (item) {
				widget = item->widget();
				if (widget)
					widget->setVisible(visible);
			}
		}
		obs_property_next(&property);
	}
}
static bool obs_encoder_parent_video_loaded = false;
static video_t *(*obs_encoder_parent_video_wrapper)(const obs_encoder_t *encoder) = nullptr;

void OBSBasicSettings::LoadOutputStats(std::vector<video_t *> *oldVideos)
{
	if (!obs_encoder_parent_video_loaded) {
		void *dl = os_dlopen("obs");
		if (dl) {
			auto sym = os_dlsym(dl, "obs_encoder_parent_video");
			if (sym)
				obs_encoder_parent_video_wrapper = (video_t * (*)(const obs_encoder_t *encoder)) sym;
			os_dlclose(dl);
		}
		obs_encoder_parent_video_loaded = true;
	}
	std::vector<std::tuple<video_t *, obs_encoder_t *, obs_output_t *>> refs;
	obs_enum_outputs(
		[](void *data, obs_output_t *output) {
			auto refs = (std::vector<std::tuple<video_t *, obs_encoder_t *, obs_output_t *>> *)data;
			auto ec = 0;
			for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
				auto venc = obs_output_get_video_encoder2(output, i);
				if (!venc)
					continue;
				ec++;
				video_t *video = obs_encoder_parent_video_wrapper ? obs_encoder_parent_video_wrapper(venc)
										  : obs_encoder_video(venc);
				if (!video)
					video = obs_output_video(output);
				refs->push_back(std::tuple<video_t *, obs_encoder_t *, obs_output_t *>(video, venc, output));
			}
			if (!ec) {
				refs->push_back(std::tuple<video_t *, obs_encoder_t *, obs_output_t *>(obs_output_video(output),
												       nullptr, output));
			}
			return true;
		},
		&refs);
	std::sort(refs.begin(), refs.end(),
		  [](std::tuple<video_t *, obs_encoder_t *, obs_output_t *> const &a,
		     std::tuple<video_t *, obs_encoder_t *, obs_output_t *> const &b) {
			  video_t *va;
			  video_t *vb;
			  obs_encoder_t *ea;
			  obs_encoder_t *eb;
			  obs_output_t *oa;
			  obs_output_t *ob;
			  std::tie(va, ea, oa) = a;
			  std::tie(vb, eb, ob) = b;
			  if (va == vb) {
				  if (ea == eb) {
					  return oa < ob;
				  }
				  return va < vb;
			  }
			  return va < vb;
		  });
	std::string stats;
	video_t *last_video = nullptr;
	auto video_count = 0;
	video_t *vertical_video = nullptr;
	auto ph = obs_get_proc_handler();
	struct calldata cd;
	calldata_init(&cd);
	if (proc_handler_call(ph, "aitum_vertical_get_video", &cd))
		vertical_video = (video_t *)calldata_ptr(&cd, "video");
	calldata_free(&cd);
	for (auto it = refs.begin(); it != refs.end(); it++) {
		video_t *video;
		obs_encoder_t *encoder;
		obs_output_t *output;
		std::tie(video, encoder, output) = *it;

		if (!video) {
			stats += "No Canvas ";
		} else if (vertical_video && vertical_video == video) {
			stats += "Vertical Canvas ";
		} else if (video == obs_get_video()) {
			stats += "Main Canvas ";
		} else if (std::find(oldVideos->begin(), oldVideos->end(), video) != oldVideos->end()) {
			stats += "Old Main Canvas ";
		} else {
			if (last_video != video) {
				video_count++;
				last_video = video;
			}
			stats += "Custom Canvas ";
			stats += std::to_string(video_count);
			stats += " ";
		}
		if (video && std::find(oldVideos->begin(), oldVideos->end(), video) != oldVideos->end()) {
			stats += obs_output_active(output) ? "Active " : "Inactive ";
		} else if (encoder) {
			stats += "(";
			stats += std::to_string(obs_encoder_get_width(encoder));
			stats += "x";
			stats += std::to_string(obs_encoder_get_height(encoder));
			stats += ") ";
			stats += obs_encoder_get_name(encoder);
			stats += "(";
			stats += obs_encoder_get_id(encoder);
			stats += ") ";
			stats += to_string_with_precision(
				video_output_get_frame_rate(video) / obs_encoder_get_frame_rate_divisor(encoder), 2);
			stats += "fps ";
			stats += obs_encoder_active(encoder) ? "Active " : "Inactive ";
			if (video) {
				stats += "skipped frames ";
				stats += std::to_string(video_output_get_skipped_frames(video));
				stats += "/";
				stats += std::to_string(video_output_get_total_frames(video));
				stats += " ";
			}
		} else if (video) {
			stats += "(";
			stats += std::to_string(video_output_get_width(video));
			stats += "x";
			stats += std::to_string(video_output_get_height(video));
			stats += ") ";
			stats += to_string_with_precision(video_output_get_frame_rate(video), 2);
			stats += "fps ";
			stats += video_output_active(video) ? "Active " : "Inactive ";
			stats += "skipped frames ";
			stats += std::to_string(video_output_get_skipped_frames(video));
			stats += "/";
			stats += std::to_string(video_output_get_total_frames(video));
			stats += " ";
		} else {
			stats += "(";
			stats += std::to_string(obs_output_get_width(output));
			stats += "x";
			stats += std::to_string(obs_output_get_height(output));
			stats += ") ";
			stats += obs_output_active(output) ? "Active " : "Inactive ";
		}
		stats += obs_output_get_name(output);
		stats += "(";
		stats += obs_output_get_id(output);
		stats += ") ";
		stats += std::to_string(obs_output_get_connect_time_ms(output));
		stats += "ms ";
		//obs_output_get_total_bytes(output);
		stats += "dropped frames ";
		stats += std::to_string(obs_output_get_frames_dropped(output));
		stats += "/";
		stats += std::to_string(obs_output_get_total_frames(output));
		obs_service_t *service = obs_output_get_service(output);
		if (service) {
			stats += " ";
			stats += obs_service_get_name(service);
			stats += "(";
			stats += obs_service_get_id(service);
			stats += ")";
			auto url = obs_service_get_connect_info(service, OBS_SERVICE_CONNECT_INFO_SERVER_URL);
			if (url) {
				stats += " ";
				stats += url;
			}
		}
		stats += "\n";
	}
	troubleshooterText->setText(QString::fromUtf8(stats));
}

void OBSBasicSettings::SetNewerVersion(QString newer_version_available)
{
	if (newer_version_available.isEmpty())
		return;
	newVersion->setText(QString::fromUtf8(obs_module_text("NewVersion")).arg(newer_version_available));
	newVersion->setVisible(true);
}
