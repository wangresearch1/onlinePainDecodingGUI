#ifndef __ACQUISITOR_H__
#define __ACQUISITOR_H__

#include <Eigen/Dense>
#include <Eigen/Core>
#include <windows.h>
#include <process.h>
#include <thread>
#include "Plexon.h"
#include <ctime>
#include <chrono>


/* consts */

// maximum number of data blocks to be read at one time from the server
const int max_data_blocks_per_read_default = 500000;
const int spike_buffer_size_default = 10000; //unit: bins
//default values
const int channels_used_default = 64;
const int max_unit_each_channel_default = 5;//5 means unit 0 is unsorted unit,plus 4 sorted unit
const long int bin_size_ms_default = 50; //unit ms

class PlexAcquisitor
{
private:
    int channelsUsed = channels_used_default;
	int maxUnitEachChannel = max_unit_each_channel_default;
    unsigned long binSize = bin_size_ms_default * 1000; // in micro seconds, this is the buffer binsize
    double binSize_s = double(bin_size_ms_default)/1000.0;
	int spikeBufferSize = spike_buffer_size_default;
	int* spikeBinsBuffer = nullptr;
	int bufferIdx = 0;
	double accessBinSize = 0.05;//unit in second,this should be times of buffer bin size
	bool stop = true;
	PL_Event* pServerDataBuffer = nullptr;  // buffer in which the server will return acquisition data
	int max_data_blocks_per_read = max_data_blocks_per_read_default;
	std::clock_t acquisitionStartTime;
	std::thread acquisitionThread;
    double clientTimeElapsed;
    int trodal;

public:
	//construtors
	//note: do not construct thread object in constructors, startAquisition/stopAquisition will start/end aquisition thread.
	PlexAcquisitor() :
		channelsUsed(channels_used_default),
		maxUnitEachChannel(max_unit_each_channel_default),
		binSize(bin_size_ms_default * 1000),
        binSize_s(double(bin_size_ms_default)/1000.0),
		spikeBufferSize(spike_buffer_size_default),
		spikeBinsBuffer(nullptr),
		bufferIdx(0),
		accessBinSize(float(bin_size_ms_default) / 1000.0),
		stop(true),
		pServerDataBuffer(nullptr),
		max_data_blocks_per_read(max_data_blocks_per_read_default),
        acquisitionStartTime(-1),
        trodal(2)
	{
		spikeBinsBuffer = new int[spikeBufferSize * maxUnitEachChannel * channelsUsed];
		memset((void*)spikeBinsBuffer, 0, sizeof(int)*spikeBufferSize * maxUnitEachChannel * channelsUsed);
		pServerDataBuffer = new PL_Event[max_data_blocks_per_read];
	}

	PlexAcquisitor(PlexAcquisitor& rhs) :
		channelsUsed(rhs.channelsUsed),
		maxUnitEachChannel(rhs.maxUnitEachChannel),
		binSize(rhs.binSize),
        binSize_s(rhs.binSize_s),
		spikeBufferSize(rhs.spikeBufferSize),
		spikeBinsBuffer(nullptr),
		bufferIdx(rhs.bufferIdx),
		accessBinSize(rhs.accessBinSize),
		stop(rhs.stop),
		pServerDataBuffer(rhs.pServerDataBuffer),
		max_data_blocks_per_read(rhs.max_data_blocks_per_read),
        acquisitionStartTime(rhs.acquisitionStartTime),
        trodal(rhs.trodal)
	{
		spikeBinsBuffer = new int[spikeBufferSize];
        memcpy(spikeBinsBuffer, rhs.spikeBinsBuffer, sizeof(int)*spikeBufferSize * maxUnitEachChannel * channelsUsed);
		pServerDataBuffer = new PL_Event[max_data_blocks_per_read];
	}

