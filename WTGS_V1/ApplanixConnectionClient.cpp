#include "stdafx.h"
#include "ApplanixConnectionClient.h"
//#pragma optimize("", off)


//PROTOTYPES
void setupApplanixConnectionLog();
unsigned __stdcall recieveLoggingThreadApplanixTCP(void* args);
unsigned __stdcall loggingLoop_ApplanixConnectionLog(void* args);
unsigned __stdcall commandThreadApplanixTCP(void* args);
void putApplanixConCliLog(RAW_APPLANIX_DATA data);
void putApplanixGroup1Packet(APPLANIX_GROUP_1 data);
void putApplanixGroup2Packet(APPLANIX_GROUP_2 data);
void putApplanixGroup5Packet(APPLANIX_GROUP_5 data);
void putApplanixGroup15Packet(APPLANIX_GROUP_15 data);
void checkRoll_ApplanixConnectionLog();
bool getApplanixGageLoggingData(APPLANIX_GROUP_10304 &data);
void putApplanixGroup10304Packet(APPLANIX_GROUP_10304 data);
bool getRecvData_Applanix_Group_10304(bool &gotData, APPLANIX_GROUP_10304 &gageData);
bool updateApplanixGlobalGating_Local(bool newValue);


long footPacketCounter = 0;

//GLOBAL FLAGS FOR CONNECTIONS
bool applanixConnectionEnable = false;

//RECIEVED DATA QUEUES / CONDITIONAL VARIABLES / MUTEXES
std::condition_variable recvBuffer_Group1_condVar;
std::queue<APPLANIX_GROUP_2> recvBuffer_Group2;
std::mutex recvBuffer_Group2_mutex;
std::condition_variable recvBuffer_Group2_condVar;
std::queue<CoreGeomPacket_A> recvBuffer_Group5;
std::mutex recvBuffer_Group5_mutex;
std::condition_variable recvBuffer_Group5_condVar;
std::queue<APPLANIX_GROUP_15> recvBuffer_Group15;
std::mutex recvBuffer_Group15_mutex;
std::condition_variable recvBuffer_Group15_condVar;

std::queue<APPLANIX_GROUP_10304> recvBuffer_Group10304;
std::mutex recvBuffer_Group10304_mutex;
std::condition_variable recvBuffer_Group10304_condVar;

//CREATING 1 FOOT PACKETS OF APPLANIX DATA
CoreGeomPacket_A oneFootBufferData[10];
int oneFootBufferData_cnt = 0;
std::mutex oneFootBufferData_mutex;
std::condition_variable oneFootBufferData_condVar;

//FOR FILE LOGGING
std::ofstream applanixConCliLog_Stream;
long applanixConCliLog_Size = 0;
std::queue<RAW_APPLANIX_DATA> applanixConCliLog_queue;
std::mutex applanixConCliLog_mutex;
std::condition_variable applanixConCliLog_condVar;
std::mutex applanixRawLogWriting_mutex;
//STORING GAGE DATA INSIDE THE FILE
std::mutex applanixGageStoring_mutex;
std::queue<APPLANIX_GROUP_10304> applanixGageStoring_queue;

//FLAG FOR CONNECTION IS UP AND WE CAN UN-GATE PLASSER PULSES
std::mutex applanixPulseGatingMutex;
bool applanixPulseGatingEnabled = true;

//SOCKET INFO
bool udpDisplaySocketConnected = false;
bool tcpControlSocketConnected = false;
bool tcpLoggingSocketConnected = false;

//TCP LOG SOCKET ENABLED
bool tcpLoggingSocketEnabled = false;

//LOG WRITER ENABLE
bool rawLogWriterEnable = false;

