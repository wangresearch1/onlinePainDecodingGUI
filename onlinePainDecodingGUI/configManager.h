#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include "INIReader.h"
#include <vector>
#include <string>

#include <iostream>
#include <sstream>
#include <numeric>

std::string sections(INIReader &reader);

namespace painBMI {
	typedef std::vector<bool> bool_vec;
	typedef std::vector<int> int_vec;
	typedef std::vector<float> f_vec;
	typedef std::vector<std::string> string_vec;
	enum Trodal
	{
		single = 1,
		stereotrode = 2,
		tetrode = 4
	};
	enum UPDATE_MODE
	{
		manual,
		auto_trial,// only update when stimulus detected
        random, // give random opto by xzd
		auto_always // always update
	};

	class AcquisitionConfig
	{
	private:
		int_vec enabled_channel_list;
		std::vector<int_vec> region_channel_list;
		int_vec emg_channel_list;
		int_vec unit_number_list;
		string_vec region_list;
		int_vec nUnits_region;
		Trodal trodal;
		float binSize_s;
		string_vec event_list;
		int_vec channel_list;
	public:
		//constructors:
		AcquisitionConfig()
		{
			enabled_channel_list.clear();
			region_channel_list.clear();
			emg_channel_list.clear();
			unit_number_list.clear();
			region_list.clear();
			nUnits_region.clear();
			trodal = single;
			binSize_s = 0.05;
		}

		//---methods---
		//set configurations from config files 
		void setEnChn(std::string configString);
		void setN_units(std::string configString);
		void setRegion(std::string configString);
		void setRegionChn(std::string configString);
		void clearRegionChn();
		void setEmgChn(std::string configString);
		void setEvent(std::string configString);
		void setEvnChn(std::string eventName, std::string configString);
        void setTrodal(Trodal trodal_) { trodal = trodal_; }
		void setTrodal(int trodal_)
		{
			switch (trodal_)
			{
			case 1: setTrodal(single); break;
			case 2: setTrodal(stereotrode); break;
			case 4: setTrodal(tetrode); break;
			default:setTrodal(single); break;
			}
		};
		void update_nUnits_region();

        void setBinSize(float binSize_) { binSize_s = binSize_; }

		//get config data
        int_vec getEnChn() const { return enabled_channel_list; }
		int getN_EnChn() const { return enabled_channel_list.size(); }
		int getN_units() const { return std::accumulate(unit_number_list.begin(), unit_number_list.end(), 0);; }
        std::vector<int_vec> getRegionChn() const { return region_channel_list; }
        int_vec getEmgChn() const { return emg_channel_list; }
        int_vec getNumUnit() const { return unit_number_list; }
        string_vec getRegions() const { return region_list; }
        Trodal getTrodal() const { return trodal; }
		float getBinSize() const { return binSize_s; }
		string_vec getEvent() const { return event_list; }
		int_vec getEvnChn() const { return channel_list; }
        // legacy, now we do not load enabled channels from config files
		int_vec getN_units_Region() const { return nUnits_region; }

		//isxx methods
		bool isChnEnabled(int chn) const
		{
			return !(std::find(enabled_channel_list.begin(), enabled_channel_list.end(), chn) == enabled_channel_list.end());
		}

		// read config file
		void readIniFile(std::string path);

		// read digital input event channel
		void readEvnChn(INIReader &reader);
	};

	class GeneralConfig // abstract base class
	{
	protected:
		string_vec param_list;
		std::string type;
		std::string name;
	public:
		std::string get_type() const { return type; }
		std::string get_name() const { return name; }
		string_vec get_param_list() const { return param_list; }
		virtual int setParam(std::string paramName, std::string configStr) = 0;
        virtual ~GeneralConfig() {}
	};

	class PLDSDecConfig : public GeneralConfig
	{
	private:
		int maxIter;
		int maxCpuTimeInSec;
		int trainSeqLengthInBin;
	public:
		//constructors
		PLDSDecConfig(int maxIter_ = 200, int maxCpuTimeInSec_ = 30, int trainSeqLengthInBin = 200) :
			maxIter(maxIter_), maxCpuTimeInSec(maxCpuTimeInSec_)
		{
			param_list.clear();
			param_list = { "type", "MaxIter", "MaxCpuTime", "TrainSeqLength" };
		}
		//methods
        int set_maxIter(int maxIter_) { maxIter = maxIter_; }
        int set_maxCpuTimeInSec(int maxCpuTimeInSec_) { maxCpuTimeInSec = maxCpuTimeInSec_; }
        int set_trainSeqLengthInBin(int trainSeqLengthInBin_) { trainSeqLengthInBin = trainSeqLengthInBin_; }

		//get functions
		int get_maxIter() const { return maxIter; }
		int get_maxCpuTimeInSec() const { return maxCpuTimeInSec; }
		int get_trainSeqLengthInBin() const { return trainSeqLengthInBin; }

