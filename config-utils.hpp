#pragma once

#include <QPushButton>
#include <QComboBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QGroupBox>
#include <QIcon>
#include <QString>
#include "obs.h"

class ConfigUtils {
public:
	static QPushButton *generateButton(QString buttonText);
	static void updateButtonStyles(QPushButton *defaultButton, QPushButton *customButton, int activeIndex);


	static QIcon getPlatformIconFromEndpoint(QString endpoint);
};
