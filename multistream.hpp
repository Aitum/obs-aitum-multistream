#pragma once

#include "config-dialog.hpp"
#include <obs.h>
#include <obs-frontend-api.h>
#include <QFrame>
#include <QPushButton>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>
#include <mutex>

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

	// Guards `outputs`. It is read on the UI thread (the refresh timer, the
	// dock buttons, the websocket vendor) and written from an output's own
	// signal thread by stream_output_stop(), which erases entries.
	//
	// Recursive because obs_output_force_stop() raises "stop" synchronously on
	// the calling thread, so a caller that already holds the lock re-enters it
	// through stream_output_stop().
	std::recursive_mutex outputs_mutex;
	std::vector<std::tuple<std::string, obs_output_t *, QPushButton *>> outputs;
	obs_data_array_t *vertical_outputs = nullptr;
	bool exiting = false;
	bool finished_loading = false;

	void LoadSettingsFile();
	void LoadSettings();
	void LoadOutput(obs_data_t *data, bool vertical);
	void SaveSettings();

	// Outcome of building and starting one output.
	//
	// The two error fields serve different audiences and deliberately do not
	// share a vocabulary: `error` is a stable snake_case identifier that goes
	// out over the websocket API and should not change once clients depend on
	// it, while `locale_key` is the obs_module_text() key for the dialog the
	// dock shows. Either may be nullptr when there is nothing useful to say.
	struct StartOutputResult {
		bool ok = false;
		const char *error = nullptr;
		const char *locale_key = nullptr;
	};

	// Builds and starts an output. Contains no UI, so the websocket vendor can
	// call it without a dialog appearing on an unattended machine.
	StartOutputResult StartOutputInternal(obs_data_t *settings, QPushButton *streamButton);

	// Dock-button entry point: confirmation dialog, StartOutputInternal, then a
	// warning box if it failed.
	bool StartOutput(obs_data_t *settings, QPushButton *streamButton);

	void outputButtonStyle(QPushButton *button);

	void storeMainStreamEncoders();

	void AskUpdate();

	QIcon streamActiveIcon = QIcon(":/aitum/media/streaming.svg");
	QIcon streamInactiveIcon = QIcon(":/aitum/media/stream.svg");

	static void frontend_event(enum obs_frontend_event event, void *private_data);

	static void stream_output_stop(void *data, calldata_t *calldata);
	static void stream_output_start(void *data, calldata_t *calldata);

private slots:
	void ApiInfo(QString info);

public:
	MultistreamDock(QWidget *parent = nullptr);
	~MultistreamDock();
	void LoadVerticalOutputs(bool firstLoad = true);

	// Remote control via the obs-websocket vendor ("aitum-multistream"). All
	// of these run on the UI thread (see run_on_dock in multistream.cpp) and
	// never show dialogs. The Remote* methods return nullptr on success, or an
	// error string for the websocket response; the Fill* methods only read.
	const char *RemoteStartOutput(const QString &name);
	const char *RemoteStopOutput(const QString &name);
	const char *RemoteStartVerticalOutput(const QString &name);
	const char *RemoteStopVerticalOutput(const QString &name);
	void FillStatus(obs_data_t *response_data);
	void FillOutputs(obs_data_t *response_data);
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
