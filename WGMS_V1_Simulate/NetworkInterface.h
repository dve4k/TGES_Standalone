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

#pragma pack(2)

#define WIN32_LEAN_AND_MEAN
#pragma comment (lib, "Ws2_32.lib")
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "5517"

//CIRCULAR BUFFER LENGTHS
#define CIRCRECIEVEBUFFERLEN 100
#define CIRCSENDBUFFERLEN 100

//EM1 COMMANDS
#define FORWARD_CMD 0
#define REVERSE_CMD 1
#define FLUSHBUFFER_CMD 2
#define SETUP_CMD 3
#define PREPARESEND_CMD 4
#define PREPARESENDASC_CMD 5
#define PREPARESENDDSC_CMD 6
#define STOP_CMD 7
#define SYNC_CMD 8
#define ACK_CMD 9
#define DATA_CMD 10

struct EM1Command
{
	int length;
	char b1;
	char b2;
	char data[1024];
	long dataBreak;
	int commandName;
};
struct DataPacket
{
	//REMEBER THAT:
	// (1) VALUES ARE SCALED BY 256
	// (2) VALUES SHOULD BE SENT 
	long dataBreak;
	short packetId = 3;
	short s1_left_unfiltered = 0x8000;
	short s1_left_filtered;
	short s1_right_unfiltered = 0x8000;
	short s1_right_filtered;
	short s1_fullGage_Filtered;
	short s2_left_unfiltered = 0x8000;
	short s2_left_filtered = 0x8000;
	short s2_right_unfiltered = 0x8000;
	short s2_right_filtered = 0x8000;
	short s2_fullGage_Filtered = 0x8000;

	bool sendAckInstead = false;
	bool hasData = false;
};
class NetworkInterface
{
public:
	NetworkInterface();
	~NetworkInterface();
	void			putSendData(DataPacket packet);
	EM1Command		getRcvData();
	void			controllerThread();
	bool			checkSocketConnection();
};