//CONSTRUCTORS - NORMAL
ApplanixConnectionClient::ApplanixConnectionClient()
{
	updateApplanixGlobalGating_Local(true);

	//RESET GLOBALS 
	oneFootBufferData_cnt = 0;
	footPacketCounter = 0;
	applanixConCliLog_Size = 0;
	//MAKE SURE THAT THE QUEUES ARE CLEAR
	int logBuffSize = applanixConCliLog_queue.size();
	for (int i = 0; i < logBuffSize; i++)
	{
		applanixConCliLog_queue.pop();
	}

	int gageStoringSize = applanixGageStoring_queue.size();
	for (int i = 0; i < gageStoringSize; i++)
	{
		applanixGageStoring_queue.pop();
	}
	for (int i = 0; i < 10; i++)
	{
		oneFootBufferData[i] = CoreGeomPacket_A();
	}

	int g10304StoringQueSize = recvBuffer_Group10304.size();
	for (int i = 0; i < g10304StoringQueSize; i++)
	{
		recvBuffer_Group10304.pop();
	}

	int g15QueSize = recvBuffer_Group15.size();
	for (int i = 0; i < g15QueSize; i++)
	{
		recvBuffer_Group15.pop();
	}

	int g5QueSize = recvBuffer_Group5.size();
	for (int i = 0; i < g5QueSize; i++)
	{
		recvBuffer_Group5.pop();
	}

	int g2QueSize = recvBuffer_Group2.size();
	for (int i = 0; i < g2QueSize; i++)
	{
		recvBuffer_Group2.pop();
	}

}

//CONSTRUCTORS - OTHER
ApplanixConnectionClient::~ApplanixConnectionClient()
{


}

//CONNECT TO APPLANIX AND ENABLE RAW LOGGING IF DESIRED
void ApplanixConnectionClient::connectApplanix(bool enableRawLogging)
{
	updateApplanixGlobalGating_Local(true);

	//SETUP LOG FILE TO WRITE TO
	setupApplanixConnectionLog();

	applanixConnectionEnable = true;

	unsigned tcpLoggingThreadID;
	HANDLE tcpLoggingThreadHandle;

	tcpLoggingThreadHandle = (HANDLE)_beginthreadex(NULL, 0, &recieveLoggingThreadApplanixTCP, NULL, NULL, &tcpLoggingThreadID);

	unsigned logWriterID;
	HANDLE logWriterHandle;
	rawLogWriterEnable = enableRawLogging;
	if (enableRawLogging)
	{
		logWriterHandle = (HANDLE)_beginthreadex(NULL, 0, &loggingLoop_ApplanixConnectionLog, NULL, NULL, &logWriterID);
	}

	//WAIT FOR RECIEVE TCP THREAD TO CLOSE
	WaitForSingleObject(tcpLoggingThreadHandle, INFINITE);
	CloseHandle(tcpLoggingThreadHandle);

	rawLogWriterEnable = false;
	
	if (enableRawLogging)
	{
		WaitForSingleObject(logWriterHandle, INFINITE);
		CloseHandle(logWriterHandle);
	}
}

