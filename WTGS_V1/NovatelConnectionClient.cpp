#include "stdafx.h"
#include "NovatelConnectionClient.h"

//PROTOTYPES
unsigned __stdcall recieveThreadNovaltel(void* args);
unsigned  __stdcall sendThreadNovatel(void* args);
void putRcvDataNovatel(CORRIMUDATAS dataStruct, int recvBufferIndex);
void putRcvDataNovatel(MARKPVA dataStruct, int recvBufferIndex);
void putRcvDataNovatel(INSSPDS dataStruct, int recvBufferIndex);
void putRcvDataNovatel(INSPVAS dataStruct, int recvBufferIndex);
//void putRcvDataNovatel(BESTPOS dataStruct, int recvBufferIndex);
void setupNovatelConnectionLog();
void writeLine_NovatelConnectionLog(NovatelConCliLogInfo info);
void putData_NovatelConnectionLog(NovatelConCliLogInfo info);
unsigned __stdcall loggingLoop_NovatelConnectionLog(void* args);

bool socketConnectedNovatel[RECEIVEBUFFERSCOUNT];

std::queue<CORRIMUDATAS> recvBuffer_CORRIMUDATAS;
std::queue<MARKPVA> recvBuffer_MARKPVA;
std::queue<INSSPDS> recvBuffer_INSSPDS;
std::queue<INSPVAS> recvBuffer_INSPVAS;
//std::queue<BESTPOS> recvBuffer_BESTPOS;
//TO-SEND QUE
//RECIEVE-TO PROCESS QUE
std::mutex toRecvMutex_NCC[RECEIVEBUFFERSCOUNT];


//SEND QUEUE
std::queue<NovatelSendCommandOb> sendBufferData_NCC;
std::mutex sendBufferMutex_NCC;
std::condition_variable sendBufferCondVar_NCC;

//FOR DEBUGGING
long insspds_recv_cnt = 0;
long inspvas_recv_cnt = 0;
long insspds_recv_cnt2 = 0;
long inspvas_recv_cnt2 = 0;

//FOR LOGGING
std::ofstream novConCliLog_Stream;
long novConCliLog_Size = 0;
std::queue<NovatelConCliLogInfo> novConCliLog_queue;
std::mutex novConCliLog_Mutex;
std::condition_variable novConCliLog_condVar;
std::mutex novConCliLogWrite_Mutex;



NovatelConnectionClient::NovatelConnectionClient()
{
	for (int i = 0; i < RECEIVEBUFFERSCOUNT; i++)
	{
		socketConnectedNovatel[i] = false;
	}
}

NovatelConnectionClient::~NovatelConnectionClient()
{

}

void NovatelConnectionClient::testFunction()
{
	controllerThread();
}

