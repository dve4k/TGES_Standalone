#include "stdafx.h"
#include "NetworkInterface.h"
//#pragma optimize("", off)

//PROTOTYPES
unsigned __stdcall		acceptEm1DataConnection(void* args);
unsigned __stdcall		recieveThread(void* args);
unsigned __stdcall		sendingThread(void* args);
bool					getSendData(DataPacket &packet);
void					putRcvData(EM1Command data);
void					packetToChar(char* buff, DataPacket packet);
void					setupEm1CommandList();
void					parsePrepareToSend(EM1Command &cmd);
void					resetSendRecBuffers();

//GLOBALS
//ARE WE SENDING DATA?
bool sendingData;

//SEND AND RECIEVE QUEUE / CONDITIONAL VARIABLE
std::queue<DataPacket> sendQue;
std::queue<EM1Command> recieveQue;
std::condition_variable sendQue_condVar;
std::condition_variable recieveQue_condVar;

//EM1 COMMAND INFO
EM1Command em1Commands[10];

//MUTEX
std::mutex toSendMutex;
std::mutex toRecvMutex;

//DO WE CURRENTLY HAVE A NETWORK CONNECTION?
bool socketConnected;

//CONSTRUCTOR
NetworkInterface::NetworkInterface()
{
	setupEm1CommandList();
	resetSendRecBuffers();
	socketConnected = false;
	sendingData = false;
}

//CONSTRUCTOR
NetworkInterface::~NetworkInterface()
{
}

//FUNCTION FOR SETTING UP THE NEWLY EST. EM1 SOCKET CONNECTION
unsigned __stdcall acceptEm1DataConnection(void* args)
{
	int iResult;
	//SPIN UP THREADS TO SEND / RECIEVE DATA FROM THE 'CIRCULAR BUFFER'
	unsigned sendThreadId;
	unsigned recieveThreadId;

	sendingData = true;

	//SPIN A RECIEVE THREAD
	HANDLE recHandle = (HANDLE)_beginthreadex(NULL, 0, &recieveThread, &*(SOCKET*)args, NULL, &recieveThreadId);
	//SPIN A SENDING THREAD
	HANDLE sendHandle = (HANDLE)_beginthreadex(NULL, 0, &sendingThread, &*(SOCKET*)args, NULL, &sendThreadId);

	WaitForSingleObject(recHandle, INFINITE);
	sendingData = false;
	CloseHandle(recHandle);
	CloseHandle(sendHandle);

	// shutdown the connection since we're done
	iResult = shutdown(*(SOCKET*)args, SD_SEND);

	// cleanup
	closesocket(*(SOCKET*)args);

	//CLIENT HAS DISCONNECTED -- PREP FOR A NEW CLIENT
	resetSendRecBuffers();
	return 1;
}

//NEAR - INFINITE WHILE LOOP FOR RECIEVING DATA TO EM1
unsigned __stdcall recieveThread(void* args)
{
	char recvBuf[1024];
	int recLen = 1024;
	int recRet = 0;

	socketConnected = true;
	bool killRec = false;

	do
	{
		recRet = recv(*(SOCKET*)args, recvBuf, recLen, 0);
		//BLOCKS HERE UNTIL SOMETHING Is RECIEVED
		if (recRet > 0) //0 = THE THREAD HAS ENDED
		{
			//DETERMINE THE PACKET COMMAND
			int cmd = -99;
			for (int i = 0; i < 10; i++)
			{
				if (recvBuf[0] == em1Commands[i].b1 && recvBuf[1] == em1Commands[i].b2)
				{
					EM1Command aCommand;
					aCommand.commandName = i;
					aCommand.b1 = em1Commands[i].b1;
					aCommand.b2 = em1Commands[i].b2;
					//IF ITS A PUT COMMAND, PARSE OUT THE DATABREAK#
					if (aCommand.commandName == PREPARESEND_CMD || aCommand.commandName == PREPARESENDASC_CMD || aCommand.commandName == PREPARESENDDSC_CMD)
					{
						parsePrepareToSend(aCommand);
					}
					putRcvData(aCommand);
					if (aCommand.commandName == STOP_CMD)
					{
						std::unique_lock<std::mutex> l(toSendMutex);
						int queSize = sendQue.size();
						l.unlock();
						while (queSize > 0)
						{
							Sleep(1);
							l.lock();
							queSize = sendQue.size();
							l.unlock();
						}
						killRec = true;
					}
					i = 100;
				}
			}
		}
	} while (recRet > 0 && !killRec);

	socketConnected = false;
	sendingData = false;
	return 1;
}