		virtual int setParam(std::string paramName, std::string configStr);

		//destructors
        virtual ~PLDSDecConfig() {}

	};

	class TLDSDecConfig : public GeneralConfig
	{
	private:
		int EmMaxIter;
		int FaMaxIter;
		float EmTol;
		float FaTol;
		std::string  TLDS_type;
		int trainSeqLengthInBin;
	public:
		//constructors
		TLDSDecConfig(int EmMaxIter_ = 300, int FaMaxIter_ = 100, float EmTol_ = 1e-6, float FaTol_ = 0.001) :
			EmMaxIter(EmMaxIter_), FaMaxIter(FaMaxIter_), EmTol(EmTol_), FaTol(FaTol_)
		{
			param_list.clear();
			param_list = { "type", "EmMaxIter", "FaMaxIter", "EmTol", "FaTol", "TLDS_type", "TrainSeqLength" };
		}
		//methods
		//int set_maxIter(int maxIter_) { maxIter = maxIter_; };
		//int set_maxCpuTimeInSec(int maxCpuTimeInSec_) { maxCpuTimeInSec = maxCpuTimeInSec_; };
		//int set_trainSeqLengthInBin(int trainSeqLengthInBin_) { trainSeqLengthInBin = trainSeqLengthInBin_; };
		//int set_trainSeqLengthInSec(int trainSeqLengthInSec_) { trainSeqLengthInSec = trainSeqLengthInSec_; };

		//get functions
		int get_EmMaxIter() const { return EmMaxIter; }
		int get_FaMaxIter() const { return FaMaxIter; }
		float get_EmTol() const { return EmTol; }
		float get_FaTol() const { return FaTol; }
		std::string  get_TLDS_type() const { return TLDS_type; }

		virtual int setParam(std::string paramName, std::string configStr);

		//destructors
        virtual ~TLDSDecConfig() {}

	};

	class ModelFreeDecConfig : public GeneralConfig
	{
	private:
		//std::string name;
	public:
		ModelFreeDecConfig()
		{
			param_list.clear();
		}

		virtual int setParam(std::string paramName, std::string configStr)
		{
			if (paramName.compare("name") == 0)
			{
				name = configStr;
				type = "ModelFree";
				return 0;
			}
			std::cout << "invalid param name: " << paramName << std::endl;
			return -1;
		}
	};

	// store config for each region
	class SingleDetectorConfig : public GeneralConfig
	{
	private:
		bool enabled;
		float thresh;
		float nStd;//confidence interval = nStd * baseline_std
		float l_base;//in second
		UPDATE_MODE updateMode;
		string region;
		string decoder;
		float base_interval;//interval in second, between current time bin and the end of baseline(when updating)
	public:
		// constructor
		SingleDetectorConfig()
		{
			param_list.clear();
			param_list = { "type", "enabled", "region", "decoder", "thresh", "nStd", "update", "interval" };
		}
		// functions
		bool get_enabled() const { return enabled; }
		string get_name() const { return name; }
		string get_region() const { return region; }
		string get_decoder() const { return decoder; }
		float get_thresh() const { return thresh; }
		float get_nStd() const { return nStd; }
		float get_l_base() const { return nStd; }
		UPDATE_MODE get_updateMode() const { return updateMode; }
		float get_base_interval() const { return base_interval; }

		int setParam(std::string paramName, float param);
		virtual int setParam(std::string paramName, std::string configStr);
	};

	class GreedyDetectorConfig : public GeneralConfig
	{
	private:
		bool enabled;
		f_vec higher_thresh_list;
		f_vec lower_thresh_list;
		string_vec detector_list;
	public:
		GreedyDetectorConfig()
		{
			param_list.clear();
            param_list = { "type", "enabled", "detector", "higher_thresh", "lower_thresh" };
		}

		int set_detector_list(std::string configString);
		int set_higher_thresh_list(std::string configString);
		int set_lower_thresh_list(std::string configString);

		// get function
		string_vec get_detector_list() const { return detector_list; }
		f_vec get_higher_thresh_list() const { return higher_thresh_list; }
		f_vec get_lower_thresh_list() const { return lower_thresh_list; }

		virtual int setParam(std::string paramName, std::string configStr);
	};

    class CCFDetectorConfig : public GeneralConfig
    {
    private:
        bool enabled;
        string_vec detector_list;
        float alpha;
        float beta;
        float ffactor;  // forgeting factor
        float nStd; // confidence interval is nStd*baseline_std
        float aThresh;// area threshold
    public:
        CCFDetectorConfig()
        {
            param_list.clear();
            param_list = { "type", "enabled", "detector", "alpha", "beta", "ffactor", "nStd", "aThresh" };
        }

        int set_detector_list(std::string configString);