//FUNCTION WITH INFINITE CONTROLLER LOOP
void NovatelConnectionClient::controllerThread()
{
	//START RECEIVE THREADS -- ONE THREAD FOR EACH RECIEVE PORT
	unsigned recvThreadID[7];
	HANDLE recvThreadHandle[7];
	int recvBufferIndex[7] =
	{
		0, 1, 2, 3, 4, 5, 6
	};

	/*unsigned sendThreadID;
	HANDLE sendThreadHandle;*/

	//SETUP LOG FILE TO WRITE TO
	setupNovatelConnectionLog();

	//sendThreadHandle = (HANDLE)_beginthreadex(NULL, 0, &sendThreadNovatel, &recvBufferIndex[6], NULL, &sendThreadID);
	
	recvThreadHandle[0] = (HANDLE)_beginthreadex(NULL, 0, &recieveThreadNovaltel, &recvBufferIndex[0], NULL, &recvThreadID[0]);

	recvThreadHandle[1] = (HANDLE)_beginthreadex(NULL, 0, &recieveThreadNovaltel, &recvBufferIndex[1], NULL, &recvThreadID[1]);

	recvThreadHandle[2] = (HANDLE)_beginthreadex(NULL, 0, &recieveThreadNovaltel, &recvBufferIndex[2], NULL, &recvThreadID[2]);

	recvThreadHandle[3] = (HANDLE)_beginthreadex(NULL, 0, &recieveThreadNovaltel, &recvBufferIndex[3], NULL, &recvThreadID[3]);

	unsigned logWriterThreadID;
	HANDLE logWriterThreadHandle;

	logWriterThreadHandle = (HANDLE)_beginthreadex(NULL, 0, &loggingLoop_NovatelConnectionLog, NULL, NULL, &logWriterThreadID);

	//WaitForSingleObject(sendThreadHandle, INFINITE);
	//WaitForSingleObject(logWriterThreadHandle, INFINITE);
	WaitForSingleObject(recvThreadHandle[0], INFINITE);
	WaitForSingleObject(recvThreadHandle[1], INFINITE);
	WaitForSingleObject(recvThreadHandle[2], INFINITE);
	WaitForSingleObject(recvThreadHandle[3], INFINITE);
	//CloseHandle(sendThreadHandle);
	//CloseHandle(logWriterThreadHandle);
	CloseHandle(recvThreadHandle[0]);
	CloseHandle(recvThreadHandle[1]);
	CloseHandle(recvThreadHandle[2]);
	CloseHandle(recvThreadHandle[3]);

}

