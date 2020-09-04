#ifndef __SETTINGS_H__
#define __SETTINGS_H__
//PDC for pain detect client
#include <vector>
#include <iostream>

namespace PDC {
const int nMaxChannel = 64;
const int nMaxUnit = 5;

const int maxIter = 50;
const int maxCpuTime_s = 30;
const float painThresh = 1.65;
const float painThreshAcc = 1.65;
const float painThreshS1 = 1.65;
const float nStd = 2;
const int baseStartBefore_s = 10;
const int baseEndBefore_s = 2;
const int trainSeqLength_s = 10;
//default enabled channels, units list


//fake setting
//acc 2017-06-29 neptune3
const std::vector<int> enabledChannels = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
const std::vector<int> enabledUnits    = {3, 4, 3, 2, 2, 3, 1, 1, 2, 2,  1,  2,  2,   3,  3};//number of enabled units
const std::vector<int> ACCChannels = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

//acc 2017-06-29 neptune3
//const std::vector<int> enabledChannels = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
//const std::vector<int> enabledUnits    = {3, 4, 3, 2, 2, 3, 1, 1, 2, 2,  1,  2,  2,   3,  3};//number of enabled units
//const std::vector<int> ACCChannels = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
//acc 2017-4-28
//const std::vector<int> enabledChannels = {1, 3, 5, 7, 17};
//const std::vector<int> enabledUnits    = {1, 1, 1, 1, 1};//number of enabled units
//const std::vector<int> ACCChannels = { 1, 2, 3, 4, 5, 6, 7, 8,17 };

//jupiter02 2017-05-04
//const std::vector<int> enabledChannels = {1, 2, 7, 16};
//const std::vector<int> enabledUnits    = {2, 1, 2, 3};//number of enabled units
//const std::vector<int> ACCChannels = { 1, 2, 3, 4, 5, 6, 7, 8 };

//jupiter02 2017-4-28
//const std::vector<int> enabledChannels = {1, 3, 4, 5, 6, 7, 8,14,15};
//const std::vector<int> enabledUnits    = {3, 2, 1, 2, 2, 2, 2, 1, 1};//number of enabled units
//const std::vector<int> ACCChannels = { 1, 2, 3, 4, 5, 6, 7, 8 };

//jupiter02 2017-4-27
//const std::vector<int> enabledChannels = {1, 7, 9,10,11, 9, 10,11,13,14,15,16};
//const std::vector<int> enabledUnits    = {2, 1, 1, 1, 2, 2, 3, 1, 1, 1, 1, 2};//number of enabled units
//const std::vector<int> ACCChannels = { 9,10,11,12,13,14,15,16 };

//const std::vector<int> enabledChannels = {1, 2, 3, 4, 9, 10,11,12};
//const std::vector<int> enabledChannels = {1, 2, 3, 4, 5, 9, 10,11,14,15};
//const std::vector<int> enabledUnits    = {3, 2, 1, 2, 2, 2, 2, 1, 1, 1};//number of enabled units
//const std::vector<int> enabledChannels = {5, 6, 7, 8, 9, 10,11};
//const std::vector<int> enabledUnits    = {1, 1, 1, 1, 2, 1, 1,};//number of enabled units
//default dual recording region channel map
//const std::vector<int> ACCChannels = { 1, 2, 3, 4, 5, 6, 7, 8 };
//const std::vector<int> S1Channels = { 9, 10, 11, 12, 13, 14, 15, 16 };
}

class PdcOptions
{
private:
    int nMaxChannel;
    int nMaxUnit;
    int maxIter;
    int maxCpuTime_s;
    float painThresh;
    float painThreshAcc;
    float painThreshS1;
    float nStd;
    int baseStartBefore_s;
    int baseEndBefore_s;
    int trainSeqLength_s;
    int nUnitsAcc;
    int nUnitsS1;

