#pragma once

#include <QDialog>
#include <QWidget>
#include <QStackedWidget>
#include <QToolButton>
#include <QString>
#include "obs-data.h"

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
	
	QToolButton *selectionButton(std::string title, QIcon icon, int selectionStep);
	
	
	// Platform icons
	QIcon platformIconTwitch = QIcon(":/aitum/media/twitch.png");
	QIcon platformIconYouTube = QIcon(":/aitum/media/youtube.png");
	QIcon platformIconKick = QIcon(":/aitum/media/kick.png");
	QIcon platformIconTikTok = QIcon(":/aitum/media/tiktok.png");
	QIcon platformIconTwitter = QIcon(":/aitum/media/twitter.png");
	QIcon platformIconTrovo = QIcon(":/aitum/media/trovo.png");
	QIcon platformIconFacebook = QIcon(":/aitum/media/facebook.png");
	QIcon platformIconUnknown = QIcon(":/aitum/media/unknown.png");
	
	obs_data_array_t *servicesData;
	
	QString outputName;
	QString outputServer;
	QString outputKey;
	
	void resetOutputs();
	obs_data_t *getService(std::string serviceName);
	
	QStackedWidget *stackedWidget;
public:
	OutputDialog(QDialog *parent);
};
