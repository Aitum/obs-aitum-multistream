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

// Reset output values, e.g. when user hits the back button
void OutputDialog::resetOutputs() {
	outputName = QString("");
	outputServer = QString("");
	outputKey = QString("");
}

// For when we're done
void OutputDialog::acceptOutputs() {
	accept();
}

// For validating the current values and then updating the confirm button state
void OutputDialog::validateOutputs(QPushButton *confirmButton) {
	
	if (outputName.isEmpty()) {
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
QLabel *generateInfoLabel(std::string text) {
	auto label = new QLabel;
	label->setTextFormat(Qt::RichText);
	label->setOpenExternalLinks(true);
	label->setText(QString::fromUtf8(obs_module_text(text.c_str())));
	label->setWordWrap(true);
	label->setAlignment(Qt::AlignTop);
	
	return label;
}

// Helper for generating form QLabels
QLabel *generateFormLabel(std::string text) {
	auto label = new QLabel(QString::fromUtf8(obs_module_text(text.c_str())));
	label->setStyleSheet("font-weight: bold;");
	
	return label;
}

// Helper for generating QPushButtons w/ style
QPushButton *generateButton(QString text) {
	auto button = new QPushButton;
	button->setText(text);
	button->setStyleSheet("padding: 4px 12px;");
	button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	
	return button;
}

QToolButton *OutputDialog::selectionButton(std::string title, QIcon icon, int selectionStep) {
	auto button = new QToolButton;
	
	button->setText(QString::fromUtf8(title));
	button->setIcon(icon);
	button->setIconSize(QSize(32, 32));
	button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	button->setStyleSheet("min-width: 110px; max-width: 110px; min-height: 90px; max-height: 90px; padding-top: 16px; font-weight: bold;");
	
	connect(button, &QPushButton::clicked, [this, selectionStep] {
		stackedWidget->setCurrentIndex(selectionStep);
	});
	
	return button;
}

// Gets a service from the service array matched by name
obs_data_t *OutputDialog::getService(std::string serviceName) {
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

OutputDialog::OutputDialog(QDialog *parent) : QDialog(parent) {
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

QWidget *OutputDialog::WizardServicePage() {
	auto page = new QWidget(this);
	auto pageLayout = new QVBoxLayout;
	pageLayout->setAlignment(Qt::AlignTop);
	
	auto description = new QLabel(QString::fromUtf8(obs_module_text("NewOutputSelectService")));
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

QWidget *OutputDialog::WizardInfoKick() {
	auto page = new QWidget(this);
	page->setStyleSheet("padding: 0px; margin: 0px;");
	
	// Layout
	auto pageLayout = new QVBoxLayout;
	pageLayout->setSpacing(12);
	
	// Heading
	auto title = new QLabel(QString::fromUtf8(obs_module_text("KickServiceInfo")));
	title->setWordWrap(true);
	title->setTextFormat(Qt::RichText);
	pageLayout->addWidget(title);
	
	// Content
	auto contentLayout = new QVBoxLayout;
	
	// Confirm button - initialised here so we can set state in form input connects
	auto confirmButton = generateButton(QString("Create Output"));
	
	// Form
	auto formLayout = new QFormLayout;
	formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
	formLayout->setSpacing(12);
	
	// Output name
	auto outputNameField = new QLineEdit;
	outputNameField->setText(QString::fromUtf8(obs_module_text("KickOutput")));
	outputNameField->setStyleSheet("padding: 4px 8px;");
	
	connect(outputNameField, &QLineEdit::textEdited, [this, outputNameField, confirmButton] {
		outputName = outputNameField->text();
		validateOutputs(confirmButton);
	});
	
	formLayout->addRow(generateFormLabel("OutputName"), outputNameField);
	
	// Server field
	auto serverSelection = new QLineEdit;
	serverSelection->setText("rtmps://fa723fc1b171.global-contribute.live-video.net");
	serverSelection->setDisabled(true);
	serverSelection->setStyleSheet("padding: 4px 8px;");
	
//	connect(serverSelection, &QLineEdit::textEdited, [this, serverSelection, confirmButton] {
//		outputServer = serverSelection->text();
//		validateOutputs(confirmButton);
//	});
	
	formLayout->addRow(generateFormLabel("KickServer"), serverSelection);
	
	// Server info
	formLayout->addWidget(generateInfoLabel("KickServerInfo"));
	
	// Server key
	auto outputKeyField = new QLineEdit;
	outputKeyField->setStyleSheet("padding: 4px 8px;");
	connect(outputKeyField, &QLineEdit::textEdited, [this, outputKeyField, confirmButton] {
		outputKey = outputKeyField->text();
		validateOutputs(confirmButton);
	});
	
	formLayout->addRow(generateFormLabel("KickStreamKey"), outputKeyField);
	
	// Server key info
	formLayout->addWidget(generateInfoLabel("KickStreamKeyInfo"));
	
	contentLayout->addLayout(formLayout);
	
	// spacing
	auto spacer = new QSpacerItem(1, 20, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
	contentLayout->addSpacerItem(spacer);
	
	pageLayout->addLayout(contentLayout);
	
	// Controls
	auto controlsLayout = new QHBoxLayout;
	controlsLayout->setSpacing(12);
	
	// back button
	auto serviceButton = generateButton(QString("< Back"));
	
	connect(serviceButton, &QPushButton::clicked, [this] {
		stackedWidget->setCurrentIndex(0);
		resetOutputs();
	});
	
	controlsLayout->addWidget(serviceButton, 0);
	controlsLayout->addStretch(1);
	
	// confirm button (initialised above so we can set state)
	connect(confirmButton, &QPushButton::clicked, [this] {
		acceptOutputs();
	});
	
	controlsLayout->addWidget(confirmButton, 0);
	
	// Hook it all together
	pageLayout->addLayout(controlsLayout);
	page->setLayout(pageLayout);
	
	// Defaults for when we're changed to
	connect(stackedWidget, &QStackedWidget::currentChanged, [this, outputNameField, serverSelection, outputKeyField, confirmButton] {
		if (stackedWidget->currentIndex() == 1) {
			outputName = outputNameField->text();
			outputServer = serverSelection->text();
			outputKey = outputKeyField->text();
			validateOutputs(confirmButton);
		}
	});
	
	
	return page;
}

QWidget *OutputDialog::WizardInfoYouTube() {
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
	auto confirmButton = generateButton(QString("Create Output"));
	
	// Form
	auto formLayout = new QFormLayout;
	formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
	formLayout->setSpacing(12);
	
	// Output name
	auto outputNameField = new QLineEdit;
	outputNameField->setText(QString::fromUtf8(obs_module_text("YouTubeOutput")));
	outputNameField->setStyleSheet("padding: 4px 8px;");
	
	connect(outputNameField, &QLineEdit::textEdited, [this, outputNameField, confirmButton] {
		outputName = outputNameField->text();
		validateOutputs(confirmButton);
	});
	
	formLayout->addRow(generateFormLabel("OutputName"), outputNameField);
	
	// Server selection
	auto serverSelection = new QComboBox;
	serverSelection->setMinimumHeight(30);
	serverSelection->setStyleSheet("padding: 4px 8px;");
	
	connect(serverSelection, &QComboBox::currentIndexChanged, [this, serverSelection, confirmButton] {
		outputServer = serverSelection->currentData().toString();
		validateOutputs(confirmButton);
	});
	
	// Set default value for server
	outputServer = serverSelection->currentData().toString();
	
	auto rawOptions = getService("YouTube - RTMPS");
	
	// turn raw options into actual selectable options
	if (rawOptions != nullptr) {
		auto servers = obs_data_get_array(rawOptions, "servers");
		auto totalServers = obs_data_array_count(servers);
		
		for (size_t idx = 0; idx < totalServers; idx++) {
			auto item = obs_data_array_item(servers, idx);
			serverSelection->addItem(obs_data_get_string(item, "name"), obs_data_get_string(item, "url"));
		}
	}
	
	
	formLayout->addRow(generateFormLabel("YouTubeServer"), serverSelection);
	
	// Server info
	formLayout->addWidget(generateInfoLabel("YouTubeServerInfo"));
	
	// Server key
	auto outputKeyField = new QLineEdit;
	outputKeyField->setStyleSheet("padding: 4px 8px;");
	connect(outputKeyField, &QLineEdit::textEdited, [this, outputKeyField, confirmButton] {
		outputKey = outputKeyField->text();
		validateOutputs(confirmButton);
	});
	
	formLayout->addRow(generateFormLabel("YouTubeStreamKey"), outputKeyField);

	// Server key info
	formLayout->addWidget(generateInfoLabel("YouTubeStreamKeyInfo"));
	
	contentLayout->addLayout(formLayout);
	
	// spacing
	auto spacer = new QSpacerItem(1, 20, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
	contentLayout->addSpacerItem(spacer);
	
	pageLayout->addLayout(contentLayout);
	
	// Controls
	auto controlsLayout = new QHBoxLayout;
	controlsLayout->setSpacing(12);
	
	// back button
	auto serviceButton = generateButton(QString("< Back"));
	
	connect(serviceButton, &QPushButton::clicked, [this] {
		stackedWidget->setCurrentIndex(0);
		resetOutputs();
	});
	
	controlsLayout->addWidget(serviceButton, 0);
	controlsLayout->addStretch(1);
	
	// confirm button (initialised above so we can set state)
	connect(confirmButton, &QPushButton::clicked, [this] {
		acceptOutputs();
	});
	
	controlsLayout->addWidget(confirmButton, 0);

	// Hook it all together
	pageLayout->addLayout(controlsLayout);
	page->setLayout(pageLayout);
	
	// Defaults for when we're changed to
	connect(stackedWidget, &QStackedWidget::currentChanged, [this, outputNameField, serverSelection, outputKeyField, confirmButton] {
		if (stackedWidget->currentIndex() == 2) {
			outputName = outputNameField->text();
			outputServer = serverSelection->currentData().toString();
			outputKey = outputKeyField->text();
			validateOutputs(confirmButton);
		}
	});
	
	
	return page;
}

QWidget *OutputDialog::WizardInfoTwitter() {
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
	auto confirmButton = generateButton(QString("Create Output"));
	
	// Form
	auto formLayout = new QFormLayout;
	formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
	formLayout->setSpacing(12);
	
	// Output name
	auto outputNameField = new QLineEdit;
	outputNameField->setText(QString::fromUtf8(obs_module_text("TwitterOutput")));
	outputNameField->setStyleSheet("padding: 4px 8px;");
	
	connect(outputNameField, &QLineEdit::textEdited, [this, outputNameField, confirmButton] {
		outputName = outputNameField->text();
		validateOutputs(confirmButton);
	});
	
	formLayout->addRow(generateFormLabel("OutputName"), outputNameField);
	
	// Server selection
	auto serverSelection = new QComboBox;
	serverSelection->setMinimumHeight(30);
	serverSelection->setStyleSheet("padding: 4px 8px;");
	
	connect(serverSelection, &QComboBox::currentIndexChanged, [this, serverSelection, confirmButton] {
		outputServer = serverSelection->currentData().toString();
		validateOutputs(confirmButton);
	});
	
	
	auto rawOptions = getService("Twitter");
	
	// turn raw options into actual selectable options
	if (rawOptions != nullptr) {
		auto servers = obs_data_get_array(rawOptions, "servers");
		auto totalServers = obs_data_array_count(servers);
		
		for (size_t idx = 0; idx < totalServers; idx++) {
			auto item = obs_data_array_item(servers, idx);
			serverSelection->addItem(obs_data_get_string(item, "name"), obs_data_get_string(item, "url"));
		}
	}
	
	// Set default value for server
	outputServer = serverSelection->currentData().toString();
	
	formLayout->addRow(generateFormLabel("TwitterServer"), serverSelection);
	
	// Server info
	formLayout->addWidget(generateInfoLabel("TwitterServerInfo"));
	
	// Server key
	auto outputKeyField = new QLineEdit;
	outputKeyField->setStyleSheet("padding: 4px 8px;");
	connect(outputKeyField, &QLineEdit::textEdited, [this, outputKeyField, confirmButton] {
		outputKey = outputKeyField->text();
		validateOutputs(confirmButton);
	});
	
	formLayout->addRow(generateFormLabel("TwitterStreamKey"), outputKeyField);

	// Server key info
	formLayout->addWidget(generateInfoLabel("TwitterStreamKeyInfo"));
	
	contentLayout->addLayout(formLayout);
	
	// spacing
	auto spacer = new QSpacerItem(1, 20, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
	contentLayout->addSpacerItem(spacer);
	
	pageLayout->addLayout(contentLayout);
	
	// Controls
	auto controlsLayout = new QHBoxLayout;
	controlsLayout->setSpacing(12);
	
	// back button
	auto serviceButton = generateButton(QString("< Back"));
	
	connect(serviceButton, &QPushButton::clicked, [this] {
		stackedWidget->setCurrentIndex(0);
		resetOutputs();
	});
	
	controlsLayout->addWidget(serviceButton, 0);
	controlsLayout->addStretch(1);
	
	// confirm button (initialised above so we can set state)
	connect(confirmButton, &QPushButton::clicked, [this] {
		acceptOutputs();
	});
	
	controlsLayout->addWidget(confirmButton, 0);

	// Hook it all together
	pageLayout->addLayout(controlsLayout);
	page->setLayout(pageLayout);
	
	// Defaults for when we're changed to
	connect(stackedWidget, &QStackedWidget::currentChanged, [this, outputNameField, serverSelection, outputKeyField, confirmButton] {
		if (stackedWidget->currentIndex() == 3) {
			outputName = outputNameField->text();
			outputServer = serverSelection->currentData().toString();
			outputKey = outputKeyField->text();
			validateOutputs(confirmButton);
		}
	});
	
	return page;
}

QWidget *OutputDialog::WizardInfoUnknown() {
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
	auto confirmButton = generateButton(QString("Create Output"));
	
	// Form
	auto formLayout = new QFormLayout;
	formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
	formLayout->setSpacing(12);
	
	// Output name
	auto outputNameField = new QLineEdit;
	outputNameField->setText(QString::fromUtf8(obs_module_text("CustomOutput")));
	outputNameField->setStyleSheet("padding: 4px 8px;");
	
	connect(outputNameField, &QLineEdit::textEdited, [this, outputNameField, confirmButton] {
		outputName = outputNameField->text();
		validateOutputs(confirmButton);
	});
	
	formLayout->addRow(generateFormLabel("OutputName"), outputNameField);
	
	// Server field
	auto serverSelection = new QLineEdit;
	serverSelection->setStyleSheet("padding: 4px 8px;");
	
	connect(serverSelection, &QLineEdit::textEdited, [this, serverSelection, confirmButton] {
		outputServer = serverSelection->text();
		validateOutputs(confirmButton);
	});
	
	formLayout->addRow(generateFormLabel("CustomServer"), serverSelection);
	
	// Server info
	formLayout->addWidget(generateInfoLabel("CustomServerInfo"));
	
	// Server key
	auto outputKeyField = new QLineEdit;
	outputKeyField->setStyleSheet("padding: 4px 8px;");
	connect(outputKeyField, &QLineEdit::textEdited, [this, outputKeyField, confirmButton] {
		outputKey = outputKeyField->text();
		validateOutputs(confirmButton);
	});
	
	formLayout->addRow(generateFormLabel("CustomStreamKey"), outputKeyField);
	
	// Server key info
	formLayout->addWidget(generateInfoLabel("CustomStreamKeyInfo"));
	
	contentLayout->addLayout(formLayout);
	
	// spacing
	auto spacer = new QSpacerItem(1, 20, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
	contentLayout->addSpacerItem(spacer);
	
	pageLayout->addLayout(contentLayout);
	
	// Controls
	auto controlsLayout = new QHBoxLayout;
	controlsLayout->setSpacing(12);
	
	// back button
	auto serviceButton = generateButton(QString("< Back"));
	
	connect(serviceButton, &QPushButton::clicked, [this] {
		stackedWidget->setCurrentIndex(0);
		resetOutputs();
	});
	
	controlsLayout->addWidget(serviceButton, 0);
	controlsLayout->addStretch(1);
	
	// confirm button (initialised above so we can set state)
	connect(confirmButton, &QPushButton::clicked, [this] {
		acceptOutputs();
	});
	
	controlsLayout->addWidget(confirmButton, 0);
	
	// Hook it all together
	pageLayout->addLayout(controlsLayout);
	page->setLayout(pageLayout);
	
	// Defaults for when we're changed to
	connect(stackedWidget, &QStackedWidget::currentChanged, [this, outputNameField, serverSelection, outputKeyField, confirmButton] {
		if (stackedWidget->currentIndex() == 4) {
			outputName = outputNameField->text();
			outputServer = serverSelection->text();
			outputKey = outputKeyField->text();
			validateOutputs(confirmButton);
		}
	});
	
	
	return page;
}

QWidget *OutputDialog::WizardInfoTwitch() {
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
	auto confirmButton = generateButton(QString("Create Output"));
	
	// Form
	auto formLayout = new QFormLayout;
	formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
	formLayout->setSpacing(12);
	
	// Output name
	auto outputNameField = new QLineEdit;
	outputNameField->setText(QString::fromUtf8(obs_module_text("TwitchOutput")));
	outputNameField->setStyleSheet("padding: 4px 8px;");
	
	connect(outputNameField, &QLineEdit::textEdited, [this, outputNameField, confirmButton] {
		outputName = outputNameField->text();
		validateOutputs(confirmButton);
	});
	
	formLayout->addRow(generateFormLabel("OutputName"), outputNameField);
	
	// Server selection
	auto serverSelection = new QComboBox;
	serverSelection->setMinimumHeight(30);
	serverSelection->setStyleSheet("padding: 4px 8px;");
	
	connect(serverSelection, &QComboBox::currentIndexChanged, [this, serverSelection, confirmButton] {
		outputServer = serverSelection->currentData().toString();
		validateOutputs(confirmButton);
	});
	
	
	auto rawOptions = getService("Twitch");
	
	// turn raw options into actual selectable options
	if (rawOptions != nullptr) {
		auto servers = obs_data_get_array(rawOptions, "servers");
		auto totalServers = obs_data_array_count(servers);
		
		for (size_t idx = 0; idx < totalServers; idx++) {
			auto item = obs_data_array_item(servers, idx);
			serverSelection->addItem(obs_data_get_string(item, "name"), obs_data_get_string(item, "url"));
		}
	}
	
	// Set default value for server
	outputServer = serverSelection->currentData().toString();
	
	formLayout->addRow(generateFormLabel("TwitchServer"), serverSelection);
	
	// Server info
	formLayout->addWidget(generateInfoLabel("TwitchServerInfo"));
	
	// Server key
	auto outputKeyField = new QLineEdit;
	outputKeyField->setStyleSheet("padding: 4px 8px;");
	connect(outputKeyField, &QLineEdit::textEdited, [this, outputKeyField, confirmButton] {
		outputKey = outputKeyField->text();
		validateOutputs(confirmButton);
	});
	
	formLayout->addRow(generateFormLabel("TwitchStreamKey"), outputKeyField);

	// Server key info
	formLayout->addWidget(generateInfoLabel("TwitchStreamKeyInfo"));
	
	contentLayout->addLayout(formLayout);
	
	// spacing
	auto spacer = new QSpacerItem(1, 20, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
	contentLayout->addSpacerItem(spacer);
	
	pageLayout->addLayout(contentLayout);
	
	// Controls
	auto controlsLayout = new QHBoxLayout;
	controlsLayout->setSpacing(12);
	
	// back button
	auto serviceButton = generateButton(QString("< Back"));
	
	connect(serviceButton, &QPushButton::clicked, [this] {
		stackedWidget->setCurrentIndex(0);
		resetOutputs();
	});
	
	controlsLayout->addWidget(serviceButton, 0);
	controlsLayout->addStretch(1);
	
	// confirm button (initialised above so we can set state)
	connect(confirmButton, &QPushButton::clicked, [this] {
		acceptOutputs();
	});
	
	controlsLayout->addWidget(confirmButton, 0);

	// Hook it all together
	pageLayout->addLayout(controlsLayout);
	page->setLayout(pageLayout);
	
	// Defaults for when we're changed to
	connect(stackedWidget, &QStackedWidget::currentChanged, [this, outputNameField, serverSelection, outputKeyField, confirmButton] {
		if (stackedWidget->currentIndex() == 5) {
			blog(LOG_WARNING, "[Aitum Multistream] default outputname %s ", outputNameField->text().toUtf8().constData());
			outputName = outputNameField->text();
			outputServer = serverSelection->currentData().toString();
			outputKey = outputKeyField->text();
			validateOutputs(confirmButton);
		}
	});
	
	return page;
}

QWidget *OutputDialog::WizardInfoTrovo() {
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
	auto confirmButton = generateButton(QString("Create Output"));
	
	// Form
	auto formLayout = new QFormLayout;
	formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
	formLayout->setSpacing(12);
	
	// Output name
	auto outputNameField = new QLineEdit;
	outputNameField->setText(QString::fromUtf8(obs_module_text("TrovoOutput")));
	outputNameField->setStyleSheet("padding: 4px 8px;");
	
	connect(outputNameField, &QLineEdit::textEdited, [this, outputNameField, confirmButton] {
		outputName = outputNameField->text();
		validateOutputs(confirmButton);
	});
	
	formLayout->addRow(generateFormLabel("OutputName"), outputNameField);
	
	// Server field
	auto serverSelection = new QLineEdit;
	serverSelection->setText("rtmp://livepush.trovo.live/live/");
	serverSelection->setDisabled(true);
	serverSelection->setStyleSheet("padding: 4px 8px;");
	
//	connect(serverSelection, &QLineEdit::textEdited, [this, serverSelection, confirmButton] {
//		outputServer = serverSelection->text();
//		validateOutputs(confirmButton);
//	});
	
	formLayout->addRow(generateFormLabel("TrovoServer"), serverSelection);
	
	// Server info
	formLayout->addWidget(generateInfoLabel("TrovoServerInfo"));
	
	// Server key
	auto outputKeyField = new QLineEdit;
	outputKeyField->setStyleSheet("padding: 4px 8px;");
	connect(outputKeyField, &QLineEdit::textEdited, [this, outputKeyField, confirmButton] {
		outputKey = outputKeyField->text();
		validateOutputs(confirmButton);
	});
	
	formLayout->addRow(generateFormLabel("TrovoStreamKey"), outputKeyField);
	
	// Server key info
	formLayout->addWidget(generateInfoLabel("TrovoStreamKeyInfo"));
	
	contentLayout->addLayout(formLayout);
	
	// spacing
	auto spacer = new QSpacerItem(1, 20, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
	contentLayout->addSpacerItem(spacer);
	
	pageLayout->addLayout(contentLayout);
	
	// Controls
	auto controlsLayout = new QHBoxLayout;
	controlsLayout->setSpacing(12);
	
	// back button
	auto serviceButton = generateButton(QString("< Back"));
	
	connect(serviceButton, &QPushButton::clicked, [this] {
		stackedWidget->setCurrentIndex(0);
		resetOutputs();
	});
	
	controlsLayout->addWidget(serviceButton, 0);
	controlsLayout->addStretch(1);
	
	// confirm button (initialised above so we can set state)
	connect(confirmButton, &QPushButton::clicked, [this] {
		acceptOutputs();
	});
	
	controlsLayout->addWidget(confirmButton, 0);
	
	// Hook it all together
	pageLayout->addLayout(controlsLayout);
	page->setLayout(pageLayout);
	
	// Defaults for when we're changed to
	connect(stackedWidget, &QStackedWidget::currentChanged, [this, outputNameField, serverSelection, outputKeyField, confirmButton] {
		if (stackedWidget->currentIndex() == 6) {
			outputName = outputNameField->text();
			outputServer = serverSelection->text();
			outputKey = outputKeyField->text();
			validateOutputs(confirmButton);
		}
	});
	
	
	return page;
}

QWidget *OutputDialog::WizardInfoTikTok() {
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
	auto confirmButton = generateButton(QString("Create Output"));
	
	// Form
	auto formLayout = new QFormLayout;
	formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
	formLayout->setSpacing(12);
	
	// Output name
	auto outputNameField = new QLineEdit;
	outputNameField->setText(QString::fromUtf8(obs_module_text("TikTokOutput")));
	outputNameField->setStyleSheet("padding: 4px 8px;");
	
	connect(outputNameField, &QLineEdit::textEdited, [this, outputNameField, confirmButton] {
		outputName = outputNameField->text();
		validateOutputs(confirmButton);
	});
	
	formLayout->addRow(generateFormLabel("OutputName"), outputNameField);
	
	// Server field
	auto serverSelection = new QLineEdit;
	serverSelection->setStyleSheet("padding: 4px 8px;");
	
	connect(serverSelection, &QLineEdit::textEdited, [this, serverSelection, confirmButton] {
		outputServer = serverSelection->text();
		validateOutputs(confirmButton);
	});

	formLayout->addRow(generateFormLabel("TikTokServer"), serverSelection);
	
	// Server info
	formLayout->addWidget(generateInfoLabel("TikTokServerInfo"));
	
	// Server key
	auto outputKeyField = new QLineEdit;
	outputKeyField->setStyleSheet("padding: 4px 8px;");
	connect(outputKeyField, &QLineEdit::textEdited, [this, outputKeyField, confirmButton] {
		outputKey = outputKeyField->text();
		validateOutputs(confirmButton);
	});
	
	formLayout->addRow(generateFormLabel("TikTokStreamKey"), outputKeyField);

	// Server key info
	formLayout->addWidget(generateInfoLabel("TikTokStreamKeyInfo"));
	
	contentLayout->addLayout(formLayout);
	
	// spacing
	auto spacer = new QSpacerItem(1, 20, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
	contentLayout->addSpacerItem(spacer);
	
	pageLayout->addLayout(contentLayout);
	
	// Controls
	auto controlsLayout = new QHBoxLayout;
	controlsLayout->setSpacing(12);
	
	// back button
	auto serviceButton = generateButton(QString("< Back"));
	
	connect(serviceButton, &QPushButton::clicked, [this] {
		stackedWidget->setCurrentIndex(0);
		resetOutputs();
	});
	
	controlsLayout->addWidget(serviceButton, 0);
	controlsLayout->addStretch(1);
	
	// confirm button (initialised above so we can set state)
	connect(confirmButton, &QPushButton::clicked, [this] {
		acceptOutputs();
	});
	
	controlsLayout->addWidget(confirmButton, 0);

	// Hook it all together
	pageLayout->addLayout(controlsLayout);
	page->setLayout(pageLayout);
	
	// Defaults for when we're changed to
	connect(stackedWidget, &QStackedWidget::currentChanged, [this, outputNameField, serverSelection, outputKeyField, confirmButton] {
		if (stackedWidget->currentIndex() == 7) {
			outputName = outputNameField->text();
			outputServer = serverSelection->text();
			outputKey = outputKeyField->text();
			validateOutputs(confirmButton);
		}
	});
	
	
	return page;
}

QWidget *OutputDialog::WizardInfoFacebook() {
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
	auto confirmButton = generateButton(QString("Create Output"));
	
	// Form
	auto formLayout = new QFormLayout;
	formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
	formLayout->setSpacing(12);
	
	// Output name
	auto outputNameField = new QLineEdit;
	outputNameField->setText(QString::fromUtf8(obs_module_text("FacebookOutput")));
	outputNameField->setStyleSheet("padding: 4px 8px;");
	
	connect(outputNameField, &QLineEdit::textEdited, [this, outputNameField, confirmButton] {
		outputName = outputNameField->text();
		validateOutputs(confirmButton);
	});
	
	formLayout->addRow(generateFormLabel("OutputName"), outputNameField);
	
	// Server field
	auto serverSelection = new QLineEdit;
	serverSelection->setText("rtmps://rtmp-api.facebook.com:443/rtmp/");
	serverSelection->setDisabled(true);
	serverSelection->setStyleSheet("padding: 4px 8px;");
	
//	connect(serverSelection, &QLineEdit::textEdited, [this, serverSelection, confirmButton] {
//		outputServer = serverSelection->text();
//		validateOutputs(confirmButton);
//	});
	
	formLayout->addRow(generateFormLabel("FacebookServer"), serverSelection);
	
	// Server info
	formLayout->addWidget(generateInfoLabel("FacebookServerInfo"));
	
	// Server key
	auto outputKeyField = new QLineEdit;
	outputKeyField->setStyleSheet("padding: 4px 8px;");
	connect(outputKeyField, &QLineEdit::textEdited, [this, outputKeyField, confirmButton] {
		outputKey = outputKeyField->text();
		validateOutputs(confirmButton);
	});
	
	formLayout->addRow(generateFormLabel("FacebookStreamKey"), outputKeyField);
	
	// Server key info
	formLayout->addWidget(generateInfoLabel("FacebookStreamKeyInfo"));
	
	contentLayout->addLayout(formLayout);
	
	// spacing
	auto spacer = new QSpacerItem(1, 20, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
	contentLayout->addSpacerItem(spacer);
	
	pageLayout->addLayout(contentLayout);
	
	// Controls
	auto controlsLayout = new QHBoxLayout;
	controlsLayout->setSpacing(12);
	
	// back button
	auto serviceButton = generateButton(QString("< Back"));
	
	connect(serviceButton, &QPushButton::clicked, [this] {
		stackedWidget->setCurrentIndex(0);
		resetOutputs();
	});
	
	controlsLayout->addWidget(serviceButton, 0);
	controlsLayout->addStretch(1);
	
	// confirm button (initialised above so we can set state)
	connect(confirmButton, &QPushButton::clicked, [this] {
		acceptOutputs();
	});
	
	controlsLayout->addWidget(confirmButton, 0);
	
	// Hook it all together
	pageLayout->addLayout(controlsLayout);
	page->setLayout(pageLayout);
	
	// Defaults for when we're changed to
	connect(stackedWidget, &QStackedWidget::currentChanged, [this, outputNameField, serverSelection, outputKeyField, confirmButton] {
		if (stackedWidget->currentIndex() == 8) {
			outputName = outputNameField->text();
			outputServer = serverSelection->text();
			outputKey = outputKeyField->text();
			validateOutputs(confirmButton);
		}
	});
	
	
	return page;
}