//THREAD FOR RECEIVING DATA FROM SPECIFIED PORT NUMBER
unsigned __stdcall recieveThreadNovaltel(void* args)
{
	int rcvBufferIndex = *(int*)args;
	//MAP PORT NUMBERS BY RECV BUFFER INDEX//
	char* portNumber = DEFAULT_PORT0;
	switch (rcvBufferIndex)
	{
	case 0:
		portNumber = DEFAULT_PORT0;
		break;
	case 1:
		portNumber = DEFAULT_PORT1;
		break;
	case 2:
		portNumber = DEFAULT_PORT2;
		break;
	case 3:
		portNumber = DEFAULT_PORT3;
		break;
	case 4:
		portNumber = DEFAULT_PORT4;
		break;
	case 5:
		portNumber = DEFAULT_PORT5;
		break;
	case 6:
		portNumber = DEFAULT_PORT6;
		break;
	}

	WSADATA wsaData;
	int iResult;

	SOCKET ClientSocket = INVALID_SOCKET;
	bool clientSocketIndex = false;

	struct addrinfo *result = NULL;
	struct addrinfo *ptr = NULL;
	struct addrinfo hints;

	char recvbuf[DEFAULT_BUFLEN];
	unsigned char* recvbufUnsigned;
	int recvbuflen = DEFAULT_BUFLEN;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		int WSA_STARTUP_FAILED = 1;
	}
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	iResult = getaddrinfo(INS_IP_ADDRESS, portNumber, &hints, &result);
	if (iResult != 0)
	{
		int GET_ADDR_INFO_ERROR = 1;
	}

	for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{
		ClientSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (ClientSocket == INVALID_SOCKET)
		{
			int INVALID_SOCKET_CONN = 1;
			closesocket(ClientSocket);
			ClientSocket = INVALID_SOCKET;
			continue;
		}

		//CONNECT TO SERVER
		iResult = connect(ClientSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR)
		{
			closesocket(ClientSocket);
			ClientSocket = INVALID_SOCKET;
		}
		break;
	}

	freeaddrinfo(result);
	if (ClientSocket == INVALID_SOCKET)
	{
		int UNABLE_TO_CONNECT = 1;
		WSACleanup();
	}

	unsigned recieveThreadID;
	HANDLE recieveThreadHandle = NULL;
	Novatel novatelParsers = Novatel();

	while (true)
	{
		//RECIEVE DATA
		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0)
		{

			//WRITE THE DATA INTO THE LOG FILE

			NovatelConCliLogInfo info;
			memcpy(info.data, recvbuf, recvbuflen);
			//for (int z = 0; z < recvbuflen; z++)
			//{
			//	info.data[z] = recvbuf[z];
			//}
			info.dataLength = recvbuflen;
			putData_NovatelConnectionLog(info);
			novConCliLog_condVar.notify_one();


			int WE_GOT_SOME_BYTES = 1;
			//CHECK FOR A VALID HEADER
			HeaderType headerType;
			MessageID messageID;
			unsigned short messageLength;
			unsigned short weekNumber;
			unsigned short headerLength;
			BESTPOS bESTPOS_struct;
			INSSPDS iNSSPDS_struct;
			INSPVAS iNSPVAS_struct;
			CORRIMUDATAS cORRIMUDATAS_struct;
			MARKPVA mARKPVA_struct;
			bool goodParse = false;
			long milliseconds;
			recvbufUnsigned = reinterpret_cast<unsigned char*>(recvbuf);
			bool goodHeader = novatelParsers.parse_header(recvbufUnsigned, headerType, messageID, messageLength, weekNumber, milliseconds, headerLength);
			if (goodHeader)
			{
				unsigned long fullPacketLength = (unsigned long)headerLength + (unsigned long)messageLength + 4L; // + 4 for the 4-byte checksum
				switch (messageID)
				{
				case BESTPOS_MSG:
					novatelParsers.parse_BESTPOS(recvbufUnsigned, bESTPOS_struct, fullPacketLength);
					//putRcvDataNovatel(bESTPOS_struct, 0);
					//OutputDebugString("Received BESTPOS\n");
					break;
				case INSSPDS_MSG:
					novatelParsers.parse_INSSPDS(recvbufUnsigned, iNSSPDS_struct, fullPacketLength);
					putRcvDataNovatel(iNSSPDS_struct, 1);
					//OutputDebugString("Received INSSPDS\n");
					break;
				case INSPVAS_MSG:
					novatelParsers.parse_INSPVAS(recvbufUnsigned, iNSPVAS_struct, fullPacketLength);
					putRcvDataNovatel(iNSPVAS_struct, 2);
					//OutputDebugString("Received INSPVAS\n");
					break;
				case CORRIMUDATAS_MSG:
					goodParse = novatelParsers.parse_CORRIMUDATAS(recvbufUnsigned, cORRIMUDATAS_struct, fullPacketLength);
					putRcvDataNovatel(cORRIMUDATAS_struct, 3);
					//OutputDebugString("Received CORRIMUDATAS\n");
					break;
				case MARK1PVA_MSG:
					novatelParsers.parse_MARKPVA(recvbufUnsigned, mARKPVA_struct, fullPacketLength);
					putRcvDataNovatel(mARKPVA_struct, 4);
					//OutputDebugString("Received MARK1PVA\n");
					break;
				case MARK2PVA_MSG:
					novatelParsers.parse_MARKPVA(recvbufUnsigned, mARKPVA_struct, fullPacketLength);
					putRcvDataNovatel(mARKPVA_struct, 5);
					//OutputDebugString("Received MARK2PVA\n");
					break;
				case MARK3PVA_MSG:
					novatelParsers.parse_MARKPVA(recvbufUnsigned, mARKPVA_struct, fullPacketLength);
					putRcvDataNovatel(mARKPVA_struct, 6);
					//OutputDebugString("Received MARK3PVA\n");
					break;
				case MARK4PVA_MSG:
					novatelParsers.parse_MARKPVA(recvbufUnsigned, mARKPVA_struct, fullPacketLength);
					putRcvDataNovatel(mARKPVA_struct, 7);
					//OutputDebugString("Received MARK3PVA\n");
					break;
				default:
					int BADPACKET_WTF = 1;
					break;
				}
			}
			else
			{
				int badHeader = 1;
			}
		}
		else
		{
			break;
			//CONNECTION HAS CLOSED
		}
	}

	closesocket(ClientSocket);
	WSACleanup();
	return 0;
}

