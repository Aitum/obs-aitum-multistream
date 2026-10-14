#pragma once

#include "config-dialog.hpp"
#include <obs-websocket-api.h>
#include <obs.h>
#include <obs-frontend-api.h>
#include <QFrame>
#include <QPushButton>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>

class OBSBasicSettings;

class MultistreamDock : public QFrame {
	Q_OBJECT

private:
	OBSBasicSettings *configDialog = nullptr;

	obs_data_t *current_config = nullptr;

	QVBoxLayout *mainLayout = nullptr;
	QVBoxLayout *mainCanvasLayout = nullptr;
	QVBoxLayout *mainCanvasOutputLayout = nullptr;
	QVBoxLayout *verticalCanvasLayout = nullptr;
	QVBoxLayout *verticalCanvasOutputLayout = nullptr;
	QPushButton *mainStreamButton = nullptr;
	QPushButton *configButton = nullptr;
	QLabel *mainPlatformIconLabel = nullptr;
	QString mainPlatformUrl;

	QString newer_version_available;
	time_t partnerBlockTime = 0;

	QTimer videoCheckTimer;
	video_t *mainVideo = nullptr;
	std::vector<video_t *> oldVideo;

	// UI-thread-only state. Output signal handlers queue work to this thread.
	std::vector<std::tuple<std::string, obs_output_t *, QPushButton *>> outputs;
	obs_data_array_t *vertical_outputs = nullptr;
	bool exiting = false;
	bool finished_loading = false;

	obs_websocket_vendor vendor = nullptr;

	void LoadSettingsFile();
	void LoadSettings();
	void LoadOutput(obs_data_t *data, bool vertical);
	void SaveSettings();

	// Keep websocket error identifiers separate from translatable dialog keys.
	struct StartOutputResult {
		bool ok = false;
		const char *error = nullptr;
		const char *locale_key = nullptr;
	};

	StartOutputResult StartOutputInternal(obs_data_t *settings, QPushButton *streamButton);

	bool StartOutput(obs_data_t *settings, QPushButton *streamButton);

	void outputButtonStyle(QPushButton *button);

	void storeMainStreamEncoders();

	void AskUpdate();
	const char *RemoteVerticalOutput(const QString &name, bool start);

	QIcon streamActiveIcon = QIcon(":/aitum/media/streaming.svg");
	QIcon streamInactiveIcon = QIcon(":/aitum/media/stream.svg");

	static void frontend_event(enum obs_frontend_event event, void *private_data);

	static void stream_output_stop(void *data, calldata_t *calldata);
	static void stream_output_start(void *data, calldata_t *calldata);

	// Static callbacks resolve the live dock through run_on_dock().
	static void vendor_request_version(obs_data_t *request_data, obs_data_t *response_data, void *self);
	static void vendor_request_get_outputs(obs_data_t *request_data, obs_data_t *response_data, void *self);
	static void vendor_request_start_output(obs_data_t *request_data, obs_data_t *response_data, void *self);
	static void vendor_request_stop_output(obs_data_t *request_data, obs_data_t *response_data, void *self);

	enum class VendorOutputAction { Start, Stop };
	static void vendor_request_output_action(obs_data_t *request_data, obs_data_t *response_data, VendorOutputAction action);
	void vendor_emit_output_state(obs_output_t *output, bool active);

	// UI-thread-only helpers; remote actions never show dialogs.
	const char *RemoteStartOutput(const QString &name);
	const char *RemoteStopOutput(const QString &name);
	void FillOutputs(obs_data_t *response_data);

private slots:
	void ApiInfo(QString info);

public:
	MultistreamDock(QWidget *parent = nullptr);
	~MultistreamDock();
	void LoadVerticalOutputs(bool firstLoad = true);
	void LoadWebsocket();
};

class AspectRatioPixmapLabel : public QLabel {
	Q_OBJECT
public:
	explicit AspectRatioPixmapLabel(QWidget *parent = 0);
	virtual int heightForWidth(int width) const;
	virtual QSize sizeHint() const;
	QPixmap scaledPixmap() const;
public slots:
	void setPixmap(const QPixmap &);
	void resizeEvent(QResizeEvent *);

private:
	QPixmap pix;
};
