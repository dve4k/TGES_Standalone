#include "stdafx.h"
#include "NetworkInterface.h"
//#pragma optimize("", off)

//PROTOTYPES
unsigned __stdcall		acceptEm1DataConnection(void* args);
unsigned __stdcall		recieveThread(void* args);
unsigned __stdcall		sendingThread(void* args);
DataPacket				getSendData();
void					putRcvData(EM1Command data);
void					packetToChar(char* buff, DataPacket packet);
void					setupEm1CommandList();
void					parsePrepareToSend(EM1Command &cmd);
void					resetSendRecBuffers();

//GLOBALS
//ARE WE SENDING DATA?
bool sendingData;
DataPacket toSendDataPackets[CIRCSENDBUFFERLEN];
EM1Command toRecieveCommands[CIRCRECIEVEBUFFERLEN];
int recvHead;
int recvTail;
int sendHead;
int sendTail;

//EM1 COMMAND INFO
EM1Command em1Commands[9];

//MUTEX
std::mutex toSendMutex;
std::mutex toRecvMutex;

//DO WE CURRENTLY HAVE A NETWORK CONNECTION?
bool socketConnected;

//CONSTRUCTOR
NetworkInterface::NetworkInterface()
{
	setupEm1CommandList();
	recvHead = 0;
	recvTail = 0;
	sendHead = 0;
	sendTail = 0;
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

	//SPIN A RECIEVE THREAD -- INFINITE LOOP
	HANDLE recHandle = (HANDLE)_beginthreadex(NULL, 0, &recieveThread, &*(SOCKET*)args, NULL, &recieveThreadId);
	//SPIN A SENDING THREAD -- INFINITE LOOP
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
		//BLOCKS HERE UNTIL SOMETHING IN RECIEVED
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
						while (sendHead > sendTail)
						{
							Sleep(1);
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
	while (sendingData) //WJE 11/2/2018 was 'true'
	{
		DataPacket packet = getSendData();
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
	buff[6] = (packet.s1_left_unfiltered >> 0) & 0xFF;
	buff[7] = (packet.s1_left_unfiltered >> 8) & 0xFF;
	//CH 1 - SET 1 / LEFT / FILTERED
	buff[8] = (packet.s1_left_filtered >> 0) & 0xFF;
	buff[9] = (packet.s1_left_filtered >> 8) & 0xFF;
	//CH 2 - SET 1 / RIGHT / UNFILTERED
	buff[10] = (packet.s1_right_unfiltered >> 0) & 0xFF;
	buff[11] = (packet.s1_right_unfiltered >> 8) & 0xFF;
	//CH 3 - SET 1 / RIGHT / FILTERED
	buff[12] = (packet.s1_right_filtered >> 0) & 0xFF;
	buff[13] = (packet.s1_right_filtered >> 8) & 0xFF;
	//CH 4 - SPARE
	buff[14] = 0x00;
	buff[15] = 0x80;
	//CH 5 - SET 1 / FULL GAGE / FILTERED
	buff[16] = (packet.s1_fullGage_Filtered >> 0) & 0xFF;
	buff[17] = (packet.s1_fullGage_Filtered >> 8) & 0xFF;
	//CH 6 - SPARE
	buff[18] = 0x00;
	buff[19] = 0x80;
	//CH 7 - SPARE
	buff[20] = 0x00;
	buff[21] = 0x80;
	//CH 8 - SPARE
	buff[22] = 0x00;
	buff[23] = 0x80;
	//CH 9 -SPARE
	buff[24] = 0x00;
	buff[25] = 0x80;
	//CH 10 - SPARE
	buff[26] = 0x00;
	buff[27] = 0x80;
	//CH 11 - SPARE
	buff[28] = 0x00;
	buff[29] = 0x80;
	//CH 12 - SET 2 / LEFT / UNFILTERED
	buff[30] = (packet.s2_left_unfiltered >> 0) & 0xFF;
	buff[31] = (packet.s2_left_unfiltered >> 8) & 0xFF;
	//CH 13 - SET 2 / LEFT / FILTERED
	buff[32] = (packet.s2_left_filtered >> 0) & 0xFF;
	buff[33] = (packet.s2_left_filtered >> 8) & 0xFF;
	//CH 14 - SET 2 / RIGHT / UNFILTERED
	buff[34] = (packet.s2_right_unfiltered >> 0) & 0xFF;
	buff[35] = (packet.s2_right_unfiltered >> 8) & 0xFF;
	//CH 15 - SET 2 / RIGHT / FILTERED
	buff[36] = (packet.s2_right_filtered >> 0) & 0xFF;
	buff[37] = (packet.s2_right_filtered >> 8) & 0xFF;
	//CH 16 - SPARE
	buff[38] = 0x00;
	buff[39] = 0x80;
	//CH 17 - SET 2 / FULL GAGE / FILTERED
	buff[40] = (packet.s2_fullGage_Filtered >> 0) & 0xFF;
	buff[41] = (packet.s2_fullGage_Filtered >> 8) & 0xFF;

	//////MANUAL ENTRY TEST PACKET -- COPIED FROM OLD NS33 DATA

	////packetBuff[0] = cnt & 0xFF;;
	////packetBuff[1] = (cnt & 0xFF00) >> 8;
	////packetBuff[2] = (cnt & 0xFF0000) >> 16;
	////packetBuff[3] = (cnt & 0xFF000000) >> 24;
	////packetBuff[4] = 0x03;
	////packetBuff[5] = 0x00;
	////packetBuff[6] = 0x00;
	////packetBuff[7] = 0x80;
	////packetBuff[8] = 0x39;
	////packetBuff[9] = 0x00;
	////packetBuff[10] = 0x61;
	////packetBuff[11] = 0x00;
	////packetBuff[12] = 0x5F;
	////packetBuff[13] = 0x00;
	////packetBuff[14] = 0x00;
	////packetBuff[15] = 0x80;
	////packetBuff[16] = 0x15;
	////packetBuff[17] = 0x00;
	////packetBuff[18] = 0x4C;
	////packetBuff[19] = 0x31;
	////packetBuff[20] = 0x00;
	////packetBuff[21] = 0x80;
	////packetBuff[22] = 0x00;
	////packetBuff[23] = 0x80;
	////packetBuff[24] = 0x00;
	////packetBuff[25] = 0x80;
	////packetBuff[26] = 0x00;
	////packetBuff[27] = 0x80;
	////packetBuff[28] = 0x00;
	////packetBuff[29] = 0x80;
	////packetBuff[30] = 0x00;
	////packetBuff[31] = 0x80;
	////packetBuff[32] = 0xE0;
	////packetBuff[33] = 0xFF;
	////packetBuff[34] = 0xC9;
	////packetBuff[35] = 0x00;
	////packetBuff[36] = 0xC7;
	////packetBuff[37] = 0x00;
	////packetBuff[38] = 0x00;
	////packetBuff[39] = 0x80;
	////packetBuff[40] = 0x36;
	////packetBuff[41] = 0x00;

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
	em1Commands[SETUP_CMD].length = 2;//????

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

	//ACK -- IM PRETTY SURE THAT THIS IS WRONG!!! :(
	//PLASSER IS STUPID!!!
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

	EM1Command tmpCmd;
	tmpCmd.commandName = -99;
	for (int i = 0; i < CIRCRECIEVEBUFFERLEN; i++)
	{
		toRecieveCommands[i] = tmpCmd;
	}

	for (int i = 0; i < CIRCSENDBUFFERLEN; i++)
	{
		toSendDataPackets[i].hasData = false;
		toSendDataPackets[i].sendAckInstead = false;
	}
	recvHead = 0;
	recvTail = 0;
	sendHead = 0;
	sendTail = 0;

	toSendMutex.unlock();
	toRecvMutex.unlock();
}

//GET DATA FROM SEND BUFFER
DataPacket getSendData()
{
	DataPacket toReturn;
	toReturn.hasData = false;
	toReturn.sendAckInstead = false;

	toSendMutex.lock();

	if (sendHead == sendTail)
	{
		//HEAD = TAIL IN BUFFER
		//(1) NO DATA TO READ
	}
	else
	{
		toReturn = toSendDataPackets[sendTail];
		if (sendTail < CIRCSENDBUFFERLEN - 1)
		{
			sendTail++;
		}
		else
		{
			sendTail = 0;
		}
	}

	toSendMutex.unlock();

	return toReturn;
}

//PUT DATA INTO SEND BUFFER
void NetworkInterface::putSendData(DataPacket packet)
{
	toSendMutex.lock();

	toSendDataPackets[sendHead] = packet;
	if (sendHead < CIRCSENDBUFFERLEN - 1)
	{
		sendHead++;
	}
	else
	{
		sendHead = 0;
	}

	if (sendHead == sendTail)
	{
		//OVERFLOW -- PROGRESS THE TAIL FORWARD ALSO
		if (sendHead < CIRCSENDBUFFERLEN - 1)
		{
			sendTail = sendHead + 1;
		}
		else
		{
			sendTail = 0;
		}
	}

	toSendMutex.unlock();
}

//GET DATA FROM RECV BUFFER
EM1Command NetworkInterface::getRcvData()
{
	EM1Command toReturn;
	toReturn.commandName = -99;

	toRecvMutex.lock();

	if (recvHead == recvTail)
	{
		//HEAD = TAIL IN BUFFER
		//(1) NO DATA IN BUFFER
	}else
	{
		toReturn = toRecieveCommands[recvTail];
		if (recvTail < CIRCRECIEVEBUFFERLEN - 1)
		{
			recvTail++;
		}
		else
		{
			recvTail = 0;
		}
	}

	toRecvMutex.unlock();

	return toReturn;
}

//PUT DATA INTO RECV BUFFER
void putRcvData(EM1Command data)
{
	toRecvMutex.lock();

	toRecieveCommands[recvHead] = data;
	if (recvHead < CIRCRECIEVEBUFFERLEN - 1)
	{
		recvHead++;
	}
	else
	{
		recvHead = 0;
	}

	if (recvHead == recvTail)
	{
		//OVERFLOW -- PROGRESS THE TAIL FORWARD ALSO
		if (recvHead < CIRCRECIEVEBUFFERLEN - 1)
		{
			recvTail = recvHead + 1;
		}
		else
		{
			recvTail = 0;
		}
	}

	toRecvMutex.unlock();
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
		//freeaddrinfo(result); //MAY HAVE TO KEEP THIS

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
