#include <windows.h>
#include <winuser.h>
#include <process.h>
#include <iostream>
#include "PlexDO.h"
#include "PlexDOController.h"

DOStatus doStatus;

void initPlexDO()
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
		exit(-1);
	}
	doStatus.deviceNum = deviceNumbers[0];
	// set line mode for GPCTR1 to clock generation
	printf("setting line mode for line %d to clock generation\n", doStatus.OptoLineChn);
	if (PL_DOSetLineMode(doStatus.deviceNum, doStatus.OptoLineChn, CLOCK_GEN))
	{
		printf("PL_DOSetLineMode failed!\n");
		exit(-1);
	}
	// clock waveform: 100 usec high, 100 usec low = 5 kHz frequency
	//printf("setting pulse period for line %d to %d ms\n", doStatus.OptoLineChn, doStatus.curOptoPulsePeriodMs);
	//if (PL_DOSetClockParams(doStatus.deviceNum, doStatus.OptoLineChn, doStatus.curOptoPulsePeriodMs * 1000 / 2, doStatus.curOptoPulsePeriodMs * 1000 / 2))
	//{
	//	printf("PL_DOSetClockParams failed!\n");
	//	exit(-1);
	//}
	return;
}

void sendICCaptureEventPulse(int channel, int sleepms)
{
	//printf("turning bit %d on\n", 1);
	if (PL_DOSetBit(1, channel))
	{
		printf("PL_DOSetBit failed!\n");
	}
	PL_Sleep(sleepms);
	//printf("turning bit %d off\n", 1);
	if (PL_DOClearBit(1, channel))
	{
		printf("PL_DOClearBit failed!\n");
		exit(-1);
	}
}

void generateRandomFreqPulse(int pulseUnitLength_ms, int pulseChannel, int eventChannel)
{
	int PulseUnitCnt = 0;
	int Cnt = 0;
	int hlFlag = 0; // 1 - high, 0 - low
	int halfPeriod = 0;
	double cpu_time_used;
	int factor = 1;

	while (1)
	{
		if (0 == PulseUnitCnt)
		{
			//halfPeriod = 100*(rand() % (10)+1) / 2;
			halfPeriod = 100 / 2 * factor;
			factor = rand() % 10 +1;
			//printf("halfPeriod = %d\n", halfPeriod);
			//factor = 1 - factor;
		}

		if (0 == Cnt && 0 == hlFlag)
		{
			//printf("turning bit %d on\n", 12);
			if (PL_DOSetBit(1, pulseChannel))
			{
				//printf("PL_DOSetBit failed!\n");
				exit(-1);
			}
			if (PL_DOSetBit(1, eventChannel))
			{
				//printf("PL_DOSetBit failed!\n");
				exit(-1);
			}
			hlFlag = 1 - hlFlag;
			Cnt++;
		}
		else if (0 == Cnt && 1 == hlFlag)
		{
			//printf("turning bit %d off\n", 12);
			if (PL_DOClearBit(1, pulseChannel))
			{
				//printf("PL_DOClearBit failed!\n");
				exit(-1);
			}
			if (PL_DOClearBit(1, eventChannel))
			{
				//printf("PL_DOClearBit failed!\n");
				exit(-1);
			}
			hlFlag = 1 - hlFlag;
			Cnt++;
		}
		else
		{
			Cnt++;
		}

		PulseUnitCnt++;

		if (Cnt >= halfPeriod)
		{
			Cnt = 0;
		}

		if (PulseUnitCnt >= pulseUnitLength_ms && 0 == Cnt && 0 == hlFlag)
		{
			PulseUnitCnt = 0;
			Cnt = 0;
			hlFlag = 0;
		}

		Sleep(1);
	}
	return;
}

