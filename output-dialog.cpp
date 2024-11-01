#include "output-dialog.hpp"

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QStackedWidget>
#include <QPushButton>
#include <QAbstractButton>
#include <QToolButton>
#include <QFormLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QSizePolicy>
#include "obs-module.h"
#include "util/platform.h"
#include "stream-key-input.hpp"

// Reset output values, e.g. when user hits the back button
void OutputDialog::resetOutputs()
{
	outputName = QString("");
	outputServer = QString("");
	outputKey = QString("");
}

// For when we're done
void OutputDialog::acceptOutputs()
{
	accept();
}

// For validating the current values and then updating the confirm button state
void OutputDialog::validateOutputs(QPushButton *confirmButton)
{

	if (outputName.isEmpty() || otherNames.contains(outputName)) {
		confirmButton->setEnabled(false);
	} else if (outputServer.isEmpty()) {
		confirmButton->setEnabled(false);
	} else if (outputKey.isEmpty()) {
		confirmButton->setEnabled(false);
	} else {
		confirmButton->setEnabled(true);
	}
}

// Helper for generating info QLabels
QLabel *generateInfoLabel(std::string text)
{
	auto label = new QLabel;
	label->setTextFormat(Qt::RichText);
	label->setOpenExternalLinks(true);
	label->setText(QString::fromUtf8(obs_module_text(text.c_str())));
	label->setWordWrap(true);
	label->setAlignment(Qt::AlignTop);

	return label;
}

// Helper for generating form QLabels
QLabel *generateFormLabel(std::string text)
{
	auto label = new QLabel(QString::fromUtf8(obs_module_text(text.c_str())));
	label->setStyleSheet("font-weight: bold;");

	return label;
}

// Helper for generating output name field
QLineEdit *OutputDialog::generateOutputNameField(std::string text, QPushButton *confirmButton, bool edit)
{
	auto field = new QLineEdit;
	field->setText(QString::fromUtf8(obs_module_text(text.c_str())));
	field->setStyleSheet("padding: 4px 8px;");

	if (edit) { // edit mode, set field value from output value
		field->setText(outputName);
	}

	connect(field, &QLineEdit::textEdited, [this, field, confirmButton] {
		outputName = field->text();
		validateOutputs(confirmButton);
	});

	return field;
}

// Helper for generating output server field
QLineEdit *OutputDialog::generateOutputServerField(QPushButton *confirmButton, bool locked, bool edit)
{
	auto field = new QLineEdit;
	field->setStyleSheet("padding: 4px 8px;");
	field->setDisabled(locked);

	if (edit) { // edit mode, set field value from output value
		field->setText(outputServer);
	}

	if (!locked) {
		connect(field, &QLineEdit::textEdited, [this, field, confirmButton] {
			outputServer = field->text();
			validateOutputs(confirmButton);
		});
	}

	return field;
}

// Helper for generating QComboBoxes for server selection
QComboBox *OutputDialog::generateOutputServerCombo(std::string service, QPushButton *confirmButton, bool edit)
{
	auto combo = new QComboBox;
	combo->setMinimumHeight(30);
	combo->setStyleSheet("padding: 4px 8px;");

	if (service == "Twitch") {
		auto twitch_cache = obs_module_get_config_path(obs_get_module("rtmp-services"), "twitch_ingests.json");
		if (twitch_cache) {
			auto json = obs_data_create_from_json_file(twitch_cache);
			bfree(twitch_cache);
			combo->addItem(QString::fromUtf8("Default"), QString::fromUtf8("rtmp://live.twitch.tv/app"));
			auto ingests = obs_data_get_array(json, "ingests");
			obs_data_array_enum(
				ingests,
				[](obs_data_t *ingest_data, void *param) {
					auto c = (QComboBox *)param;
					auto url = QString::fromUtf8(obs_data_get_string(ingest_data, "url_template"));
					url.replace(QString::fromUtf8("/{stream_key}"), QString::fromUtf8(""));
					c->addItem(QString::fromUtf8(obs_data_get_string(ingest_data, "name")), url);
				},
				combo);
			obs_data_array_release(ingests);
			obs_data_release(json);
		}
	}

	auto rawOptions = getService(service);

	// turn raw options into actual selectable options
	if (rawOptions != nullptr) {
		auto servers = obs_data_get_array(rawOptions, "servers");
		auto totalServers = obs_data_array_count(servers);

		for (size_t idx = 0; idx < totalServers; idx++) {
			auto item = obs_data_array_item(servers, idx);
			combo->addItem(obs_data_get_string(item, "name"), obs_data_get_string(item, "url"));
		}
	}

	// If we're edit, look up the current value for outputServer and try to set the index
	if (edit) {
		auto selectionIndex = combo->findData(outputServer);
		//		blog(LOG_WARNING, "[Aitum Multistream] edit selection %i for value %s", selectionIndex, outputServer.toStdString().c_str());
		if (selectionIndex != -1) {
			combo->setCurrentIndex(selectionIndex);
		}
	}

	// Hook event up after
	connect(combo, &QComboBox::currentIndexChanged, [this, combo, confirmButton] {
		outputServer = combo->currentData().toString();
		validateOutputs(confirmButton);
	});

	// Set default value for server
	//	outputServer = combo->currentData().toString();

	return combo;
}

