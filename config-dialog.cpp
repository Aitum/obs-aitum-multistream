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

#include "obs-module.h"
#include "version.h"
#include <util/dstr.h>

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
	//cogsIcon
	listwidgetitem->setText(QString::fromUtf8(obs_module_text("General")));

	listwidgetitem = new QListWidgetItem(listWidget);
	listwidgetitem->setIcon(QIcon(QString::fromUtf8(":/settings/images/settings/stream.svg")));
	listwidgetitem->setText(QString::fromUtf8(obs_module_text("MainCanvas")));

	//listwidgetitem = new QListWidgetItem(listWidget);
	//listwidgetitem->setIcon(QIcon(QString::fromUtf8(":/settings/images/settings/stream.svg")));
	//listwidgetitem->setIcon(main_window->property("defaultIcon").value<QIcon>());
	//listwidgetitem->setText(QString::fromUtf8(obs_module_text("Vertical outputs")));

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

	/*
	auto verticalOutputsPage = new QWidget;
	scrollArea = new QScrollArea;
	scrollArea->setWidget(verticalOutputsPage);
	scrollArea->setWidgetResizable(true);
	scrollArea->setLineWidth(0);
	scrollArea->setFrameShape(QFrame::NoFrame);
	settingsPages->addWidget(scrollArea);*/

	auto troubleshooterPage = new QWidget;
	scrollArea = new QScrollArea;
	scrollArea->setWidget(troubleshooterPage);
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
		if (!settings)
			return;
		auto outputs = obs_data_get_array(settings, "outputs");
		if (!outputs) {
			outputs = obs_data_array_create();
			obs_data_set_array(settings, "outputs", outputs);
		}
		auto s = obs_data_create();
		obs_data_set_string(s, "name", obs_module_text("Unnamed"));
		obs_data_array_push_back(outputs, s);
		obs_data_array_release(outputs);
		AddServer(mainOutputsLayout, s);
		obs_data_release(s);
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

	auto main_title = new QLabel(QString::fromUtf8(obs_module_text("MainOutput")));
	main_title->setStyleSheet(QString::fromUtf8("font-weight: bold;"));
	serverLayout->addRow(main_title);

	serverGroup->setLayout(serverLayout);

	mainOutputsLayout->addRow(serverGroup);

	mainOutputsPage->setLayout(mainOutputsLayout);

	const auto version =
		new QLabel(QString::fromUtf8(obs_module_text("Version")) + " " + QString::fromUtf8(PROJECT_VERSION) + " " +
			   QString::fromUtf8(obs_module_text("MadeBy")) + " <a href=\"https://aitum.tv\">Aitum</a>");
	version->setOpenExternalLinks(true);
	version->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

	QPushButton *okButton = new QPushButton(QString::fromUtf8(obs_frontend_get_locale_string("OK")));
	connect(okButton, &QPushButton::clicked, [this] { accept(); });

	QPushButton *cancelButton = new QPushButton(QString::fromUtf8(obs_frontend_get_locale_string("Cancel")));
	connect(cancelButton, &QPushButton::clicked, [this] { reject(); });

	QHBoxLayout *bottomLayout = new QHBoxLayout;
	bottomLayout->addWidget(version, 1, Qt::AlignLeft);
	//bottomLayout->addWidget(newVersion, 1, Qt::AlignLeft);
	bottomLayout->addWidget(okButton, 0, Qt::AlignRight);
	bottomLayout->addWidget(cancelButton, 0, Qt::AlignRight);

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
	//listWidget->item(2)->setIcon(icon);
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