//THREAD FOR RECIEVING DATA FROM SPECIFIED TCP PORT
unsigned __stdcall recieveLoggingThreadApplanixTCP(void* args)
{
	updateApplanixGlobalGating_Local(true);
	bool connectSuccess = true;

	char* portNumber = const_cast<char*>(TCP_REALTIME_DATAPORT);//(TCP_REALTIME_DATAPORT);
	WSADATA wsaData;
	int iResult;

	SOCKET ClientSocket = INVALID_SOCKET;
	bool clientSocketIndex = false;

	struct addrinfo *result = NULL;
	struct addrinfo *ptr = NULL;
	struct addrinfo hints;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		connectSuccess = false;
		return 1;
		int WSA_STARTUP_FAILED = 1;
	}
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	iResult = getaddrinfo(INS_IP_ADDRESS, portNumber, &hints, &result);
	if (iResult != 0)
	{
		connectSuccess = false;
		return 1;
		int GET_ADDR_INFO_ERROR = 1;
	}

	for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{
		ClientSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (ClientSocket == INVALID_SOCKET)
		{
			connectSuccess = false;
			int INVALID_SOCKET_CONN = 1;
			closesocket(ClientSocket);
			ClientSocket = INVALID_SOCKET;
			return 1;
			continue;
		}

		//CONNECT TO SERVER
		iResult = connect(ClientSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR)
		{
			connectSuccess = false;
			closesocket(ClientSocket);
			ClientSocket = INVALID_SOCKET;
			return 1;
		}
		break;
	}

	freeaddrinfo(result);
	if (ClientSocket == INVALID_SOCKET)
	{
		connectSuccess = false;
		int UNABLE_TO_CONNECT = 1;
		WSACleanup();
		return 1;
	}

	unsigned recieveThreadID;
	HANDLE recieveThreadHandle = NULL;
	Applanix applanixlParsers = Applanix();

	char recvbuf[DEFAULT_BUFLEN_AP];
	int recvbuflen = DEFAULT_BUFLEN_AP;

	char tmpMessageBuf[DEFAULT_BUFLEN_AP];
	char tmpHeaderBuf[34];

	char syncChar1[2];
	char syncChars[4];
	int lastFootIndex = 0;
	double lastFootSec = 0;

	//WE HAVE A GOOD CONNECTION -- WE NEED TO OPEN UN-GATE THE PLASSER PULSES
	updateApplanixGlobalGating_Local(false);
	while (connectSuccess && applanixConnectionEnable)
	{

		//memset(syncChar1, 0x00, 2);
		//memset(syncChars, 0x00, 4);
		//memset(tmpHeaderBuf, 0x00, 34);
		//memset(tmpMessageBuf, 0x00, DEFAULT_BUFLEN_AP);
		//memset(recvbuf, 0x00, DEFAULT_BUFLEN_AP);
		//RECIEVE DATA
		//CHECK HOW MANY BYTES ARE ON RECIEVE BUFFER
		u_long val;
		ioctlsocket(ClientSocket, FIONREAD, &val);
		if (val > 10000)
		{
			//OutputDebugString(std::to_string(val).c_str());
			//OutputDebugString("TCP BUFFER\n");
			int problem = 1;
		}
		iResult = recv(ClientSocket, syncChar1, 1, MSG_WAITALL);

		//ASSURE WE RECIEVED SOME DATA
		if (iResult == 0)
		{
			//NO DATA RECIEVED -- CONNECTION CLOSED
			connectSuccess = false;
			return 1;
		}

		//if (rawLogWriterEnable)
		//{
		//	//SEND THE RAW DATA TO BE SAVED TO FILE
		//	RAW_APPLANIX_DATA rawData0;
		//	memcpy(rawData0.data, syncChar1, 1);
		//	rawData0.dataLength = 1;
		//	putApplanixConCliLog(rawData0);
		//}

		//CHECK FOR START OF SYNC
		if (syncChar1[0] == '$')
		{
			//READ 2ND SYNC CHAR
			iResult = recv(ClientSocket, &syncChar1[1], 1, MSG_WAITALL);

			//if (rawLogWriterEnable)
			//{
			//	//SEND THE RAW DATA TO BE SAVED TO FILE
			//	RAW_APPLANIX_DATA rawData1;
			//	memcpy(rawData1.data, &syncChar1[1], 1);
			//	rawData1.dataLength = 1;
			//	putApplanixConCliLog(rawData1);
			//}

			if (iResult == 0 || iResult < 1)
			{
				//NO DATA RECIEVED -- CONNECTION CLOSED
				connectSuccess = false;
				return 1;
			}

			if (syncChar1[1] == 'G')
			{

				//READ THE SYNC CHARS
				iResult = recv(ClientSocket, syncChars, 2, MSG_WAITALL);

				//ASSURE WE RECIEVED SOME DATA
				if (iResult == 0 || iResult < 2)
				{
					//NO DATA RECIEVED -- CONNECTION CLOSED
					connectSuccess = false;
					return 1;
				}

				//if (rawLogWriterEnable)
				//{
				//	//SEND THE RAW DATA TO BE SAVED TO FILE
				//	RAW_APPLANIX_DATA rawData2;
				//	memcpy(rawData2.data, syncChars, 2);
				//	rawData2.dataLength = 2;
				//	putApplanixConCliLog(rawData2);
				//}

				memmove(&syncChars[2], &syncChars, 2);
				memcpy(&syncChars[0], syncChar1, 2);

				if (syncChars[2] == 'R' && syncChars[3] == 'P')
				{
					//READ THE FULL HEADER
					iResult = recv(ClientSocket, tmpHeaderBuf, 30, MSG_WAITALL);

					//ASSURE WE RECIEVED SOME DATA
					if (iResult == 0 || iResult < 30)
					{
						//NO DATA RECIEVED -- CONNECTION CLOSED
						connectSuccess = false;
						return 1;
					}

					//if (rawLogWriterEnable)
					//{
					//	//SEND THE RAW DATA TO BE SAVED TO FILE
					//	RAW_APPLANIX_DATA rawData3;
					//	memcpy(rawData3.data, tmpHeaderBuf, 30);
					//	rawData3.dataLength = 30;
					//	putApplanixConCliLog(rawData3);
					//}

					memmove(&tmpHeaderBuf[4], &tmpHeaderBuf, 30);
					memcpy(&tmpHeaderBuf[0], syncChars, 4);

					//CAST INTO A HEADER STRUCT
					unsigned char* tmpHeaderUnsigned = reinterpret_cast<unsigned char*>(tmpHeaderBuf);
					APPLANIX_HEADER tmpHeaderStruct;
					bool goodHeaderParse = applanixlParsers.parse_Applanix_Header(tmpHeaderStruct, tmpHeaderUnsigned);

					if (goodHeaderParse)
					{
						iResult = recv(ClientSocket, recvbuf, tmpHeaderStruct.byteCount + 8 - 34, MSG_WAITALL);

						//ASSURE WE RECIEVED SOME DATA
						if (iResult == 0 || iResult < tmpHeaderStruct.byteCount + 8 - 34)
						{
							//NO DATA RECIEVED -- CONNECTION CLOSED
							connectSuccess = false;
							return 1;
						}

						//if (rawLogWriterEnable)
						//{
						//	//SEND THE RAW DATA TO BE SAVED TO FILE
						//	RAW_APPLANIX_DATA rawData4;
						//	memcpy(rawData4.data, recvbuf, tmpHeaderStruct.byteCount + 8 - 34);
						//	rawData4.dataLength = tmpHeaderStruct.byteCount + 8 - 34;
						//	putApplanixConCliLog(rawData4);
						//}

						memmove(&recvbuf[34], recvbuf, tmpHeaderStruct.byteCount + 8 - 34);
						memcpy(&recvbuf[0], tmpHeaderBuf, 34);
						unsigned char* packetUnsigned = reinterpret_cast<unsigned char*>(recvbuf);

						if (rawLogWriterEnable)
						{
							//SEND THE RAW DATA TO BE SAVED TO FILE
							RAW_APPLANIX_DATA rawData4;
							memcpy(rawData4.data, packetUnsigned, tmpHeaderStruct.byteCount + 8);
							rawData4.dataLength = tmpHeaderStruct.byteCount + 8;
							putApplanixConCliLog(rawData4);
						}

						switch (tmpHeaderStruct.groupNumber)
						{
						case 1:
							APPLANIX_GROUP_1 tmpGroup1;
							if (applanixlParsers.parse_Applanix_Group_1(tmpGroup1, packetUnsigned))
							{
								putApplanixGroup1Packet(tmpGroup1);
								int gotAPG1 = 1;
							}
							else
							{
								int problem = 1;
							}
							break;
						case 2:
							APPLANIX_GROUP_2 tmpGroup2;
							if (applanixlParsers.parse_Applanix_Group_2(tmpGroup2, packetUnsigned))
							{
								//putApplanixGroup2Packet(tmpGroup2);
							}
							break;
						case 5:
							APPLANIX_GROUP_5 tmpGroup5;
							if (applanixlParsers.parse_Applanix_Group_5(tmpGroup5, packetUnsigned))
							{
								putApplanixGroup5Packet(tmpGroup5);
								int gotAGP5 = 1;
								if (lastFootIndex == 0)
								{
									lastFootIndex = tmpGroup5.pulseNumber;
									lastFootSec = tmpGroup5.header.time1;
								}
								else
								{
									if (lastFootIndex != tmpGroup5.pulseNumber - 1 && lastFootSec != 0.0)
									{
										double deltaTime = tmpGroup5.header.time1 - lastFootSec;
										int problem = 1;
									}
									lastFootIndex = tmpGroup5.pulseNumber;
									lastFootSec = tmpGroup5.header.time1;
								}
							}
							else
							{
								int problem = 1;
							}
							break;
						case 10304:
							APPLANIX_GROUP_10304 tmpGroup10304;
							if (applanixlParsers.parse_Applanix_Group_10304(tmpGroup10304, packetUnsigned))
							{
								putApplanixGroup10304Packet(tmpGroup10304);
								int gotAPG1 = 1;
							}
							else
							{
								int problem = 1;
							}
							break;
						//case 15:
						//	if (junkbufferforerror > 0)
						//	{
						//		int problem = 1;
						//	}
						//	APPLANIX_GROUP_15 tmpGroup15;
						//	if (applanixlParsers.parse_Applanix_Group_15(tmpGroup15, packetUnsigned))
						//	{
						//		//putApplanixGroup15Packet(tmpGroup15);
						//	}
						//	break;
						//case 331:
						//	APPLANIX_GROUP_5 tmpGroup5_a;
						//	tmpGroup5_a.header = tmpHeaderStruct;
						//	tmpGroup5_a.pulseNumber = 1;
						//	putApplanixGroup5Packet(tmpGroup5_a);
						//	break;
						}
					}
					else
					{
						int badHeaderParse = 1;
					}
				}
			}
		}
	}
	closesocket(ClientSocket);
	WSACleanup();
	int notRecieving = 1;
}

