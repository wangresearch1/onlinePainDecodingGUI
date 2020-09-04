#ifndef DODIALOG_H
#define DODIALOG_H

#include <QDialog>
#include "plexDoManager.h"
#include "qthreadmanager.h"
#include "configManager.h"

namespace Ui {
class DoDialog;
}

using namespace painBMI;

class DoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DoDialog(QWidget *parent = 0);
    ~DoDialog();

private slots:

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

private:
    Ui::DoDialog *ui;
    PlexDo* pDo;
    ThreadManager* laserThread;
    ThreadManager* optoThread;
    ConfigManager conf;
    std::string lastPath;
    std::string lastFolder;
    AlgorithmManager* p_alg_;
private slots:
    void sendLaserh();
    void sendLaserl();
    void stopLaser();
};

#endif // DODIALOG_H
