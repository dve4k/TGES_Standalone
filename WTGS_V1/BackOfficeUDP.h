#pragma once
#include "stdafx.h"
#include <thread>
#include "windows.h"
#include "atlstr.h"
#include <cstdlib>
//TCP/IP SERVER
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <condition_variable>
#include <queue>
#include <mutex>

#pragma pack(1)
struct TGES_BACKOFFICE_PACKET
{
	int programState;
	int measureDirection;
	int directiontravel;
	INT64 dataBreakCount;
	INT64 gageCount;
	INT64 inertialCount;
	int g_i_difference;
	double estMilepost;
	int estTrack;
	int versionNumber;
	INT64 dataBreaksAdded;
	char dateTime[15];
	char estRID[6];
};

class BackOfficeUDP
{
public:
	BackOfficeUDP();
	int setupBackOfficeUdpSend();
	bool backOffice_udp_putDataToSend(TGES_BACKOFFICE_PACKET dataToSend);
	bool shutdownBackOfficeUdpSend();
};