//THREAD FOR CONTROLLING / SENDING COMMANDS TO APPLANIX VIA TCP
unsigned __stdcall commandThreadApplanixTCP(void* args)
{
	bool connectSuccess = true;

	char* portNumber = const_cast<char*>(TCP_CONTROLPORT);
	WSADATA wsaData;
	int iResult;

	SOCKET ClientSocket = INVALID_SOCKET;
	bool clientSocketIndex = false;

	struct addrinfo *result = NULL;
	struct addrinfo *ptr = NULL;
	struct addrinfo hints;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		connectSuccess = false;
		return 1;
		int WSA_STARTUP_FAILED = 1;
	}
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	iResult = getaddrinfo(INS_IP_ADDRESS, portNumber, &hints, &result);
	if (iResult != 0)
	{
		connectSuccess = false;
		return 1;
		int GET_ADDR_INFO_ERROR = 1;
	}

	for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{
		ClientSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (ClientSocket == INVALID_SOCKET)
		{
			connectSuccess = false;
			int INVALID_SOCKET_CONN = 1;
			closesocket(ClientSocket);
			ClientSocket = INVALID_SOCKET;
			return 1;
			continue;
		}

		//CONNECT TO SERVER
		iResult = connect(ClientSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR)
		{
			connectSuccess = false;
			closesocket(ClientSocket);
			ClientSocket = INVALID_SOCKET;
			return 1;
		}
		break;
	}

	freeaddrinfo(result);
	if (ClientSocket == INVALID_SOCKET)
	{
		connectSuccess = false;
		int UNABLE_TO_CONNECT = 1;
		WSACleanup();
		return 1;
	}

	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec =250000;
	while (connectSuccess && tcpControlSocketConnected)
	{
		int selectRes;
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(ClientSocket, &fds);
		////CHECK FOR RECIEVE DATA
		selectRes = select(ClientSocket, &fds, NULL, NULL, &timeout);
		if (selectRes > 0)
		{
		}

		////SEND DATA

	}
}

