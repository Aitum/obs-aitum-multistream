#include "output-dialog.hpp"

#include <QDialog>
#include <QWizard>
#include <QWizardPage>

OutputDialog::OutputDialog(QDialog *parent) : QWizard(parent) {
	setWindowTitle("this is my wizard title!!");
	
	addPage(WizardPage1());
	addPage(WizardPage2());
	addPage(WizardPage3());
	
	show();
}

QWizardPage *OutputDialog::WizardPage1() {
	auto page = new QWizardPage(this);
	
	page->setTitle(QString("page 1"));
	
	return page;
}

QWizardPage *OutputDialog::WizardPage2() {
	auto page = new QWizardPage(this);
	
	page->setTitle(QString("page 2"));
	
	return page;
}

QWizardPage *OutputDialog::WizardPage3() {
	auto page = new QWizardPage(this);
	
	page->setTitle(QString("page 3"));
	page->setFinalPage(true);
	
	return page;
}

