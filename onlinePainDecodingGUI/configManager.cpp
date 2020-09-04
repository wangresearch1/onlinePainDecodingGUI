/**************************************************************************
---------------------------------------------------------------------------
The config manager for real-time pain detection BMI system.
Here contains classes and methods for get configurations for the BMI system
by reading custom designed .ini config file, and interfaces for accessing the
configuration information

author: Sile Hu, yuehusile@gmail.com
version: v1.0
date: 2017-12-12
___________________________________________________________________________
***************************************************************************/

#include "configManager.h"
#include <algorithm>

// read ini file sections
std::string sections(INIReader &reader)
{
	std::stringstream ss;
	std::set<std::string> sections = reader.Sections();
	for (std::set<std::string>::iterator it = sections.begin(); it != sections.end(); ++it)
		ss << *it << ",";
	return ss.str();
}

namespace painBMI{
/************************************************************************
*               Acquisition params config                               *
*************************************************************************/
	void AcquisitionConfig::setEnChn(std::string configString)
	{
		//separate by comma
		std::stringstream ss(configString);
		int start,end,chn;
		enabled_channel_list.clear();
		while (ss >> chn)
		{
			if (std::find(enabled_channel_list.begin(), enabled_channel_list.end(), chn) == enabled_channel_list.end())
				enabled_channel_list.push_back(chn);

			if (ss.peek() == '-')
			{
				ss.ignore();
				start = chn+1;
				ss >> end;
				for (int i = start; i <= end; i++)
				{
					if (std::find(enabled_channel_list.begin(), enabled_channel_list.end(), i) == enabled_channel_list.end())
						enabled_channel_list.push_back(i);
				}
			}

			while (ss.peek() == ',' || ss.peek() == ' ')
				ss.ignore();
		}
		std::sort(enabled_channel_list.begin(), enabled_channel_list.end());
	}

	void AcquisitionConfig::setN_units(std::string configString)
	{
		//separate by comma
		std::stringstream ss(configString);
		int start, end, chn;
		unit_number_list.clear();
		while (ss >> chn)
		{
			unit_number_list.push_back(chn);

			while (ss.peek() == ',' || ss.peek() == ' ')
				ss.ignore();
		}
	}

	void AcquisitionConfig::setRegion(std::string configString)
	{
		//separate by comma
		std::stringstream ss(configString);
		region_list.clear();
		char ch;
		std::string str;
		str.clear();
		while (ss >> ch)
		{
			str.push_back(ch);
			while (ss.peek() == ' ')
				ss.ignore();
			if (ss.peek() == ',')
			{
				region_list.push_back(str);
				str.clear();
				ss.ignore();
			}
			while (ss.peek() == ' ')
				ss.ignore();
		}
		if (str.length()>0)
			region_list.push_back(str);
	}
	void AcquisitionConfig::setRegionChn(std::string configString)
	{
		//separate by comma
		std::stringstream ss(configString);
		int start, end, chn;
		int_vec tmp_vec;

		while (ss >> chn)
		{
			if (std::find(tmp_vec.begin(), tmp_vec.end(), chn) == tmp_vec.end())
				tmp_vec.push_back(chn);

			if (ss.peek() == '-')
			{
				ss.ignore();
				start = chn + 1;
				ss >> end;
				for (int i = start; i <= end; i++)
				{
					if (std::find(tmp_vec.begin(), tmp_vec.end(), i) == tmp_vec.end())
						tmp_vec.push_back(i);
				}
			}

			while (ss.peek() == ',' || ss.peek() == ' ')
				ss.ignore();
		}
		bool overlap = false;
		for (std::vector<int_vec>::iterator iter = region_channel_list.begin(); iter < region_channel_list.end(); iter++)
		{
			for (int_vec::iterator iter_chn = (*iter).begin(); iter_chn < (*iter).end(); iter_chn++)
			{
				if (std::find(tmp_vec.begin(), tmp_vec.end(), *iter_chn) != tmp_vec.end())
				{
					overlap = true;
					break;
				}
			}
		}
		std::sort(tmp_vec.begin(), tmp_vec.end());
		region_channel_list.push_back(tmp_vec);
		if (overlap)
			std::cout << "There's overlapped channel in different regions, please check the config file and reload" << std::endl;
	}

