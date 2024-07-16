#include "output-dialog.hpp"

#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QStackedWidget>
#include <QPushButton>
#include <QAbstractButton>
#include <QToolButton>
#include "obs-module.h"


QToolButton *selectionButton(std::string title, QIcon icon) {
	auto button = new QToolButton;
	
	button->setText(QString::fromUtf8(title));
	button->setIcon(icon);
	button->setIconSize(QSize(32, 32));
	button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	button->setStyleSheet("min-width: 110px; max-width: 110px; min-height: 90px; max-height: 90px; padding-top: 16px; font-weight: bold;");
	
	return button;
}

OutputDialog::OutputDialog(QDialog *parent) : QDialog(parent) {
	setModal(true);
	stackedWidget = new QStackedWidget;
	
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
	
	setWindowTitle(obs_module_text("NewOutputWindowTitle"));
	
	auto stackedLayout = new QVBoxLayout;
	stackedLayout->addWidget(stackedWidget);
	stackedLayout->setAlignment(stackedWidget, Qt::AlignTop);
	stackedLayout->setContentsMargins(4, 4, 4, 4);
	
	setMinimumSize(650, 400);
	setMaximumSize(650, 400);
		
	setLayout(stackedLayout);
	show();
}

QWidget *OutputDialog::WizardServicePage() {
	auto page = new QWidget(this);
		
	auto pageLayout = new QVBoxLayout;
	
	auto description = new QLabel(QString::fromUtf8(obs_module_text("NewOutputSelectService")));
	description->setStyleSheet("margin-bottom: 20px;");
	pageLayout->addWidget(description);
	
	// layout for service selection
	auto selectionLayout = new QVBoxLayout;
	
	auto spacerTest = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
	
	pageLayout->addSpacerItem(spacerTest);
	
	// row 1
	auto rowOne = new QHBoxLayout;
	
	rowOne->addWidget(selectionButton("Twitch", platformIconTwitch));
	rowOne->addWidget(selectionButton("YouTube", platformIconYouTube));
	rowOne->addWidget(selectionButton("TikTok", platformIconTikTok));
	rowOne->addWidget(selectionButton("Facebook", platformIconFacebook));
	
	selectionLayout->addLayout(rowOne);
	
	// row 2
	auto rowTwo = new QHBoxLayout;
	
	rowTwo->addWidget(selectionButton("Trovo", platformIconTrovo));
	rowTwo->addWidget(selectionButton("X (Twitter)", platformIconTwitter));
	rowTwo->addWidget(selectionButton("Kick", platformIconKick));
	rowTwo->addWidget(selectionButton(obs_module_text("OtherService"), platformIconUnknown));
	
	selectionLayout->addLayout(rowTwo);
		
	//
	pageLayout->addSpacerItem(spacerTest);
	pageLayout->addLayout(selectionLayout);
	

	//	pageLayout->setAlignment(Qt::AlignCenter);
	
	page->setLayout(pageLayout);
	
	return page;
}

QWidget *OutputDialog::WizardInfoKick() {
	auto page = new QWidget(this);
		
	auto pageLayout = new QVBoxLayout;
	
	auto title = new QLabel(QString("Kick Service page"));
	pageLayout->addWidget(title);
	
	page->setLayout(pageLayout);
	
	return page;
}

QWidget *OutputDialog::WizardInfoYouTube() {
	auto page = new QWidget(this);
		
	auto pageLayout = new QVBoxLayout;
	
	auto title = new QLabel(QString("YouTube Service page"));
	pageLayout->addWidget(title);
	
	page->setLayout(pageLayout);
	
	return page;
}

QWidget *OutputDialog::WizardInfoTwitter() {
	auto page = new QWidget(this);
		
	auto pageLayout = new QVBoxLayout;
	
	auto title = new QLabel(QString("Twitter Service page"));
	pageLayout->addWidget(title);
	
	page->setLayout(pageLayout);
	
	return page;
}

QWidget *OutputDialog::WizardInfoUnknown() {
	auto page = new QWidget(this);
		
	auto pageLayout = new QVBoxLayout;
	
	auto title = new QLabel(QString("Unknown Service page"));
	pageLayout->addWidget(title);
	
	page->setLayout(pageLayout);
	
	return page;
}

QWidget *OutputDialog::WizardInfoTwitch() {
	auto page = new QWidget(this);
		
	auto pageLayout = new QVBoxLayout;
	
	auto title = new QLabel(QString("Twitch Service page"));
	pageLayout->addWidget(title);
	
	page->setLayout(pageLayout);
	
	return page;
}

QWidget *OutputDialog::WizardInfoTrovo() {
	auto page = new QWidget(this);
		
	auto pageLayout = new QVBoxLayout;
	
	auto title = new QLabel(QString("Trovo Service page"));
	pageLayout->addWidget(title);
	
	page->setLayout(pageLayout);
	
	return page;
}

QWidget *OutputDialog::WizardInfoTikTok() {
	auto page = new QWidget(this);
		
	auto pageLayout = new QVBoxLayout;
	
	auto title = new QLabel(QString("Tiktok Service page"));
	pageLayout->addWidget(title);
	
	page->setLayout(pageLayout);
	
	return page;
}

QWidget *OutputDialog::WizardInfoFacebook() {
	auto page = new QWidget(this);
		
	auto pageLayout = new QVBoxLayout;
	
	auto title = new QLabel(QString("Facebook Service page"));
	pageLayout->addWidget(title);
	
	page->setLayout(pageLayout);
	
	return page;
}
