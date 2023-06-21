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
struct NI_RECV_DATA
{
	long long FOOT_COUNT;
	int DIRECTION_OF_TRAVEL;
	int OUT_FOOT_ENABLE;
	int SPEED_CNT;
	int RELAY_0;
	int RELAY_1;
	int RELAY_2;
	int RELAY_3;
	int RELAY_4;
	int RELAY_5;
	int RELAY_6;
	int RELAY_7;
	int SCALE_FACTOR;
	int MEASURING_DIRECTION;
	int SCANNER_L_CNT;
	int SCANNER_R_CNT;
	int SIM_ENABLE;
	int SIM_SPEED;
	double APPLANIX_DMI;
	double APPLANIX_TIME1;
	double APPLANIX_LAT;
	double APPLANIX_LON;
	double APPLANIX_ALT;
	double APPLANIX_SPEED;
	double APPLANIX_HEADING;
	double APPLANIX_GROUND_TRACK;
};

#pragma pack(1)
struct NI_SEND_DATA
{
	int FOOT_ENABLE;
	int SCALE_FACTOR;
	int RELAY_0;
	int RELAY_1;
	int RELAY_2;
	int RELAY_3;
	int RELAY_4;
	int RELAY_5;
	int RELAY_6;
	int RELAY_7;
	int RESET_FT;
	int MEASURE_DIRECTION;
	int SIM_ENABLE;
	int SIM_SPEED;
};

class NI_UDP_Interface
{
public:
	NI_UDP_Interface();
	~NI_UDP_Interface();
	int setupNiUdpSend();
	int setupNiUdpRecv();
	bool ni_udp_putDataToSend(NI_SEND_DATA dataToSend);
	//bool ni_udp_getRecvData(NI_RECV_DATA& data);
	bool shutdownNiUdpSend();
	bool shutdownNiUdpRecv();
	NI_SEND_DATA getRecentSendDataNI();
	NI_RECV_DATA getRecentRecvDataNI();
};