void KBListener()
{
	enum{ HLON_KEYID = 1, LLON_KEYID = 2, HLWARM_KEYID = 3, LLWARM_KEYID = 4, STOPLASER_KEYID = 5, STARTOPTO_KEYID = 6,};
	if (RegisterHotKey(NULL, HLON_KEYID, MOD_ALT | MOD_NOREPEAT, doStatus.HPLaserOnKey))  // High Power Laser On
	{
		std::cout << "Hotkey 'ALT+" << doStatus.HPLaserOnKey << "' registered, for High Power Laser On" << std::endl;
	}

	if (RegisterHotKey(NULL, LLON_KEYID, MOD_ALT | MOD_NOREPEAT, doStatus.LPLaserOnKey))  // Low Power Laser On
	{
		std::cout << "Hotkey 'ALT+" << doStatus.LPLaserOnKey << "' registered, for Low Power Laser On" << std::endl;
	}

	if (RegisterHotKey(NULL, HLWARM_KEYID, MOD_ALT | MOD_NOREPEAT, doStatus.HPLaserWarmKey)) // High Power Laser Warm
	{
		std::cout << "Hotkey 'ALT+" << doStatus.HPLaserWarmKey << "' registered, for High Power Laser Warm" << std::endl;
	}

	if (RegisterHotKey(NULL, LLWARM_KEYID, MOD_ALT | MOD_NOREPEAT, doStatus.LPLaserWarmKey)) // Low Power Laser Warm
	{
		std::cout << "Hotkey 'ALT+" << doStatus.LPLaserWarmKey << "' registered, for Low Power Laser Warm" << std::endl;
	}

	if (RegisterHotKey(NULL, STOPLASER_KEYID, MOD_ALT | MOD_NOREPEAT, doStatus.stopLaserKey))  //Stop Laser
	{
		std::cout << "Hotkey 'ALT+" << doStatus.stopLaserKey << "' registered, for Stop Laser" << std::endl;
	}

	if (RegisterHotKey(NULL, STARTOPTO_KEYID, MOD_ALT | MOD_NOREPEAT, doStatus.startOptoKey))  //Stop Laser
	{
		std::cout << "Hotkey 'ALT+" << doStatus.startOptoKey << "' registered, for start opto" << std::endl;
	}

	MSG msg = { 0 };
	while (GetMessage(&msg, NULL, 0, 0) != 0)
	{
		if (msg.message == WM_HOTKEY)
		{
			switch (msg.wParam)
			{
			case HLON_KEYID:
				printf("WM_HOTKEY  High Power Laser On received\n");
				sendLaserOn(HighPowerLaserChannel, HighPowerLaserEventChannel, LaserOnStimuliTimeMs);
				break;
			case LLON_KEYID:
				printf("WM_HOTKEY Low Power Laser On received\n");
				sendLaserOn(LowPowerLaserChannel, LowPowerLaserEventChannel, LaserOnStimuliTimeMs);
				break;
			case HLWARM_KEYID:
				printf("WM_HOTKEY High Power Laser Warm received\n");
				sendLaserOn(HighPowerLaserChannel, NoEvent, LaserOnWarmTimeMs);
				break;
			case LLWARM_KEYID:
				printf("WM_HOTKEY Low Power Laser Warm received\n");
				sendLaserOn(LowPowerLaserChannel, NoEvent, LaserOnWarmTimeMs);
				break;
			case STOPLASER_KEYID:
				printf("WM_HOTKEY Stop Laser received\n");
				stopLaserOn();
				break;
			case STARTOPTO_KEYID:
				printf("WM_HOTKEY Start Opto received\n");
				sendOptoOn(OptoPulseHighMs, OptoPulseLowMs, OptoPulseDurationMs);
				break;
			default:
				break;
			}
		}
		Sleep(1);
	}

	return;
}

void stopLaserOn()
{
	if (doStatus.laserOn)
	{
		doStatus.laserOn = false;
	}
}

