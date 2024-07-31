#pragma once

#include <QLineEdit>

class StreamKeyInput : public QLineEdit 
{
	Q_OBJECT
	
public:
	explicit StreamKeyInput(QWidget *parent = nullptr);
	
signals:
	void focusGained();
	void focusLost();
	
protected:
	void focusInEvent(QFocusEvent *event) override;
	void focusOutEvent(QFocusEvent *event) override;
};
