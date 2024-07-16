#include "output-dialog.hpp"

#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QStackedWidget>

OutputDialog::OutputDialog(QDialog *parent) : QDialog(parent) {
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
	
	
	auto stackedLayout = new QVBoxLayout;
	stackedLayout->addWidget(stackedWidget);
	stackedLayout->setAlignment(stackedWidget, Qt::AlignTop);
	stackedLayout->setContentsMargins(4, 4, 4, 4);
	
	setMinimumSize(650, 400);
		
	setLayout(stackedLayout);
	show();
}

QWidget *OutputDialog::WizardServicePage() {
	auto page = new QWidget(this);
		
	auto pageLayout = new QVBoxLayout;
	
	auto title = new QLabel(QString("This is my title"));
	pageLayout->addWidget(title);
	
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