void OBSBasicSettings::AddServer(QFormLayout *outputsLayout, obs_data_t *settings)
{
	auto serverGroup = new QGroupBox;
	serverGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
	serverGroup->setProperty("altColor", QVariant(true));
	serverGroup->setProperty("customTitle", QVariant(true));
	serverGroup->setStyleSheet(
		QString("QGroupBox[altColor=\"true\"]{background-color: %1;} QGroupBox[customTitle=\"true\"]{padding-top: 4px;}")
			.arg(palette().color(QPalette::ColorRole::Mid).name(QColor::HexRgb)));

	auto serverLayout = new QFormLayout;
	serverLayout->setContentsMargins(9, 2, 9, 9);
	serverLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	serverLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);

	auto server_title_layout = new QHBoxLayout;
	auto streaming_title = new QLabel(QString::fromUtf8(obs_data_get_string(settings, "name")));
	streaming_title->setStyleSheet(QString::fromUtf8("font-weight: bold;"));
	auto title_stack = new QStackedWidget;
	title_stack->addWidget(streaming_title);
	auto title_edit = new QLineEdit;
	connect(title_edit, &QLineEdit::textChanged,
		[title_edit, settings] { obs_data_set_string(settings, "name", title_edit->text().toUtf8().constData()); });
	title_stack->addWidget(title_edit);
	title_stack->setCurrentWidget(streaming_title);
	server_title_layout->addWidget(title_stack, 1, Qt::AlignLeft);

	auto configButton = new QPushButton;
	configButton->setMinimumHeight(30);

	auto renameButton = new QPushButton(QString::fromUtf8(obs_frontend_get_locale_string("Rename")));
	renameButton->setCheckable(true);
	connect(renameButton, &QPushButton::clicked, [renameButton, title_stack, title_edit, streaming_title, settings] {
		if (title_stack->currentWidget() == title_edit) {
			streaming_title->setText(title_edit->text());
			title_stack->setCurrentWidget(streaming_title);
		} else {
			title_edit->setText(streaming_title->text());
			title_stack->setCurrentWidget(title_edit);
		}
	});
	//renameButton->setProperty("themeID", "configIconSmall");
	server_title_layout->addWidget(renameButton, 0, Qt::AlignRight);

	const bool advanced = obs_data_get_bool(settings, "advanced");
	auto advancedGroup = new QGroupBox(QString::fromUtf8(obs_frontend_get_locale_string("Advanced")));
	advancedGroup->setVisible(advanced);
	auto advancedGroupLayout = new QFormLayout;
	advancedGroup->setLayout(advancedGroupLayout);

	auto videoEncoder = new QComboBox;
	videoEncoder->addItem(QString::fromUtf8(obs_module_text("MainEncoder")), QVariant(QString::fromUtf8("")));
	videoEncoder->setCurrentIndex(0);
	advancedGroupLayout->addRow(QString::fromUtf8(obs_module_text("VideoEncoder")), videoEncoder);

	auto videoEncoderIndex = new QComboBox;
	for (int i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
		videoEncoderIndex->addItem(QString::number(i + 1));
	}
	videoEncoderIndex->setCurrentIndex(obs_data_get_int(settings, "video_encoder_index"));
	connect(videoEncoderIndex, &QComboBox::currentIndexChanged, [videoEncoderIndex, settings] {
		if (videoEncoderIndex->currentIndex() >= 0)
			obs_data_set_int(settings, "video_encoder_index", videoEncoderIndex->currentIndex());
	});
	advancedGroupLayout->addRow(QString::fromUtf8(obs_module_text("VideoEncoderIndex")), videoEncoderIndex);

	auto videoEncoderGroup = new QGroupBox(QString::fromUtf8(obs_module_text("VideoEncoder")));
	videoEncoderGroup->setProperty("altColor", QVariant(true));
	auto videoEncoderGroupLayout = new QFormLayout();
	videoEncoderGroup->setLayout(videoEncoderGroupLayout);
	advancedGroupLayout->addRow(videoEncoderGroup);

	connect(videoEncoder, &QComboBox::currentIndexChanged,
		[this, serverGroup, advancedGroupLayout, videoEncoder, videoEncoderIndex, videoEncoderGroup,
		 videoEncoderGroupLayout, settings] {
			auto encoder_string = videoEncoder->currentData().toString().toUtf8();
			auto encoder = encoder_string.constData();
			obs_data_set_string(settings, "video_encoder", encoder);
			if (!encoder || encoder[0] == '\0') {
				advancedGroupLayout->setRowVisible(videoEncoderIndex, true);
				videoEncoderGroup->setVisible(false);
			} else {
				advancedGroupLayout->setRowVisible(videoEncoderIndex, false);
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
	advancedGroupLayout->addRow(QString::fromUtf8(obs_module_text("AudioEncoder")), audioEncoder);

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
	advancedGroupLayout->addRow(QString::fromUtf8(obs_module_text("AudioTrack")), audioTrack);

	auto audioEncoderIndex = new QComboBox;
	for (int i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++) {
		audioEncoderIndex->addItem(QString::number(i + 1));
	}
	audioEncoderIndex->setCurrentIndex(obs_data_get_int(settings, "audio_encoder_index"));
	connect(audioEncoderIndex, &QComboBox::currentIndexChanged, [audioEncoderIndex, settings] {
		if (audioEncoderIndex->currentIndex() >= 0)
			obs_data_set_int(settings, "audio_encoder_index", audioEncoderIndex->currentIndex());
	});
	advancedGroupLayout->addRow(QString::fromUtf8(obs_module_text("AudioEncoderIndex")), audioEncoderIndex);

	auto audioEncoderGroup = new QGroupBox(QString::fromUtf8(obs_module_text("AudioEncoder")));
	audioEncoderGroup->setProperty("altColor", QVariant(true));
	auto audioEncoderGroupLayout = new QFormLayout();
	audioEncoderGroup->setLayout(audioEncoderGroupLayout);
	advancedGroupLayout->addRow(audioEncoderGroup);

	connect(audioEncoder, &QComboBox::currentIndexChanged,
		[this, serverGroup, advancedGroupLayout, audioEncoder, audioEncoderIndex, audioEncoderGroup,
		 audioEncoderGroupLayout, audioTrack, settings] {
			auto encoder_string = audioEncoder->currentData().toString().toUtf8();
			auto encoder = encoder_string.constData();
			obs_data_set_string(settings, "audio_encoder", encoder);
			if (!encoder || encoder[0] == '\0') {
				advancedGroupLayout->setRowVisible(audioEncoderIndex, true);
				advancedGroupLayout->setRowVisible(audioTrack, false);
				audioEncoderGroup->setVisible(false);
			} else {
				advancedGroupLayout->setRowVisible(audioEncoderIndex, false);
				advancedGroupLayout->setRowVisible(audioTrack, true);
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
	advancedGroupLayout->setRowVisible(audioTrack, audioEncoder->currentIndex() > 0);

	auto advancedButton = new QPushButton(QString::fromUtf8(obs_frontend_get_locale_string("Advanced")));
	advancedButton->setProperty("themeID", "configIconSmall");
	advancedButton->setCheckable(true);
	advancedButton->setChecked(advanced);
	connect(advancedButton, &QPushButton::clicked, [advancedButton, advancedGroup, settings] {
		const bool advanced = advancedButton->isChecked();
		advancedGroup->setVisible(advanced);
		obs_data_set_bool(settings, "advanced", advanced);
	});
	server_title_layout->addWidget(advancedButton, 0, Qt::AlignRight);

	auto removeButton =
		new QPushButton(QIcon(":/res/images/minus.svg"), QString::fromUtf8(obs_frontend_get_locale_string("Remove")));
	removeButton->setProperty("themeID", QVariant(QString::fromUtf8("removeIconSmall")));
	connect(removeButton, &QPushButton::clicked, [this, outputsLayout, serverGroup, settings] {
		outputsLayout->removeWidget(serverGroup);
		RemoveWidget(serverGroup);
		auto outputs = obs_data_get_array(this->settings, "outputs");
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
		obs_data_array_release(outputs);
	});

	server_title_layout->addWidget(removeButton, 0, Qt::AlignRight);

	serverLayout->addRow(server_title_layout);

	serverLayout->addRow(advancedGroup);

	auto server = new QComboBox;
	server->setEditable(true);

	server->addItem("rtmps://a.rtmps.youtube.com:443/live2");
	server->addItem("rtmps://b.rtmps.youtube.com:443/live2?backup=1");
	server->addItem("rtmp://a.rtmp.youtube.com/live2");
	server->addItem("rtmp://b.rtmp.youtube.com/live2?backup=1");
	server->setCurrentText(QString::fromUtf8(obs_data_get_string(settings, "server")));
	connect(server, &QComboBox::currentTextChanged,
		[server, settings] { obs_data_set_string(settings, "server", server->currentText().toUtf8().constData()); });
	serverLayout->addRow(QString::fromUtf8(obs_module_text("Server")), server);

	QLayout *subLayout = new QHBoxLayout();
	auto key = new QLineEdit;
	key->setEchoMode(QLineEdit::Password);
	key->setText(QString::fromUtf8(obs_data_get_string(settings, "key")));
	connect(key, &QLineEdit::textChanged,
		[key, settings] { obs_data_set_string(settings, "key", key->text().toUtf8().constData()); });

	QPushButton *show = new QPushButton();
	show->setText(QString::fromUtf8(obs_frontend_get_locale_string("Show")));
	show->setCheckable(true);
	show->connect(show, &QAbstractButton::toggled, [=](bool hide) {
		show->setText(
			QString::fromUtf8(hide ? obs_frontend_get_locale_string("Hide") : obs_frontend_get_locale_string("Show")));
		key->setEchoMode(hide ? QLineEdit::Normal : QLineEdit::Password);
	});

	subLayout->addWidget(key);
	subLayout->addWidget(show);

	serverLayout->addRow(QString::fromUtf8(obs_module_text("Key")), subLayout);

	serverGroup->setLayout(serverLayout);

	outputsLayout->addRow(serverGroup);
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
			d->AddServer(d->mainOutputsLayout, data);
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
