#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include "acquisitor.h"
#include <numeric>

#include <windows.h>
#include <tchar.h>
#include <winerror.h>

void PlexAcquisitor::initialize()
{
	int h = PL_InitClientEx3(0, NULL, NULL);
	int numch;
	int npw;
	int npre;
	int gainmult;
    std::cout <<"0"<<std::endl;
	PL_GetGlobalPars(&numch, &npw, &npre, &gainmult);
	if (channelsUsed > numch)
	{
		channelsUsed = numch;
	}
    std::cout <<"1"<<std::endl;
    int num = 0;
    for (int i=0;i<16;i++)
    {
        num = i;
        //PL_GetNumUnits(&num);
        std::cout <<num<<std::endl;
    }
    //std::cout <<num<<std::endl;
}

void PlexAcquisitor::startAcquisition()
{
	if (false == isStop())
	{
		std::cout << "acquisition already started!" << std::endl;
		return;
	}

	acquisitionThread = std::thread(&PlexAcquisitor::acquisition, std::ref(*this));
	acquisitionThread.detach();
	stop = false;
	acquisitionStartTime = std::clock();
}

void PlexAcquisitor::acquisition()
{
    HANDLE hServerPollEvent;   // handle to Win32 synchronization event

    // open the Win32 synchronization event used to synchronize with the server
    hServerPollEvent = OpenEvent(SYNCHRONIZE, FALSE, L"PlexonServerEvent");
    if (!hServerPollEvent)
    {
      printf("Couldn't open server poll event, I can't continue!\r\n");
      Sleep(3000); //** pause before console window closes
      return;
    }

    int NumDataBlocks;
	DWORD backlog = 0;
	for (;;)
	{
		NumDataBlocks = max_data_blocks_per_read;
		PL_GetTimeStampStructures(&NumDataBlocks, pServerDataBuffer);
		backlog += NumDataBlocks;
		if (NumDataBlocks < max_data_blocks_per_read)
			break;
	}
	std::cout << "read " << backlog << " blocks of pending data before entering main loop" << std::endl;
	
	int tsTic = PL_GetTimeStampTick();//get Plexon time stamp resolution, unit micro second 
	unsigned long firstTs = 0;//record first time stamp of current bin

    auto startTime = std::chrono::high_resolution_clock::now();
    auto endTime = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(startTime-endTime);

    int timerBufferIdx = 0;
    double binStartTime = 0;

    static int last_bufferIdx=0;
	while (1)
	{
		if (false == isStop())
		{
            endTime = std::chrono::high_resolution_clock::now();
            elapsed = std::chrono::duration_cast<std::chrono::microseconds>(endTime-startTime);

            startTime = std::chrono::high_resolution_clock::now();

            if (firstTs>0 && clientTimeElapsed-binStartTime>binSize_s)
            {
                std::cout <<"acq1: timerBufferIdx="<< timerBufferIdx<<" bufferIdx="<<bufferIdx<<std::endl;
                timerBufferIdx = (timerBufferIdx + 1) % spikeBufferSize;
                binStartTime += binSize_s;
                bufferIdx = (bufferIdx + 1) % spikeBufferSize;
                firstTs = (firstTs + binSize / tsTic ) & 0xFFFFFFFF;
                memset((void*)(spikeBinsBuffer + bufferIdx * maxUnitEachChannel * channelsUsed), 0, sizeof(int) * maxUnitEachChannel * channelsUsed);
            }
            NumDataBlocks = max_data_blocks_per_read;

			// call the server to get all the data blocks since the last time we called PL_GetTimeStampStructures
			//currently only need events, so last param = 0;
			PL_GetTimeStampStructures(&NumDataBlocks, pServerDataBuffer);
			// check to see if the incoming data completely filled the buffer (indicating that more was available);
            if (NumDataBlocks == max_data_blocks_per_read)
			{
                printf("(more than %d data blocks were available)\r\n", max_data_blocks_per_read);
			}

			if (NumDataBlocks)
			{
				if (0 == firstTs)
				{
                    binStartTime = clientTimeElapsed;
					firstTs = pServerDataBuffer[0].TimeStamp;
					printf("initial fts=%u\n", firstTs);
				}
			}

			// step through the array of data blocks
			for (int i = 0; i < NumDataBlocks; i++)
			{
                if (PL_TetrodeWFType == pServerDataBuffer[i].Type && pServerDataBuffer[i].Unit < maxUnitEachChannel)
                {
                    std::cout<<"PL_TetrodeWFType event"<<std::endl;
                }

                while (pServerDataBuffer[i].TimeStamp>firstTs && pServerDataBuffer[i].TimeStamp - firstTs > binSize/tsTic)
                {
                    bufferIdx = (bufferIdx + 1) % spikeBufferSize;
                    firstTs = (firstTs + binSize / tsTic) & 0xFFFFFFFF;
                    //clear the memory of new bin before counting
                    memset((void*)(spikeBinsBuffer + bufferIdx * maxUnitEachChannel * channelsUsed), 0, sizeof(int) * maxUnitEachChannel * channelsUsed);
                }
				//currently all events are spikes; only record units whose unit number < maxUnitEachChannel
				if (PL_SingleWFType == pServerDataBuffer[i].Type && pServerDataBuffer[i].Unit<maxUnitEachChannel)
				{
					//put spike into the right bin buffer
                    if (pServerDataBuffer[i].Channel <= 64)//channelsUsed)
					{
						int idx = bufferIdx * maxUnitEachChannel * channelsUsed + (pServerDataBuffer[i].Channel - 1)*maxUnitEachChannel + pServerDataBuffer[i].Unit;
                        spikeBinsBuffer[idx]++;
                    }
				}
                if (PL_ExtEventType == pServerDataBuffer[i].Type)
                {
                    if (pServerDataBuffer[i].Channel<4)
                    std::cout<<"ext event on channel "<<pServerDataBuffer[i].Channel<<std::endl;
                }
			}
            if (bufferIdx - last_bufferIdx > 1)
            {
                printf("bufferIdx-last_bufferIdx=%d\n",bufferIdx-last_bufferIdx);
            }
            last_bufferIdx = bufferIdx;
            Sleep(1);
		}
		else
		{
            Sleep(100);
		}
	}
}
int PlexAcquisitor::getSpikeCountOflastBin(const int channel, const int unit) const
{
	int nbins = (int)(accessBinSize * 1000000) / binSize;
	int bId = (bufferIdx - nbins + spikeBufferSize) % spikeBufferSize;

	int idx = bId * maxUnitEachChannel * channelsUsed + (channel - 1)*maxUnitEachChannel + unit;
	int cnt = -1;

	if (spikeBinsBuffer != NULL)
	{
		cnt = spikeBinsBuffer[idx];
		for (int i = 0; i < nbins - 1; i++)
		{
			bId = (bId + 1) % spikeBufferSize;
			idx = bId * maxUnitEachChannel * channelsUsed + (channel - 1)*maxUnitEachChannel + unit;
			cnt += spikeBinsBuffer[idx];
		}
		printf("last bin :bufferIdx=%d bId = %d ch = %d, unit = %d, idx = %d, cnt = %d\n", bufferIdx, bId, channel, unit, idx, cnt);
	}
	return cnt;
}
Eigen::MatrixXf PlexAcquisitor::getCounts4ModelFree(int& bufferIdx_out, const std::vector<int>& channels, const std::vector<int>& units, int bufferIdx_in, int nBins) const
{
    int nUnits = std::accumulate(units.begin(), units.end(), 0);
    Eigen::VectorXf tmpCounts = Eigen::VectorXf::Zero(nUnits);

    int nbins = (int)(accessBinSize * 1000000) / binSize;
    int bId, bId_start;
    static int last_bufferIdx_out;

    if (bufferIdx_in>0 && bufferIdx_in<spikeBufferSize)
    {
        bId_start = bufferIdx_in;
    }
    else
    {
        bId_start = (bufferIdx-bufferIdx%nbins - nbins - 1 + spikeBufferSize) % spikeBufferSize;
    }
    bufferIdx_out = bId_start;
    int step;
    if (nBins>0 && nBins<spikeBufferSize)
    {
        step = nBins;
    }
    else
    {
        step = bufferIdx_out-last_bufferIdx_out;
    }

    if (step>1000) step = 1;
    if (step>0) bId_start = (bId_start-nbins*step + spikeBufferSize)% spikeBufferSize;
    if (step<=0) step = 1;
    Eigen::MatrixXf tmpMat(nUnits,step);

    last_bufferIdx_out = bufferIdx_out;
    int cnt = 0;
    int channel = -1;
    int idx;
    int unit;

    for (int j=0;j<step;j++)
    {
        unit = 0;
        bId_start = (bId_start+nbins + spikeBufferSize)%spikeBufferSize;
        tmpCounts = Eigen::VectorXf::Zero(nUnits);
        for (int ch = 0; ch < channels.size(); ch++)
        {
            channel = (channels.at(ch)-1)*trodal;

            //for debug
            //channel = channels.at(ch)-1;
            for (int u = 1; u <= units.at(ch); u++)//unit 0 of each channel is unsorted units
            {
                bId = bId_start;
                for (int i = 0; i < nbins; i++)
                {
                    bId = (bId + 1) % spikeBufferSize;
                    idx = bId * maxUnitEachChannel * channelsUsed + channel*maxUnitEachChannel + u;
                    tmpCounts(unit) += spikeBinsBuffer[idx];
                }
                unit++;
            }
        }
        tmpMat.col(j) = tmpCounts;
    }

    return tmpMat;
}

 std::vector<Eigen::VectorXf> PlexAcquisitor::getCountsCont(int& bufferIdx_out, const std::vector<int>& channels, const std::vector<int>& units, int bufferIdx_in, int nBins) const
 {
     int nUnits = std::accumulate(units.begin(), units.end(), 0);
     Eigen::VectorXf tmpCounts = Eigen::VectorXf::Zero(nUnits);
     std::vector<Eigen::VectorXf> tmpVec;

     int nbins = (int)(accessBinSize * 1000000) / binSize;
     int bId, bId_start;
     static int last_bufferIdx_out;

     if (bufferIdx_in>0 && bufferIdx_in<spikeBufferSize)
     {
         bId_start = bufferIdx_in;
     }
     else
     {
         bId_start = (bufferIdx-bufferIdx%nbins - nbins - 1 + spikeBufferSize) % spikeBufferSize;
     }
     bufferIdx_out = bId_start;
     int step;
     if (nBins>0 && nBins<spikeBufferSize)
     {
         step = nBins;
     }
     else
     {
         step = bufferIdx_out-last_bufferIdx_out;
     }

     if (step>5) step = 1;
     if (step>0) bId_start = (bId_start-nbins*step + spikeBufferSize)% spikeBufferSize;
     if (step<=0) step = 1;

     last_bufferIdx_out = bufferIdx_out;
     int cnt = 0;
     int channel = -1;
     int idx;
     int unit;

     for (int j=0;j<step;j++)
     {
         unit = 0;
         bId_start = (bId_start+nbins + spikeBufferSize)%spikeBufferSize;
         tmpCounts = Eigen::VectorXf::Zero(nUnits);
         for (int ch = 0; ch < channels.size(); ch++)
         {
             channel = (channels.at(ch)-1)*trodal;

             //for debug
             //channel = channels.at(ch)-1;
             for (int u = 1; u <= units.at(ch); u++)//unit 0 of each channel is unsorted units
             {
                 bId = bId_start;
                 for (int i = 0; i < nbins; i++)
                 {
                     bId = (bId + 1) % spikeBufferSize;
                     idx = bId * maxUnitEachChannel * channelsUsed + channel*maxUnitEachChannel + u;
                     tmpCounts(unit) += spikeBinsBuffer[idx];
                 }
                 unit++;
             }
         }
         tmpVec.push_back(tmpCounts);
     }

     return tmpVec;
 }
