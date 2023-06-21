#pragma once
#include "stdafx.h"
#include <thread>
#include "windows.h"
#include <mutex>
#include "atlstr.h"
#include <cstdlib>
//TCP/IP SERVER
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "Novatel.h"
#include <queue>
#include "CoreGeometry.h"
#include <condition_variable>
#include "Settings.h"
#include <ctime>
#include <vector>
#include <iostream>
#include <fstream>
#include "TGC_Location.h"

#pragma pack(2)

#define WIN32_LEAN_AND_MEAN
#pragma comment (lib, "Ws2_32.lib")
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT0 "3001"
#define DEFAULT_PORT1 "3002"
#define DEFAULT_PORT2 "3003"
#define DEFAULT_PORT3 "3004"
#define DEFAULT_PORT4 "3005"
#define DEFAULT_PORT5 "3006"
#define DEFAULT_PORT6 "3007"


//CIRCULAR BUFFER LENGTHS
#define MAXRCVBUFFERLEN 200//1024
#define CIRCSENDBUFFERLEN 100
#define RECEIVEBUFFERSCOUNT 7

struct NovatelConCliLogInfo
{
	char data[DEFAULT_BUFLEN];
	int dataLength;
};


class NovatelConnectionClient
{
public:
	NovatelConnectionClient();
	~NovatelConnectionClient();
	void controllerThread();
	void testFunction();
	void getRcvData_CORRIMUDATAS(int recvBufferIndex, bool &gotData, CoreGeometry &coreGeom);
	void getRcvData_MARKPVA(int recvBufferIndex, bool &gotData, CoreGeometry &coreGeom, TGC_Location &location);
	void getRcvData_INSSPDS(int recvBufferIndex, bool &gotData, CoreGeometry &coreGeom);
	void getRcvData_INSPVAS(int recvBufferIndex, bool &gotData, CoreGeometry &coreGeom);
	void putSendData(NovatelSendCommandOb cmd);
	void sendSetupLogCommands();
	//void NovatelConnectionClient::getRcvData_BESTPOS(int recvBufferIndex, bool &gotData, CoreGeometry &coreGeom);

private:
	//GLOBALS



};
