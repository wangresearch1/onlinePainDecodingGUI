#ifndef __DataCollector_H__
#define __DataCollector_H__

#include <Eigen/Dense>
#include <Eigen/Core>
#include <windows.h>
#include <process.h>
#include <thread>
#include "Plexon.h"
#include <ctime>
#include <chrono>
#include "bufferManager.h"
#include "configManager.h"

namespace painBMI
{
	class PlexDataCollector
	{
	private:
		float binSize_s;
		Trodal trodal;
		string_vec region_list;
		string_vec event_list;
		int channelsUsed;
		int nUnits;
		circular_buffer2d<float> *spike_buf;
        circular_buffer2d<float> *emg_buf;

		unsigned long binSize; // in micro seconds, this is the buffer binsize
		bool stop;
		PL_Event* pServerDataBuffer = nullptr;  // buffer in which the server will return acquisition data
        //PL_Wave* pServerWaveBuffer = nullptr;  // buffer in which the server will return acquisition data
        PL_WaveLong* pServerWaveBuffer = nullptr;
        int max_data_blocks_per_read;
		int_vec nUnits_chn;
		int_vec enChn;// 1- based channel number
		int_vec emgChn;
		int_vec evnChn;
		std::vector<int_vec> chn_list_region;
		int_vec nUnits_region;
        float* spikeTmpBuf;
        bool dataReady;

		// for acquired events
		string_vec eventQueue;
		// to be done: EMG members

		//tmp for test: std thread component
		std::thread acquisitionThread;
		// accumulative units number, for mapping (chn,unit) to buf idx.
		int_vec nUnits_acc;
        std::vector<int_vec> buf_idx_region;

	public:
		//construtors
		//note: do not construct thread object in constructors, startAquisition/stopAquisition will start/end aquisition thread.
		
		PlexDataCollector() :
			binSize_s(float(50) / 1000.0),
			trodal(single),
			region_list(),
			event_list(),
			channelsUsed(64),
			nUnits(0),// will be updated after initialization
			enChn(),
			spike_buf(nullptr),//actual construction of buffer will run after initialization of acquisition 

			binSize(50 * 1000),
			stop(true),
			pServerDataBuffer(nullptr),
			max_data_blocks_per_read(500000)
		{}
		

		PlexDataCollector(AcquisitionConfig acqConf) :
			binSize_s(acqConf.getBinSize()),
			trodal(acqConf.getTrodal()),
			region_list(acqConf.getRegions()),
			event_list(acqConf.getEvent()),

			//channelsUsed(acqConf.getN_EnChn()),
			channelsUsed(),// will be updated after initialization
			nUnits(0),// will be updated after initialization
			spike_buf(nullptr),//actual construction of buffer will run after initialization of acquisition 
			//nUnits(acqConf.getN_units()),// will be updated after initialization
			//spike_buf(1000, nUnits),//actual construction of buffer will run after initialization of acquisition 

			binSize(binSize_s * 1000 * 1000),
			stop(true),
			pServerDataBuffer(nullptr),
			max_data_blocks_per_read(500000),
			nUnits_chn(),// will be updated after initialization
			//nUnits_chn(acqConf.getNumUnit()),
			//enChn(acqConf.getEnChn()),
			enChn(),// will be updated after initialization
			emgChn(acqConf.getEmgChn()),
			evnChn(acqConf.getEvnChn()),
			chn_list_region(acqConf.getRegionChn()),
            // legacy, now we do not load enabled channels from config files
            //nUnits_region(acqConf.getN_units_Region()),
            nUnits_region(),//  will be updated after initialization
            spikeTmpBuf(nullptr),
            dataReady(false)
		{}

		//destructors
		~PlexDataCollector()
		{
			if (spike_buf != nullptr)
			{
				delete spike_buf;
			}
			spike_buf = nullptr;

            if (pServerDataBuffer != nullptr)
            {
                delete pServerDataBuffer;
            }
            pServerDataBuffer = nullptr;

            if (spikeTmpBuf != nullptr)
            {
                delete spikeTmpBuf;
            }
            spikeTmpBuf = nullptr;
		}

