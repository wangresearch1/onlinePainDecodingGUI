/**************************************************************************
---------------------------------------------------------------------------
Data collectors based on SDKs from acquisition system providers.
current implemented:
PlexonDataCollector for Plexon OmniPlex(Plexon Inc. Dallas)
Plexon C++ SDK required.

author: Sile Hu, yuehusile@gmail.com
version: v1.0
date: 2017-12-12
___________________________________________________________________________
***************************************************************************/

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include "dataCollector.h"
#include <numeric>

#include <windows.h>
#include <tchar.h>
#include <winerror.h>

namespace painBMI
{

/************************************************************************
*               Model based decoder                                     *
*************************************************************************/
/*
 * update baseline mean & std of model-based decoders.
 * input: data - baseline latent variables, for PLDS/TLDS data is vector
 * ouput: void
 */
int PlexDataCollector::init()
{
    int h = PL_InitClientEx3(0, NULL, NULL);
    int numch;
    int npw;
    int npre;
    int gainmult;
    PL_GetGlobalPars(&numch, &npw, &npre, &gainmult);

    std::cout << "Plexon acquisition init: numch = " << numch << std::endl;

    // update nUnits and number of units in each channel
    int num[128];
    PL_GetNumUnits(num);

    // update enabled channels based on trodal
    enChn.clear();
    nUnits_chn.clear();
    nUnits_acc.clear();
    buf_idx_region.clear();
    int_vec tmp_vec;
    for (int i = 0; i < region_list.size(); i++)
    {
        buf_idx_region.push_back(tmp_vec);
        nUnits_region.push_back(0);
    }
    int acc = 0;
    int cnt = 0;
    bool break_flag = false;
    int step = 1;
    if (trodal == single) step = 1;
    if (trodal == stereotrode) step = 2;
    if (trodal == tetrode) step = 4;
    for (int i = 0; i < numch; i+=step)
    {
        cnt++;
        if (num[i] > 0)
        {
            enChn.push_back(i + 1);// channel number is 1-based
            nUnits_chn.push_back(num[i]);
            nUnits_acc.push_back(acc);

            // get the buf idx list for each region
            for (int j = 0; j < region_list.size(); j++)
            {
                for (int k = 0; k < chn_list_region[j].size(); k++)
                {
                    if (cnt == chn_list_region[j][k])
                    {
                        for (int idx = acc; idx < acc + num[i]; idx++)
                        {
                            buf_idx_region[j].push_back(idx);
                            nUnits_region[j]++;
                        }
                        break;
                    }
                }
                if (break_flag) break;
            }
            acc += num[i];
        }
    }
    std::cout <<"enabled channels= "<< std::endl;
    for (int j = 0; j < enChn.size(); j++)
    {
        std::cout <<"channel="<< (enChn[j]-1)/step+1<<" nUnits="<<nUnits_chn[j]<<std::endl;
    }
    std::cout << "total nUnits = "<< acc<<std::endl;
    for (int j = 0; j < region_list.size(); j++)
    {
        std::cout <<region_list[j]<< " nUnits = "<< nUnits_region[j]<<std::endl;
    }

    nUnits = acc;
    spike_buf = new circular_buffer2d<float>(10000, nUnits);
    emg_buf = new circular_buffer2d<float>(10000, 1);
    // init data collection
    HANDLE hServerPollEvent;   // handle to Win32 synchronization event

    std::cout << "in collect thread" << std::endl;

    // open the Win32 synchronization event used to synchronize with the server
    hServerPollEvent = OpenEvent(SYNCHRONIZE, FALSE, L"PlexonServerEvent");
    if (!hServerPollEvent)
    {
        printf("Couldn't open server poll event, I can't continue!\r\n");
        Sleep(3000); //** pause before console window closes
        return -1;
    }

    pServerDataBuffer = (PL_Event*)malloc(sizeof(PL_Event)*max_data_blocks_per_read);
    if (pServerDataBuffer == NULL)
    {
        printf("Couldn't allocate memory, I can't continue!\r\n");
        Sleep(3000); // pause before console window closes
        return -1;
    }

    pServerWaveBuffer = (PL_WaveLong*)malloc(sizeof(PL_WaveLong)*max_data_blocks_per_read);
    if (pServerWaveBuffer == NULL)
    {
        printf("Couldn't allocate memory, I can't continue!\r\n");
        Sleep(3000); // pause before console window closes
        return -1;
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

    spikeTmpBuf = (float*)malloc(sizeof(float)*nUnits);
    if (spikeTmpBuf == NULL)
    {
        printf("Couldn't allocate spikeTmpBuf memory, I can't continue!\r\n");
        Sleep(3000); // pause before console window closes
        return -1;
    }
    return 0;
}

void PlexDataCollector::start()
{
    if (!stop)
    {
        std::cout << "acquisition already started!" << std::endl;
        return;
    }

    // for debug
    //acquisitionThread = std::thread(&PlexDataCollector::collect_thread, std::ref(*this));
    //acquisitionThread.detach();
    stop = false;
    std::cout << "acquisition started!" << std::endl;
    //collect();
}

void PlexDataCollector::collect_thread()
{
    int spk_cnt = 0;
    int NumDataBlocks;
    int NumWaveBlocks;

    while (1)
    {
        eventQueue.clear();

        spk_cnt = 0;
        if (!stop)
        {

            NumDataBlocks = max_data_blocks_per_read;

            // call the server to get all the data blocks since the last time we called PL_GetTimeStampStructures
            //currently only need events, so last param = 0;
            PL_GetTimeStampStructures(&NumDataBlocks, pServerDataBuffer);
            // check to see if the incoming data completely filled the buffer (indicating that more was available);
            if (NumDataBlocks == max_data_blocks_per_read)
            {
                printf("(more than %d data blocks were available)\r\n", max_data_blocks_per_read);
            }

            memset(spikeTmpBuf, 0, sizeof(float)*nUnits);
            std::cout << "NumDataBlocks=" << NumDataBlocks << std::endl;
            // step through the array of data blocks
            for (int i = 0; i < NumDataBlocks; i++)
            {
                // continuous event for EMG?
                /*if (pServerWaveBuffer[i].Type == PL_ADDataType) // to reduce output, we only dump one channel
                {
                    int ContinuousChannel = pServerDataBuffer[i].Channel;
                    int NumContinuousSamples = pServerDataBuffer[i].NumberOfDataWords;
                    int SampleTime = pServerDataBuffer[i].TimeStamp; // timestamp of the first sample in the block
                }*/

                // spikes
                if (PL_SingleWFType == pServerDataBuffer[i].Type)
                {
                    std::cout << "spike: chn=" << pServerDataBuffer[i].Channel << " unit=" << pServerDataBuffer[i].Unit << std::endl;
                    // check if the channel and unit are valid and enabled
                    for (int j = 0; j < enChn.size(); j++)
                    {
                        if (enChn[j] == pServerDataBuffer[i].Channel)
                        {
                            if (pServerDataBuffer[i].Unit <= nUnits_chn[j] && pServerDataBuffer[i].Unit>0)//ignore Unit=0, it's unsorted unit.
                            {
                                spk_cnt++;
                                spikeTmpBuf[getBufIdx(j, pServerDataBuffer[i].Unit)]++;
                                std::cout << "buf idx=" << getBufIdx(j, pServerDataBuffer[i].Unit) << std::endl;
                            }
                            break;
                        }
                    }
                }

                // update external event queue
                if (PL_ExtEventType == pServerDataBuffer[i].Type)
                {
                    std::string evn = getEvent(pServerDataBuffer[i].Channel);
                    if (evn.compare("unknown") != 0)
                    {
                        eventQueue.push_back(evn);
                        std::cout << "chn=" << pServerDataBuffer[i].Channel << " evn=" << evn << std::endl;
                    }
                }
            }
            spike_buf->put(spikeTmpBuf);

            // for debug
            //Eigen::VectorXf vec0;
            //if (spk_cnt > 0)
            //{
            //	spike_buf->put(spikeTmpBuf);
            //	vec0 = spike_buf->read();
            //	for (int ii = 0; ii < vec0.size();ii++)
            //	std::cout << "vec0["<<ii<<"]= " << vec0[ii] << std::endl;
            //}

            // for debug
            //for (int i = 0; i < nUnits; i++)
            //{
            //	spikeTmpBuf[i] = i;
            //}
            //spike_buf->put(spikeTmpBuf);
            //for (int i = 0; i < nUnits; i++)
            //{
            //	spikeTmpBuf[i] = i+100;
            //}
            //spike_buf->put(spikeTmpBuf);
            //std::cout << read("S1",1,0) << std::endl;


            Sleep(50);
        }
        else
        {
            Sleep(100);
        }
    }

}

void PlexDataCollector::collect() // interface with plexon data collection, both continuous and time stamps
{
    int spk_cnt = 0;
    int evn_cnt = 0;
    int NumDataBlocks;
    int NumWaveBlocks;

    eventQueue.clear();
    int freq, gain, chns;
    //PL_GetSlowInfo(&freq, &chns, &gain);
    bool lfp_done;

    spk_cnt = 0;
    evn_cnt = 0;
    int Dummy;               // dummy parameter for function calls
    if (!stop)
    {

        lfp_done = false;
        memset(spikeTmpBuf, 0, sizeof(float)*nUnits);

        NumWaveBlocks = max_data_blocks_per_read;
        PL_GetLongWaveFormStructures(&NumWaveBlocks, pServerWaveBuffer,&Dummy,&Dummy);
        if (NumWaveBlocks == max_data_blocks_per_read)
        {
            printf("(more than %d data blocks were available)\r\n", max_data_blocks_per_read);
        }

        for (int i = 0; i < NumWaveBlocks; i++)
        {
            int NumContinuousSamples = pServerWaveBuffer[i].NumberOfDataWords;
            if (pServerWaveBuffer[i].Type == PL_ADDataType && !lfp_done) // to reduce output, we only dump one channel
            {
                int ContinuousChannel = pServerWaveBuffer[i].Channel;
                //std::cout << "i="<<i<<" "<<ContinuousChannel<<std::endl;
                //std::cout << "NumContinuousSamples="<<NumContinuousSamples<<std::endl;
                if (ContinuousChannel==129+get_emg_chn()[0]-1)// Plex channel 129, 1st channel of LFP
                {
                    float* emg;
                    float tmp = float(pServerWaveBuffer[i].WaveForm[0]);
                    //tmp = tmp * 400/128*1000;  // mv
                    emg =  &tmp;
                    emg_buf->put(emg);
                    //break;
                    lfp_done = true;
                }
            }
            // spikes
            if (PL_SingleWFType == pServerWaveBuffer[i].Type)
            {
                // check if the channel and unit are valid and enabled
                for (int j = 0; j < enChn.size(); j++)
                {
                    if (enChn[j] == pServerWaveBuffer[i].Channel)
                    {
                        if (pServerWaveBuffer[i].Unit <= nUnits_chn[j] && pServerWaveBuffer[i].Unit>0)//ignore Unit=0, it's unsorted unit.
                        {
                            spk_cnt++;
                            spikeTmpBuf[getBufIdx(j, pServerWaveBuffer[i].Unit)]++; //tempory one bin spike buffer, 1-D vector
                        }
                        break;
                    }
                }
            }
            // update external event queue
            if (PL_ExtEventType == pServerWaveBuffer[i].Type)
            {
                std::string evn = getEvent(pServerWaveBuffer[i].Channel);
                if (evn.compare("unknown") != 0)
                {
                    evn_cnt++;
                    eventQueue.push_back(evn); // get event name
                    std::cout << "chn=" << pServerWaveBuffer[i].Channel << " evn=" << evn << std::endl;
                }
            }

        }
/*
        NumDataBlocks = max_data_blocks_per_read;

        // call the server to get all the data blocks since the last time we called PL_GetTimeStampStructures
        PL_GetTimeStampStructures(&NumDataBlocks, pServerDataBuffer);
        // check to see if the incoming data completely filled the buffer (indicating that more was available);
        if (NumDataBlocks == max_data_blocks_per_read)
        {
            printf("(more than %d data blocks were available)\r\n", max_data_blocks_per_read);
        }

        memset(spikeTmpBuf, 0, sizeof(float)*nUnits);
        // step through the array of data blocks
        for (int i = 0; i < NumDataBlocks; i++)
        {
            // continuous event for EMG?
            //if (pServerDataBuffer[i].Type == PL_ADDataType) // to reduce output, we only dump one channel
            //{
            //	int ContinuousChannel = pServerDataBuffer[i].Channel;
            //	int NumContinuousSamples = pServerDataBuffer[i].NumberOfDataWords;
            //	int SampleTime = pServerDataBuffer[i].TimeStamp; // timestamp of the first sample in the block
            //}

            // spikes
            if (PL_SingleWFType == pServerDataBuffer[i].Type)
            {
                // check if the channel and unit are valid and enabled
                for (int j = 0; j < enChn.size(); j++)
                {
                    if (enChn[j] == pServerDataBuffer[i].Channel)
                    {
                        if (pServerDataBuffer[i].Unit <= nUnits_chn[j] && pServerDataBuffer[i].Unit>0)//ignore Unit=0, it's unsorted unit.
                        {
                            spk_cnt++;
                            spikeTmpBuf[getBufIdx(j, pServerDataBuffer[i].Unit)]++; //tempory one bin spike buffer, 1-D vector
                        }
                        break;
                    }
                }
            }

            // update external event queue
            if (PL_ExtEventType == pServerDataBuffer[i].Type)
            {
                std::string evn = getEvent(pServerDataBuffer[i].Channel);
                if (evn.compare("unknown") != 0)
                {
                    evn_cnt++;
                    eventQueue.push_back(evn); // get event name
                    std::cout << "chn=" << pServerDataBuffer[i].Channel << " evn=" << evn << std::endl;
                }
            }
        }
        */
        spike_buf->put(spikeTmpBuf); // total spike buffer, 2-D matrix
        dataReady = true;
    }
}

Eigen::MatrixXf PlexDataCollector::read(std::string region, int start, int end) const
{
    Eigen::MatrixXf m;
    //find the region idx
    int idx = -1;
    for (int i = 0; i < region_list.size(); i++)
    {
        if (region.compare(region_list[i]) == 0)
        {
            idx = i;
            break;
        }
    }
    if (idx >= 0)
    {
        m = spike_buf->read(start, end, buf_idx_region[idx]);
    }

    //std::cout << "Debug : " << m.rows() << m.cols() << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
    return m;
}
}
