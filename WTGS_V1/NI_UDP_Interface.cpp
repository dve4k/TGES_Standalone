#include "stdafx.h"
#include "NI_UDP_Interface.h"

#pragma comment(lib,"ws2_32.lib") //Winsock Library

#define SEND_PORT 9992	//The port on which to send data
#define RECV_PORT 9993 //The port on which to listen for incoming data

//PROTOTYPES
bool ni_udp_putRecvData(NI_RECV_DATA data);


NI_RECV_DATA recentRecvState;
NI_SEND_DATA recentSendState;

bool NI_UDP_SEND_RUNNING = true;
bool NI_UDP_RECV_RUNNING = true;
SOCKET NiUdpRecvSocket;

std::mutex niUdpSending_mutex;
std::queue<NI_SEND_DATA> niUdpSending_queue;
std::condition_variable niUdpSending_conditionVariable;

std::mutex niUdpReceiving_mutex;
std::queue<NI_RECV_DATA> niUdpReceiving_queue;
std::condition_variable niUdpReceiving_conditionVariable;


NI_UDP_Interface::NI_UDP_Interface()
{

}
NI_UDP_Interface::~NI_UDP_Interface()
{

}

//SETUP AND RUN INFINITE SEND LOOP
int NI_UDP_Interface::setupNiUdpSend()
{
	NI_UDP_SEND_RUNNING = true;
	int iResult;
	WSADATA wsaData;

	SOCKET SendSocket = INVALID_SOCKET;
	sockaddr_in RecvAddr;

	unsigned short Port = SEND_PORT;

	char SendBuf[1024];
	int BufLen = 1024;

	//----------------------
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		wprintf(L"WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	//---------------------------------------------
	// Create a socket for sending data
	SendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (SendSocket == INVALID_SOCKET) {
		wprintf(L"socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}
	//---------------------------------------------
	// Set up the RecvAddr structure with the IP address of
	// the receiver (in this example case "192.168.1.1")
	// and the specified port number.
	RecvAddr.sin_family = AF_INET;
	RecvAddr.sin_port = htons(Port);
	//RecvAddr.sin_addr.s_addr = inet_addr("192.168.1.1");
	RecvAddr.sin_addr.s_addr = inet_addr("10.255.255.1");

	//---------------------------------------------
	// Send a datagram to the receiver
	//wprintf(L"Sending a datagram to the receiver...\n");

	//SET OUTPUT TO GATE
	//int gateOutput = 0;
	//iResult = sendto(SendSocket, reinterpret_cast<const char*>(&gateOutput), sizeof(gateOutput), 0, (SOCKADDR*)& RecvAddr, sizeof(RecvAddr));



	while (NI_UDP_SEND_RUNNING)
	{
		std::unique_lock<std::mutex> l(niUdpSending_mutex);
		niUdpSending_conditionVariable.wait_for(l, std::chrono::milliseconds(500));
		if (niUdpSending_queue.size() > 0)
		{
			NI_SEND_DATA poppedData = niUdpSending_queue.front();
			//int dataLen = 32;
			niUdpSending_queue.pop();
			//iResult = sendto(SendSocket, SendBuf, BufLen, 0, (SOCKADDR*)& RecvAddr, sizeof(RecvAddr));
			iResult = sendto(SendSocket, reinterpret_cast<const char*>(&poppedData), sizeof(poppedData), 0, (SOCKADDR*)& RecvAddr, sizeof(RecvAddr));
		}
		l.unlock();
	}
	if (iResult == SOCKET_ERROR) {
		wprintf(L"sendto failed with error: %d\n", WSAGetLastError());
		closesocket(SendSocket);
		WSACleanup();
		return 1;
	}
	//---------------------------------------------
	// When the application is finished sending, close the socket.
	//wprintf(L"Finished sending. Closing socket.\n");
	iResult = closesocket(SendSocket);
	if (iResult == SOCKET_ERROR) {
		wprintf(L"closesocket failed with error: %d\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}
	//---------------------------------------------
	// Clean up and quit.
	wprintf(L"Exiting.\n");
	WSACleanup();
	return 0;
	
}

//SETUP AND RUN INFINITE RECEIVE LOOP
int NI_UDP_Interface::setupNiUdpRecv()
{
	NI_UDP_RECV_RUNNING = true;

	int iResult = 0;
	WSADATA wsaData;
	sockaddr_in RecvAddr;
	unsigned short Port = RECV_PORT;

	char RecvBufNIUDP[1024];
	int BufLen = 1024;

	sockaddr_in SenderAddr;
	int SenderAddrSize = sizeof(SenderAddr);

	//-----------------------------------------------
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		wprintf(L"WSAStartup failed with error %d\n", iResult);
		return 0;
	}
	//-----------------------------------------------
	// Create a receiver socket to receive datagrams
	NiUdpRecvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (NiUdpRecvSocket == INVALID_SOCKET) {
		wprintf(L"socket failed with error %d\n", WSAGetLastError());
		return 0;
	}
	//-----------------------------------------------
	// Bind the socket to any address and the specified port.
	RecvAddr.sin_family = AF_INET;
	RecvAddr.sin_port = htons(Port);
	RecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	iResult = bind(NiUdpRecvSocket, (SOCKADDR*)& RecvAddr, sizeof(RecvAddr));
	if (iResult != 0) {
		wprintf(L"bind failed with error %d\n", WSAGetLastError());
		return 0;
	}

	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 500000;

	int selectRes;
	while (NI_UDP_RECV_RUNNING)
	{
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(NiUdpRecvSocket, &fds);
		////RECIEVE DATA
		selectRes = select(NiUdpRecvSocket, &fds, NULL, NULL, &timeout);
		if (selectRes > 0)
		{
			iResult = recvfrom(NiUdpRecvSocket,
				RecvBufNIUDP, BufLen, 0, (SOCKADDR*)& SenderAddr, &SenderAddrSize);
			if (iResult > 0)
			{
				NI_RECV_DATA* tmp = reinterpret_cast<NI_RECV_DATA*>(RecvBufNIUDP);
				NI_RECV_DATA udp1Packet = *tmp;
				ni_udp_putRecvData(udp1Packet);
			}
		}
	}

	iResult = closesocket(NiUdpRecvSocket);
	WSACleanup();
	return 0;
}

//PLACE DATA ONTO THE SEND QUEUE
bool NI_UDP_Interface::ni_udp_putDataToSend(NI_SEND_DATA dataToSend)
{
	std::unique_lock<std::mutex> l(niUdpSending_mutex);
	niUdpSending_queue.push(dataToSend);
	recentSendState = dataToSend;
	l.unlock();
	niUdpSending_conditionVariable.notify_one();
	return true;
}

//PLACE DATA ONTO THE RECV QUEUE
bool ni_udp_putRecvData(NI_RECV_DATA data)
{
	std::unique_lock<std::mutex> l(niUdpReceiving_mutex);
	//niUdpReceiving_queue.push(data);
	recentRecvState = data;
	l.unlock();
	//niUdpReceiving_conditionVariable.notify_one();
	return true;
}

////GET DATA FROM RECV QUEUE
//bool NI_UDP_Interface::ni_udp_getRecvData(NI_RECV_DATA &data)
//{
//	std::unique_lock<std::mutex> l(niUdpReceiving_mutex);
//	niUdpReceiving_conditionVariable.wait_for(l, std::chrono::milliseconds(500));
//	if (niUdpReceiving_queue.size() > 0)
//	{
//		NI_RECV_DATA poppedData = niUdpReceiving_queue.front();
//		niUdpReceiving_queue.pop();
//		data = poppedData;
//		l.unlock();
//		return true;
//	}
//	l.unlock();
//	return false;
//}

//SHUTDOWN THE SENDING LOOP
bool NI_UDP_Interface::shutdownNiUdpSend()
{
	NI_UDP_SEND_RUNNING = false;
	return true;
}

//SHUTDOWN THE RECEIVING LOOP
bool NI_UDP_Interface::shutdownNiUdpRecv()
{
	NI_UDP_RECV_RUNNING = false;
	return true;
}

NI_SEND_DATA NI_UDP_Interface::getRecentSendDataNI()
{
	std::unique_lock<std::mutex> l(niUdpSending_mutex);
	NI_SEND_DATA toRet = recentSendState;
	l.unlock();
	return toRet;
}

NI_RECV_DATA NI_UDP_Interface::getRecentRecvDataNI()
{
	std::unique_lock<std::mutex> l(niUdpReceiving_mutex);
	NI_RECV_DATA toRet = recentRecvState;
	l.unlock();
	return toRet;
}