//INFINITE WHILE LOOP FOR SENDING DATA TO EM1
unsigned __stdcall sendingThread(void* args)
{
	//SEND DATA PACKET
	int cnt = 1;
	while (sendingData)
	{
		DataPacket packet;
		bool gotData = getSendData(packet);
		if (gotData)
		{
			if (packet.hasData)
			{
				//WE HAVE DATA TO SEND
				char dataToSend[42];
				packetToChar(dataToSend, packet);
				send(*(SOCKET*)args, dataToSend, 42, 0);
			}
			if (packet.sendAckInstead)
			{
				//WE HAVE DATA TO SEND
				int resp = send(*(SOCKET*)args, em1Commands[ACK_CMD].data, 42, 0);
				if (resp == -1)
				{
					int weHaveError = WSAGetLastError();
					int errorWas = weHaveError;
				}
			}
		}
	}
	return 1;
}

//PUT DATA PACKET INTO PROPERLY FORMATED CHAR* FOR SENDING
void packetToChar(char* buff, DataPacket packet)
{
	//DATA BREAK
	buff[0] = (packet.dataBreak >> 0) & 0xFF;
	buff[1] = (packet.dataBreak >> 8) & 0xFF;
	buff[2] = (packet.dataBreak >> 16) & 0xFF;
	buff[3] = (packet.dataBreak >> 24) & 0xFF;

	//PACKET ID
	buff[4] = (packet.packetId >> 0) & 0xFF;
	buff[5] = (packet.packetId >> 8) & 0xFF;

	//CH 0 - SET 1 / LEFT / UNFILTERED
	buff[6] = (packet.HGL1U >> 0) & 0xFF;
	buff[7] = (packet.HGL1U >> 8) & 0xFF;

	//CH 1 - SET 1 / LEFT / FILTERED
	buff[8] = (packet.HGL1F >> 0) & 0xFF;
	buff[9] = (packet.HGL1F >> 8) & 0xFF;

	//CH 2 - SET 1 / RIGHT / UNFILTERED
	buff[10] = (packet.HGR1U >> 0) & 0xFF;
	buff[11] = (packet.HGR1U >> 8) & 0xFF;

	//CH 3 - SET 1 / RIGHT / FILTERED
	buff[12] = (packet.HGR1F >> 0) & 0xFF;
	buff[13] = (packet.HGR1F >> 8) & 0xFF;

	//CH 4 - SPARE 1
	buff[14] = (packet.GAU1U >> 0) & 0xFF;
	buff[15] = (packet.GAU1U >> 8) & 0xFF;

	//CH 5 - SET 1 / FULL GAGE / FILTERED
	buff[16] = (packet.GAU1F >> 0) & 0xFF;
	buff[17] = (packet.GAU1F >> 8) & 0xFF;

	//CH 6 - SPARE 2
	buff[18] = (packet.MGAU1 >> 0) & 0xFF;
	buff[19] = (packet.MGAU1 >> 8) & 0xFF;

	//CH 7 - SPARE 3
	buff[20] = (packet.MGAF1 >> 0) & 0xFF;
	buff[21] = (packet.MGAF1 >> 8) & 0xFF;

	//CH 8 - SPARE 4
	buff[22] = (packet.HL1U >> 0) & 0xFF;
	buff[23] = (packet.HL1U >> 8) & 0xFF;

	//CH 9 -SPARE 5
	buff[24] = (packet.HL1F >> 0) & 0xFF;
	buff[25] = (packet.HL1F >> 8) & 0xFF;

	//CH 10 - SPARE 6
	buff[26] = (packet.HR1U >> 0) & 0xFF;
	buff[27] = (packet.HR1U >> 8) & 0xFF;

	//CH 11 - SPARE 7
	buff[28] = (packet.HR1F >> 0) & 0xFF;
	buff[29] = (packet.HR1F >> 8) & 0xFF;

	//CH 12 - SET 2 / LEFT / UNFILTERED
	buff[30] = (packet.HGL2U >> 0) & 0xFF;
	buff[31] = (packet.HGL2U >> 8) & 0xFF;

	//CH 13 - SET 2 / LEFT / FILTERED
	buff[32] = (packet.HGL2F >> 0) & 0xFF;
	buff[33] = (packet.HGL2F >> 8) & 0xFF;

	//CH 14 - SET 2 / RIGHT / UNFILTERED
	buff[34] = (packet.HGR2U >> 0) & 0xFF;
	buff[35] = (packet.HGR2U >> 8) & 0xFF;

	//CH 15 - SET 2 / RIGHT / FILTERED
	buff[36] = (packet.HGR2F >> 0) & 0xFF;
	buff[37] = (packet.HGR2F >> 8) & 0xFF;

	//CH 16 - SPARE 8
	buff[38] = (packet.GAU2F >> 0) & 0xFF;
	buff[39] = (packet.GAU2F >> 8) & 0xFF;

	//CH 17 - SET 2 / FULL GAGE / FILTERED
	buff[40] = (packet.SPARE >> 0) & 0xFF;
	buff[41] = (packet.SPARE >> 8) & 0xFF;

}