        // get function
        string_vec get_detector_list() const { return detector_list; }
        float get_alpha() const { return alpha; }
        float get_beta() const { return beta; }
        float get_ffactor() const { return ffactor; }
        float get_nStd() const { return nStd; }
        float get_aThresh() const { return aThresh; }

        int setParam(std::string paramName, float param);
        virtual int setParam(std::string paramName, std::string configStr);
    };

    class EMGDetectorConfig : public GeneralConfig
    {
    private:
        bool enabled;
        float Thresh;// area threshold
    public:
        EMGDetectorConfig()
        {
            param_list.clear();
            param_list = { "type", "enabled", "thresh" };
        }
        float get_thresh() const {return Thresh;}
        int setParam(std::string paramName, float param);
        virtual int setParam(std::string paramName, std::string configStr);
    };

	class AlgorithmConfig
	{
	private:
		string_vec decoder_list;
		string_vec detector_list;
		std::vector<GeneralConfig*> decoder_config_list;
		std::vector<GeneralConfig*> detector_config_list;
	public:
		// set functions
		void set_decoder_list(std::string configString);
		void set_detector_list(std::string configString);
		void read_decoder_config_list(INIReader &reader, std::string decoderName);
		void read_detector_config_list(INIReader &reader, std::string detectorName);

		// get functions
		string_vec get_decoder_list() const { return decoder_list; }
		string_vec get_detector_list() const { return detector_list; }
        const std::vector<GeneralConfig*>& get_decoder_config_list()  { return decoder_config_list; }
        const std::vector<GeneralConfig*>& get_detector_config_list() const { return detector_config_list; }

		void readIniFile(std::string path);

		//destructor
		~AlgorithmConfig()
		{
			// release decoder config classes
			for (std::vector<GeneralConfig*>::iterator iter = decoder_config_list.begin(); iter < decoder_config_list.end(); iter++)
			{
				if ((*iter) != nullptr)
				{
					delete (*iter);
				}
			}
			// release detector config classes
			for (std::vector<GeneralConfig*>::iterator iter = detector_config_list.begin(); iter < detector_config_list.end(); iter++)
			{
				if ((*iter) != nullptr)
				{
					delete (*iter);
				}
			}
		}
	};

	// Plexon do config
	class PlexDoConfig : public GeneralConfig
	{
	private:
		string_vec do_list;
		int_vec doChn_list;

		string_vec hotKey_name_list;
		std::string hotKey_list;
		int stim_time;  // in ms
		int warm_time;  // in ms
		std::string device;
		int opto_pulse_h;
		int opto_pulse_l;
		int opto_duration;
        int opto_random_freq;

	public:
		//constructor
		PlexDoConfig()
		{
			device = "Plexon";
			stim_time = 5000;
			warm_time = 45000;
			param_list.clear();
			hotKey_name_list = { "h_laser_on", "l_laser_on", "h_laser_warm", "l_laser_warm", "laser_stop", "opto_on" };
			hotKey_list = "QWASZO";
            param_list = { "warm_time","stim_time","opto_pulse_h", "opto_pulse_l", "opto_duration" };
		}
		virtual int setParam(std::string paramName, std::string configStr);
		void readIniFile(std::string path);
		void set_output_list(std::string configStr);
		void read_doChn(INIReader &reader);
		void set_doChn(std::string doName, std::string configStr);

        string_vec get_do() const { return do_list; }
        int_vec get_doChn() const { return doChn_list; }

        string_vec get_hotKey_name_list() const {return hotKey_name_list;}
        std::string get_hotKey_list() const {return hotKey_list;}
        int get_stim_time()   const {return stim_time;}
        int get_warm_time()  const {return warm_time;}
        std::string get_device() const {return device;}
        int get_opto_pulse_h() const {return opto_pulse_h;}
        int get_opto_pulse_l() const {return opto_pulse_l;}
        int get_opto_duration() const {return opto_duration;}
        int get_opto_random_freq() const {return opto_random_freq;}
	};

	class ConfigManager
	{
	public:
		AcquisitionConfig* acqConfig;
		AlgorithmConfig* algConfig;
		PlexDoConfig* doConfig;
	public:
		ConfigManager()
		{
			acqConfig = new AcquisitionConfig;
			algConfig = new AlgorithmConfig;
			doConfig = new PlexDoConfig;
		}
		void updateConfig(std::string filename)
		{
			acqConfig->readIniFile(filename);
			algConfig->readIniFile(filename);
			doConfig->readIniFile(filename);
		}

		~ConfigManager()
		{
			if (acqConfig != nullptr)
			{
				delete acqConfig;
			}
			if (algConfig != nullptr)
			{
				delete algConfig;
			}
			if (doConfig != nullptr)
			{
				delete doConfig;
			}
		}
	};
}
#endif
