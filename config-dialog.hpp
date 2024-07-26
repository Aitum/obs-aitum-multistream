#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QMainWindow>
#include <QTextEdit>
#include <obs-frontend-api.h>
#include <QGroupBox>
#include <QListWidget>
#include <QSpinBox>
#include <QFormLayout>
#include <QRadioButton>
#include <QLabel>
#include <QIcon>
#include <QString>

class OBSBasicSettings : public QDialog {
	Q_OBJECT
	Q_PROPERTY(QIcon generalIcon READ GetGeneralIcon WRITE SetGeneralIcon DESIGNABLE true)
	Q_PROPERTY(QIcon streamIcon READ GetStreamIcon WRITE SetStreamIcon DESIGNABLE true)
	Q_PROPERTY(QIcon outputIcon READ GetOutputIcon WRITE SetOutputIcon DESIGNABLE true)
	Q_PROPERTY(QIcon audioIcon READ GetAudioIcon WRITE SetAudioIcon DESIGNABLE true)
	Q_PROPERTY(QIcon videoIcon READ GetVideoIcon WRITE SetVideoIcon DESIGNABLE true)
	Q_PROPERTY(QIcon hotkeysIcon READ GetHotkeysIcon WRITE SetHotkeysIcon DESIGNABLE true)
	Q_PROPERTY(QIcon accessibilityIcon READ GetAccessibilityIcon WRITE SetAccessibilityIcon DESIGNABLE true)
	Q_PROPERTY(QIcon advancedIcon READ GetAdvancedIcon WRITE SetAdvancedIcon DESIGNABLE true)
private:
	QListWidget *listWidget;

	QIcon GetGeneralIcon() const;
	QIcon GetStreamIcon() const;
	QIcon GetOutputIcon() const;
	QIcon GetAudioIcon() const;
	QIcon GetVideoIcon() const;
	QIcon GetHotkeysIcon() const;
	QIcon GetAccessibilityIcon() const;
	QIcon GetAdvancedIcon() const;

	void AddServer(QFormLayout *outputsLayout, obs_data_t *settings, obs_data_array_t *outputs);
	void AddProperty(obs_properties_t *properties, obs_property_t *property, obs_data_t *settings, QFormLayout *layout);
	void RefreshProperties(obs_properties_t *properties, QFormLayout *layout);

	obs_data_t *settings = nullptr;
	obs_data_array_t *vertical_outputs = nullptr;

	std::map<obs_property_t *, QWidget *> encoder_property_widgets;
	std::map<QWidget *, obs_properties_t *> encoder_properties;

	QFormLayout *mainOutputsLayout;
	QFormLayout *verticalOutputsLayout;
	QLabel *newVersion;

	QTextEdit *troubleshooterText;

	QPushButton *verticalAddButton;

	// Platform icons
	QIcon platformIconTwitch = QIcon(":/aitum/media/twitch.png");
	QIcon platformIconYouTube = QIcon(":/aitum/media/youtube.png");
	QIcon platformIconKick = QIcon(":/aitum/media/kick.png");
	QIcon platformIconTikTok = QIcon(":/aitum/media/tiktok.png");
	QIcon platformIconTwitter = QIcon(":/aitum/media/twitter.png");
	QIcon platformIconTrovo = QIcon(":/aitum/media/trovo.png");
	QIcon platformIconFacebook = QIcon(":/aitum/media/facebook.png");
	QIcon platformIconUnknown = QIcon(":/aitum/media/unknown.png");
	QIcon getPlatformFromEndpoint(QString endpoint);

private slots:
	void SetGeneralIcon(const QIcon &icon);
	void SetStreamIcon(const QIcon &icon);
	void SetOutputIcon(const QIcon &icon);
	void SetAudioIcon(const QIcon &icon);
	void SetVideoIcon(const QIcon &icon);
	void SetHotkeysIcon(const QIcon &icon);
	void SetAccessibilityIcon(const QIcon &icon);
	void SetAdvancedIcon(const QIcon &icon);

public:
	OBSBasicSettings(QMainWindow *parent = nullptr);
	~OBSBasicSettings();

	void LoadSettings(obs_data_t *settings);
	void LoadVerticalSettings();
	void SaveVerticalSettings();
	void LoadOutputStats(std::vector<video_t *> *oldVideos);
	void SetNewerVersion(QString newer_version_available);

	QStringList mainEncoderDescriptions = QStringList(MAX_OUTPUT_VIDEO_ENCODERS);

public slots:
};
