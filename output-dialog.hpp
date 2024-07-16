#pragma once

#include <QWizard>
#include <QWizardPage>
#include <QDialog>

class OutputDialog : public QWizard {
	Q_OBJECT
private:
	QWizardPage *WizardPage1();
	QWizardPage *WizardPage2();
	QWizardPage *WizardPage3();
	
public:
	OutputDialog(QDialog *parent);
};
