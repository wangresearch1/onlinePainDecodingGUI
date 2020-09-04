#include <windows.h>
#include <process.h>
#include <iostream>
//#include <tisudshl.h>
#include "PlexDO.h"
#include "PlexDoManager.h"

namespace painBMI{

int PlexDo::init()
{
    int          numDOCards;
    unsigned int deviceNumbers[16];
    unsigned int numDigitalOutputBits[16];
    unsigned int numDigitalOutputLines[16];
    // Get info on available digital output devices
    printf("\n\nPlexDO Digital Output Sample Application\n");
    printf("========================================\n");
    printf("\ngetting info on digital output devices...\n");
    numDOCards = PL_DOGetDigitalOutputInfo(deviceNumbers, numDigitalOutputBits,
                                           numDigitalOutputLines);
    if (PL_DOInitDevice(1, false)) // false == not shared by MAP
    {
        printf("PL_DOInitDevice failed!\n");
        return(-1);
    }
    deviceNum = deviceNumbers[0];
    // set line mode for GPCTR1 to clock generation
    printf("setting line mode for line %d to clock generation\n", 2);
    if (PL_DOSetLineMode(deviceNum, 2, CLOCK_GEN))
    {
        printf("PL_DOSetLineMode failed!\n");
        return(-1);
    }
    return 0;
}
void PlexDo::sendLaserOn(LaserType type)
{
    laser_stop_required = false;
    laserOn = true;
    int chn =0;
    int evn_chn = 0;
    switch (type)
    {
    case High_Power:
        chn = get_do_chn("h_laser_on");
        evn_chn = get_do_chn("h_laser_on_event");
        break;
    case Low_Power:
        chn = get_do_chn("l_laser_on");
        evn_chn = get_do_chn("l_laser_on_event");
        break;
    default:
        chn = get_do_chn("h_laser_on");
        evn_chn = get_do_chn("h_laser_on_event");
    }

    if (PL_DOSetBit(1, chn))
    {
        printf("PL_DOSetBit laser failed!\n");
    }
    if (PL_DOSetBit(1, evn_chn))
    {
        printf("PL_DOSetBit event failed!\n");
    }

    int cnt = get_stim_time();
    while (cnt > 0 && !laser_stop_required)
    {
        PL_Sleep(min(1000, cnt));
        cnt -= 1000;
        if (cnt > 0)
        {
            std::cout << cnt / 1000 << std::endl;
        }
    }

    if (PL_DOClearBit(1, chn))
    {
        printf("PL_DOClearBit failed!\n");
        //return;
    }

    if (PL_DOClearBit(1, evn_chn))
    {
        printf("PL_DOClearBit event failed!\n");
        //return;
    }
    laserOn = false;
    return;
}
void PlexDo::sendLaserWarm(LaserType type)
{
    laser_stop_required = false;
    laserOn = true;
    int chn =0;
    int evn_chn = 0;
    switch (type)
    {
    case High_Power:
        chn = get_do_chn("h_laser_on");
        break;
    case Low_Power:
        chn = get_do_chn("l_laser_on");
        break;
    default:
        chn = get_do_chn("h_laser_on");
    }

    if (PL_DOSetBit(1, chn))
    {
        printf("PL_DOSetBit laser failed!\n");
    }

    int cnt = get_warm_time();
    while (cnt > 0 && !laser_stop_required)
    {
        PL_Sleep(min(1000, cnt));
        cnt -= 1000;
        if (cnt > 0)
        {
            std::cout << cnt / 1000 << std::endl;
        }
    }

    if (PL_DOClearBit(1, chn))
    {
        printf("PL_DOClearBit failed!\n");
        //return;
    }
    laserOn = false;
    return;
}
void PlexDo::sendOptoOn()
{
    opto_stop_required = false;
    OptoOn = true;
    // clock waveform: 100 usec high, 100 usec low = 5 kHz frequency
    printf("setting pulse for line %d to high=%d ms, low = %d ms\n", 2, opto_pulse_h, opto_pulse_l);
    if (int n = PL_DOSetClockParams(deviceNum, 2, opto_pulse_h * 1000, opto_pulse_l * 1000))
    {
        printf("PL_DOSetClockParams failed!\n");
        //exit(-1);
    }

    // start the clock
    printf("starting clock on line %d\n", 2);
    if (PL_DOStartClock(deviceNum, 2))
    {
        printf("PL_DOStartClock failed!\n");
        //exit(-1);
    }
    //send opto start pulse
    if (PL_DOSetBit(1, get_do_chn("opto_start")))
    {
        printf("PL_DOSetBit opto start failed!\n");
    }
    printf("%f Hz clock is now running on line %d\n", 1000.0 / (opto_pulse_h + opto_pulse_l), 2);
    int cnt = opto_duration;
    while (cnt > 0 && !opto_stop_required)
    {
        PL_Sleep(min(1000, cnt));
        cnt -= 1000;
        if (cnt > 0)
        {
            std::cout << cnt / 1000 <<" s"<< std::endl;
        }
    }
    // stop the clock
    printf("stopping clock on line %d\n", 2);
    if (PL_DOStopClock(deviceNum, 2))
    {
        printf("PL_DOStopClock failed!\n");
        //exit(-1);
    }
    if (PL_DOClearBit(1, get_do_chn("opto_start")))
    {
        printf("PL_DOClearBit opto start failed!\n");
    }
    OptoOn = false;
    return;
}
void PlexDo::sendtrigger()
{
    triggerOn = true;
    // current 100 hz
    int period = 10;
    int chn = get_do_chn("trigger");
    if (PL_DOSetBit(1, chn))
    {
        printf("PL_DOSetBit trigger failed! chn=%d\n",chn);
    }

    PL_Sleep(period/2);

    if (PL_DOClearBit(1, chn))
    {
        printf("PL_DOClearBit failed!\n");
        //return;
    }
    triggerOn = false;
    return;
}
void PlexDo::sendDetection(std::string name)
{
    int chn = get_do_chn(name);
    //triggerOn = true;
    // current 100 hz
    int period = 10;
    if (PL_DOSetBit(1, chn))
    {
        printf("PL_DOSetBit trigger failed! chn=%d\n",chn);
    }

    PL_Sleep(period/2);

    if (PL_DOClearBit(1, chn))
    {
        printf("PL_DOClearBit failed!\n");
        //return;
    }
    //triggerOn = false;
    return;
}
}
