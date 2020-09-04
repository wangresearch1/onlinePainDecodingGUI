#ifndef THREADMANAGER_H
#define THREADMANAGER_H
#include <QThread>
#include <QString>
#include<QDebug>
#include<string>
#include "algorithms.h"
#include "dataCollector.h"

namespace painBMI
{
    enum ThreadType{Acquisitor,Training,Decoding};

    class ThreadManager:public QThread
    {
    private:
        ThreadType type_;
        std::string name_;
        AlgorithmManager* p_alg;
        PlexDataCollector* p_collector;
        std::string detector;
    public:
        ThreadManager(ThreadType type,AlgorithmManager* p_alg_,PlexDataCollector* p_collector_,
                      std::string name = "unnamed",std::string detector_ = "")
            :type_(type),name_(name),
            p_alg(p_alg_),p_collector(p_collector_),detector(detector_)
        {}

        void run()
        {
            switch (type)
            {
            case Acquisitor:
                //call acquisitor func
                break;
            case Training:
                //call training func

                break;
            case Decoding:
                //call acquisitor func
                break;
            }
        }
    };
}


#endif // THREADMANAGER_H
