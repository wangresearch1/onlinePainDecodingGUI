#ifndef QTHREADMANAGER_H
#define QTHREADMANAGER_H
#include <QThread>
#include <QString>
#include<QDebug>
#include<string>
#include "algorithms.h"
#include "dataCollector.h"
//#include "ICCallBack.h"
#include "plexDoManager.h"
//#include "paindetectclient.h"
#include "ui_paindetectclient.h"

namespace painBMI
{
enum ThreadType{Collecting,Training,Decoding,Laser,Opto,Trigger,Detection};

class ThreadManager: public QThread
{
private:
    ThreadType type_;
    QString name_;
    bool abort_;
    AlgorithmManager* p_alg_;
    PlexDo* p_do;
    PlexDataCollector* p_collector_;
    Ui::PainDetectClient *p_ui_;
    std::string detector_;
    std::vector<std::string> param;
    int cnt = 0;
public:
    ThreadManager(ThreadType type, AlgorithmManager* p_alg, PlexDataCollector* p_collector,
                  Ui::PainDetectClient* p_ui=nullptr, PlexDo* p_do_=nullptr,
                  std::string detector="",std::string name = "unnamed")
        :type_(type),name_(QString::fromStdString(name)),abort_(false),
          p_alg_(p_alg),p_collector_(p_collector),p_ui_(p_ui),p_do(p_do_),
          detector_(detector),param()
    {}

    void set_ui(Ui::PainDetectClient* ui_)
    {
        p_ui_ = ui_;
    }

    void run()
    {
        switch (type_)
        {
        case Collecting:
            std::cout << "data collector"<<std::endl;
            break;
        case Training:
            std::cout << "training:"<<detector_<<cnt++<<std::endl;
            runTrainingThread();
            break;
        case Decoding:
            std::cout << "decoding"<<std::endl;
            break;
        case Laser:
            //std::cout << p_alg_->ready<<std::endl;
            std::cout << "p_alg_->ready="<<p_alg_->ready<<std::endl;
            while (!(p_alg_->ready))
            {
                sleep(1);
                std::cout << "p_alg_->ready="<<p_alg_->ready<<std::endl;
            };
            runLaserThread();
            std::cout << "Laser sent"<<std::endl;
            break;
        case Opto:
            runOptoThread();
            std::cout << "Opto"<<std::endl;
            break;
        case Trigger:
            runTriggerThread();
            //std::cout << "Detection"<<std::endl;
            break;
        case Detection:
            runDetectionThread();
            //std::cout << "Trigger"<<std::endl;
            break;
        default:
            std::cout << "invalid thread type"<<std::endl;
            break;
        }
    }

    std::string get_detector()const {return detector_;}
    ThreadType getType()const {return type_;}
    std::string getName() const {return name_.toStdString();}

    void runTrainingThread()
    {
        for (int i=0;i<p_alg_->get_detector_list().size();i++)
        {
            if (detector_.compare(p_alg_->get_detector_list()[i]->get_name())==0)
            {
                //std::cout<< "train start: detector="<<p_alg_->get_detector_list()[i]->get_name()<<std::endl;
                ModelDetector* mdp = static_cast<ModelDetector*>(p_alg_->get_detector_list()[i]);
                int l_train = mdp->get_l_train();
                std::cout << "===================== train_length " << l_train << "======================" <<std::endl;
                std::string region = mdp->get_region();
                Eigen::MatrixXf trainData =p_collector_->read(region,l_train,0);
                for (int i=0; i<trainData.rows(); i++)
                {
                    QTableWidgetItem *item = p_ui_->tableWidget_2->item(i, 0);
                    if(item->checkState()==Qt::Unchecked)
                    {
                        trainData.row(i) << 0;
                    }
                }
                mdp->train(trainData.transpose());
                break;
            }
        }
    }

    void runLaserThread()
    {
        if (param.size()<2)
        {
            std::cout <<"not enough params"<<std::endl;
            return;
        }
        if (param[0].compare("Stim")==0)
        {
            if (param[1].compare("High")==0)
            {
                p_do->sendLaserOn(High_Power);
                std::cout <<"high power laser on"<<std::endl;
            }
            else
            {
                p_do->sendLaserOn(Low_Power);
                std::cout <<"low power laser on"<<std::endl;
            }
        }
        else
        {
            if (param[1].compare("High")==0)
            {
                p_do->sendLaserWarm(High_Power);
                std::cout <<"high power laser warm"<<std::endl;
            }
            else
            {
                p_do->sendLaserWarm(Low_Power);
                std::cout <<"low power laser warm"<<std::endl;
            }
        }
    }

    void runOptoThread()
    {
        p_do->sendOptoOn();
        std::cout <<"opto started"<<std::endl;
    }

    void runTriggerThread()
    {
        p_do->sendtrigger();
        //std::cout <<"send trigger"<<std::endl;
    }

    void runDetectionThread()
    {
        p_do->sendDetection(name_.toStdString());
        std::cout <<"send "<<name_.toStdString()<<std::endl;
    }

    void setParam(const std::vector<std::string> param_){param=param_;}

    void abort()
    {
        abort_ = true;
    }

};
}

#endif // QTHREADMANAGER_H