	//operator
	PlexAcquisitor& operator = (PlexAcquisitor& rhs)
	{
		if (this == &rhs) return *this;

		channelsUsed = rhs.channelsUsed;
		maxUnitEachChannel = rhs.maxUnitEachChannel;
		binSize = rhs.binSize;
        binSize_s = rhs.binSize_s;
		spikeBufferSize = rhs.spikeBufferSize;
		bufferIdx = rhs.bufferIdx;
		accessBinSize = rhs.accessBinSize;
		stop = rhs.stop;
		acquisitionStartTime = rhs.acquisitionStartTime;
        trodal = rhs.trodal;

		int* p_spikeBinsBuffer_origin = spikeBinsBuffer;
		spikeBinsBuffer = new int[spikeBufferSize];
        memcpy(spikeBinsBuffer, rhs.spikeBinsBuffer, sizeof(int)*spikeBufferSize * maxUnitEachChannel * channelsUsed);
		if (nullptr != p_spikeBinsBuffer_origin)
			delete [] p_spikeBinsBuffer_origin;

		PL_Event* pServerDataBuffer_origin = pServerDataBuffer;
		pServerDataBuffer = new PL_Event[max_data_blocks_per_read];
		memcpy(pServerDataBuffer, rhs.pServerDataBuffer, sizeof(PL_Event)*max_data_blocks_per_read);
		if (nullptr != pServerDataBuffer_origin)
			delete [] pServerDataBuffer_origin;

		return *this;
	}

	//destructors
	~PlexAcquisitor()
	{
		if (nullptr != spikeBinsBuffer)
			delete [] spikeBinsBuffer;
		if (nullptr != pServerDataBuffer)
			delete[] pServerDataBuffer;
	}

	//inlines: gets sets and is
	int getChannelsUsed() const { return channelsUsed; }
	int getMaxUnitEachChannel() const { return maxUnitEachChannel; }
	int getBinSize() const { return binSize; }
	int getSpikeBufferSize() const { return spikeBufferSize; }
	int getBufferIdx() const { return bufferIdx; }
    int getLastBufferIdx() const { return (bufferIdx-1+spikeBufferSize)%spikeBufferSize; }
	double getAccessBinSize() const { return accessBinSize; }
	int getMax_data_blocks_per_read() const { return max_data_blocks_per_read; }
	bool isStop() const { return stop; }
    double getClientTimeElapsed() const {return clientTimeElapsed;}
    bool isTetrodeMode() const {return trodal==4;}
    bool isStereoMode() const {return trodal==2;}

	//acquisitor params can only be set when acquisition has stopped
	void setChannelsUsed(const int channelsUsed_){ if (isStop()) channelsUsed = channelsUsed_; }
	void setMaxUnitEachChannel(const int maxUnitEachChannel_){ if (isStop()) maxUnitEachChannel = maxUnitEachChannel_; }
	void setBinSize(const int binSize_){ if (isStop()) binSize = binSize_; }
	void setSpikeBufferSize(const int spikeBufferSize_){ if (isStop()) spikeBufferSize = spikeBufferSize_; }
	void setBufferIdx(const int bufferIdx_){ if (isStop()) bufferIdx = bufferIdx_; }
	void setAccessBinSize(const double accessBinSize_){ if (isStop()) accessBinSize = accessBinSize_; }
	void setMax_data_blocks_per_read(const int max_data_blocks_per_read_){ if (isStop()) max_data_blocks_per_read = max_data_blocks_per_read_; }
    void stopAcquisition(){ if (!isStop()) { stop = true; acquisitionStartTime = -1; /*CloseHandle(acquisitionThread.native_handle());*/ } }
    void setClientTimeElapsed(const double time){clientTimeElapsed = time;}
    void setTetrodeMode(){trodal = 4;}
    void setSingleMode(){trodal = 1;}
    void setStereoMode(){trodal = 2;}

	// methods
	void initialize();
	void startAcquisition();
	void acquisition();
	int getSpikeCountOflastBin(const int channel, const int unit) const;
    Eigen::MatrixXf getLastTrainSeq(int & bIdOut, const std::vector<int> channels, const std::vector<int> units, const double trainSetLength_s) const;
    Eigen::MatrixXf getLastTrainSeq(const std::vector<int> channels, const std::vector<int> units, const double trainSetLength_s, const int startBid) const;
    Eigen::VectorXf getCounts(const std::vector<int>& channels, const std::vector<int>& units, int &bufferIdx) const;
    Eigen::VectorXf getCounts(int& bufferIdx, const std::vector<int>& channels, const std::vector<int>& units, int bufferIdx_in=-1) const;
    std::vector<Eigen::VectorXf> getCountsCont(int& bufferIdx, const std::vector<int>& channels, const std::vector<int>& units, int bufferIdx_in=-1, int nBins=-1) const;
    Eigen::MatrixXf getCounts4ModelFree(int& bufferIdx_out, const std::vector<int>& channels, const std::vector<int>& units, int bufferIdx_in, int nBins) const;
};

#endif
