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
	
	
	// Platform icons
	QIcon platformIconTwitch = QIcon(":/aitum/media/twitch.png");
	QIcon platformIconYouTube = QIcon(":/aitum/media/youtube.png");
	QIcon platformIconKick = QIcon(":/aitum/media/kick.png");
	QIcon platformIconTikTok = QIcon(":/aitum/media/tiktok.png");
	QIcon platformIconTwitter = QIcon(":/aitum/media/twitter.png");
	QIcon platformIconTrovo = QIcon(":/aitum/media/trovo.png");
	QIcon platformIconFacebook = QIcon(":/aitum/media/facebook.png");
	QIcon platformIconUnknown = QIcon(":/aitum/media/unknown.png");
	
	QStackedWidget *stackedWidget;
public:
	OutputDialog(QDialog *parent);
};