Eigen::VectorXf PlexAcquisitor::getCounts(int& bufferIdx_out,const std::vector<int>& channels, const std::vector<int>& units,int bufferIdx_in) const
{
    int nUnits = std::accumulate(units.begin(), units.end(), 0);
    Eigen::VectorXf tmpCounts = Eigen::VectorXf::Zero(nUnits);

    int nbins = (int)(accessBinSize * 1000000) / binSize;
    int bId, bId_start;
    static int last_bufferIdx_out;

    if (bufferIdx_in>0 && bufferIdx_in<spikeBufferSize)
    {
        bId_start = bufferIdx_in;
    }
    else
    {
        bId_start = (bufferIdx-bufferIdx%nbins - nbins - 1 + spikeBufferSize) % spikeBufferSize;
    }

    bufferIdx_out = bId_start;
    last_bufferIdx_out = bufferIdx_out;
    int cnt = 0;
    int channel = -1;
    int idx;
    int unit;

    unit = 0;
    for (int ch = 0; ch < channels.size(); ch++)
    {
        channel = (channels.at(ch)-1)*trodal;

        //for debug
        //channel = channels.at(ch)-1;
        for (int u = 1; u <= units.at(ch); u++)//unit 0 of each channel is unsorted units
        {
            bId = bId_start;
            for (int i = 0; i < nbins; i++)
            {
                bId = (bId + 1) % spikeBufferSize;
                idx = bId * maxUnitEachChannel * channelsUsed + channel*maxUnitEachChannel + u;
                tmpCounts(unit) += spikeBinsBuffer[idx];
            }
            unit++;
        }
    }

    return tmpCounts;
}

