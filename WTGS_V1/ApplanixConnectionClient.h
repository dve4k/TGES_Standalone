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

//CORE GEOMETRY
#include <condition_variable>
#include <queue>
#include <mutex>
#include "Settings.h"
#include <ctime>
#include <vector>
#include <iostream>
#include <fstream>
#include "TGC_Location.h"

#include "Applanix.h"
#include "CoreGeometry_A.h"



#pragma pack(2)

#define WIN32_LEAN_AND_MEAN
#pragma comment (lib, "Ws2_32.lib")
#define DEFAULT_BUFLEN_AP 5012
#define UDP_DISPLAYPORT "5600"
#define TCP_LOGGING_DATAPORT "5603"
#define TCP_REALTIME_DATAPORT "5602"
#define TCP_CONTROLPORT "5601"
#define TCP_PACKET_BUFFER_LEN 200

//STRUCT FOR HOLDING RAW MESSAGING DATA TO STORE
struct RAW_APPLANIX_DATA
{
	char data[DEFAULT_BUFLEN_AP];
	int dataLength;
};

class ApplanixConnectionClient
{
public:
	ApplanixConnectionClient();
	~ApplanixConnectionClient();
	void connectApplanix(bool enableRawLogging);
	//bool getRcvData_Applanix_Group_1(bool &gotData, CoreGeometry_A &coreGeom);
	bool getRcvData_Applanix_Group_5(bool &gotData, CoreGeometry_A &coreGeom, TGC_Location &location);
	void shutdownApplanix();
	void putApplanixGageLoggingData(APPLANIX_GROUP_10304 data);
	bool enableApplanixGlobalGating();
	bool checkApplanixGlobalGating();
};
