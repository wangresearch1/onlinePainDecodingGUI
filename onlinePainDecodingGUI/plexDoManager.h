/*
* Author: Sile Hu, yuehusile@gmail.com
* Created: 2017-11-18
* Copy Right: 
* Jing Wang lab, Department of Anesthesiology, NYU School of Medicine, NY, USA
* Zhe Chen lab, Department of Psychiatry, NYU School of Medicine, NY, USA
* ---------------------------------------------------------------------------------------------------------
* Description:
* This software is a part of the 'real-time pain detection rodent BMI system'.
* Using the Plexon DO API to control laser, opto, send event signals, and do camera synchronization.
* Here we are also trying to synchronize a camera from Imaging Source Inc. by using callbacks of their APIs 
*
*/
#ifndef __PLEXDOMANAGER_H__
#define __PLEXDOMANAGER_H__

#include <vector>
#include <string>
#include "configManager.h"

// *** default configurations ***
// HotKeys
#define HighPowerLaserOnKey 'Q'
#define LowPowerLaserOnKey 'W'
#define HighPowerLaserWarmKey 'A'
#define LowPowerLaserWarmKey 'S'
#define StopLaserKey 'Z'
#define StartOptoKey 'O'

// BNC
#define HighPowerLaserChannel 10
#define LowPowerLaserChannel 11
#define CameraAudioPulseChannel 12
#define OptogeneticsLineChannel 2

// DI
#define RecordingTriggerChannel 1
#define HighPowerLaserEventChannel 2

#define LowPowerLaserEventChannel 3
#define ICCameraEventChannel 4
#define CameraAudioEventChannel 5
#define OptoEventChannel 6
#define OptoStartEventChannel 7
#define NoEvent -1

//laser on time
#define LaserOnStimuliTimeMs 6000 //5s
#define LaserOnWarmTimeMs 45000 //45s
//default opto period,duration
#define OptoPulseHighMs 10 //1s,period,1 Hz
#define OptoPulseLowMs 40 //1s,period,1 Hz
#define OptoPulseDurationMs 3000 //10s, pulse train duration

namespace painBMI{

    enum LaserType{High_Power, Low_Power};
    class PlexDo
    {
    private:
        std::string device;
        int stim_time;  // in ms
        int warm_time;  // in ms
        int opto_pulse_h;
        int opto_pulse_l;
        int opto_duration;
        int opto_random_freq;
        //flags
        bool laser_stop_required;
        bool opto_stop_required;
        bool laserOn;
        bool OptoOn;
        bool triggerOn;
        int deviceNum;

        std::vector<std::string> signal_list;
        std::vector<int> channel_list;
        std::vector<std::string> hotKey_name_list;
        std::string hotKey_list;
        std::vector<std::string> param_name_list;
    public:
        PlexDo(PlexDoConfig* pdc):
            device(pdc->get_device()),
            stim_time(pdc->get_stim_time()),warm_time(pdc->get_warm_time()),
            opto_pulse_h(pdc->get_opto_pulse_h()),opto_pulse_l(pdc->get_opto_pulse_l()),
            opto_duration(pdc->get_opto_duration()),opto_random_freq(pdc->get_opto_random_freq()),
            laser_stop_required(false),opto_stop_required(false),
            laserOn(false),OptoOn(false),triggerOn(false),deviceNum(0),
            signal_list(pdc->get_do()),channel_list(pdc->get_doChn()),
            hotKey_list(pdc->get_hotKey_list()),hotKey_name_list(pdc->get_hotKey_name_list())
        {}

        const std::vector<std::string>& get_signal_list() const {return signal_list;}
        const std::vector<std::string>& get_hotKey_list() const {return hotKey_name_list;}

        int get_stim_time()   const {return stim_time;}
        int get_warm_time()  const {return warm_time;}
        std::string get_device() const {return device;}
        int get_opto_pulse_h() const {return opto_pulse_h;}
        int get_opto_pulse_l() const {return opto_pulse_l;}
        int get_opto_duration() const {return opto_duration;}
        int get_opto_random_freq() const {return opto_random_freq;}

        int get_do_chn(std::string sig_) const//{return channelTable[sig_]; }
        {
            //std::cout <<"sig="<<sig_<<","<<std::endl;
            for (auto iter=signal_list.begin();iter<signal_list.end();iter++)
            {
                //std::cout <<"iter="<<*iter<<","<<std::endl;
                if (sig_.compare(*iter)==0)
                {
                    //std::cout <<"idx="<<iter-signal_list.begin()<<","<<std::endl;
                    return channel_list[iter-signal_list.begin()];
                }
            }
            std::cout << "invalid signal name= "<<sig_<<std::endl;
            return 0;
        }
        char get_do_hotKey(std::string name_) //{return channelTable[sig_]; }
        {
            char c;
            for (auto iter=hotKey_name_list.begin();iter<hotKey_name_list.end();iter++)
            {
                if (name_.compare(*iter)==0)
                {
                    char c = hotKey_list.c_str()[iter-hotKey_name_list.begin()];
                    return c;
                }
            }
            std::cout << "invalid hotKey name= "<<name_<<std::endl;
            c = ' ';
            return c;
        }

        int init();
        void sendLaserOn(LaserType type);
        void sendLaserWarm(LaserType type);
        void stopLaser(){laser_stop_required = true;}
        void stopOpto(){opto_stop_required = true;}
        void sendOptoOn();
        void sendtrigger();
        void sendDetection(std::string name);

        bool isOptoOn() const {return OptoOn;}

        void set_opto_pulse_h(const int opto_pulse_h_){opto_pulse_h = opto_pulse_h_;}
        void set_opto_pulse_l(const int opto_pulse_l_){opto_pulse_l = opto_pulse_l_;}
        void set_opto_duration(const int opto_duration_){opto_duration = opto_duration_;}
        void set_opto_random_freq(const int opto_random_freq_){opto_random_freq = opto_random_freq_;}

        ~PlexDo(){}
    };

}

#endif
