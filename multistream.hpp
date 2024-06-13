#pragma once

#include "config-dialog.hpp"
#include <obs.h>
#include <obs-frontend-api.h>
#include <QFrame>
#include <QPushButton>
#include <QVBoxLayout>

class OBSBasicSettings;

class MultistreamDock : public QFrame {
	Q_OBJECT

private:
	OBSBasicSettings *configDialog = nullptr;

	obs_data_t *current_config = nullptr;

	QVBoxLayout *mainCanvasLayout = nullptr;
	QPushButton *mainStreamButton = nullptr;

	std::map<std::string, obs_output_t *> outputs;

	void LoadSettingsFile();
	void LoadSettings();
	void LoadOutput(obs_data_t *data);
	void SaveSettings();

	bool StartOutput(obs_data_t *settings, QPushButton *streamButton);

	QIcon streamActiveIcon = QIcon(":/aitum/media/streaming.svg");
	QIcon streamInactiveIcon = QIcon(":/aitum/media/stream.svg");

	static void frontend_event(enum obs_frontend_event event, void *private_data);

	static void stream_output_stop(void *data, calldata_t *calldata);
	static void stream_output_start(void *data, calldata_t *calldata);

public:
	MultistreamDock(QWidget *parent = nullptr);
	~MultistreamDock();
};
