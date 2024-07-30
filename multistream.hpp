#pragma once

#include "config-dialog.hpp"
#include <obs.h>
#include <obs-frontend-api.h>
#include <QFrame>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTimer>
#include <QString>

class OBSBasicSettings;

class MultistreamDock : public QFrame {
	Q_OBJECT

private:
	OBSBasicSettings *configDialog = nullptr;

	obs_data_t *current_config = nullptr;

	QVBoxLayout *mainCanvasLayout = nullptr;
	QVBoxLayout *mainCanvasOutputLayout = nullptr;
	QVBoxLayout *verticalCanvasLayout = nullptr;
	QVBoxLayout *verticalCanvasOutputLayout = nullptr;
	QPushButton *mainStreamButton = nullptr;
	QPushButton *configButton = nullptr;
	QLabel *mainPlatformIconLabel = nullptr;
	QString mainPlatformUrl;

	QString newer_version_available;

	QTimer videoCheckTimer;
	video_t *mainVideo = nullptr;
	std::vector<video_t *> oldVideo;

	std::vector<std::tuple<std::string, obs_output_t *, QPushButton *>> outputs;

	void LoadSettingsFile();
	void LoadSettings();
	void LoadOutput(obs_data_t *data, bool vertical);
	void SaveSettings();

	bool StartOutput(obs_data_t *settings, QPushButton *streamButton);

	void outputButtonStyle(QPushButton *button);

	void storeMainStreamEncoders();

	QIcon streamActiveIcon = QIcon(":/aitum/media/streaming.svg");
	QIcon streamInactiveIcon = QIcon(":/aitum/media/stream.svg");

	static void frontend_event(enum obs_frontend_event event, void *private_data);

	static void stream_output_stop(void *data, calldata_t *calldata);
	static void stream_output_start(void *data, calldata_t *calldata);

private slots:
	void NewerVersionAvailable(QString version);

public:
	MultistreamDock(QWidget *parent = nullptr);
	~MultistreamDock();
	void LoadVerticalOutputs(bool firstLoad = true);
};
