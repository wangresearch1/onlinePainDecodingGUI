#include "settings.h"
#include <algorithm>    // std::find
#include <vector>       // std::vector
void PdcOptions::setChannelStatus(int ch,bool state)
{
    isEnChannels.at(ch) = state;
    if (state)
    {
        if (isAccChannels.at(ch))
        {
            if (std::find(enabledACCChannels.begin(), enabledACCChannels.end(), ch) == enabledACCChannels.end())
            {
                enabledACCChannels.push_back(ch);
                enabledACCUnits.push_back(enUnits.at(ch));
            }
        }
        else
        {
            if (std::find(enabledS1Channels.begin(), enabledS1Channels.end(), ch) == enabledS1Channels.end())
            {
                enabledS1Channels.push_back(ch);
                enabledS1Units.push_back(enUnits.at(ch));
            }
        }

        std::cout<<"tetrode "<< ch << " checked"<<std::endl;
    }
    else
    {
        if (isAccChannels.at(ch))
        {
            if (std::find(enabledACCChannels.begin(), enabledACCChannels.end(), ch) != enabledACCChannels.end())
            {
                ptrdiff_t pos = std::find(enabledACCChannels.begin(), enabledACCChannels.end(), ch)-enabledACCChannels.begin();
                enabledACCChannels.erase(enabledACCChannels.begin()+pos);
                enabledACCUnits.erase(enabledACCUnits.begin()+pos);
            }
        }
        else
        {
            if (std::find(enabledS1Channels.begin(), enabledS1Channels.end(), ch) != enabledS1Channels.end())
            {
                ptrdiff_t pos = std::find(enabledS1Channels.begin(), enabledS1Channels.end(), ch)-enabledS1Channels.begin();
                enabledS1Channels.erase(enabledS1Channels.begin()+pos);
                enabledS1Units.erase(enabledS1Units.begin()+pos);
            }
        }

        std::cout<<"tetrode "<< ch << " unchecked"<<std::endl;
    }

    std::cout<<"enabled ACC tetrodes"<<std::endl;
    for (int i=0;i<enabledACCChannels.size();i++)
    {
        std::cout<<" "<< enabledACCChannels.at(i);
    }
    std::cout<<std::endl;
    std::cout<<"enabled ACC units"<<std::endl;
    for (int i=0;i<enabledACCUnits.size();i++)
    {
        std::cout<<" "<< enabledACCUnits.at(i);
    }
    std::cout<<std::endl;
    std::cout<<"enabled S1 tetrodes"<<std::endl;
    for (int i=0;i<enabledS1Channels.size();i++)
    {
        std::cout<<" "<< enabledS1Channels.at(i);
    }
    std::cout<<std::endl;
    std::cout<<"enabled S1 units"<<std::endl;
    for (int i=0;i<enabledS1Units.size();i++)
    {
        std::cout<<" "<< enabledS1Units.at(i);
    }
    std::cout<<std::endl;
}

void PdcOptions::setUnitNum(int ch,int u)
{
    if (u)
    {
        if (isAccChannels.at(ch))
        {
            if (std::find(enabledACCChannels.begin(), enabledACCChannels.end(), ch) != enabledACCChannels.end())
            {
                ptrdiff_t pos = std::find(enabledACCChannels.begin(), enabledACCChannels.end(), ch)-enabledACCChannels.begin();
                //enabledACCChannels.erase(enabledACCChannels.begin()+pos);
                *(enabledACCUnits.begin()+pos) = u;
            }
        }
        else
        {
            if (std::find(enabledS1Channels.begin(), enabledS1Channels.end(), ch) != enabledS1Channels.end())
            {
                ptrdiff_t pos = std::find(enabledS1Channels.begin(), enabledS1Channels.end(), ch)-enabledS1Channels.begin();
                //enabledS1Channels.erase(enabledS1Channels.begin()+pos);
                //enabledS1Units.erase(enabledS1Units.begin()+pos);
                *(enabledS1Units.begin()+pos) = u;
            }
        }
        isEnChannels.at(ch) = true;
        enUnits.at(ch) = u;//u>enUnits.at(ch)?u:enUnits.at(ch);
    }
    else
    {
        enUnits.at(ch) = u;
    }
    std::cout<< "set unit number of tetrode "<<ch<<" to"<<enUnits.at(ch)<<std::endl;
}