//THREAD FOR SENDING DATA TO SPECIFIED PORT NUMBER
unsigned  __stdcall sendThreadNovatel(void* args)
{
	int rcvBufferIndex = *(int*)args;
	//MAP PORT NUMBERS BY RECV BUFFER INDEX//
	char* portNumber = DEFAULT_PORT0;
	switch (rcvBufferIndex)
	{
	case 0:
		portNumber = DEFAULT_PORT0;
		break;
	case 1:
		portNumber = DEFAULT_PORT1;
		break;
	case 2:
		portNumber = DEFAULT_PORT2;
		break;
	case 3:
		portNumber = DEFAULT_PORT3;
		break;
	case 4:
		portNumber = DEFAULT_PORT4;
		break;
	case 5:
		portNumber = DEFAULT_PORT5;
		break;
	case 6:
		portNumber = DEFAULT_PORT6;
		break;
	}

	WSADATA wsaData;
	int iResult;

	SOCKET ClientSocket = INVALID_SOCKET;
	bool clientSocketIndex = false;

	struct addrinfo *result = NULL;
	struct addrinfo *ptr = NULL;
	struct addrinfo hints;

	char recvbuf[DEFAULT_BUFLEN];
	unsigned char* recvbufUnsigned;
	int recvbuflen = DEFAULT_BUFLEN;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		int WSA_STARTUP_FAILED = 1;
	}
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	iResult = getaddrinfo(INS_IP_ADDRESS, portNumber, &hints, &result);
	if (iResult != 0)
	{
		int GET_ADDR_INFO_ERROR = 1;
	}

	for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{
		ClientSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (ClientSocket == INVALID_SOCKET)
		{
			int INVALID_SOCKET_CONN = 1;
			closesocket(ClientSocket);
			ClientSocket = INVALID_SOCKET;
			continue;
		}

		//CONNECT TO SERVER
		iResult = connect(ClientSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR)
		{
			closesocket(ClientSocket);
			ClientSocket = INVALID_SOCKET;
		}
		break;
	}

	freeaddrinfo(result);
	if (ClientSocket == INVALID_SOCKET)
	{
		int UNABLE_TO_CONNECT = 1;
		WSACleanup();
	}

	unsigned recieveThreadID;
	HANDLE recieveThreadHandle = NULL;
	Novatel novatelParsers = Novatel();

	while (true)
	{
		std::unique_lock<std::mutex> l(sendBufferMutex_NCC);
		//sendBufferCondVar_NCC.wait(l);
		for (int j = 0; j < sendBufferData_NCC.size(); j++)
		{
			NovatelSendCommandOb cmd = sendBufferData_NCC.front();cmd.data;
			sendBufferData_NCC.pop();

			//WE HAVE DATA TO SEND
			int resp = send(ClientSocket, cmd.data, cmd.dataLength, 0);
			if (resp == -1)
			{
				int weHaveError = WSAGetLastError();
				int errorWas = weHaveError;
			}
		}

	}

	return 0;
}

//PUT DATA INTO SPECIFIED RECV BUFFER - CORRIMUDATAS
void putRcvDataNovatel(CORRIMUDATAS dataStruct, int recvBufferIndex)
{
	toRecvMutex_NCC[recvBufferIndex].lock();
	recvBuffer_CORRIMUDATAS.push(dataStruct);
	toRecvMutex_NCC[recvBufferIndex].unlock();
}

//PUT DATA INTO SPECIFIED RECV BUFFER - MARKPVA
void putRcvDataNovatel(MARKPVA dataStruct, int recvBufferIndex)
{
	toRecvMutex_NCC[recvBufferIndex].lock();
	recvBuffer_MARKPVA.push(dataStruct);
	toRecvMutex_NCC[recvBufferIndex].unlock();
}

