#pragma once

#include <QDialog>
#include <QWidget>
#include <QStackedWidget>
#include <QToolButton>
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QString>
#include "obs-data.h"

class OutputDialog : public QDialog {
	Q_OBJECT
private:
	QWidget *WizardServicePage();
	
	QToolButton *selectionButton(std::string title, QIcon icon, int selectionStep);
	
	QWidget *WizardInfoKick(bool edit = false);
	QWidget *WizardInfoYouTube(bool edit = false);
	QWidget *WizardInfoTwitter(bool edit = false);
	QWidget *WizardInfoUnknown(bool edit = false);
	QWidget *WizardInfoTwitch(bool edit = false);
	QWidget *WizardInfoTrovo(bool edit = false);
	QWidget *WizardInfoTikTok(bool edit = false);
	QWidget *WizardInfoFacebook(bool edit = false);
	
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
		
	void resetOutputs();
	void acceptOutputs();
	void validateOutputs(QPushButton *confirmButton);
	
	// Generators
	QLineEdit *generateOutputNameField(std::string text, QPushButton *confirmButton);
	QLineEdit *generateOutputServerField(QPushButton *confirmButton, bool locked);
	QComboBox *generateOutputServerCombo(std::string service, QPushButton *confirmButton, bool edit = false);
	QLineEdit *generateOutputKeyField(QPushButton *confirmButton);

	obs_data_t *getService(std::string serviceName);
	
	QStackedWidget *stackedWidget;
public:
	OutputDialog(QDialog *parent);
	OutputDialog(QDialog *parent, QString name, QString server, QString key);
	
	QString outputName;
	QString outputServer;
	QString outputKey;
};
