#include "dodialog.h"
#include "ui_dodialog.h"
#include <iostream>
#include <qfiledialog.h>
#include <qmessagebox.h>
#include <qshortcut.h>

DoDialog::DoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DoDialog),
    laserThread(nullptr),
    optoThread(nullptr),
    conf()
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
        conf.updateConfig(lastPath);
        pDo = new PlexDo(conf.doConfig);
        pDo->init();
    }

    QString textStr=QString::fromStdString("last file path: "+lastPath);
    ui->label->setText(textStr);

    QShortcut* pslh = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_Q), this, SLOT(sendLaserh()));
    QShortcut* psll = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_W), this, SLOT(sendLaserl()));
    QShortcut* pssl = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_Z), this, SLOT(stopLaser()));

    p_alg_ = new AlgorithmManager(conf.algConfig);
    p_alg_->ready = true;
    laserThread = new ThreadManager(Laser,p_alg_,nullptr,nullptr,pDo);
    optoThread = new ThreadManager(Laser,p_alg_,nullptr,nullptr,pDo);
}

DoDialog::~DoDialog()
{
    delete pDo;
    delete ui;
}

void DoDialog::on_pushButton_clicked()
{
    system("start D:\\Silas\\IC_grabber_DO\\IC_grabber_DO_vc10.exe");
}

void DoDialog::on_pushButton_2_clicked()
{
    QString filename = QFileDialog::getOpenFileName(
                this,
                tr("Open Config File"),
                QString::fromStdString(lastFolder),
                "All Files (*.*);;INI file (*.ini)"
                );
    std::cout << filename.toStdString() << "loaded" << std::endl;
    lastPath = filename.toStdString();
    lastFolder = lastPath.substr(0, lastPath.find_last_of("\\/"));

    conf.updateConfig(lastPath);
    delete pDo;
    pDo = new PlexDo(conf.doConfig);
}

void DoDialog::sendLaserh()
{
    std::cout << "laser on received"<<std::endl;
    if (!laserThread->isRunning())
    {
        std::vector<std::string> tmp;
        tmp.push_back("Stim");
        tmp.push_back("High");
        laserThread->setParam(tmp);
        laserThread->start(QThread::LowestPriority);
        std::cout << "laser on running"<<std::endl;
    }
}

void DoDialog::sendLaserl()
{
    std::cout << "laser on received"<<std::endl;
    if (!laserThread->isRunning())
    {
        std::vector<std::string> tmp;
        tmp.push_back("Stim");
        tmp.push_back("Low");
        laserThread->setParam(tmp);
        laserThread->start(QThread::LowestPriority);
        std::cout << "laser on running"<<std::endl;
    }
}

void DoDialog::stopLaser()
{
    std::cout << "stop laser on received"<<std::endl;
    pDo->stopLaser();
}
