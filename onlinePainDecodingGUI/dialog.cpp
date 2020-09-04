#include "dialog.h"
#include "ui_dialog.h"
#include "configManager.h"
#include <QFileDialog>
#include <QMessageBox>

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    INIReader reader("D:\\Silas\\onlinePainDecodingGUI\\defaults.ini");
    //INIReader reader("E:\\Silas\\OnlineDecodingSystem\\defaults.ini");
    if (reader.ParseError() < 0)
    {
        std::cout << "Can't load '"<< "defaults.ini" <<"'\n";
        lastPath = "";
        lastFolder = "C:\\";
        QMessageBox::information(this,tr("load defaults failed"),"D:\\Silas\\onlinePainDecodingGUI\\defaults.ini");
        //return;
    }
    else
    {
        lastPath = reader.Get("config_file", "last_path", "UNKNOWN");
        lastFolder = reader.Get("config_file", "last_folder", "UNKNOWN");
    }

    QString textStr=QString::fromStdString("last file path: "+lastPath);
    ui->label->setText(textStr);
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::on_pushButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(
                this,
                tr("Open Config File"),
                QString::fromStdString(lastFolder),
                "All Files (*.*);;INI file (*.ini)"
                );
    std::cout << filename.toStdString() << "loaded" << std::endl;
    lastPath = filename.toStdString();
    //configManager.updateConfig(filename.toStdString());
    //QMessageBox::information(this,tr("File Name"),filename);
    lastFolder = lastPath.substr(0, lastPath.find_last_of("\\/"));
    //std::cout << lastFolder << std::endl;
    done(QDialog::Accepted);
}

void Dialog::on_pushButton_2_clicked()
{
    //configManager.updateConfig(lastPath);
    done(QDialog::Accepted);
}
