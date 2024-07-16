#pragma once

#include <QDialog>
#include <QWidget>
#include <QStackedWidget>

class OutputDialog : public QDialog {
	Q_OBJECT
private:
	QWidget *WizardServicePage();
	
	QWidget *WizardInfoKick();
	QWidget *WizardInfoYouTube();
	QWidget *WizardInfoTwitter();
	QWidget *WizardInfoUnknown();
	QWidget *WizardInfoTwitch();
	QWidget *WizardInfoTrovo();
	QWidget *WizardInfoTikTok();
	QWidget *WizardInfoFacebook();
	
	QStackedWidget *stackedWidget;
public:
	OutputDialog(QDialog *parent);
};
