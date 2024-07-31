#include "stream-key-input.hpp"
#include <QFocusEvent>

StreamKeyInput::StreamKeyInput(QWidget *parent) : QLineEdit(parent)
{
}

void StreamKeyInput::focusInEvent(QFocusEvent *event)
{
	QLineEdit::focusInEvent(event);
	emit focusGained();
}

void StreamKeyInput::focusOutEvent(QFocusEvent *event)
{
	QLineEdit::focusOutEvent(event);
	emit focusLost();
}