// Helper for generating output key field
QLineEdit *OutputDialog::generateOutputKeyField(QPushButton *confirmButton, bool edit)
{
	auto field = new StreamKeyInput;
	field->setStyleSheet("padding: 4px 8px;");

	if (edit) { // edit mode, set field value from output value
		field->setText(outputKey);
	}

	// Immediately hide
	field->setEchoMode(StreamKeyInput::EchoMode::Password);

	// On focus, show field
	connect(field, &StreamKeyInput::focusGained, [this, field] { field->setEchoMode(StreamKeyInput::EchoMode::Normal); });

	// On blur, hide field
	connect(field, &StreamKeyInput::focusLost, [this, field] { field->setEchoMode(StreamKeyInput::EchoMode::Password); });

	connect(field, &QLineEdit::textEdited, [this, field, confirmButton] {
		outputKey = field->text();
		validateOutputs(confirmButton);
	});

	return field;
}

// Helper for generating wizard button layout
QHBoxLayout *OutputDialog::generateWizardButtonLayout(QPushButton *confirmButton, QPushButton *serviceButton, bool edit)
{
	auto layout = new QHBoxLayout;
	layout->setSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);

	if (!edit && serviceButton != nullptr) { // Not edit mode and we have a service (back) button
		layout->addWidget(serviceButton, 1);
		layout->addStretch(1);

		layout->addWidget(confirmButton, 0, Qt::AlignRight);
	} else { // edit mode, push the button to the right
		layout->addWidget(confirmButton, 0, Qt::AlignRight);
	}

	return layout;
}

// Helper for generating QPushButtons w/ style
QPushButton *generateButton(QString text)
{
	auto button = new QPushButton;
	button->setText(text);
	button->setStyleSheet("padding: 4px 12px;");
	button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	return button;
}

// Helper for generating back button
QPushButton *OutputDialog::generateBackButton()
{
	auto button = generateButton(QString("< Back"));

	connect(button, &QPushButton::clicked, [this] {
		stackedWidget->setCurrentIndex(0);
		resetOutputs();
	});

	return button;
}

QToolButton *OutputDialog::selectionButton(std::string title, QIcon icon, int selectionStep)
{
	auto button = new QToolButton;

	button->setText(QString::fromUtf8(title));
	button->setIcon(icon);
	button->setIconSize(QSize(32, 32));
	button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	button->setStyleSheet(
		"min-width: 110px; max-width: 110px; min-height: 90px; max-height: 90px; padding-top: 16px; font-weight: bold;");

	connect(button, &QPushButton::clicked, [this, selectionStep] { stackedWidget->setCurrentIndex(selectionStep); });

	return button;
}

// Gets a service from the service array matched by name
obs_data_t *OutputDialog::getService(std::string serviceName)
{
	auto total = obs_data_array_count(servicesData);

	for (size_t idx = 0; idx < total; idx++) {
		auto item = obs_data_array_item(servicesData, idx);
		//		blog(LOG_WARNING, "[Aitum Multistream] %s", obs_data_get_string(item, "name"));

		if (serviceName == obs_data_get_string(item, "name")) {
			return item;
		}
	}

	return nullptr;
}