void sendLaserOn(int channel, int eventChannel, int ms)
{
	if (!doStatus.laserOn)
	{
		doStatus.curChn = channel;
		doStatus.curEvChn = eventChannel;
		doStatus.curDurTimeMs = ms;
		doStatus.laserOn = true;
	}
}
void sendOptoOn(int highMs, int lowMs, int durationMs)
{
	doStatus.curOptoPulseHighMs = highMs;
	doStatus.curOptoPulseLowMs = lowMs;
	doStatus.curOptoDurationMs = durationMs;
	doStatus.optoOn = true;
}
void PlexOptoThread(void* p)
{
	while (1)
	{
		if (doStatus.optoOn)
		{
			//// set line mode for GPCTR1 to clock generation
			//printf("setting line mode for line %d to clock generation\n", doStatus.OptoLineChn);
			//if (PL_DOSetLineMode(doStatus.deviceNum, doStatus.OptoLineChn, PULSE_GEN))
			//{
			//	printf("PL_DOSetLineMode failed!\n");
			//	exit(-1);
			//}
			//if (PL_DOSetLineMode(doStatus.deviceNum, doStatus.OptoLineChn, CLOCK_GEN))
			//{
			//	printf("PL_DOSetLineMode failed!\n");
			//	exit(-1);
			//}

			// clock waveform: 100 usec high, 100 usec low = 5 kHz frequency
			printf("setting pulse for line %d to high=%d ms, low = %d ms\n", doStatus.OptoLineChn, doStatus.curOptoPulseHighMs, doStatus.curOptoPulseLowMs);
			if (int n = PL_DOSetClockParams(doStatus.deviceNum, doStatus.OptoLineChn, doStatus.curOptoPulseHighMs * 1000, doStatus.curOptoPulseLowMs * 1000))
			{
				printf("PL_DOSetClockParams failed!\n");
				exit(-1);
			}

			// start the clock 
			printf("starting clock on line %d\n", doStatus.OptoLineChn);
			if (PL_DOStartClock(doStatus.deviceNum, doStatus.OptoLineChn))
			{
				printf("PL_DOStartClock failed!\n");
				exit(-1);
			}
			//send opto start pulse
			if (PL_DOSetBit(1, doStatus.OptoStEvChn))
			{
				printf("PL_DOSetBit opto start failed!\n");
			}
			printf("%f Hz clock is now running on line %d\n", 1000.0 / (doStatus.curOptoPulseHighMs + doStatus.curOptoPulseLowMs), doStatus.OptoLineChn);
			int cnt = doStatus.curOptoDurationMs;
			while (cnt > 0 && doStatus.optoOn)
			{
				PL_Sleep(min(1000, cnt));
				cnt -= 1000;
				if (cnt > 0)
				{
					std::cout << cnt / 1000 << std::endl;
				}
			}
			// stop the clock
			printf("stopping clock on line %d\n", doStatus.OptoLineChn);
			if (PL_DOStopClock(doStatus.deviceNum, doStatus.OptoLineChn))
			{
				printf("PL_DOStopClock failed!\n");
				exit(-1);
			}
			if (PL_DOClearBit(1, doStatus.OptoStEvChn))
			{
				printf("PL_DOClearBit opto start failed!\n");
			}
			doStatus.optoOn = false;
		}
		//Sleep(doStatus.curOptoDurationMs);
	}
	Sleep(1);
}
void PlexLaserThread(void* p)
{
	while (1)
	{
		if (doStatus.laserOn)
		{
			// printf("turning bit %d on\n", 1);
			if (PL_DOSetBit(1, doStatus.curChn))
			{
				printf("PL_DOSetBit laser failed!\n");
			}
			if (doStatus.curEvChn > 0)
			{
				if (PL_DOSetBit(1, doStatus.curEvChn))
				{
					printf("PL_DOSetBit event failed!\n");
				}
			}

			int cnt = doStatus.curDurTimeMs;
			while (cnt > 0 && doStatus.laserOn)
			{
				PL_Sleep(min(1000, cnt));
				cnt -= 1000;
				if (cnt > 0)
				{
					std::cout << cnt / 1000 << std::endl;
				}
			}
			//printf("turning bit %d off\n", 1);
			if (PL_DOClearBit(1, doStatus.curChn))
			{
				printf("PL_DOClearBit failed!\n");
				exit(-1);
			}

			if (doStatus.curEvChn > 0)
			{
				if (PL_DOClearBit(1, doStatus.curEvChn))
				{
					printf("PL_DOClearBit event failed!\n");
					exit(-1);
				}
			}
			doStatus.laserOn = false;
		}
		Sleep(100);
	}
}