//PUT DATA INTO SPECIFIED RECV BUFFER - INSSPDS
void putRcvDataNovatel(INSSPDS dataStruct, int recvBufferIndex)
{
	toRecvMutex_NCC[recvBufferIndex].lock();
	insspds_recv_cnt++;
	recvBuffer_INSSPDS.push(dataStruct);
	toRecvMutex_NCC[recvBufferIndex].unlock();
}

//PUT DATA INTO SPECIFIED RECV BUFFER - INSPVAS
void putRcvDataNovatel(INSPVAS dataStruct, int recvBufferIndex)
{
	toRecvMutex_NCC[recvBufferIndex].lock();
	inspvas_recv_cnt++;
	recvBuffer_INSPVAS.push(dataStruct);
	toRecvMutex_NCC[recvBufferIndex].unlock();
}

//PUT DATA INTO SEND DATA QUEUE
void NovatelConnectionClient::putSendData(NovatelSendCommandOb cmd)
{
	sendBufferMutex_NCC.lock();
	sendBufferData_NCC.push(cmd);
	sendBufferMutex_NCC.unlock();
	sendBufferCondVar_NCC.notify_all();
}

////PUT DATA INTO SPECIFIED RECV BUFFER - BESTPOS
//void putRcvDataNovatel(BESTPOS dataStruct, int recvBufferIndex)
//{
//	toRecvMutex_NCC[recvBufferIndex].lock();
//	recvBuffer_BESTPOS.push(dataStruct);
//	toRecvMutex_NCC[recvBufferIndex].unlock();
//}

//GET DATA FROM SPECIFIED RECV BUFFER - CORRIMUDATAS
void NovatelConnectionClient::getRcvData_CORRIMUDATAS(int recvBufferIndex, bool &gotData, CoreGeometry &coreGeom)
{
	toRecvMutex_NCC[recvBufferIndex].lock();
	int queSize = recvBuffer_CORRIMUDATAS.size();
	if (queSize < 1)
	{
		gotData = false;
		toRecvMutex_NCC[recvBufferIndex].unlock();
		return;
	}

	//IS THE QUE TOO LARGE -- IF SO, POP IT//
	if (queSize > MAXRCVBUFFERLEN)
	{
		gotData = true;
		recvBuffer_CORRIMUDATAS.pop();
		toRecvMutex_NCC[recvBufferIndex].unlock();
		return;
	}

	//TRY TO MATCH PACKET TO A FOOT//
	gotData = true;
	CORRIMUDATAS tmpPacket = recvBuffer_CORRIMUDATAS.front();
	toRecvMutex_NCC[recvBufferIndex].unlock();
	bool popPacket;

	bool foundMatch = coreGeom.putMsgIntoCoreGeomPacket_CORRIMUDATAS(tmpPacket, popPacket);
	if (popPacket)
	{
		toRecvMutex_NCC[recvBufferIndex].lock();
		recvBuffer_CORRIMUDATAS.pop();
		toRecvMutex_NCC[recvBufferIndex].unlock();
	}
}

//GET DATA FROM SPECIFIED RECV BUFFER - MARKPVA
void NovatelConnectionClient::getRcvData_MARKPVA(int recvBufferIndex, bool &gotData, CoreGeometry &coreGeom, TGC_Location &location)
{
	//WE RECIEVED A NEW FOOT -- POPULATE A NEW FOOT IN CORE GOMETRY 512 FOOT BUFFER
	toRecvMutex_NCC[recvBufferIndex].lock();
	if (recvBuffer_MARKPVA.empty())
	{
		gotData = false;
		toRecvMutex_NCC[recvBufferIndex].unlock();
		return;
	}

	MARKPVA tmpPacket = recvBuffer_MARKPVA.front();
	recvBuffer_MARKPVA.pop();
	toRecvMutex_NCC[recvBufferIndex].unlock();
	MP_LOCATION tmpLoc = location.getLocation_Increment();
	coreGeom.newFoot(tmpPacket, tmpLoc);
	gotData = true;
}