	void AcquisitionConfig::clearRegionChn()
	{
		region_channel_list.clear();
	}

    // legacy, now we do not load enabled channels from config files
	void AcquisitionConfig::update_nUnits_region()
	{
		for (int i = 0; i < region_list.size(); i++)
		{
			int nUnits_ = 0;
            //std::cout <<"size="<<enabled_channel_list.size()<<std::endl;
			for (int_vec::iterator iter = region_channel_list[i].begin(); iter < region_channel_list[i].end();iter++)
			{
				// find related idx in enabled_channel_list
                //std::cout <<(*iter)<<std::endl;
				for (int j = 0; j < enabled_channel_list.size();j++)
				{
					if (enabled_channel_list[j] == (*iter))
					{
						nUnits_ += unit_number_list[j];
						break;
					}
				}
			}
			nUnits_region.push_back(nUnits_);
		}
	}

	void AcquisitionConfig::setEmgChn(std::string configString)
	{
		//separate by comma
		std::stringstream ss(configString);
		int start, end, chn;
		emg_channel_list.clear();
		while (ss >> chn)
		{
			if (std::find(emg_channel_list.begin(), emg_channel_list.end(), chn) == emg_channel_list.end())
				emg_channel_list.push_back(chn);

			if (ss.peek() == '-')
			{
				ss.ignore();
				start = chn + 1;
				ss >> end;
				for (int i = start; i <= end; i++)
				{
					if (std::find(emg_channel_list.begin(), emg_channel_list.end(), i) == emg_channel_list.end())
						emg_channel_list.push_back(i);
				}
			}

			while (ss.peek() == ',' || ss.peek() == ' ')
				ss.ignore();
		}
		std::sort(emg_channel_list.begin(), emg_channel_list.end());
	}

	void AcquisitionConfig::setEvent(std::string configString)
	{
		//separate by comma
		std::stringstream ss(configString);
		event_list.clear();
		char ch;
		std::string str;
		str.clear();
		while (ss >> ch)
		{
			str.push_back(ch);
			while (ss.peek() == ' ')
				ss.ignore();
			if (ss.peek() == ',')
			{
				event_list.push_back(str);
				str.clear();
				ss.ignore();
			}
			while (ss.peek() == ' ')
				ss.ignore();
		}
		if (str.length()>0)
			event_list.push_back(str);
	}

	void AcquisitionConfig::setEvnChn(std::string eventName,std::string configString)
	{
		if (eventName.compare("detect") == 0)
		{
			//separate by comma
			std::stringstream ss(configString);
			int start, end, chn;
			int cnt = 0;
			//emg_channel_list.clear();
			while (ss >> chn)
			{
				if (std::find(channel_list.begin(), channel_list.end(), chn) == channel_list.end())
				{
					channel_list.push_back(chn);
					cnt++;
				}
					
				if (ss.peek() == '-')
				{
					ss.ignore();
					start = chn + 1;
					ss >> end;
					for (int i = start; i <= end; i++)
					{
						if (std::find(channel_list.begin(), channel_list.end(), i) == channel_list.end())
						{
							channel_list.push_back(i);
							cnt++;
						}
					}
				}

				while (ss.peek() == ',' || ss.peek() == ' ')
					ss.ignore();
			}
			if (cnt > 0)
			{
				for (string_vec::iterator iter = event_list.begin(); iter < event_list.end(); iter++)
				{
					if (eventName.compare(*iter) == 0)
					{
						int idx = iter - event_list.begin();
						(*iter) = "detect1";
						for (int i = 1; i < cnt; i++)
						{
							event_list.insert(event_list.begin()+idx+i,"detect"+ std::to_string(i+1));
						}
						break;
					}
				}
			}
		}
		else
		{
			for (string_vec::iterator iter = event_list.begin(); iter < event_list.end(); iter++)
			{
				if (eventName.compare(*iter) == 0)
				{
					channel_list.push_back(std::stoi(configString.c_str()));
				}
			}
		}
	}