OutputDialog::OutputDialog(QDialog *parent, QStringList _otherNames) : QDialog(parent), otherNames(_otherNames)
{
	// Load the services from rtmp-services plugin
	auto servicesPath = obs_module_get_config_path(obs_get_module("rtmp-services"), "services.json");
	auto absolutePath = os_get_abs_path_ptr(servicesPath);
	auto allData = obs_data_create_from_json_file(absolutePath);

	servicesData = obs_data_get_array(allData, "services");

	bfree(allData);
	bfree(servicesPath);
	bfree(absolutePath);

	// Blank info
	resetOutputs();

	//
	setModal(true);
	setContentsMargins(0, 0, 0, 0);
	setFixedSize(650, 400);

	stackedWidget = new QStackedWidget;

	stackedWidget->setStyleSheet("padding: 0px; margin: 0px;");

	// Service selection page
	stackedWidget->addWidget(WizardServicePage());

	// Pages for each service
	stackedWidget->addWidget(WizardInfoKick());
	stackedWidget->addWidget(WizardInfoYouTube());
	stackedWidget->addWidget(WizardInfoTwitter());
	stackedWidget->addWidget(WizardInfoUnknown());
	stackedWidget->addWidget(WizardInfoTwitch());
	stackedWidget->addWidget(WizardInfoTrovo());
	stackedWidget->addWidget(WizardInfoTikTok());
	stackedWidget->addWidget(WizardInfoFacebook());

	stackedWidget->setCurrentIndex(0);

	stackedWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	setWindowTitle(obs_module_text("NewOutputWindowTitle"));

	auto stackedLayout = new QVBoxLayout;
	stackedLayout->setContentsMargins(0, 0, 0, 0);
	stackedLayout->addWidget(stackedWidget, 1);

	setLayout(stackedLayout);
	show();
}

// Edit mode
OutputDialog::OutputDialog(QDialog *parent, QString name, QString server, QString key, QStringList _otherNames)
	: QDialog(parent),
	  otherNames(_otherNames)
{

	// Load the services from rtmp-services plugin
	auto servicesPath = obs_module_get_config_path(obs_get_module("rtmp-services"), "services.json");
	auto absolutePath = os_get_abs_path_ptr(servicesPath);
	auto allData = obs_data_create_from_json_file(absolutePath);

	servicesData = obs_data_get_array(allData, "services");

	bfree(allData);
	bfree(servicesPath);
	bfree(absolutePath);

	// Blank info
	resetOutputs();

	// Set info from constructor
	outputName = name;
	outputServer = server;
	outputKey = key;

	//
	setModal(true);
	setContentsMargins(0, 0, 0, 0);
	setFixedSize(650, 400);

	auto layout = new QVBoxLayout();

	// Add the appropriate page to the layout based upon the server url
	if (outputServer.contains(QString::fromUtf8("ingest.global-contribute.live-video.net")) ||
	    outputServer.contains(QString::fromUtf8(".contribute.live-video.net")) ||
	    outputServer.contains(QString::fromUtf8(".twitch.tv"))) { // twitch
		layout->addWidget(WizardInfoTwitch(true));
	} else if (outputServer.contains(QString::fromUtf8(".youtube.com"))) { // youtube
		layout->addWidget(WizardInfoYouTube(true));
	} else if (outputServer.contains(QString::fromUtf8("fa723fc1b171.global-contribute.live-video.net"))) { // kick
		layout->addWidget(WizardInfoKick(true));
	} else if (outputServer.contains(QString::fromUtf8(".tiktokcdn-"))) { // tiktok
		layout->addWidget(WizardInfoTikTok(true));
	} else if (outputServer.contains(QString::fromUtf8(".pscp.tv"))) { // twitter
		layout->addWidget(WizardInfoTwitter(true));
	} else if (outputServer.contains(QString::fromUtf8("livepush.trovo.live"))) { // trovo
		layout->addWidget(WizardInfoTrovo(true));
	} else if (outputServer.contains(QString::fromUtf8(".fbcdn.net")) ||
		   outputServer.contains(QString::fromUtf8(".facebook.com"))) { // facebook
		layout->addWidget(WizardInfoFacebook(true));
	} else { // unknown
		layout->addWidget(WizardInfoUnknown(true));
	}

	setLayout(layout);

	setWindowTitle(obs_module_text("EditOutputWindowTitle"));

	show();
}

QWidget *OutputDialog::WizardServicePage()
{
	auto page = new QWidget(this);
	auto pageLayout = new QVBoxLayout;
	pageLayout->setAlignment(Qt::AlignTop);

	auto description = new QLabel(QString::fromUtf8(obs_module_text("NewOutputSelectService")));
	description->setWordWrap(true);
	description->setStyleSheet("margin-bottom: 20px;");
	pageLayout->addWidget(description);

	// layout for service selection
	auto selectionLayout = new QVBoxLayout;
	selectionLayout->setAlignment(Qt::AlignCenter);

	auto spacerTest = new QSpacerItem(20, 45, QSizePolicy::Fixed, QSizePolicy::Fixed);

	pageLayout->addSpacerItem(spacerTest);

	// row 1
	auto rowOne = new QHBoxLayout;

	rowOne->addWidget(selectionButton("Twitch", platformIconTwitch, 5));
	rowOne->addWidget(selectionButton("YouTube", platformIconYouTube, 2));
	rowOne->addWidget(selectionButton("TikTok", platformIconTikTok, 7));
	rowOne->addWidget(selectionButton("Facebook", platformIconFacebook, 8));

	selectionLayout->addLayout(rowOne);

	// row 2
	auto rowTwo = new QHBoxLayout;

	rowTwo->addWidget(selectionButton("Trovo", platformIconTrovo, 6));
	rowTwo->addWidget(selectionButton("X (Twitter)", platformIconTwitter, 3));
	rowTwo->addWidget(selectionButton("Kick", platformIconKick, 1));
	rowTwo->addWidget(selectionButton(obs_module_text("OtherService"), platformIconUnknown, 4));

	selectionLayout->addLayout(rowTwo);

	//
	pageLayout->addLayout(selectionLayout);
	page->setLayout(pageLayout);

	return page;
}