//SETUP STRUCTURE WITH COMMAND INFO
void setupEm1CommandList()
{
	//FORWARD
	em1Commands[FORWARD_CMD].b1 = 0x01;
	em1Commands[FORWARD_CMD].b2 = 0x46;
	em1Commands[FORWARD_CMD].length = 2;

	//REVERSE
	em1Commands[REVERSE_CMD].b1 = 0x01;
	em1Commands[REVERSE_CMD].b2 = 0x52;
	em1Commands[REVERSE_CMD].length = 2;

	//FLUSH BUFFER
	em1Commands[FLUSHBUFFER_CMD].b1 = 0x01;
	em1Commands[FLUSHBUFFER_CMD].b2 = 0x42;
	em1Commands[FLUSHBUFFER_CMD].length = 2;
	
	//SETUP
	em1Commands[SETUP_CMD].b1 = 0x02;
	em1Commands[SETUP_CMD].b2 = 0x63;
	em1Commands[SETUP_CMD].length = 2;

	//PREPARE TO SEND
	em1Commands[PREPARESEND_CMD].b1 = 0x03;
	em1Commands[PREPARESEND_CMD].b2 = 0x01;
	em1Commands[PREPARESEND_CMD].length = 6;

	//PREPARE TO SEND ASC
	em1Commands[PREPARESENDASC_CMD].b1 = 0x03;
	em1Commands[PREPARESENDASC_CMD].b2 = 0x01;
	em1Commands[PREPARESENDASC_CMD].length = 6;

	//PREPARE TO SEND DSC
	em1Commands[PREPARESENDDSC_CMD].b1 = 0x03;
	em1Commands[PREPARESENDDSC_CMD].b2 = 0x0B;
	em1Commands[PREPARESENDDSC_CMD].length = 6;

	//STOP SENDING
	em1Commands[STOP_CMD].b1 = 0x01;
	em1Commands[STOP_CMD].b2 = 0x0A;
	em1Commands[STOP_CMD].length = 2;

	//SYNC
	em1Commands[SYNC_CMD].b1 = 0x01;
	em1Commands[SYNC_CMD].b2 = 0x53;
	em1Commands[SYNC_CMD].length = 2;

	//ACK
	em1Commands[ACK_CMD].b1 = 0x80;
	em1Commands[ACK_CMD].b2 = 0x00;
	em1Commands[ACK_CMD].length = 42;
	for (int i = 0; i < 42; i++)
	{
		switch (i)
		{
		case 3:
			em1Commands[ACK_CMD].data[i] = 0x80;
			break;
		case 4:
			em1Commands[ACK_CMD].data[i] = 0x01;
			break;
		default:
			em1Commands[ACK_CMD].data[i] = 0x00;
			break;
		}
	}
}

//RESET OBJECTS IN SEND / RECIEVE BUFFERS
void resetSendRecBuffers()
{
	toSendMutex.lock();
	toRecvMutex.lock();

	for (int i = 0; i < sendQue.size(); i++)
	{
		sendQue.pop();
	}

	for (int i = 0; i < recieveQue.size(); i++)
	{
		recieveQue.pop();
	}

	toSendMutex.unlock();
	toRecvMutex.unlock();
}

//GET DATA FROM SEND QUE
bool getSendData(DataPacket &packet)
{
	DataPacket toReturn;
	std::unique_lock<std::mutex> l(toSendMutex);
	sendQue_condVar.wait_for(l, std::chrono::milliseconds(100), [] {return sendQue.size() > 0; });
	if (sendQue.size() > 10)
	{
		int problem = 9;
	}
	if (sendQue.size() > 0)
	{
		packet = sendQue.front();
		sendQue.pop();
		l.unlock();
		return true;
	}
	l.unlock();
	return false;
}

//PUT DATA INTO SEND QUE
void NetworkInterface::putSendData(DataPacket packet)
{
	toSendMutex.lock();
	sendQue.push(packet);
	toSendMutex.unlock();
	sendQue_condVar.notify_one();
}