	void AcquisitionConfig::readIniFile(std::string path)
	{
		INIReader reader(path);

		if (reader.ParseError() < 0) 
		{
			std::cout << "Can't load '"<< path <<"'\n";
			return;
		}

        // regions
        std::string regionStr = reader.Get("acquisition", "Regions", "UNKNOWN");
		setRegion(regionStr);
		std::cout << "Config loaded from 'test.ini': found sections=" << sections(reader)
			<< ", Regions=" << regionStr << std::endl;
		// channels for each region
		std::string tmpStr, regionName;
		clearRegionChn();
		for (int i = 0; i < getRegions().size(); i++)
		{
			regionName = getRegions().at(i);
			tmpStr = reader.Get("acquisition", regionName, "UNKNOWN");
			setRegionChn(tmpStr);
			std::cout << regionName << " Channels=" << tmpStr << std::endl;
		}

		update_nUnits_region();

		//emg channel
		std::string emgStr = reader.Get("acquisition", "EMG", "UNKNOWN");
		setEmgChn(emgStr);
		//trodal
		int tmp = reader.GetInteger("acquisition", "Trodal", -1);
		setTrodal(tmp);
		//digital input events
		std::string eventStr = reader.Get("acquisition", "event", "UNKNOWN");
		setEvent(eventStr);
		//digital input channel
		readEvnChn(reader);
		return;
	}

	void AcquisitionConfig::readEvnChn(INIReader & reader)
	{
		channel_list.clear();
		std::string eventName,tmpStr;
		int event_sz = getEvent().size();
		for (int i = 0; i < event_sz; i++)
		{
			eventName = getEvent().at(i);
			tmpStr = reader.Get("acquisition", eventName, "UNKNOWN");
			setEvnChn(eventName,tmpStr);
			std::cout << eventName << " Channels=" << tmpStr << std::endl;
		}
	}

    /************************************************************************
    *               algorithm params config                                 *
    *************************************************************************/
	int PLDSDecConfig::setParam(std::string paramName, std::string configStr)
	{
		if (paramName.compare("name") == 0)
		{
			name = configStr;
			return 0;
		}
		if (paramName.compare("type") == 0)
		{
			type = configStr;
			return 0;
		}
		if (paramName.compare("MaxIter") == 0)
		{
			maxIter = std::stoi(configStr);
			return 0;
		}
		if (paramName.compare("MaxCpuTime") == 0)
		{
			maxCpuTimeInSec = std::stoi(configStr);
			return 0;
		}
		if (paramName.compare("TrainSeqLength") == 0)
		{
			trainSeqLengthInBin = std::stoi(configStr);
			return 0;
		}

		std::cout << "invalid param name: " << paramName << std::endl;
		return -1;
	}

	int TLDSDecConfig::setParam(std::string paramName, std::string configStr)
	{
		if (paramName.compare("name") == 0)
		{
			name = configStr;
			return 0;
		}
		if (paramName.compare("type") == 0)
		{
			type = configStr;
			return 0;
		}
		if (paramName.compare("EmMaxIter") == 0)
		{
			EmMaxIter = std::stoi(configStr);
			return 0;
		}
		if (paramName.compare("FaMaxIter") == 0)
		{
			FaMaxIter = std::stoi(configStr);
			return 0;
		}
		if (paramName.compare("TrainSeqLength") == 0)
		{
			trainSeqLengthInBin = std::stoi(configStr);
			return 0;
		}
		if (paramName.compare("EmTol") == 0)
		{
			EmTol = std::stof(configStr);
			return 0;
		}
		if (paramName.compare("FaTol") == 0)
		{
			FaTol = std::stof(configStr);
			return 0;
		}
		if (paramName.compare("TLDS_type") == 0)
		{
			TLDS_type = configStr;
			return 0;
		}

		std::cout << "invalid param name: " << paramName << std::endl;
		return -1;
	}