Eigen::VectorXf PlexAcquisitor::getCounts(const std::vector<int> &channels, const std::vector<int> &units, int & bufferIdx_in_out) const
{
	int nUnits = std::accumulate(units.begin(), units.end(), 0);
	Eigen::VectorXf tmpCounts = Eigen::VectorXf::Zero(nUnits);

	int nbins = (int)(accessBinSize * 1000000) / binSize;
	int bId, bId_start;

    if (-1 == bufferIdx_in_out)//bufferIdx_in==-1 means last bin;
	{
        bId_start = (bufferIdx-bufferIdx%nbins - nbins - 1 + spikeBufferSize) % spikeBufferSize;
	}
	else
	{
        bId_start = (bufferIdx_in_out + spikeBufferSize) % spikeBufferSize;
	}

	int cnt = 0;
	int channel = -1;
	int idx;
	int unit;

	unit = 0;
	for (int ch = 0; ch < channels.size(); ch++)
    {
        channel = (channels.at(ch)-1)*trodal;

        //for debug
        //channel = channels.at(ch)-1;
		for (int u = 1; u <= units.at(ch); u++)//unit 0 of each channel is unsorted units
		{
			bId = bId_start;
			for (int i = 0; i < nbins; i++)
			{
				bId = (bId + 1) % spikeBufferSize;
                idx = bId * maxUnitEachChannel * channelsUsed + channel*maxUnitEachChannel + u;
				tmpCounts(unit) += spikeBinsBuffer[idx];
			}
			unit++;
		}
	}
    bufferIdx_in_out = bId;

	return tmpCounts;
}