//GET DATA FROM SPECIFIED RECV BUFFER - INSSPDS
void NovatelConnectionClient::getRcvData_INSSPDS(int recvBufferIndex, bool &gotData, CoreGeometry &coreGeom)
{
	toRecvMutex_NCC[recvBufferIndex].lock();
	int queSize = recvBuffer_INSSPDS.size();
	if (queSize < 1)
	{
		gotData = false;
		toRecvMutex_NCC[recvBufferIndex].unlock();
		return;
	}

	//IS THE QUE TOO LARGE -- IF SO, POP IT//
	if (queSize > MAXRCVBUFFERLEN)
	{
		gotData = true;
		recvBuffer_INSSPDS.pop();
		toRecvMutex_NCC[recvBufferIndex].unlock();
		return;
	}

	//TRY TO MATCH PACKET TO A FOOT//
	gotData = true;
	INSSPDS tmpPacket = recvBuffer_INSSPDS.front();
	toRecvMutex_NCC[recvBufferIndex].unlock();
	bool popPacket;

	bool foundMatch = coreGeom.putMsgIntoCoreGeomPacket_INSSPDS(tmpPacket, popPacket);
	if (popPacket)
	{
		toRecvMutex_NCC[recvBufferIndex].lock();
		recvBuffer_INSSPDS.pop();
		toRecvMutex_NCC[recvBufferIndex].unlock();
	}
	if (popPacket && gotData)
	{
		insspds_recv_cnt2++;
	}
}

//GET DATA FROM SPECIFIED RECV BUFFER - INSPVAS
void NovatelConnectionClient::getRcvData_INSPVAS(int recvBufferIndex, bool &gotData, CoreGeometry &coreGeom)
{
	toRecvMutex_NCC[recvBufferIndex].lock();
	int queSize = recvBuffer_INSPVAS.size();
	if (queSize < 1)
	{
		gotData = false;
		toRecvMutex_NCC[recvBufferIndex].unlock();
		return;
	}

	//IS THE QUE TOO LARGE -- IF SO, POP IT//
	if (queSize > MAXRCVBUFFERLEN)
	{
		gotData = true;
		recvBuffer_INSPVAS.pop();
		toRecvMutex_NCC[recvBufferIndex].unlock();
		return;
	}

	//TRY TO MATCH PACKET TO A FOOT//
	gotData = true;
	INSPVAS tmpPacket = recvBuffer_INSPVAS.front();
	toRecvMutex_NCC[recvBufferIndex].unlock();
	bool popPacket;

	bool foundMatch = coreGeom.putMsgIntoCoreGeomPacket_INSPVAS(tmpPacket, popPacket);
	if (popPacket)
	{
		toRecvMutex_NCC[recvBufferIndex].lock();
		recvBuffer_INSPVAS.pop();
		toRecvMutex_NCC[recvBufferIndex].unlock();
	}

	if (popPacket && gotData)
	{
		inspvas_recv_cnt2++;
	}
}

//SEND SETUP LOG COMMANDS
void NovatelConnectionClient::sendSetupLogCommands()
{
	Novatel nov = Novatel();
	char* data = "log ICOM1 INSPVASB ONTIME 0.01\r";
	char data2[512];
	strcpy(data2, data);
	NovatelSendCommandOb sendCmd;
	memcpy(sendCmd.data, data2, 512);
	sendCmd.dataLength = 512;
	putSendData(sendCmd);
}

//OPEN / SETUP THE FILE FOR LOGGING
void setupNovatelConnectionLog()
{

	//GET CURRENT TIME
	std::chrono::time_point<std::chrono::system_clock> timePoint = std::chrono::system_clock::now();
	time_t now = time(0);
	struct tm * timeinfo;
	timeinfo = localtime(&now);
	char dt[80];// = ctime(&now);
	strftime(dt, 80, "%G%m%d%H%M%S", timeinfo);
	CString cstringDt1 = LOG_DIRECTORY + CString(dt) + "_RAW.csv";
	novConCliLog_Size = 0;
	//lineCnt = 0;
	novConCliLog_Stream.open(cstringDt1);
}