	void AlgorithmConfig::set_decoder_list(std::string configString)
	{
		//separate by comma
		std::stringstream ss(configString);
		decoder_list.clear();
		char ch;
		std::string str;
		str.clear();
		while (ss >> ch)
		{
			str.push_back(ch);
			while (ss.peek() == ' ')
				ss.ignore();
			if (ss.peek() == ',')
			{
				decoder_list.push_back(str);
				str.clear();
				ss.ignore();
			}
			while (ss.peek() == ' ')
				ss.ignore();
		}
		if (str.length()>0)
			decoder_list.push_back(str);
	}
	void AlgorithmConfig::set_detector_list(std::string configString)
	{
		//separate by comma
		std::stringstream ss(configString);
		detector_list.clear();
		char ch;
		std::string str;
		str.clear();
		while (ss >> ch)
		{
			str.push_back(ch);
			while (ss.peek() == ' ')
				ss.ignore();
			if (ss.peek() == ',')
			{
				detector_list.push_back(str);
				str.clear();
				ss.ignore();
			}
			while (ss.peek() == ' ')
				ss.ignore();
		}
		if (str.length()>0)
			detector_list.push_back(str);
	}
	void AlgorithmConfig::read_decoder_config_list(INIReader &reader, std::string decoderName)
	{
		GeneralConfig * configPtr = nullptr;

		string configStr;
		configStr = reader.Get(decoderName, "type", "UNKNOWN");

		if (configStr.compare("PLDS") == 0)
		{
			configPtr = new PLDSDecConfig;
		}
		if (configStr.compare("TLDS") == 0)
		{
			configPtr = new TLDSDecConfig;
		}
		if (configStr.compare("ModelFree") == 0)
		{
			configPtr = new ModelFreeDecConfig;
		}
		if (configPtr == nullptr)
		{
			std::cout << "invalid decoder type: decoder=" << decoderName << " type=" << configStr << std::endl;
		}
		else
		{
			string_vec param_list = configPtr->get_param_list();
			//string configStr;
			configPtr->setParam("name", decoderName);
			for (string_vec::iterator iter = param_list.begin(); iter < param_list.end(); iter++)
			{
				configStr = reader.Get(decoderName, *iter, "UNKNOWN");
				configPtr->setParam(*iter, configStr);
			}
			decoder_config_list.push_back(configPtr);
		}
	}
	void AlgorithmConfig::read_detector_config_list(INIReader &reader, std::string detectorName)
	{
		GeneralConfig * configPtr = nullptr;
		string configStr,typeStr;

		typeStr = reader.Get(detectorName, "type", "UNKNOWN");
		if (typeStr.compare("single")==0)
		{
			configPtr = new SingleDetectorConfig;
		}
		if (typeStr.compare("greedy")==0)
		{
			configPtr = new GreedyDetectorConfig;
		}
        if (typeStr.compare("ccf")==0)
        {
            configPtr = new CCFDetectorConfig;
        }
        if (typeStr.compare("emg")==0)
        {
            configPtr = new EMGDetectorConfig;
        }
		
		if (configPtr == nullptr)
		{
			std::cout << "invalid detector name: " << detectorName <<" or type£º "<< typeStr << std::endl;
		}
		else
		{
			string_vec param_list = configPtr->get_param_list();
			string configStr;
			configPtr->setParam("name", detectorName);
			for (string_vec::iterator iter = param_list.begin(); iter < param_list.end(); iter++)
			{
				configStr = reader.Get(detectorName, *iter, "UNKNOWN");
				configPtr->setParam(*iter, configStr);
			}
			detector_config_list.push_back(configPtr);
//            if (typeStr.compare("greedy")==0)
//            {
//                int size = static_cast<GreedyDetectorConfig*>(configPtr)->get_detector_list().size();
//                std::cout << size<<std::endl;
//            }
		}

	}
	void AlgorithmConfig::readIniFile(std::string path)
	{
		INIReader reader(path);

		if (reader.ParseError() < 0)
		{
			std::cout << "Can't load '" << path << "'\n";
			return;
		}

		// decoder list
		string decoderListStr = reader.Get("decoder", "Decoders", "UNKNOWN");
		set_decoder_list(decoderListStr);
		// detector list
		string detectorListStr = reader.Get("detector", "Detectors", "UNKNOWN");
		set_detector_list(detectorListStr);
		// read decoder config
		for (std::vector<GeneralConfig*>::iterator iter = decoder_config_list.begin(); iter < decoder_config_list.end(); iter++)
		{
			delete (*iter);//clear previous configurations
		}
		decoder_config_list.clear();
		for (string_vec::iterator iter = decoder_list.begin(); iter < decoder_list.end(); iter++)
		{
			read_decoder_config_list(reader, *iter);
		}
		// read detector config
		for (std::vector<GeneralConfig*>::iterator iter = detector_config_list.begin(); iter < detector_config_list.end(); iter++)
		{
			delete (*iter);//clear previous configurations
		}
		detector_config_list.clear();
		for (string_vec::iterator iter = detector_list.begin(); iter < detector_list.end(); iter++)
		{
			read_detector_config_list(reader, *iter);
		}

		//--------- decoder config ------------
		//GeneralConfig *decoderConfig;
		return;
	}

