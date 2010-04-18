#include "help.h"
#include "ui_help.h"

HelpForm::HelpForm(QWidget *parent) : QWidget(parent), ui(new Ui::HelpFormClass)  {
	ui->setupUi(this);
}

HelpForm::~HelpForm() {
}

void HelpForm::loadText(const QString& text) {
	ui->textBrowser->setText(text);
}