//OPEN AND SETUP THE FILE FOR LOGGING RAW APPLANIX DATA
void setupApplanixConnectionLog()
{
	//GET CURRENT TIME
	std::chrono::time_point<std::chrono::system_clock> timePoint = std::chrono::system_clock::now();
	time_t now = time(0);
	struct tm * timeinfo;
	timeinfo = localtime(&now);
	char dt[80];// = ctime(&now);
	strftime(dt, 80, "%G%m%d%H%M%S", timeinfo);
	CString cstringDt1 = LOG_DIRECTORY + CString(dt) + "_RAW.csv";
	applanixConCliLog_Size = 0;
	if (applanixConCliLog_Stream.is_open())
	{
		applanixConCliLog_Stream.close();
	}
	applanixConCliLog_Stream.open(cstringDt1, std::ios::ios_base::binary);
}

//WRITE A PACKET TO RAW LOGGING
void writeLine_ApplanixConnectionLog(RAW_APPLANIX_DATA data)
{
	applanixConCliLog_Stream.write(data.data, data.dataLength);
	applanixConCliLog_Size = applanixConCliLog_Size + data.dataLength;
}

//CHECK TO ROLL RAW LOGGING FILE
void checkRoll_ApplanixConnectionLog()
{
	if (applanixConCliLog_Size > RAW_APPLANIX_MAX_LOG_SIZE)
	{
		//ROLL TO A NEW FILE
		//GET CURRENT TIME AND CREATE FILE NAME
		std::chrono::time_point<std::chrono::system_clock> timePoint = std::chrono::system_clock::now();
		time_t now = time(0);
		struct tm * timeinfo;
		timeinfo = localtime(&now);
		char dt[80];// = ctime(&now);
		strftime(dt, 80, "%G%m%d%H%M%S", timeinfo);
		CString cstringDt1 = LOG_DIRECTORY + CString(dt) + "_RAW.csv";

		//CLOSE EXISTING FILE
		applanixConCliLog_Stream.close();

		//OPEN A NEW FILE AND RESET FILE SIZE TO 0
		applanixConCliLog_Size = 0;
		if (applanixConCliLog_Stream.is_open())
		{
			applanixConCliLog_Stream.close();
		}
		applanixConCliLog_Stream.open(cstringDt1, std::ios::ios_base::binary);
	}
}