    std::vector<bool> isEnChannels;
    std::vector<bool> isAccChannels;
    std::vector<int> enUnits;
    std::vector<int> enabledACCChannels;
    std::vector<int> enabledS1Channels;
    std::vector<int> enabledACCUnits;
    std::vector<int> enabledS1Units;
public:
    //constructor:
    PdcOptions():
        nMaxChannel(PDC::nMaxChannel),
        nMaxUnit(PDC::nMaxUnit),
        maxIter(PDC::maxIter),
        maxCpuTime_s(PDC::maxCpuTime_s),
        painThresh(PDC::painThresh),
        painThreshAcc(PDC::painThreshAcc),
        painThreshS1(PDC::painThreshS1),
        nStd(PDC::nStd),
        baseStartBefore_s(PDC::baseStartBefore_s),
        baseEndBefore_s(PDC::baseEndBefore_s),
        trainSeqLength_s(PDC::trainSeqLength_s),
        nUnitsAcc(0),
        nUnitsS1(0),
        isEnChannels(std::vector<bool>(nMaxChannel,false)),
        isAccChannels(std::vector<bool>(nMaxChannel,false)),
        enUnits(std::vector<int>(nMaxChannel,0))
    {
        unsigned int i;
        for(i=0;i<PDC::enabledChannels.size();i++)
        {
            isEnChannels.at(PDC::enabledChannels.at(i)) = true;
            enUnits.at(PDC::enabledChannels.at(i)) = PDC::enabledUnits.at(i);
            if (enUnits.at(PDC::enabledChannels.at(i))>nMaxUnit)
            {
                enUnits.at(PDC::enabledChannels.at(i)) = nMaxUnit;
            }
        }
        for(i=0;i<PDC::ACCChannels.size();i++)
        {
            isAccChannels.at(PDC::ACCChannels.at(i)) = true;
        }
        for(i=0;i<nMaxChannel;i++)
        {
            if (isEnChannels.at(i))
            {
                if (isAccChannels.at(i))
                {
                    enabledACCChannels.push_back(i);
                    enabledACCUnits.push_back(enUnits.at(i));
                }
                else
                {
                    enabledS1Channels.push_back(i);
                    enabledS1Units.push_back(enUnits.at(i));
                }
            }
        }
        nUnitsAcc = enabledACCUnits.size();
        nUnitsS1 = enabledS1Units.size();
    }
    //sets and gets
    int getNMaxChannel() const {return nMaxChannel;}
    int getNMaxUnit()const {return nMaxUnit;}
    int getMaxIter()const {return maxIter;}
    int getMaxCpuTime_s()const {return maxCpuTime_s;}
    float getPainThresh()const {return painThresh;}
    float getPainThreshAcc()const {return painThreshAcc;}
    float getPainThreshS1()const {return painThreshS1;}
    float getNStd()const {return nStd;}
    int getBaseStartBefore_s()const {return baseStartBefore_s;}
    int getBaseEndBefore_s()const {return baseEndBefore_s;}
    int getTrainSeqLength_s()const {return trainSeqLength_s;}
    int getNUnitsAcc() const {return nUnitsAcc;}
    int getNUnitsS1() const {return nUnitsS1;}
    std::vector<int> getEnUnits() const {return enUnits;}
    std::vector<int> getEnabledACCChannels() const {return enabledACCChannels;}
    std::vector<int> getEnabledS1Channels() const {return enabledS1Channels;}
    std::vector<int> getEnabledACCUnits() const {return enabledACCUnits;}
    std::vector<int> getEnabledS1Units() const {return enabledS1Units;}
    bool isEnabledChannel(int ch) const {return isEnChannels.at(ch);}
    bool isEnabledUnit(int ch,int u) const {return (isEnChannels.at(ch) && enUnits.at(ch)>=u);}

    void setNMaxChannel(int nMaxChannel_) {nMaxChannel = nMaxChannel_;}
    void setNMaxUnit(int nMaxUnit_){nMaxUnit = nMaxUnit_;}
    void setMaxIter(int maxIter_){maxIter = maxIter_;}
    void setMaxCpuTime_s(int maxCpuTime_s_){ maxCpuTime_s=maxCpuTime_s_;}
    void setPainThresh(float painThresh_){ painThresh=painThresh_;}
    void setPainThreshAcc(float painThreshAcc_){ painThreshAcc=painThreshAcc_;}
    void setPainThreshS1(float painThreshS1_){ painThreshS1=painThreshS1_;}
    void setNStd(float nStd_){ nStd=nStd_;}
    void setBaseStartBefore_s(int baseStartBefore_s_) { baseStartBefore_s = baseStartBefore_s_;}
    void setBaseEndBefore_s(int baseEndBefore_s_) { baseEndBefore_s = baseEndBefore_s_;}
    void setTrainSeqLength_s(int trainSeqLength_s_) { trainSeqLength_s = trainSeqLength_s_;}
    void setEnUnits(std::vector<int>& enUnits_){enUnits = enUnits_;}
    void setEnabledACCChannels(std::vector<int> &enabledACCChannels_) { enabledACCChannels = enabledACCChannels_;}
    void setEnabledS1Channels(std::vector<int> &enabledS1Channels_) { enabledS1Channels = enabledS1Channels_;}
    void setEnabledACCUnits(std::vector<int> &enabledACCUnits_) { enabledACCUnits = enabledACCUnits_;}
    void setEnabledS1Units(std::vector<int> &enabledS1Units_) { enabledS1Units = enabledS1Units_;}
    void setUnitNum(int ch,int u);
    void setChannelStatus(int ch,bool status);

};

#endif