	int SingleDetectorConfig::setParam(std::string paramName, float param)
	{
		if (paramName.compare("thresh") == 0)
		{
			thresh = param;
			return 0;
		}
		if (paramName.compare("nStd") == 0)
		{
			nStd = param;
			return 0;
		}
		if (paramName.compare("interval") == 0)
		{
			base_interval = param;
			return 0;
		}
		std::cout << "invalid param name: " << paramName << std::endl;
		return -1;
	}

	int SingleDetectorConfig::setParam(std::string paramName, std::string configStr)
	{
		if (paramName.compare("name") == 0)
		{
			name = configStr;
			return 0;
		}
		if (paramName.compare("type") == 0)
		{
			type = configStr;
			return 0;
		}
		if (paramName.compare("enabled") == 0)
		{
			if (configStr.compare("true") == 0)
			{
				enabled = true;
				return 0;
			}
			if (configStr.compare("false") == 0)
			{
				enabled = false;
				return 0;
			}
			std::cout << "invalid enabled config: " << configStr << std::endl;
			return -1;
		}
		if (paramName.compare("region") == 0)
		{
			region = configStr;
			return 0;
		}
		if (paramName.compare("decoder") == 0)
		{
			decoder = configStr;
			return 0;
		}
		if (paramName.compare("thresh") == 0)
		{
			return setParam(paramName, std::atof(configStr.c_str()));
		}
		if (paramName.compare("nStd") == 0)
		{
			return setParam(paramName, std::atof(configStr.c_str()));
		}
		if (paramName.compare("interval") == 0)
		{
			return setParam(paramName, std::atof(configStr.c_str()));
		}
		if (paramName.compare("update") == 0)
		{
			if (configStr.compare("manual") == 0)
			{
				updateMode = manual;
				return 0;
			}
			if (configStr.compare("auto_trial") == 0)
			{
				updateMode = auto_trial;
				return 0;
			}
			if (configStr.compare("auto_always") == 0)
			{
				updateMode = auto_always;
				return 0;
			}
			std::cout << "invalid updateMode config: " << configStr << std::endl;
			return -1;
		}
		std::cout << "invalid param name: " << paramName << std::endl;
		return -1;
	}

	int GreedyDetectorConfig::set_detector_list(std::string configString)
	{
		//separate by comma
		std::stringstream ss(configString);
		detector_list.clear();
		char ch;
		std::string str;
		str.clear();
		while (ss >> ch)
		{
			str.push_back(ch);
			while (ss.peek() == ' ')
				ss.ignore();
			if (ss.peek() == ',')
			{
				detector_list.push_back(str);
				str.clear();
				ss.ignore();
			}
			while (ss.peek() == ' ')
				ss.ignore();
		}
		if (str.length()>0)
			detector_list.push_back(str);
        auto p = &detector_list;
		return 0;
	}

	int GreedyDetectorConfig::set_higher_thresh_list(std::string configString)
	{
		//separate by comma
		std::stringstream ss(configString);
        higher_thresh_list.clear();
		char ch;
		std::string str;
		str.clear();
		while (ss >> ch)
		{
			str.push_back(ch);
			while (ss.peek() == ' ')
				ss.ignore();
			if (ss.peek() == ',')
			{
				higher_thresh_list.push_back(std::atof(str.c_str()));
				str.clear();
				ss.ignore();
			}
			while (ss.peek() == ' ')
				ss.ignore();
		}
		if (str.length()>0)
			higher_thresh_list.push_back(std::atof(str.c_str()));
		return 0;
	}

	int GreedyDetectorConfig::set_lower_thresh_list(std::string configString)
	{
		//separate by comma
		std::stringstream ss(configString);
        lower_thresh_list.clear();
		char ch;
		std::string str;
		str.clear();
		while (ss >> ch)
		{
			str.push_back(ch);
			while (ss.peek() == ' ')
				ss.ignore();
			if (ss.peek() == ',')
			{
				lower_thresh_list.push_back(std::atof(str.c_str()));
				str.clear();
				ss.ignore();
			}
			while (ss.peek() == ' ')
				ss.ignore();
		}
		if (str.length()>0)
			lower_thresh_list.push_back(std::atof(str.c_str()));
		return 0;
	}