Eigen::MatrixXf PlexAcquisitor::getLastTrainSeq(int & bIdOut, const std::vector<int> channels, const std::vector<int> units, const double trainSetLength_s) const
{
	//acquisition not started or train Set Length larger than acquisited buffer
	if (-1 == acquisitionStartTime || (std::clock() - acquisitionStartTime) / (double)CLOCKS_PER_SEC < trainSetLength_s)
	{
		std::cout << "acquisition not started or train Set Length larger than acquisited buffer" << std::endl;
		std::cout <<"current duration: "<< (std::clock() - acquisitionStartTime) / (double)CLOCKS_PER_SEC << std::endl;
		return Eigen::MatrixXf::Zero(1, 1);
	}

	int nUnits = std::accumulate(units.begin(), units.end(), 0);
	int nSeqBins = floor(trainSetLength_s / accessBinSize);
	Eigen::MatrixXf tmpSeq = Eigen::MatrixXf::Zero(nSeqBins, nUnits);

	int nbins = (int)(accessBinSize * 1000000) / binSize;
	int bId = (bufferIdx - nbins*nSeqBins-1 + spikeBufferSize) % spikeBufferSize;

	if (spikeBinsBuffer != NULL)
	{
		for (int bin = 0; bin < nSeqBins; bin++)
		{
			tmpSeq.row(bin) = getCounts(channels, units, bId).transpose();
		}
	}
    bIdOut = bId;

	return tmpSeq;
}

Eigen::MatrixXf PlexAcquisitor::getLastTrainSeq(const std::vector<int> channels, const std::vector<int> units, const double trainSetLength_s, const int startBid) const
{
    //acquisition not started or train Set Length larger than acquisited buffer
    if (-1 == acquisitionStartTime || (std::clock() - acquisitionStartTime) / (double)CLOCKS_PER_SEC < trainSetLength_s)
    {
        std::cout << "acquisition not started or train Set Length larger than acquisited buffer" << std::endl;
        std::cout <<"current duration: "<< (std::clock() - acquisitionStartTime) / (double)CLOCKS_PER_SEC << std::endl;
        return Eigen::MatrixXf::Zero(1, 1);
    }

    int nUnits = std::accumulate(units.begin(), units.end(), 0);
    int nbins = (int)(accessBinSize * 1000000) / binSize;
    int nSeqBins = floor(bufferIdx-startBid / nbins);
    Eigen::MatrixXf tmpSeq = Eigen::MatrixXf::Zero(nSeqBins, nUnits);
    int bId = (startBid - 1 + spikeBufferSize) % spikeBufferSize;

    if (spikeBinsBuffer != NULL)
    {
        for (int bin = 0; bin < nSeqBins; bin++)
        {
            tmpSeq.row(bin) = getCounts(channels, units, bId).transpose();
        }
    }

    return tmpSeq;
}