//GET DATA FROM RECIEVE QUE
bool NetworkInterface::getRcvData(EM1Command &command)
{
	EM1Command toReturn;
	std::unique_lock<std::mutex> l(toRecvMutex);
	recieveQue_condVar.wait_for(l, std::chrono::milliseconds(100), [] {return recieveQue.size() > 0; });
	if (recieveQue.size() > 0)
	{
		command = recieveQue.front();
		recieveQue.pop();
		l.unlock();
		return true;
	}
	l.unlock();
	return false;
}

//PUT DATA INTO RECV BUFFER
void putRcvData(EM1Command data)
{
	toRecvMutex.lock();
	recieveQue.push(data);
	toRecvMutex.unlock();
	recieveQue_condVar.notify_one();
}

//PARSE PREPARE TO SEND DATA BREAK#
void parsePrepareToSend(EM1Command &cmd)
{
	long toReturn = 0x00000000;
	toReturn = toReturn | cmd.data[2];
	toReturn = toReturn | (cmd.data[3] << 8);
	toReturn = toReturn | (cmd.data[4] << 16);
	toReturn = toReturn | (cmd.data[5] << 24);
	cmd.dataBreak = toReturn;
}

//FUNCTION WITH INFINITE CONTROLLER LOOP
void NetworkInterface::controllerThread()
{
	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket;
	ClientSocket = INVALID_SOCKET;
	bool clientSocketIndex = false;
	unsigned em1AcceptThreadID;
	HANDLE em1AcceptHandle;
	em1AcceptHandle = NULL;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);

	while (true)
	{
		ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);

		iResult = listen(ListenSocket, SOMAXCONN);
		// Accept a client socket
		ClientSocket = accept(ListenSocket, NULL, NULL);

		if (em1AcceptHandle != NULL)
		{
			CloseHandle(em1AcceptHandle);
		}
		em1AcceptHandle = (HANDLE)_beginthreadex(NULL, 0, &acceptEm1DataConnection, &ClientSocket, NULL, &em1AcceptThreadID);

		clientSocketIndex = !clientSocketIndex;
		// No longer need server socket
		closesocket(ListenSocket);
	}

	WSACleanup();
}

//SEE IF WE HAVE A NETWORK SOCKET CONNECTION
bool NetworkInterface::checkSocketConnection()
{
	return socketConnected;
}

//PLACE CORE GEOMETRY CALCULATION PACKET DATA INTO PLASSER DATAPACKET
DataPacket NetworkInterface::packPlasserDataWithCGCalc(CoreGeomCalculationPacket gPacket, long dataBreakNumber)
{
	DataPacket dPacket;
	dPacket.HGL1U = gPacket.L_VERT_MCO_31_OC * 256;	//CH 0
	dPacket.HGL1F = gPacket.L_VERT_MCO_62_OC * 256;	//CH 1
	dPacket.HGR1U = gPacket.R_VERT_MCO_31_OC * 256;	//CH 2
	dPacket.HGR1F = gPacket.R_VERT_MCO_62_OC * 256; //CH 3
	if (gPacket.fullGage_filtered < 59.0 && gPacket.fullGage_filtered > 55.0)
	{
		dPacket.GAU1U = (gPacket.fullGage_filtered - 56.5) * 256; //CH 4
	}
	else
	{
		dPacket.GAU1U = 0x8000;
	}

	dPacket.GAU1F = gPacket.filteredCurvature * 256; //CH 5
	dPacket.MGAU1 = gPacket.grade_raw * 256;	//CH 6
	dPacket.MGAF1 = gPacket.crosslevel_OC * 256; //CH 7
	dPacket.HL1U = gPacket.L_VERT_MCO_11_OC * 256; //CH 8
	dPacket.HL1F = gPacket.L_HORIZ_MCO_62_NOC * 256; //CH 9
	dPacket.HR1U = gPacket.R_VERT_MCO_11_OC * 256; //CH 10
	dPacket.HR1F = gPacket.R_HORIZ_MCO_62_NOC * 256; //CH11
	dPacket.HGL2U = gPacket.lVertGage * 256; //CH 12
	dPacket.HGL2F = gPacket.lHalfGage * 256; //CH 13
	dPacket.HGR2U = gPacket.rVertGage * 256; //CH 14
	dPacket.HGR2F = gPacket.rHalfGage * 256; //CH 15
	dPacket.GAU2F = gPacket.L_VERT_MCO_62_NOC * 256; //CH 16
	dPacket.SPARE = gPacket.L_VERT_MCO_31_NOC * 256; //CH 17

	dPacket.dataBreak = dataBreakNumber;
	dPacket.hasData = true;
	return dPacket;
}