	int GreedyDetectorConfig::setParam(std::string paramName, std::string configStr)
	{
		if (paramName.compare("name") == 0)
		{
			name = configStr;
			return 0;
		}
		if (paramName.compare("type") == 0)
		{
			type = configStr;
			return 0;
		}
		if (paramName.compare("enabled") == 0)
		{
			if (configStr.compare("true") == 0)
			{
				enabled = true;
				return 0;
			}
			if (configStr.compare("false") == 0)
			{
				enabled = false;
				return 0;
			}
			std::cout << "invalid enabled config: " << configStr << std::endl;
			return -1;
		}
        if (paramName.compare("detector") == 0)
		{
			return set_detector_list(configStr);
		}
		if (paramName.compare("higher_thresh") == 0)
		{
			return set_higher_thresh_list(configStr);
		}
		if (paramName.compare("lower_thresh") == 0)
		{
			return set_lower_thresh_list(configStr);
		}
		
		std::cout << "invalid param name: " << paramName << std::endl;
		return -1;
	}

    int CCFDetectorConfig::set_detector_list(std::string configString)
    {
        //separate by comma
        std::stringstream ss(configString);
        detector_list.clear();
        char ch;
        std::string str;
        str.clear();
        while (ss >> ch)
        {
            str.push_back(ch);
            while (ss.peek() == ' ')
                ss.ignore();
            if (ss.peek() == ',')
            {
                detector_list.push_back(str);
                str.clear();
                ss.ignore();
            }
            while (ss.peek() == ' ')
                ss.ignore();
        }
        if (str.length()>0)
            detector_list.push_back(str);
        auto p = &detector_list;
        return 0;
    }

    int CCFDetectorConfig::setParam(std::string paramName, float param)
    {
        if (paramName.compare("alpha") == 0)
        {
            alpha = param;
            return 0;
        }
        if (paramName.compare("beta") == 0)
        {
            beta = param;
            return 0;
        }
        if (paramName.compare("ffactor") == 0)
        {
            ffactor = param;
            return 0;
        }
        if (paramName.compare("nStd") == 0)
        {
            nStd = param;
            return 0;
        }
        if (paramName.compare("aThresh") == 0)
        {
            aThresh = param;
            return 0;
        }
        std::cout << "invalid param name: " << paramName << std::endl;
        return -1;
    }

    int CCFDetectorConfig::setParam(std::string paramName, std::string configStr)
    {
        if (paramName.compare("name") == 0)
        {
            name = configStr;
            return 0;
        }
        if (paramName.compare("type") == 0)
        {
            type = configStr;
            return 0;
        }
        if (paramName.compare("enabled") == 0)
        {
            if (configStr.compare("true") == 0)
            {
                enabled = true;
                return 0;
            }
            if (configStr.compare("false") == 0)
            {
                enabled = false;
                return 0;
            }
            std::cout << "invalid enabled config: " << configStr << std::endl;
            return -1;
        }

        if (paramName.compare("detector") == 0)
        {
            return set_detector_list(configStr);
        }

        return setParam(paramName,std::atof(configStr.c_str()));
    }

    int EMGDetectorConfig::setParam(std::string paramName, float param)
    {
        if (paramName.compare("thresh") == 0)
        {
            Thresh = param;
            return 0;
        }
        std::cout << "invalid param name: " << paramName << std::endl;
        return -1;
    }