		//inlines: gets sets and is
		int getChannelsUsed() const { return channelsUsed; }
		int getnUnits() const { return nUnits; }
        int getnUnits(const int re_idx) const
        {
            if (re_idx<nUnits_region.size())
                return nUnits_region[re_idx];
            else
                return 0;
        }
        string_vec getRegion() const {return region_list;}

        //this return binsize in second
		float getBinSize() const { return binSize_s; }
		size_t getBufSize() const 
		{
			if (spike_buf!=nullptr)
				return spike_buf->size();
			else
				return 0;
		}
		bool isBufFull() { return (spike_buf != nullptr) && spike_buf->full(); }
		bool isBufEmpty() { return (spike_buf != nullptr) && spike_buf->empty(); }
        bool isDataReady() {return (spike_buf != nullptr) && dataReady; }
		int getMax_data_blocks_per_read() const { return max_data_blocks_per_read; }
		bool isStop() const { return stop; }
		Trodal getTrodal() const { return trodal; }
		bool isEvent(std::string eventName)
		{
			for (string_vec::iterator iter = eventQueue.begin(); iter < eventQueue.end(); iter++)
			{
				if ((*iter).compare(eventName) == 0)
				{
					return true;
				}
			}
			return false;
		}

		int getBufIdx(const int chn_idx, const int unit)// note: the first param is the idx of chn in enChn, not the actual chn number
		{
			assert(unit > 0);

			//std::cout << "nUnits_acc[" << chn_idx << "]=" << nUnits_acc[chn_idx] << std::endl;
			return nUnits_acc[chn_idx] + unit-1;//************* pServerDataBuffer[i].Unit==0 -> unsorted or storted?
		}

		std::string getEvent(const int chn)
		{
			for (int i = 0; i < evnChn.size(); i++)
			{
				if (chn == evnChn[i])
				{
					return event_list[i];
				}
			}
            //std::cout << "invalid event channel = " << chn << std::endl;
			return "unknown";
        }

        int_vec get_emg_chn() const {return emgChn;}

        string_vec get_eventQueue() const {return eventQueue;}

        string_vec get_event_list() const {return event_list;}

        //should be friend
        //circular_buffer2d<float> *get_spike_buf_ptr(){return spike_buf;}

        void resetReady(){dataReady = false;}
		//acquisitor params can only be set through configuration file
		void stopAcquisition(){ if (!stop) { stop = true; } }

		// methods
        int init();
		void start();
		void collect();
        void collect_thread();
		Eigen::MatrixXf read(std::string region, int start=0, int end=0) const;
        float read_emg(int start=0, int end=0) const
        {
            Eigen::MatrixXf tmp = emg_buf->read(start,end);
            //std::cout <<"read emg tmp = "<< tmp<<std::endl;
            if (tmp.rows()==0)
                return 0;
            return tmp(0);
        }

		//for training
		//Eigen::MatrixXf getLastTrainSeq(int & bIdOut, const std::vector<int> channels, const std::vector<int> units, const double trainSetLength_s) const;

		//Eigen::MatrixXf getLastTrainSeq(const std::vector<int> channels, const std::vector<int> units, const double trainSetLength_s, const int startBid) const;
		// for decoding
		//Eigen::VectorXf getCounts(const std::vector<int>& channels, const std::vector<int>& units, int &bufferIdx) const;
		
		//Eigen::VectorXf getCounts(int& bufferIdx, const std::vector<int>& channels, const std::vector<int>& units, int bufferIdx_in = -1) const;
		//std::vector<Eigen::VectorXf> getCountsCont(int& bufferIdx, const std::vector<int>& channels, const std::vector<int>& units, int bufferIdx_in = -1, int nBins = -1) const;
		//Eigen::MatrixXf getCounts4ModelFree(int& bufferIdx_out, const std::vector<int>& channels, const std::vector<int>& units, int bufferIdx_in, int nBins) const;
	};
}
#endif
