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
#include <QDialogButtonBox>

#include "obs-module.h"
#include "version.h"
#include <util/dstr.h>

#include <sstream>
#include <util/platform.h>
#include <util/config-file.h>
#include "output-dialog.hpp"
#include "config-utils.hpp"

#ifndef _WIN32
#include <dlfcn.h>
#endif

template<typename T> std::string to_string_with_precision(const T a_value, const int n = 6)
{
	std::ostringstream out;
	out.precision(n);
	out << std::fixed << a_value;
	return std::move(out).str();
}

void RemoveWidget(QWidget *widget);
void RemoveLayoutItem(QLayoutItem *item);

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
	//listwidgetitem->setProperty("class", "icon-gear");
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
	listwidgetitem->setHidden(true);

	listwidgetitem = new QListWidgetItem(listWidget);
	listwidgetitem->setIcon(main_window->property("defaultIcon").value<QIcon>());
	listwidgetitem->setText(QString::fromUtf8(obs_module_text("Help")));

	listwidgetitem = new QListWidgetItem(listWidget);
	listwidgetitem->setIcon(QIcon(QString::fromUtf8(":/aitum/media/aitum.png")));
	listwidgetitem->setText(QString::fromUtf8(obs_module_text("SupportButton")));

	listWidget->setCurrentRow(0);
	listWidget->setSpacing(1);

	auto settingsPages = new QStackedWidget;
	settingsPages->setContentsMargins(0, 0, 0, 0);
	settingsPages->setFrameShape(QFrame::NoFrame);
	settingsPages->setLineWidth(0);

	QWidget *generalPage = new QWidget;
	auto generalPageLayout = new QVBoxLayout;
	generalPage->setLayout(generalPageLayout);

	auto infoBox = ConfigUtils::generateSettingsGroupBox(QString::fromUtf8(obs_module_text("WelcomeTitle")));
	infoBox->setStyleSheet("padding-top: 12px");
	auto infoLayout = new QVBoxLayout;
	infoBox->setLayout(infoLayout);

	auto infoLabel = new QLabel(QString::fromUtf8(obs_module_text("WelcomeText")));
	infoLabel->setWordWrap(true);
	infoLayout->addWidget(infoLabel, 1);

	auto buttonGroupBox = new QWidget();
	auto buttonLayout = new QHBoxLayout;
	buttonLayout->setSpacing(8);
	buttonLayout->setAlignment(Qt::AlignCenter);

	generalMainButton = ConfigUtils::generateMenuButton(QString::fromUtf8(obs_module_text("SettingsMainOutputsButton")),
							    QIcon(QString::fromUtf8(":/settings/images/settings/stream.svg")));
	generalVerticalButton = ConfigUtils::generateMenuButton(QString::fromUtf8(obs_module_text("SettingsVerticalOutputsButton")),
								QIcon(QString::fromUtf8(":/settings/images/settings/stream.svg")));
	generalHelpButton = ConfigUtils::generateMenuButton(QString::fromUtf8(obs_module_text("SettingsHelpButton")),
							    main_window->property("defaultIcon").value<QIcon>());
	generalSupportAitumButton = ConfigUtils::generateMenuButton(QString::fromUtf8(obs_module_text("SupportButton")),
								    QIcon(QString::fromUtf8(":/aitum/media/aitum.png")));

	buttonLayout->addWidget(generalMainButton, 0);
	buttonLayout->addWidget(generalVerticalButton, 0);
	buttonLayout->addWidget(generalHelpButton, 0);
	buttonLayout->addWidget(generalSupportAitumButton, 0);

	buttonGroupBox->setLayout(buttonLayout);

	generalPageLayout->addWidget(infoBox, 0);
	generalPageLayout->addWidget(buttonGroupBox, 1);

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

	// Help page
	auto helpPage = new QWidget;
	auto helpPageLayout = new QVBoxLayout;
	helpPage->setLayout(helpPageLayout);
	scrollArea = new QScrollArea;
	scrollArea->setWidget(helpPage);
	scrollArea->setWidgetResizable(true);
	scrollArea->setLineWidth(0);
	scrollArea->setFrameShape(QFrame::NoFrame);

	auto helpInfoBox = ConfigUtils::generateSettingsGroupBox(QString::fromUtf8(obs_module_text("HelpTitle")));
	helpInfoBox->setStyleSheet("padding-top: 12px");
	auto helpLayout = new QVBoxLayout;
	helpInfoBox->setLayout(helpLayout);

	auto helpLabel = new QLabel(QString::fromUtf8(obs_module_text("HelpText")));
	helpLabel->setStyleSheet("font-size: 14px");
	helpLabel->setWordWrap(true);
	helpLabel->setTextFormat(Qt::RichText);
	helpLabel->setOpenExternalLinks(true);
	helpLayout->addWidget(helpLabel, 1);
	helpPageLayout->addWidget(helpInfoBox, 1, Qt::AlignTop);

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
	addButton->setProperty("class", "icon-plus");

	connect(addButton, &QPushButton::clicked, [this] {
		QStringList otherNames;
		auto main_outputs = obs_data_get_array(main_settings, "outputs");
		obs_data_array_enum(
			main_outputs,
			[](obs_data_t *data2, void *param) {
				((QStringList *)param)->append(QString::fromUtf8(obs_data_get_string(data2, "name")));
			},
			&otherNames);
		obs_data_array_release(main_outputs);
		otherNames.removeDuplicates();
		auto outputDialog = new OutputDialog(this, otherNames);

		outputDialog->setWindowModality(Qt::WindowModal);
		outputDialog->setModal(true);

		if (outputDialog->exec() == QDialog::Accepted) {
			// create a new output
			if (!main_settings)
				return;
			auto outputs = obs_data_get_array(main_settings, "outputs");
			if (!outputs) {
				outputs = obs_data_array_create();
				obs_data_set_array(main_settings, "outputs", outputs);
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
	mainTitle->setStyleSheet("font-weight: bold;");
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
	// 	addButton->setProperty("class", "icon-plus");
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
	verticalAddButton->setProperty("class", "icon-plus");

	connect(verticalAddButton, &QPushButton::clicked, [this] {
		QStringList otherNames;
		obs_data_array_enum(
			vertical_outputs,
			[](obs_data_t *data2, void *param) {
				((QStringList *)param)->append(QString::fromUtf8(obs_data_get_string(data2, "name")));
			},
			&otherNames);
		otherNames.removeDuplicates();
		auto outputDialog = new OutputDialog(this, otherNames);

		outputDialog->setWindowModality(Qt::WindowModal);
		outputDialog->setModal(true);

		if (outputDialog->exec() == QDialog::Accepted) {
			// create a new output
			if (!vertical_outputs)
				return;
			auto s = obs_data_create();
			obs_data_set_bool(s, "enabled", true);
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

	// Support page
	QWidget *supportPage = new QWidget;
	auto supportPageLayout = new QVBoxLayout;
	supportPage->setLayout(supportPageLayout);

	auto supportInfoBox = ConfigUtils::generateSettingsGroupBox(QString::fromUtf8(obs_module_text("SupportTitle")));
	supportInfoBox->setStyleSheet("padding-top: 12px");
	auto supportLayout = new QVBoxLayout;
	supportInfoBox->setLayout(supportLayout);

	auto supportLabel = new QLabel(QString::fromUtf8(obs_module_text("SupportText")));
	supportLabel->setStyleSheet("font-size: 14px");
	supportLabel->setWordWrap(true);
	supportLabel->setTextFormat(Qt::RichText);
	supportLabel->setOpenExternalLinks(true);
	supportLayout->addWidget(supportLabel, 1);
	supportPageLayout->addWidget(supportInfoBox, 1, Qt::AlignTop);

	settingsPages->addWidget(supportPage);

	///
	const auto version =
		new QLabel(QString::fromUtf8(obs_module_text("Version")) + " " + QString::fromUtf8(PROJECT_VERSION) + " " +
			   QString::fromUtf8(obs_module_text("MadeBy")) + " <a href=\"https://aitum.tv\">Aitum</a>");
	version->setOpenExternalLinks(true);
	version->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

	newVersion = new QLabel;
	newVersion->setProperty("themeID", "warning");
	newVersion->setProperty("class", "text-warning");
	newVersion->setVisible(false);
	newVersion->setOpenExternalLinks(true);
	newVersion->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

	auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

	connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

	QHBoxLayout *bottomLayout = new QHBoxLayout;
	bottomLayout->addWidget(version, 1, Qt::AlignLeft);
	bottomLayout->addWidget(newVersion, 1, Qt::AlignLeft);
	bottomLayout->addWidget(buttonBox, 0, Qt::AlignRight);

	QHBoxLayout *contentLayout = new QHBoxLayout;
	contentLayout->addWidget(listWidget);

	contentLayout->addWidget(settingsPages, 1);

	listWidget->connect(listWidget, &QListWidget::currentRowChanged, settingsPages, &QStackedWidget::setCurrentIndex);
	listWidget->setCurrentRow(0);

	QVBoxLayout *vlayout = new QVBoxLayout;
	vlayout->setContentsMargins(11, 11, 11, 11);
	vlayout->addLayout(contentLayout);
	vlayout->addLayout(bottomLayout);
	setLayout(vlayout);

	// Button connects for general page, clean this up in the future when we abstract pages
	connect(generalMainButton, &QPushButton::clicked, [this] { listWidget->setCurrentRow(1); });

	connect(generalVerticalButton, &QPushButton::clicked, [this] { listWidget->setCurrentRow(2); });

	connect(generalHelpButton, &QPushButton::clicked, [this] { listWidget->setCurrentRow(listWidget->count() - 2); });

	connect(generalSupportAitumButton, &QPushButton::clicked, [this] { listWidget->setCurrentRow(listWidget->count() - 1); });
}

OBSBasicSettings::~OBSBasicSettings()
{
	if (vertical_outputs)
		obs_data_array_release(vertical_outputs);
	for (auto it = video_encoder_properties.begin(); it != video_encoder_properties.end(); it++)
		obs_properties_destroy(it->second);
	for (auto it = audio_encoder_properties.begin(); it != audio_encoder_properties.end(); it++)
		obs_properties_destroy(it->second);
}

QIcon OBSBasicSettings::GetGeneralIcon() const
{
	return listWidget->item(0)->icon();
}

QIcon OBSBasicSettings::GetAppearanceIcon() const
{
	return QIcon();
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

void OBSBasicSettings::SetAppearanceIcon(const QIcon &icon)
{
	UNUSED_PARAMETER(icon);
}

void OBSBasicSettings::SetStreamIcon(const QIcon &icon)
{
	listWidget->item(1)->setIcon(icon);
	listWidget->item(2)->setIcon(icon);
	generalVerticalButton->setIcon(icon);
	generalMainButton->setIcon(icon);
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
	auto platformIcon =
		ConfigUtils::getPlatformIconFromEndpoint(QString::fromUtf8(obs_data_get_string(settings, "stream_server")));
	platformIconLabel->setPixmap(platformIcon.pixmap(36, 36));
	server_title_layout->addWidget(platformIconLabel, 0);

	auto streaming_title = new QLabel(QString::fromUtf8(obs_data_get_string(settings, "name")));
	streaming_title->setStyleSheet(QString::fromUtf8("font-weight: bold;"));
	server_title_layout->addWidget(streaming_title, 1, Qt::AlignLeft);

	// Advanced settings
	const bool advanced = obs_data_get_bool(settings, "advanced");
	auto advancedGroup = new QGroupBox(QString::fromUtf8(obs_module_text("AdvancedGroupHeader")));
	advancedGroup->setContentsMargins(0, 4, 0, 0);

	advancedGroup->setProperty("customTitle", QVariant(true));
	advancedGroup->setStyleSheet(
		"QGroupBox[customTitle=\"true\"]::title { subcontrol-origin: margin; subcontrol-position: top right; padding: 12px 18px 0 0; }"
		"QGroupBox[customTitle=\"true\"] { padding-top: 4px; padding-bottom: 0;}");
	if (!advanced)
		advancedGroup->setVisible(false);

	auto advancedGroupLayout = new QVBoxLayout;
	advancedGroup->setLayout(advancedGroupLayout);

	// Tab widget
	// 1 = bg for active tab + pane, 2 = inactive tabs, 3 = tab text colour, 4 = border colour for pane
	auto tabStyles =
		QString("QTabWidget::pane {  background: %1; border-bottom-left-radius: 4px; border-bottom-right-radius: 4px; border-top-right-radius: 4px; margin-top: -1px; padding-top: 8px; border: 1px solid %4; } QTabWidget::tab-bar { margin-bottom: 0; padding-bottom: 0; border-color: %4; } QTabBar::tab { color: %3; padding: 10px; margin-bottom: 0; border: 1px solid %4; } QTabBar::tab:selected { background: %1; font-weight: bold; border-bottom: none; } QTabBar::tab:!selected { background: %2; }")
			.arg(palette().color(QPalette::ColorRole::Mid).name(QColor::HexRgb),
			     palette().color(QPalette::ColorRole::Light).name(QColor::HexRgb),
			     palette().color(QPalette::ColorRole::Text).name(QColor::HexRgb),
			     palette().color(QPalette::ColorRole::Light).name(QColor::HexRgb));

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

	const bool main = outputsLayout == mainOutputsLayout;

	// VIDEO ENCODER
	auto videoEncoder = new QComboBox;
	videoEncoder->addItem(QString::fromUtf8(obs_module_text(main ? "MainEncoder" : "VerticalEncoder")),
			      QVariant(QString::fromUtf8("")));

	videoEncoder->setCurrentIndex(0);
	videoPageLayout->addRow(QString::fromUtf8(obs_module_text("VideoEncoder")), videoEncoder);

	bool allEmpty = false;
	QComboBox *videoEncoderIndex = nullptr;
	if (main) {
		videoEncoderIndex = new QComboBox;
		for (int i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
			QString settingName = QString::fromUtf8("video_encoder_description") + QString::number(i);
			auto description = obs_data_get_string(main_settings, settingName.toUtf8().constData());
			if (!description || description[0] == '\0') {
				if (i != 0 && !allEmpty) {
					break;
				}
				allEmpty = true;
			}
			videoEncoderIndex->addItem(QString::number(i + 1) + " " + description);
		}
		videoEncoderIndex->setCurrentIndex((int)obs_data_get_int(settings, "video_encoder_index"));

		connect(videoEncoderIndex, &QComboBox::currentIndexChanged, [videoEncoderIndex, settings] {
			if (videoEncoderIndex->currentIndex() >= 0)
				obs_data_set_int(settings, "video_encoder_index", videoEncoderIndex->currentIndex());
		});
		videoPageLayout->addRow(QString::fromUtf8(obs_module_text("VideoEncoderIndex")), videoEncoderIndex);

		if (videoEncoderIndex->currentIndex() != 0 ||
		    !config_get_bool(obs_frontend_get_profile_config(), "Stream1", "EnableMultitrackVideo"))
			videoPageLayout->setRowVisible(videoEncoderIndex, false);
	}

	auto videoEncoderGroup = new QWidget();
	videoEncoderGroup->setProperty("altColor", QVariant(true));
	auto videoEncoderGroupLayout = new QFormLayout();
	videoEncoderGroup->setLayout(videoEncoderGroupLayout);
	videoPageLayout->addRow(videoEncoderGroup);

	struct obs_video_info ovi;
	obs_get_video_info(&ovi);
	double fps = ovi.fps_den > 0 ? (double)ovi.fps_num / (double)ovi.fps_den : 0.0;
	auto fpsDivisor = new QComboBox;
	auto frd = obs_data_get_int(settings, "frame_rate_divisor");
	for (int i = 1; i <= 10; i++) {
		if (i == 1) {
			fpsDivisor->addItem(QString::number(fps, 'g', 3) + " " + QString::fromUtf8(obs_module_text("Original")), i);
			fpsDivisor->setCurrentIndex(0);
		} else {
			fpsDivisor->addItem(QString::number(fps / i, 'g', 3), i);
			if (frd == i) {
				fpsDivisor->setCurrentIndex(fpsDivisor->count() - 1);
			}
		}
	}
	connect(fpsDivisor, &QComboBox::currentIndexChanged,
		[fpsDivisor, settings] { obs_data_set_int(settings, "frame_rate_divisor", fpsDivisor->currentData().toInt()); });

	videoEncoderGroupLayout->addRow(QString::fromUtf8(obs_frontend_get_locale_string("Basic.Settings.Video.FPS")), fpsDivisor);
	//obs_encoder_get_frame_rate_divisor

	auto scale = new QGroupBox(QString::fromUtf8(obs_frontend_get_locale_string("Basic.Settings.Output.Adv.Rescale")));
	scale->setCheckable(true);
	scale->setChecked(obs_data_get_bool(settings, "scale"));

	connect(scale, &QGroupBox::toggled, [scale, settings] { obs_data_set_bool(settings, "scale", scale->isChecked()); });

	auto scaleLayout = new QFormLayout();
	scale->setLayout(scaleLayout);

	auto downscale = new QComboBox;

	auto downscale_type = obs_data_get_int(settings, "scale_type");
	if (downscale_type == OBS_SCALE_DISABLE) {
		downscale_type = OBS_SCALE_BILINEAR;
		obs_data_set_int(settings, "scale_type", downscale_type);
	}
	downscale->addItem(QString::fromUtf8(obs_frontend_get_locale_string("Basic.Settings.Video.DownscaleFilter.Bilinear")),
			   OBS_SCALE_BILINEAR);
	if (downscale_type == OBS_SCALE_BILINEAR)
		downscale->setCurrentIndex(0);
	downscale->addItem(QString::fromUtf8(obs_frontend_get_locale_string("Basic.Settings.Video.DownscaleFilter.Area")),
			   OBS_SCALE_AREA);
	if (downscale_type == OBS_SCALE_AREA)
		downscale->setCurrentIndex(1);
	downscale->addItem(QString::fromUtf8(obs_frontend_get_locale_string("Basic.Settings.Video.DownscaleFilter.Bicubic")),
			   OBS_SCALE_BICUBIC);
	if (downscale_type == OBS_SCALE_BICUBIC)
		downscale->setCurrentIndex(2);
	downscale->addItem(QString::fromUtf8(obs_frontend_get_locale_string("Basic.Settings.Video.DownscaleFilter.Lanczos")),
			   OBS_SCALE_LANCZOS);
	if (downscale_type == OBS_SCALE_LANCZOS)
		downscale->setCurrentIndex(3);

	connect(downscale, &QComboBox::currentIndexChanged,
		[downscale, settings] { obs_data_set_int(settings, "scale_type", downscale->currentData().toInt()); });

	scaleLayout->addRow(QString::fromUtf8(obs_frontend_get_locale_string("Basic.Settings.Video.DownscaleFilter")), downscale);

	auto resolution = new QComboBox;
	resolution->setEditable(true);
	if (main) {
		resolution->addItem("1280x720");
		resolution->addItem("1920x1080");
		resolution->addItem("2560x1440");
	} else {
		resolution->addItem("720x1280");
		resolution->addItem("1080x1920");
		resolution->addItem("1440x2560");
	}
	resolution->setCurrentText(QString::number(obs_data_get_int(settings, "width")) + "x" +
				   QString::number(obs_data_get_int(settings, "height")));
	if (resolution->currentText() == "0x0")
		resolution->setCurrentText(QString::number(ovi.output_width) + "x" + QString::number(ovi.output_height));

	connect(resolution, &QComboBox::currentTextChanged, [settings, resolution] {
		const auto res = resolution->currentText();
		uint32_t width, height;
		if (sscanf(res.toUtf8().constData(), "%dx%d", &width, &height) == 2 && width > 0 && height > 0) {
			obs_data_set_int(settings, "width", width);
			obs_data_set_int(settings, "height", height);
		}
	});

	scaleLayout->addRow(QString::fromUtf8(obs_frontend_get_locale_string("Basic.Settings.Video.ScaledResolution")), resolution);

	videoEncoderGroupLayout->addRow(scale);

	connect(videoEncoder, &QComboBox::currentIndexChanged,
		[this, serverGroup, advancedGroupLayout, videoPageLayout, videoEncoder, videoEncoderIndex, videoEncoderGroup,
		 videoEncoderGroupLayout, settings, videoPage, main] {
			auto encoder_string = videoEncoder->currentData().toString().toUtf8();
			auto encoder = encoder_string.constData();
			const bool encoder_changed = strcmp(obs_data_get_string(settings, "video_encoder"), encoder) != 0;
			if (encoder_changed)
				obs_data_set_string(settings, "video_encoder", encoder);
			if (!encoder || encoder[0] == '\0') {
				if (!videoEncoderIndex) {
				} else if (config_get_bool(obs_frontend_get_profile_config(), "Stream1", "EnableMultitrackVideo")) {
					videoPageLayout->setRowVisible(videoEncoderIndex, true);
				} else {
					videoPageLayout->setRowVisible(videoEncoderIndex, false);
					if (videoEncoderIndex->currentIndex() != 0)
						videoEncoderIndex->setCurrentIndex(0);
				}
				videoEncoderGroup->setVisible(false);
			} else {
				if (videoEncoderIndex)
					videoPageLayout->setRowVisible(videoEncoderIndex, false);
				if (!videoEncoderGroup->isVisibleTo(videoPage))
					videoEncoderGroup->setVisible(true);
				auto t = video_encoder_properties.find(serverGroup);
				if (t != video_encoder_properties.end()) {
					obs_properties_destroy(t->second);
					video_encoder_properties.erase(t);
				}
				for (int i = videoEncoderGroupLayout->rowCount() - 1; i >= 2; i--) {
					videoEncoderGroupLayout->removeRow(i);
				}
				auto ves = encoder_changed ? nullptr : obs_data_get_obj(settings, "video_encoder_settings");
				if (!ves) {
					auto defaults = obs_encoder_defaults(encoder);
					if (defaults) {
						ves = obs_data_get_defaults(defaults);
						obs_data_release(defaults);
					} else {
						ves = obs_data_create();
					}
					obs_data_set_obj(settings, "video_encoder_settings", ves);
				}
				auto stream_encoder_properties = obs_get_encoder_properties(encoder);
				video_encoder_properties[serverGroup] = stream_encoder_properties;

				obs_property_t *property = obs_properties_first(stream_encoder_properties);
				while (property) {
					AddProperty(stream_encoder_properties, property, ves, videoEncoderGroupLayout);
					obs_property_next(&property);
				}
				obs_data_release(ves);
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
	if (videoEncoder->currentIndex() <= 0)
		videoEncoderGroup->setVisible(false);

	auto audioEncoder = new QComboBox;
	audioPageLayout->addRow(QString::fromUtf8(obs_module_text("AudioEncoder")), audioEncoder);

	auto audioTrack = new QComboBox;
	for (int i = 0; i < 6; i++) {
		auto trackConfigName = QString::fromUtf8("Track") + QString::number(i + 1) + QString::fromUtf8("Name");
		auto trackName = QString::fromUtf8(
			config_get_string(obs_frontend_get_profile_config(), "AdvOut", trackConfigName.toUtf8().constData()));
		if (trackName.isEmpty()) {
			auto trackTranslationName =
				QString::fromUtf8("Basic.Settings.Output.Adv.Audio.Track") + QString::number(i + 1);
			trackName = QString::fromUtf8(obs_frontend_get_locale_string(trackTranslationName.toUtf8().constData()));
		}
		if (trackName.isEmpty())
			trackName = QString::number(i + 1);
		audioTrack->addItem(trackName);
	}
	audioTrack->setCurrentIndex((int)obs_data_get_int(settings, "audio_track"));
	connect(audioTrack, &QComboBox::currentIndexChanged, [audioTrack, settings] {
		if (audioTrack->currentIndex() >= 0)
			obs_data_set_int(settings, "audio_track", audioTrack->currentIndex());
	});
	audioPageLayout->addRow(QString::fromUtf8(obs_module_text("AudioTrack")), audioTrack);

	auto audioEncoderGroup = new QWidget();
	audioEncoderGroup->setProperty("altColor", QVariant(true));
	auto audioEncoderGroupLayout = new QFormLayout();
	audioEncoderGroup->setLayout(audioEncoderGroupLayout);
	audioPageLayout->addRow(audioEncoderGroup);

	int audio_encoder_index = 0;
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
			audio_encoder_index = audioEncoder->count() - 1;
	}

	connect(audioEncoder, &QComboBox::currentIndexChanged,
		[this, serverGroup, advancedGroupLayout, audioPageLayout, audioEncoder, audioEncoderGroup, audioEncoderGroupLayout,
		 audioTrack, settings, audioPage] {
			auto encoder_string = audioEncoder->currentData().toString().toUtf8();
			auto encoder = encoder_string.constData();
			const bool encoder_changed = !encoder_string.isEmpty() &&
						     strcmp(obs_data_get_string(settings, "audio_encoder"), encoder) != 0;
			if (encoder_changed)
				obs_data_set_string(settings, "audio_encoder", encoder);

			auto t = audio_encoder_properties.find(serverGroup);
			if (t != audio_encoder_properties.end()) {
				obs_properties_destroy(t->second);
				audio_encoder_properties.erase(t);
			}
			for (int i = audioEncoderGroupLayout->rowCount() - 1; i >= 0; i--) {
				audioEncoderGroupLayout->removeRow(i);
			}
			auto aes = encoder_changed ? nullptr : obs_data_get_obj(settings, "audio_encoder_settings");
			if (!aes) {
				auto defaults = obs_encoder_defaults(encoder);
				if (defaults) {
					aes = obs_data_get_defaults(defaults);
					obs_data_release(defaults);
				} else {
					aes = obs_data_create();
				}
				obs_data_set_obj(settings, "audio_encoder_settings", aes);
			}
			auto stream_encoder_properties = obs_get_encoder_properties(encoder);
			audio_encoder_properties[serverGroup] = stream_encoder_properties;

			obs_property_t *property = obs_properties_first(stream_encoder_properties);
			while (property) {
				AddProperty(stream_encoder_properties, property, aes, audioEncoderGroupLayout);
				obs_property_next(&property);
			}
			obs_data_release(aes);
		});

	if (audio_encoder_index == audioEncoder->currentIndex())
		audioEncoder->setCurrentIndex(-1);
	audioEncoder->setCurrentIndex(audio_encoder_index);

	auto advancedButton = new QPushButton(QString::fromUtf8(obs_module_text("EditEncoderSettings")));
	advancedButton->setProperty("themeID", "configIconSmall");
	advancedButton->setProperty("class", "icon-gear");
	advancedButton->setCheckable(true);
	advancedButton->setChecked(advanced);
	connect(advancedButton, &QPushButton::clicked, [advancedButton, advancedGroup, settings] {
		const bool is_advanced = advancedButton->isChecked();
		advancedGroup->setVisible(is_advanced);
		obs_data_set_bool(settings, "advanced", is_advanced);
	});

	// Hook up
	advancedTabWidget->addTab(videoPage, QString::fromUtf8(obs_module_text("VideoEncoderSettings")));
	advancedTabWidget->addTab(audioPage, QString::fromUtf8(obs_module_text("AudioEncoderSettings")));
	advancedGroupLayout->addWidget(advancedTabWidget, 1);

	// Remove button
	auto removeButton =
		new QPushButton(QIcon(":/res/images/minus.svg"), QString::fromUtf8(obs_frontend_get_locale_string("Remove")));
	removeButton->setProperty("themeID", QVariant(QString::fromUtf8("removeIconSmall")));
	removeButton->setProperty("class", "icon-minus");
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
	editButton->setProperty("class", "icon-gear");

	connect(editButton, &QPushButton::clicked, [this, settings, outputs] {
		QStringList otherNames;
		obs_data_array_enum(
			outputs,
			[](obs_data_t *data2, void *param) {
				((QStringList *)param)->append(QString::fromUtf8(obs_data_get_string(data2, "name")));
			},
			&otherNames);
		otherNames.removeDuplicates();
		otherNames.removeOne(QString::fromUtf8(obs_data_get_string(settings, "name")));
		auto outputDialog = new OutputDialog(this, obs_data_get_string(settings, "name"),
						     obs_data_get_string(settings, "stream_server"),
						     obs_data_get_string(settings, "stream_key"), otherNames);

		outputDialog->setWindowModality(Qt::WindowModal);
		outputDialog->setModal(true);

		if (outputDialog->exec() == QDialog::Accepted) { // edit an output
			if (!settings)
				return;

			if (outputs == vertical_outputs)
				obs_data_set_bool(settings, "enabled", true);
			// Set the info from the output dialog
			obs_data_set_string(settings, "name", outputDialog->outputName.toUtf8().constData());
			obs_data_set_string(settings, "stream_server", outputDialog->outputServer.toUtf8().constData());
			obs_data_set_string(settings, "stream_key", outputDialog->outputKey.toUtf8().constData());

			// Reload
			LoadSettings(main_settings);
			LoadVerticalSettings(false);
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

void OBSBasicSettings::LoadVerticalSettings(bool load)
{
	while (verticalOutputsLayout->rowCount() > 1) {
		auto i = verticalOutputsLayout->takeRow(1).fieldItem;
		RemoveLayoutItem(i);
		verticalOutputsLayout->removeRow(1);
	}
	if (load) {
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
		calldata_free(&cd);
	}
	obs_data_array_enum(
		vertical_outputs,
		[](obs_data_t *data2, void *param) {
			auto d = (OBSBasicSettings *)param;
			d->AddServer(d->verticalOutputsLayout, data2, d->vertical_outputs);
		},
		this);
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
	main_settings = settings;
	auto outputs = obs_data_get_array(settings, "outputs");
	obs_data_array_enum(
		outputs,
		[](obs_data_t *data2, void *param) {
			auto d = (OBSBasicSettings *)param;
			auto outputs2 = obs_data_get_array(d->main_settings, "outputs");
			d->AddServer(d->mainOutputsLayout, data2, outputs2);
			obs_data_array_release(outputs2);
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
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
		connect(widget, &QCheckBox::checkStateChanged, [this, properties, property, settings, widget, layout] {
#else
		connect(widget, &QCheckBox::stateChanged, [this, properties, property, settings, widget, layout] {
#endif
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
#ifdef _WIN32
		void *dl = os_dlopen("obs");
#else
		void *dl = dlopen(nullptr, RTLD_LAZY);
#endif
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
		[](void *param, obs_output_t *output) {
			auto refs2 = (std::vector<std::tuple<video_t *, obs_encoder_t *, obs_output_t *>> *)param;
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
				refs2->push_back(std::tuple<video_t *, obs_encoder_t *, obs_output_t *>(video, venc, output));
			}
			if (!ec) {
				refs2->push_back(std::tuple<video_t *, obs_encoder_t *, obs_output_t *>(obs_output_video(output),
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