    int EMGDetectorConfig::setParam(std::string paramName, std::string configStr)
    {
        if (paramName.compare("name") == 0)
        {
            name = configStr;
            return 0;
        }
        if (paramName.compare("type") == 0)
        {
            type = configStr;
            return 0;
        }
        if (paramName.compare("enabled") == 0)
        {
            if (configStr.compare("true") == 0)
            {
                enabled = true;
                return 0;
            }
            if (configStr.compare("false") == 0)
            {
                enabled = false;
                return 0;
            }
            std::cout << "invalid enabled config: " << configStr << std::endl;
            return -1;
        }

        return setParam(paramName,std::atof(configStr.c_str()));
    }
    /************************************************************************
    *               Plexon digital output params config                     *
    *************************************************************************/
    int PlexDoConfig::setParam(std::string paramName, std::string param)
	{
		if (paramName.compare("opto_pulse_h") == 0)
		{
			opto_pulse_h = std::stoi(param.c_str());
			return 0;
		}
		if (paramName.compare("opto_pulse_l") == 0)
		{
			opto_pulse_l = std::stoi(param.c_str());
			return 0;
		}
		if (paramName.compare("opto_duration") == 0)
		{
			opto_duration = std::stoi(param.c_str());
			return 0;
		}
        if (paramName.compare("opto_random_freq") == 0)
        {
            opto_random_freq = std::stoi(param.c_str());
            return 0;
        }
		std::cout << "invalid param name: " << paramName << std::endl;
		return -1;
	}
	void PlexDoConfig::readIniFile(std::string path)
	{
		INIReader reader(path);

		if (reader.ParseError() < 0)
		{
			std::cout << "Can't load '" << path << "'\n";
			return;
		}

		// decoder list
		std::string outputListStr = reader.Get("DO", "output", "UNKNOWN");
		set_output_list(outputListStr);
		//read output channels
		read_doChn(reader);
		//read params
		std::string tmp;
		for (string_vec::iterator iter = param_list.begin(); iter < param_list.end(); iter++)
		{
			tmp = reader.Get("DO", (*iter), "UNKNOWN");
			setParam((*iter), tmp);
		}
	}
	void PlexDoConfig::set_output_list(std::string configStr)
	{
		//separate by comma
		std::stringstream ss(configStr);
		do_list.clear();
		char ch;
		std::string str;
		str.clear();
		while (ss >> ch)
		{
			str.push_back(ch);
			while (ss.peek() == ' ')
				ss.ignore();
			if (ss.peek() == ',')
			{
				do_list.push_back(str);
				str.clear();
				ss.ignore();
			}
			while (ss.peek() == ' ')
				ss.ignore();
		}
		if (str.length()>0)
			do_list.push_back(str);
	}
	void PlexDoConfig::read_doChn(INIReader & reader)
	{
		doChn_list.clear();
		std::string doName, tmpStr;
		int do_sz = get_do().size();
		for (int i = 0; i < do_sz; i++)
		{
			doName = get_do().at(i);
			tmpStr = reader.Get("DO", doName, "UNKNOWN");
			set_doChn(doName, tmpStr);
			std::cout << doName << " Channels=" << tmpStr << std::endl;
		}
	}
	void PlexDoConfig::set_doChn(std::string doName, std::string configString)
	{
		if (doName.compare("detect") == 0)
		{
			//separate by comma
			std::stringstream ss(configString);
			int start, end, chn;
			int cnt = 0;
			//emg_channel_list.clear();
			while (ss >> chn)
			{
				if (std::find(doChn_list.begin(), doChn_list.end(), chn) == doChn_list.end())
				{
					doChn_list.push_back(chn);
					cnt++;
				}

				if (ss.peek() == '-')
				{
					ss.ignore();
					start = chn + 1;
					ss >> end;
					for (int i = start; i <= end; i++)
					{
						if (std::find(doChn_list.begin(), doChn_list.end(), i) == doChn_list.end())
						{
							doChn_list.push_back(i);
							cnt++;
						}
					}
				}

				while (ss.peek() == ',' || ss.peek() == ' ')
					ss.ignore();
			}
			if (cnt > 0)
			{
				for (string_vec::iterator iter = do_list.begin(); iter < do_list.end(); iter++)
				{
					if (doName.compare(*iter) == 0)
					{
						int idx = iter - do_list.begin();
						(*iter) = "detect1";
						for (int i = 1; i < cnt; i++)
						{
							do_list.insert(do_list.begin() + idx + i, "detect" + std::to_string(i + 1));
						}
						break;
					}
				}
			}
		}
        else if (configString.compare("UNKNOWN")!=0)
		{
			for (string_vec::iterator iter = do_list.begin(); iter < do_list.end(); iter++)
			{
				if (doName.compare(*iter) == 0)
				{
					doChn_list.push_back(std::stoi(configString.c_str()));
				}
			}
		}
	}
}
