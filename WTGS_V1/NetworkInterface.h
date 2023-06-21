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

#include <queue>
#include <condition_variable>

#include "CoreGeomCalculationPacket.h"

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
	short HGL1U = 0x8000;
	short HGL1F = 0x8000;
	short HGR1U = 0x8000;
	short HGR1F = 0x8000;
	short GAU1U = 0x8000;
	short GAU1F = 0x8000;
	short MGAU1 = 0x8000;
	short MGAF1 = 0x8000;
	short HL1U = 0x8000;
	short HL1F = 0x8000;
	short HR1U = 0x8000;
	short HR1F = 0x8000;
	short HGL2U = 0x8000;
	short HGL2F = 0x8000;
	short HGR2U = 0x8000;
	short HGR2F = 0x8000;
	short GAU2F = 0x8000;
	short SPARE = 0x8000;
	//short s1_left_unfiltered = 0x8000;
	//short s1_left_filtered = 0x8000;
	//short s1_right_unfiltered = 0x8000;
	//short s1_right_filtered = 0x8000;
	//short s1_fullGage_Filtered = 0x8000;
	//short s2_left_unfiltered = 0x8000;
	//short s2_left_filtered = 0x8000;
	//short s2_right_unfiltered = 0x8000;
	//short s2_right_filtered = 0x8000;
	//short s2_fullGage_Filtered = 0x8000;
	//short spare1 = 0x8000;
	//short spare2 = 0x8000;
	//short spare3 = 0x8000;
	//short spare4 = 0x8000;
	//short spare5 = 0x8000;
	//short spare6 = 0x8000;
	//short spare7 = 0x8000;
	//short spare8 = 0x8000;

	bool sendAckInstead = false;
	bool hasData = false;
};
class NetworkInterface
{
public:
	NetworkInterface();
	~NetworkInterface();
	void			putSendData(DataPacket packet);
	bool			getRcvData(EM1Command &command);
	void			controllerThread();
	bool			checkSocketConnection();
	DataPacket		packPlasserDataWithCGCalc(CoreGeomCalculationPacket gPacket, long dataBreakNumber);
};