QWidget *OutputDialog::WizardInfoKick(bool edit)
{
	auto page = new QWidget(this);
	page->setStyleSheet("padding: 0px; margin: 0px;");

	// Layout
	auto pageLayout = new QVBoxLayout;
	pageLayout->setSpacing(12);

	// Heading
	auto title = new QLabel(QString::fromUtf8(obs_module_text(edit ? "KickServiceInfoEdit" : "KickServiceInfo")));
	title->setWordWrap(true);
	title->setTextFormat(Qt::RichText);
	pageLayout->addWidget(title);

	// Content
	auto contentLayout = new QVBoxLayout;

	// Confirm button - initialised here so we can set state in form input connects
	auto confirmButton = generateButton(QString(obs_module_text(edit ? "SaveOutput" : "CreateOutput")));

	// Form
	auto formLayout = new QFormLayout;
	formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
	formLayout->setSpacing(12);

	// Output name
	auto outputNameField = generateOutputNameField("KickOutput", confirmButton, edit);
	formLayout->addRow(generateFormLabel("OutputName"), outputNameField);

	// Server field
	auto serverSelection = generateOutputServerField(confirmButton, true, edit);
	serverSelection->setText("rtmps://fa723fc1b171.global-contribute.live-video.net");
	formLayout->addRow(generateFormLabel("KickServer"), serverSelection);

	// Server info
	formLayout->addWidget(generateInfoLabel("KickServerInfo"));

	// Server key
	auto outputKeyField = generateOutputKeyField(confirmButton, edit);
	formLayout->addRow(generateFormLabel("KickStreamKey"), outputKeyField);

	// Server key info
	formLayout->addWidget(generateInfoLabel("KickStreamKeyInfo"));

	contentLayout->addLayout(formLayout);

	// spacing
	auto spacer = new QSpacerItem(1, 20, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
	contentLayout->addSpacerItem(spacer);

	pageLayout->addLayout(contentLayout);

	// back button
	auto serviceButton = edit ? nullptr : generateBackButton();

	// Controls layout
	auto controlsLayout = generateWizardButtonLayout(confirmButton, serviceButton, edit);

	// confirm button (initialised above so we can set state)
	connect(confirmButton, &QPushButton::clicked, [this] { acceptOutputs(); });

	// Hook it all together
	pageLayout->addLayout(controlsLayout, 1);
	page->setLayout(pageLayout);

	// Defaults for when we're changed to
	if (!edit) {
		connect(stackedWidget, &QStackedWidget::currentChanged,
			[this, outputNameField, serverSelection, outputKeyField, confirmButton] {
				if (stackedWidget->currentIndex() == 1) {
					outputName = outputNameField->text();
					outputServer = serverSelection->text();
					outputKey = outputKeyField->text();
					validateOutputs(confirmButton);
				}
			});
	}

	return page;
}

QWidget *OutputDialog::WizardInfoYouTube(bool edit)
{
	auto page = new QWidget(this);
	page->setStyleSheet("padding: 0px; margin: 0px;");

	// Layout
	auto pageLayout = new QVBoxLayout;
	pageLayout->setSpacing(12);

	// Heading
	auto title = new QLabel(QString::fromUtf8(obs_module_text("YouTubeServiceInfo")));
	pageLayout->addWidget(title);

	// Content
	auto contentLayout = new QVBoxLayout;

	// Confirm button - initialised here so we can set state in form input connects
	auto confirmButton = generateButton(QString(obs_module_text(edit ? "SaveOutput" : "CreateOutput")));

	// Form
	auto formLayout = new QFormLayout;
	formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
	formLayout->setSpacing(12);

	// Output name
	auto outputNameField = generateOutputNameField("YouTubeOutput", confirmButton, edit);
	formLayout->addRow(generateFormLabel("OutputName"), outputNameField);

	// Server selection
	auto serverSelection = generateOutputServerCombo("YouTube - RTMPS", confirmButton, edit);
	formLayout->addRow(generateFormLabel("YouTubeServer"), serverSelection);

	// Server info
	formLayout->addWidget(generateInfoLabel("YouTubeServerInfo"));

	// Server key
	auto outputKeyField = generateOutputKeyField(confirmButton, edit);
	formLayout->addRow(generateFormLabel("YouTubeStreamKey"), outputKeyField);

	// Server key info
	formLayout->addWidget(generateInfoLabel("YouTubeStreamKeyInfo"));

	contentLayout->addLayout(formLayout);

	// spacing
	auto spacer = new QSpacerItem(1, 20, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
	contentLayout->addSpacerItem(spacer);

	pageLayout->addLayout(contentLayout);

	// back button
	auto serviceButton = edit ? nullptr : generateBackButton();

	// Controls layout
	auto controlsLayout = generateWizardButtonLayout(confirmButton, serviceButton, edit);

	// confirm button (initialised above so we can set state)
	connect(confirmButton, &QPushButton::clicked, [this] { acceptOutputs(); });

	// Hook it all together
	pageLayout->addLayout(controlsLayout, 1);
	page->setLayout(pageLayout);

	// Defaults for when we're changed to
	if (!edit) {
		connect(stackedWidget, &QStackedWidget::currentChanged,
			[this, outputNameField, serverSelection, outputKeyField, confirmButton] {
				if (stackedWidget->currentIndex() == 2) {
					outputName = outputNameField->text();
					outputServer = serverSelection->currentData().toString();
					outputKey = outputKeyField->text();
					validateOutputs(confirmButton);
				}
			});
	}

	return page;
}

QWidget *OutputDialog::WizardInfoTwitter(bool edit)
{
	auto page = new QWidget(this);
	page->setStyleSheet("padding: 0px; margin: 0px;");

	// Layout
	auto pageLayout = new QVBoxLayout;
	pageLayout->setSpacing(12);

	// Heading
	auto title = new QLabel(QString::fromUtf8(obs_module_text("TwitterServiceInfo")));
	pageLayout->addWidget(title);

	// Content
	auto contentLayout = new QVBoxLayout;

	// Confirm button - initialised here so we can set state in form input connects
	auto confirmButton = generateButton(QString(obs_module_text(edit ? "SaveOutput" : "CreateOutput")));

	// Form
	auto formLayout = new QFormLayout;
	formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
	formLayout->setSpacing(12);

	// Output name
	auto outputNameField = generateOutputNameField("TwitterOutput", confirmButton, edit);
	formLayout->addRow(generateFormLabel("OutputName"), outputNameField);

	// Server selection
	auto serverSelection = generateOutputServerCombo("Twitter", confirmButton, edit);
	formLayout->addRow(generateFormLabel("TwitterServer"), serverSelection);

	// Server info
	formLayout->addWidget(generateInfoLabel("TwitterServerInfo"));

	// Server key
	auto outputKeyField = generateOutputKeyField(confirmButton, edit);
	formLayout->addRow(generateFormLabel("TwitterStreamKey"), outputKeyField);

	// Server key info
	formLayout->addWidget(generateInfoLabel("TwitterStreamKeyInfo"));

	contentLayout->addLayout(formLayout);

	// spacing
	auto spacer = new QSpacerItem(1, 20, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
	contentLayout->addSpacerItem(spacer);

	pageLayout->addLayout(contentLayout);

	// back button
	auto serviceButton = edit ? nullptr : generateBackButton();

	// Controls layout
	auto controlsLayout = generateWizardButtonLayout(confirmButton, serviceButton, edit);

	// confirm button (initialised above so we can set state)
	connect(confirmButton, &QPushButton::clicked, [this] { acceptOutputs(); });

	// Hook it all together
	pageLayout->addLayout(controlsLayout, 1);
	page->setLayout(pageLayout);

	// Defaults for when we're changed to
	if (!edit) {
		connect(stackedWidget, &QStackedWidget::currentChanged,
			[this, outputNameField, serverSelection, outputKeyField, confirmButton] {
				if (stackedWidget->currentIndex() == 3) {
					outputName = outputNameField->text();
					outputServer = serverSelection->currentData().toString();
					outputKey = outputKeyField->text();
					validateOutputs(confirmButton);
				}
			});
	}

	return page;
}

QWidget *OutputDialog::WizardInfoUnknown(bool edit)
{
	auto page = new QWidget(this);
	page->setStyleSheet("padding: 0px; margin: 0px;");

	// Layout
	auto pageLayout = new QVBoxLayout;
	pageLayout->setSpacing(12);

	// Heading
	auto title = new QLabel(QString::fromUtf8(obs_module_text("CustomServiceInfo")));
	title->setWordWrap(true);
	title->setTextFormat(Qt::RichText);
	pageLayout->addWidget(title);

	// Content
	auto contentLayout = new QVBoxLayout;

	// Confirm button - initialised here so we can set state in form input connects
	auto confirmButton = generateButton(QString(obs_module_text(edit ? "SaveOutput" : "CreateOutput")));

	// Form
	auto formLayout = new QFormLayout;
	formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
	formLayout->setSpacing(12);

	// Output name
	auto outputNameField = generateOutputNameField("CustomOutput", confirmButton, edit);
	formLayout->addRow(generateFormLabel("OutputName"), outputNameField);

	// Server field
	auto serverSelection = generateOutputServerField(confirmButton, false, edit);
	formLayout->addRow(generateFormLabel("CustomServer"), serverSelection);

	// Server info
	formLayout->addWidget(generateInfoLabel("CustomServerInfo"));

	// Server key
	auto outputKeyField = generateOutputKeyField(confirmButton, edit);
	formLayout->addRow(generateFormLabel("CustomStreamKey"), outputKeyField);

	// Server key info
	formLayout->addWidget(generateInfoLabel("CustomStreamKeyInfo"));

	contentLayout->addLayout(formLayout);

	// spacing
	auto spacer = new QSpacerItem(1, 20, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
	contentLayout->addSpacerItem(spacer);

	pageLayout->addLayout(contentLayout);

	// back button
	auto serviceButton = edit ? nullptr : generateBackButton();

	// Controls layout
	auto controlsLayout = generateWizardButtonLayout(confirmButton, serviceButton, edit);

	// confirm button (initialised above so we can set state)
	connect(confirmButton, &QPushButton::clicked, [this] { acceptOutputs(); });

	// Hook it all together
	pageLayout->addLayout(controlsLayout, 1);
	page->setLayout(pageLayout);

	// Defaults for when we're changed to
	if (!edit) {
		connect(stackedWidget, &QStackedWidget::currentChanged,
			[this, outputNameField, serverSelection, outputKeyField, confirmButton] {
				if (stackedWidget->currentIndex() == 4) {
					outputName = outputNameField->text();
					outputServer = serverSelection->text();
					outputKey = outputKeyField->text();
					validateOutputs(confirmButton);
				}
			});
	}

	return page;
}

QWidget *OutputDialog::WizardInfoTwitch(bool edit)
{
	auto page = new QWidget(this);
	page->setStyleSheet("padding: 0px; margin: 0px;");

	// Layout
	auto pageLayout = new QVBoxLayout;
	pageLayout->setSpacing(12);

	// Heading
	auto title = new QLabel(QString::fromUtf8(obs_module_text("TwitchServiceInfo")));
	pageLayout->addWidget(title);

	// Content
	auto contentLayout = new QVBoxLayout;

	// Confirm button - initialised here so we can set state in form input connects
	auto confirmButton = generateButton(QString(obs_module_text(edit ? "SaveOutput" : "CreateOutput")));

	// Form
	auto formLayout = new QFormLayout;
	formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
	formLayout->setSpacing(12);

	// Output name
	auto outputNameField = generateOutputNameField("TwitchOutput", confirmButton, edit);
	formLayout->addRow(generateFormLabel("OutputName"), outputNameField);

	// Server selection
	auto serverSelection = generateOutputServerCombo("Twitch", confirmButton, edit);
	formLayout->addRow(generateFormLabel("TwitchServer"), serverSelection);

	// Server info
	formLayout->addWidget(generateInfoLabel("TwitchServerInfo"));

	// Server key
	auto outputKeyField = generateOutputKeyField(confirmButton, edit);
	formLayout->addRow(generateFormLabel("TwitchStreamKey"), outputKeyField);

	// Server key info
	formLayout->addWidget(generateInfoLabel("TwitchStreamKeyInfo"));

	contentLayout->addLayout(formLayout);

	// spacing
	auto spacer = new QSpacerItem(1, 20, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
	contentLayout->addSpacerItem(spacer);

	pageLayout->addLayout(contentLayout);

	// back button
	auto serviceButton = edit ? nullptr : generateBackButton();

	// Controls layout
	auto controlsLayout = generateWizardButtonLayout(confirmButton, serviceButton, edit);

	// confirm button (initialised above so we can set state)
	connect(confirmButton, &QPushButton::clicked, [this] { acceptOutputs(); });

	// Hook it all together
	pageLayout->addLayout(controlsLayout, 1);
	page->setLayout(pageLayout);

	// Defaults for when we're changed to
	if (!edit) {
		connect(stackedWidget, &QStackedWidget::currentChanged,
			[this, outputNameField, serverSelection, outputKeyField, confirmButton] {
				if (stackedWidget->currentIndex() == 5) {
					blog(LOG_WARNING, "[Aitum Multistream] default outputname %s ",
					     outputNameField->text().toUtf8().constData());
					outputName = outputNameField->text();
					outputServer = serverSelection->currentData().toString();
					outputKey = outputKeyField->text();
					validateOutputs(confirmButton);
				}
			});
	}

	return page;
}

QWidget *OutputDialog::WizardInfoTrovo(bool edit)
{
	auto page = new QWidget(this);
	page->setStyleSheet("padding: 0px; margin: 0px;");

	// Layout
	auto pageLayout = new QVBoxLayout;
	pageLayout->setSpacing(12);

	// Heading
	auto title = new QLabel(QString::fromUtf8(obs_module_text("TrovoServiceInfo")));
	title->setWordWrap(true);
	title->setTextFormat(Qt::RichText);
	pageLayout->addWidget(title);

	// Content
	auto contentLayout = new QVBoxLayout;

	// Confirm button - initialised here so we can set state in form input connects
	auto confirmButton = generateButton(QString(obs_module_text(edit ? "SaveOutput" : "CreateOutput")));

	// Form
	auto formLayout = new QFormLayout;
	formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
	formLayout->setSpacing(12);

	// Output name
	auto outputNameField = generateOutputNameField("TrovoOutput", confirmButton, edit);
	formLayout->addRow(generateFormLabel("OutputName"), outputNameField);

	// Server field
	auto serverSelection = generateOutputServerField(confirmButton, true, edit);
	serverSelection->setText("rtmp://livepush.trovo.live/live/");

	formLayout->addRow(generateFormLabel("TrovoServer"), serverSelection);

	// Server info
	formLayout->addWidget(generateInfoLabel("TrovoServerInfo"));

	// Server key
	auto outputKeyField = generateOutputKeyField(confirmButton, edit);
	formLayout->addRow(generateFormLabel("TrovoStreamKey"), outputKeyField);

	// Server key info
	formLayout->addWidget(generateInfoLabel("TrovoStreamKeyInfo"));

	contentLayout->addLayout(formLayout);

	// spacing
	auto spacer = new QSpacerItem(1, 20, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
	contentLayout->addSpacerItem(spacer);

	pageLayout->addLayout(contentLayout);

	// back button
	auto serviceButton = edit ? nullptr : generateBackButton();

	// Controls layout
	auto controlsLayout = generateWizardButtonLayout(confirmButton, serviceButton, edit);

	// confirm button (initialised above so we can set state)
	connect(confirmButton, &QPushButton::clicked, [this] { acceptOutputs(); });

	// Hook it all together
	pageLayout->addLayout(controlsLayout, 1);
	page->setLayout(pageLayout);

	// Defaults for when we're changed to
	if (!edit) {
		connect(stackedWidget, &QStackedWidget::currentChanged,
			[this, outputNameField, serverSelection, outputKeyField, confirmButton] {
				if (stackedWidget->currentIndex() == 6) {
					outputName = outputNameField->text();
					outputServer = serverSelection->text();
					outputKey = outputKeyField->text();
					validateOutputs(confirmButton);
				}
			});
	}

	return page;
}

QWidget *OutputDialog::WizardInfoTikTok(bool edit)
{
	auto page = new QWidget(this);
	page->setStyleSheet("padding: 0px; margin: 0px;");

	// Layout
	auto pageLayout = new QVBoxLayout;
	pageLayout->setSpacing(12);

	// Heading
	auto title = new QLabel(QString::fromUtf8(obs_module_text("TikTokServiceInfo")));
	title->setWordWrap(true);
	title->setTextFormat(Qt::RichText);
	pageLayout->addWidget(title);

	// Content
	auto contentLayout = new QVBoxLayout;

	// Confirm button - initialised here so we can set state in form input connects
	auto confirmButton = generateButton(QString(obs_module_text(edit ? "SaveOutput" : "CreateOutput")));

	// Form
	auto formLayout = new QFormLayout;
	formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
	formLayout->setSpacing(12);

	// Output name
	auto outputNameField = generateOutputNameField("TikTokOutput", confirmButton, edit);
	formLayout->addRow(generateFormLabel("OutputName"), outputNameField);

	// Server field
	auto serverSelection = generateOutputServerField(confirmButton, false, edit);
	formLayout->addRow(generateFormLabel("TikTokServer"), serverSelection);

	// Server info
	formLayout->addWidget(generateInfoLabel("TikTokServerInfo"));

	// Server key
	auto outputKeyField = generateOutputKeyField(confirmButton, edit);
	formLayout->addRow(generateFormLabel("TikTokStreamKey"), outputKeyField);

	// Server key info
	formLayout->addWidget(generateInfoLabel("TikTokStreamKeyInfo"));

	contentLayout->addLayout(formLayout);

	// spacing
	auto spacer = new QSpacerItem(1, 20, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
	contentLayout->addSpacerItem(spacer);

	pageLayout->addLayout(contentLayout);

	// back button
	auto serviceButton = edit ? nullptr : generateBackButton();

	// Controls layout
	auto controlsLayout = generateWizardButtonLayout(confirmButton, serviceButton, edit);

	// confirm button (initialised above so we can set state)
	connect(confirmButton, &QPushButton::clicked, [this] { acceptOutputs(); });

	// Hook it all together
	pageLayout->addLayout(controlsLayout, 1);
	page->setLayout(pageLayout);

	// Defaults for when we're changed to
	if (!edit) {
		connect(stackedWidget, &QStackedWidget::currentChanged,
			[this, outputNameField, serverSelection, outputKeyField, confirmButton] {
				if (stackedWidget->currentIndex() == 7) {
					outputName = outputNameField->text();
					outputServer = serverSelection->text();
					outputKey = outputKeyField->text();
					validateOutputs(confirmButton);
				}
			});
	}

	return page;
}

QWidget *OutputDialog::WizardInfoFacebook(bool edit)
{
	auto page = new QWidget(this);
	page->setStyleSheet("padding: 0px; margin: 0px;");

	// Layout
	auto pageLayout = new QVBoxLayout;
	pageLayout->setSpacing(12);

	// Heading
	auto title = new QLabel(QString::fromUtf8(obs_module_text("FacebookServiceInfo")));
	title->setWordWrap(true);
	title->setTextFormat(Qt::RichText);
	pageLayout->addWidget(title);

	// Content
	auto contentLayout = new QVBoxLayout;

	// Confirm button - initialised here so we can set state in form input connects
	auto confirmButton = generateButton(QString(obs_module_text(edit ? "SaveOutput" : "CreateOutput")));

	// Form
	auto formLayout = new QFormLayout;
	formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
	formLayout->setSpacing(12);

	// Output name
	auto outputNameField = generateOutputNameField("FacebookOutput", confirmButton, edit);
	formLayout->addRow(generateFormLabel("OutputName"), outputNameField);

	// Server field
	auto serverSelection = generateOutputServerField(confirmButton, false, edit);
	if (!edit)
		serverSelection->setText("rtmps://rtmp-api.facebook.com:443/rtmp/");

	formLayout->addRow(generateFormLabel("FacebookServer"), serverSelection);

	// Server info
	formLayout->addWidget(generateInfoLabel("FacebookServerInfo"));

	// Server key
	auto outputKeyField = generateOutputKeyField(confirmButton, edit);
	formLayout->addRow(generateFormLabel("FacebookStreamKey"), outputKeyField);

	// Server key info
	formLayout->addWidget(generateInfoLabel("FacebookStreamKeyInfo"));

	contentLayout->addLayout(formLayout);

	// spacing
	auto spacer = new QSpacerItem(1, 20, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
	contentLayout->addSpacerItem(spacer);

	pageLayout->addLayout(contentLayout);

	// back button
	auto serviceButton = edit ? nullptr : generateBackButton();

	// Controls layout
	auto controlsLayout = generateWizardButtonLayout(confirmButton, serviceButton, edit);

	// confirm button (initialised above so we can set state)
	connect(confirmButton, &QPushButton::clicked, [this] { acceptOutputs(); });

	// Hook it all together
	pageLayout->addLayout(controlsLayout, 1);
	page->setLayout(pageLayout);

	// Defaults for when we're changed to
	if (!edit) {
		connect(stackedWidget, &QStackedWidget::currentChanged,
			[this, outputNameField, serverSelection, outputKeyField, confirmButton] {
				if (stackedWidget->currentIndex() == 8) {
					outputName = outputNameField->text();
					outputServer = serverSelection->text();
					outputKey = outputKeyField->text();
					validateOutputs(confirmButton);
				}
			});
	}

	return page;
}