//INFINITE LOGGING LOOP FOR APPLANIX RAW DATA
unsigned __stdcall loggingLoop_ApplanixConnectionLog(void* args)
{
	while (rawLogWriterEnable && applanixConnectionEnable)
	{
		std::unique_lock<std::mutex> l(applanixConCliLog_mutex);
		applanixConCliLog_condVar.wait_for(l, std::chrono::milliseconds(100), [] {return applanixConCliLog_queue.size() > 0; });
		int bufferSize = applanixConCliLog_queue.size();
		if (bufferSize > 0)
		{
			RAW_APPLANIX_DATA data = applanixConCliLog_queue.front();
			applanixConCliLog_queue.pop();

			l.unlock();

			applanixRawLogWriting_mutex.lock();
			//SEE IF WE NEED TO ROLL FILE -- AND THEN ROLL IF NEEDED
			checkRoll_ApplanixConnectionLog();
			//WRITE DATA TO FILE
			writeLine_ApplanixConnectionLog(data);
			applanixRawLogWriting_mutex.unlock();
		}
		APPLANIX_GROUP_10304 gageData;
		bool gotGageData = getApplanixGageLoggingData(gageData);
		if (gotGageData)
		{
			applanixRawLogWriting_mutex.lock();
			//WRITE DATA TO FILE
			RAW_APPLANIX_DATA rawAppGageData;
			rawAppGageData.dataLength = 64;//64??
			memcpy(&rawAppGageData.data, &gageData, 64/*sizeof(rawAppGageData)*/);
			writeLine_ApplanixConnectionLog(rawAppGageData);
			applanixRawLogWriting_mutex.unlock();
		}
	}
	return 0;
}

//PUT RAW DATA TO BE SAVED ONTO QUEUE
void putApplanixConCliLog(RAW_APPLANIX_DATA data)
{
	std::unique_lock<std::mutex> l(applanixConCliLog_mutex);
	applanixConCliLog_queue.push(data);
	l.unlock();
	applanixConCliLog_condVar.notify_one();
}

