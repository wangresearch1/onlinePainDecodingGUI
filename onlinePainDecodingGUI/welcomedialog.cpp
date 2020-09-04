#include "welcomedialog.h"
#include "ui_welcomedialog.h"
#include <qprocess.h>

WelcomeDialog::WelcomeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::WelcomeDialog)
{
    ui->setupUi(this);
}

WelcomeDialog::~WelcomeDialog()
{
    delete ui;
}

void WelcomeDialog::on_pushButton_clicked()
{
    done(1);
}

void WelcomeDialog::on_pushButton_2_clicked()
{
    done(2);
}

void WelcomeDialog::on_pushButton_3_clicked()
{
    system("start D:\\Silas\\IC_grabber_DO\\IC_grabber_DO_vc10.exe");
}