////CHECK FILE SIZE AND ROLL-OVER IF NEEDED
//void checkAndRoll_NovatelConnectionLog()
//{
//	if (novConCliLog_Size > 500000) //IN KB? 500,000KB = 500MB
//	{
//		novConCliLogWrite_Mutex.lock();
//		novConCliLog_Size = 0;
//
//		//GET CURRENT TIME
//		std::chrono::time_point<std::chrono::system_clock> timePoint = std::chrono::system_clock::now();
//		time_t now = time(0);
//		struct tm * timeinfo;
//		timeinfo = localtime(&now);
//		char dt[80];// = ctime(&now);
//		strftime(dt, 80, "%G%m%d%H%M%S", timeinfo);
//		CString cstringDt1 = LOG_DIRECTORY + CString(dt) + "_RAW.csv";
//		novConCliLog_Size = 0;
//		//lineCnt = 0;
//		//CLOSE THE FILE
//		novConCliLog_Stream.close();
//
//		//OPEN THE FILE
//		novConCliLog_Stream.open(cstringDt1);
//
//
//		novConCliLogWrite_Mutex.unlock();
//	}
//}

//WRITE A LINE TO THE FILE FOR LOGGING
void writeLine_NovatelConnectionLog(NovatelConCliLogInfo info)
{
	//DETERMINE LENGTH TO WRITE: 0xcc indicates junk data
	if ((unsigned char)info.data[0] != 0xAA && (unsigned char)info.data[1] != 0x44)
	{
		return;
		//THIS CONFIRMS THAT THE FIRST TWO BYTES ARE THE 'SYNC' BYTES....not ascii or some garbage [ICOM1] / [ICOM2]...ect
	}
	int len = 0;
	for (int i = 0; i < info.dataLength; i++)
	{
		if ((unsigned char)info.data[i] == 0xcc)
		{
			break;
		}
		else
		{
			len++;
		}
	}
	novConCliLog_Stream.write(info.data, len);
}

//PUT DATA INTO NOVATEL CONNECTION LOG QUEUE
void putData_NovatelConnectionLog(NovatelConCliLogInfo info)
{
	novConCliLog_Mutex.lock();

	novConCliLog_queue.push(info);

	novConCliLog_Mutex.unlock();
}

//INFINITE LOOP FOR LOGGING
unsigned __stdcall loggingLoop_NovatelConnectionLog(void* args)
{
	while (true)
	{
		std::unique_lock<std::mutex> l(novConCliLog_Mutex);
		novConCliLog_condVar.wait(l, [] {return novConCliLog_queue.size() > 0; });
		int bufferSize = novConCliLog_queue.size();
		if ( bufferSize > 0)
		{
			if (bufferSize > 10)
			{
				int breakHere = 1;
			}

			NovatelConCliLogInfo info = novConCliLog_queue.front();
			novConCliLog_queue.pop();

			l.unlock();

			novConCliLogWrite_Mutex.lock();

			writeLine_NovatelConnectionLog(info);

			novConCliLogWrite_Mutex.unlock();

		}
	}

	return 0;
}



////GET DATA FROM SPECIFIED RECV BUFFER - BESTPOS
//void NovatelConnectionClient::getRcvData_BESTPOS(int recvBufferIndex, bool &gotData, CoreGeometry &coreGeom)
//{
//	toRecvMutex_NCC[recvBufferIndex].lock();
//	if (recvBuffer_BESTPOS.empty())
//	{
//		gotData = false;
//	}
//	else
//	{
//		dataStruct = recvBuffer_BESTPOS.front();
//		recvBuffer_INSPVAS.pop();
//		gotData = true;
//	}
//	toRecvMutex_NCC[recvBufferIndex].unlock();
//}