//PUT APPLANIX GROUP 1 DATA INTO QUEUE
void putApplanixGroup1Packet(APPLANIX_GROUP_1 data)
{
	std::unique_lock<std::mutex> l(oneFootBufferData_mutex);
	if (oneFootBufferData_cnt > 0)
	{
		int index = 0;

		for (int i = 1; i < oneFootBufferData_cnt; i++)
		{
			if (oneFootBufferData[i - 1].seconds > data.header.time1 && oneFootBufferData_cnt > 1)
			{
				index = i;
			}
		}

		//SHIFT AG1 BUFFER
		memmove(&oneFootBufferData[index].buffer_AG1[1], &oneFootBufferData[index].buffer_AG1[0], (sizeof APPLANIX_GROUP_1) * 49);
		oneFootBufferData[index].buffer_AG1[0] = data;
		if (oneFootBufferData[index].AG1_BufferIndex < 50)
		{
			oneFootBufferData[index].AG1_BufferIndex++;
		}
	}
	else
	{
		//JUST THROW THE DATA AWAY
	}
	l.unlock();
}

//PUT APPLANIX GROUP 2 DATA INTO QUEUE
void putApplanixGroup2Packet(APPLANIX_GROUP_2 data)
{
	std::unique_lock<std::mutex> l(recvBuffer_Group2_mutex);
	recvBuffer_Group2.push(data);
	l.unlock();
	recvBuffer_Group2_condVar.notify_one();
}

//PUT APPLANIX GROUP 5 DATA INTO QUEUE
void putApplanixGroup5Packet(APPLANIX_GROUP_5 data)
{
	bool pushToQueue = false;
	CoreGeomPacket_A packetToPush;
	footPacketCounter++;

	std::unique_lock<std::mutex> l(oneFootBufferData_mutex);
	
	//SHIFT BUFFER
	memmove(&oneFootBufferData[1], &oneFootBufferData[0], (sizeof CoreGeomPacket_A()) * 9);

	//POPULATE A NEW ELEMENT
	oneFootBufferData[0].oneFootPacket = data;
	oneFootBufferData[0].seconds = data.header.time1;
	oneFootBufferData[0].AG1_BufferIndex = 0;
	oneFootBufferData[0].AG1_complete = false;
	oneFootBufferData[0].CoreGeom_complete = false;

	
	if (oneFootBufferData_cnt < 10)
	{
		oneFootBufferData_cnt++;
	}

	if (oneFootBufferData_cnt >= 10)
	{
		memcpy(&packetToPush, &oneFootBufferData[9], sizeof CoreGeomPacket_A());
		pushToQueue = true;
	}

	l.unlock();

	if (pushToQueue)
	{
		packetToPush.AG1_complete = true;
		if (packetToPush.AG1_BufferIndex < 1)
		{
			int problem = 1;
		}
		std::unique_lock<std::mutex> r(recvBuffer_Group5_mutex);
		recvBuffer_Group5.push(packetToPush);
		r.unlock();
		recvBuffer_Group5_condVar.notify_one();
	}
}

//PUT APPLANIX GROUP 15 DATA INTO QUEUE
void putApplanixGroup15Packet(APPLANIX_GROUP_15 data)
{
	std::unique_lock<std::mutex> l(recvBuffer_Group15_mutex);
	recvBuffer_Group15.push(data);
	l.unlock();
	recvBuffer_Group15_condVar.notify_one();
}

//PUT APPLANIX GROUP 10304 INTO QUEUE
void putApplanixGroup10304Packet(APPLANIX_GROUP_10304 data)
{
	std::unique_lock<std::mutex> l(recvBuffer_Group10304_mutex);
	recvBuffer_Group10304.push(data);
	l.unlock();
	recvBuffer_Group10304_condVar.notify_one();
}

//PUT APPLANIX GAGE PACKET INTO QUEUE
void ApplanixConnectionClient::putApplanixGageLoggingData(APPLANIX_GROUP_10304 data)
{
	std::unique_lock<std::mutex> l(applanixGageStoring_mutex);
	applanixGageStoring_queue.push(data);
	l.unlock();
}

