#include "config-utils.hpp"

#include <QPushButton>
#include <QString>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QAbstractButton>
#include <QWidget>
#include <QIcon>

#include "obs.h"
#include "obs-module.h"
#include "obs-frontend-api.h"
#include <util/dstr.h>


// Generate buttons for default/custom stuff
QPushButton *ConfigUtils::generateButton(QString buttonText) {
	auto styles = QString::fromUtf8("QPushButton[unselected=\"true\"] { background: pink; }");
	
	auto button = new QPushButton(buttonText);
	button->setStyleSheet(styles);
	
	return button;
}

// For setting the active property stuff on buttons
void ConfigUtils::updateButtonStyles(QPushButton *defaultButton, QPushButton *customButton, int activeIndex) {
	defaultButton->setProperty("unselected", activeIndex != 0 ? true : false);
	customButton->setProperty("unselected", activeIndex != 1 ? true : false);
}


// Platform icons deciphered from endpoints
QIcon ConfigUtils::getPlatformIconFromEndpoint(QString endpoint)
{

	if (endpoint.contains(QString::fromUtf8(".contribute.live-video.net")) ||
		endpoint.contains(QString::fromUtf8(".twitch.tv"))) { // twitch
		return QIcon(":/aitum/media/twitch.png");
	} else if (endpoint.contains(QString::fromUtf8(".youtube.com"))) { // youtube
		return QIcon(":/aitum/media/youtube.png");
	} else if (endpoint.contains(QString::fromUtf8("fa723fc1b171.global-contribute.live-video.net"))) { // kick
		return QIcon(":/aitum/media/kick.png");
	} else if (endpoint.contains(QString::fromUtf8(".tiktokcdn-"))) { // tiktok
		return QIcon(":/aitum/media/tiktok.png");
	} else if (endpoint.contains(QString::fromUtf8(".pscp.tv"))) { // twitter
		return QIcon(":/aitum/media/twitter.png");
	} else if (endpoint.contains(QString::fromUtf8("livepush.trovo.live"))) { // trovo
		return QIcon(":/aitum/media/trovo.png");
	} else if (endpoint.contains(QString::fromUtf8(".facebook.com"))) { // facebook
		return QIcon(":/aitum/media/facebook.png");
	} else { // unknown
		return QIcon(":/aitum/media/unknown.png");
	}
}