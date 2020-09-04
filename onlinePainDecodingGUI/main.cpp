#include "paindetectclient.h"
#include <QApplication>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
//#include "pldsmodel.h"
//#include "detector.h"
//#include "acquisitor.h"
#include <numeric>
//#include "settings.h"
#include <welcomedialog.h>
#include <dodialog.h>
#include <dialog.h>
#include "dataCollector.h"
#include "algorithms.h"
//#include "ICCallBack.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //painBMI::DoManager doManager;
    //doManager.initPlexDO();
    //runGrabber(".");

    WelcomeDialog wd;
    int rwd = wd.exec();
    if (rwd==0) return 0;

    //std::cout <<"rwd="<< rwd <<std::endl;

    PainDetectClient w;
    // online decoding mode
    if (rwd==1)
    {
        Dialog t;
        //t = new Dialog();
        int r = t.exec();
        std::cout << r<<std::endl;

        if (r==0) return 0;
        w.init(t.lastPath);
        w.show();
        return a.exec();
    }

    // do recording mode
    if (rwd==2)
    {
        //Dialog t;
        //t = new Dialog();
        //int r = t.exec();
        //std::cout << r<<std::endl;

        //if (r==0) return 0;
        std::cout << "DO mode"<<std::endl;
        DoDialog dow;
        //dow.init(t.lastPath);
        dow.exec();
        return 0;
    }

    //PainDetectClient w;
    // w.show();
    return a.exec();
}