//GET APPLANIX GAGE PACKET FROM QUEUE
bool getApplanixGageLoggingData(APPLANIX_GROUP_10304 &data)
{
	std::unique_lock<std::mutex> l(applanixGageStoring_mutex);
	int queSize = applanixGageStoring_queue.size();
	if (queSize > 0)
	{
		data = applanixGageStoring_queue.front();
		applanixGageStoring_queue.pop();
		l.unlock();
		return true;
	}
	l.unlock();
	return false;
}

//GET APPLANIX GROUP 5 DATA FROM QUEUE
bool ApplanixConnectionClient::getRcvData_Applanix_Group_5(bool &gotData, CoreGeometry_A &coreGeom, TGC_Location &location)
{
	
	std::unique_lock<std::mutex> l(recvBuffer_Group5_mutex);
	//recvBuffer_Group5_condVar.wait(l, [] {return recvBuffer_Group5.size() > 0; });
	recvBuffer_Group5_condVar.wait_for(l, std::chrono::milliseconds(20), [] {return recvBuffer_Group5.size() > 0; });
	int queSize = recvBuffer_Group5.size();
	if (queSize > 20)
	{
		int problem = 1;
	}
	if (queSize > 0)
	{
		//APPLANIX_GROUP_5 tmp = recvBuffer_Group5.front();
		//recvBuffer_Group5.pop();
		CoreGeomPacket_A tmp = recvBuffer_Group5.front();
		recvBuffer_Group5.pop();
		l.unlock();
		if (ACCEPT_APPLANIX_GAGE)
		{
			bool gotGageData = false;
			while (!gotGageData)
			{
				APPLANIX_GROUP_10304 tmp10304;
				bool got10304;
				getRecvData_Applanix_Group_10304(gotGageData, tmp10304);
				if (gotGageData)
				{
					tmp.gageData_complete = true;
					tmp.gageData.full_gage = tmp10304.fullGage;
					tmp.gageData.right_height = tmp10304.rightVertical;
					tmp.gageData.right_width = tmp10304.rightGage;
					tmp.gageData.left_height = tmp10304.leftVertical;
					tmp.gageData.left_width = tmp10304.leftGage;
				}
			}		
		}
		MP_LOCATION tmpLoc = location.getLocation_Increment();
		coreGeom.newFoot_A(tmp, tmpLoc);
		return true;
	}
	l.unlock();
	return false;
}

//GET APPLANIX GROUP 10304 DATA FROM QUEUE
bool getRecvData_Applanix_Group_10304(bool &gotData, APPLANIX_GROUP_10304 &gageData)
{
	std::unique_lock<std::mutex> l(recvBuffer_Group10304_mutex);
	recvBuffer_Group10304_condVar.wait_for(l, std::chrono::milliseconds(3), [] {return recvBuffer_Group5.size() > 0; });
	if (recvBuffer_Group10304.size() > 0)
	{
		gageData = recvBuffer_Group10304.front();
		recvBuffer_Group10304.pop();
		l.unlock();
		gotData = true;
		return true;
	}
	l.unlock();
	gotData = false;
	return false;
}

//DISABLE TCP CONNECTION FLAG
void ApplanixConnectionClient::shutdownApplanix()
{
	applanixConnectionEnable = false;
}

//ENABLE THE FOOT PULSE GATING
bool ApplanixConnectionClient::enableApplanixGlobalGating()
{
	std::unique_lock<std::mutex> l(applanixPulseGatingMutex);
	applanixPulseGatingEnabled = true;
	l.unlock();
	return true;
}

//UPDATE THE FOOT PULSE GATING GLOBAL .. LOCAL
bool updateApplanixGlobalGating_Local(bool newValue)
{
	std::unique_lock<std::mutex> l(applanixPulseGatingMutex);
	applanixPulseGatingEnabled = newValue;
	l.unlock();
	return true;
}

//RETURN THE CURRENT APPLANIX GATING STATE
bool ApplanixConnectionClient::checkApplanixGlobalGating()
{
	std::unique_lock<std::mutex> l(applanixPulseGatingMutex);
	bool currentState = applanixPulseGatingEnabled;
	l.unlock();
	return currentState;
}
