#include "stdafx.h"
#include "GeometryUDPFeed.h"

#pragma comment(lib,"ws2_32.lib") //Winsock Library

#define GEOM_UDP_SEND_PORT 9980	//The port on which to send data
#define EXC_UDP_SEND_PORT 9981 //The port on which to send data

//PROGRAM WILL UNICAST DATA TO 10.255.255.21

//GLOBALS
std::queue<CoreGeomCalculationPacket> udpGeomOutQueue;
std::queue<WTGS_Exception> udpExcOutQueue;
std::condition_variable udpGeomOutQueue_condVar;
std::condition_variable udpExcOutQueue_condVar;

std::mutex udpGeomOutMutex;
std::mutex udpExcOutMutex;

bool GEOM_UDP_SEND_RUNNING = true;
bool EXC_UDP_SEND_RUNNING = true;

//CONSTRUCTOR
GeometryUDPFeed::GeometryUDPFeed()
{

}

GeometryUDPFeed::~GeometryUDPFeed()
{

}

//SETUP AND RUN INFINITE SEND LOOP -- GEOMETRY UDP OUTPUT
int GeometryUDPFeed::setupGeomUdpSend()
{
	GEOM_UDP_SEND_RUNNING = true;
	int iResult;
	WSADATA wsaData;

	SOCKET SendSocket = INVALID_SOCKET;
	sockaddr_in RecvAddr;

	unsigned short Port = GEOM_UDP_SEND_PORT;

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
	// the receiver 
	// and the specified port number.
	RecvAddr.sin_family = AF_INET;
	RecvAddr.sin_port = htons(Port);
	RecvAddr.sin_addr.s_addr = inet_addr("10.255.255.21");

	//---------------------------------------------
	// Send a datagram to the receiver
	//wprintf(L"Sending a datagram to the receiver...\n");

	while (GEOM_UDP_SEND_RUNNING)
	{
		std::unique_lock<std::mutex> l(udpGeomOutMutex);
		udpGeomOutQueue_condVar.wait_for(l, std::chrono::milliseconds(500));
		if (udpGeomOutQueue.size() > 0)
		{
			CoreGeomCalculationPacket poppedData = udpGeomOutQueue.front();
			//int dataLen = 32;
			udpGeomOutQueue.pop();
			//iResult = sendto(SendSocket, SendBuf, BufLen, 0, (SOCKADDR*)& RecvAddr, sizeof(RecvAddr));
			iResult = sendto(SendSocket, reinterpret_cast<const char*>(&poppedData), sizeof(poppedData), 0, (SOCKADDR*)&RecvAddr, sizeof(RecvAddr));
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

//SETUP AND RUN INFINITE SEND LOOP -- EXCEPTION UDP OUTPUT
int GeometryUDPFeed::setupExcUdpSend()
{
	EXC_UDP_SEND_RUNNING = true;
	int iResult;
	WSADATA wsaData;

	SOCKET SendSocket = INVALID_SOCKET;
	sockaddr_in RecvAddr;

	unsigned short Port = EXC_UDP_SEND_PORT;

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
	RecvAddr.sin_addr.s_addr = inet_addr("10.255.255.21");

	//---------------------------------------------
	// Send a datagram to the receiver
	//wprintf(L"Sending a datagram to the receiver...\n");

	while (EXC_UDP_SEND_RUNNING)
	{
		std::unique_lock<std::mutex> l(udpExcOutMutex);
		udpExcOutQueue_condVar.wait_for(l, std::chrono::milliseconds(500));
		if (udpExcOutQueue.size() > 0)
		{
			WTGS_Exception poppedData = udpExcOutQueue.front();
			//int dataLen = 32;
			udpExcOutQueue.pop();
			//iResult = sendto(SendSocket, SendBuf, BufLen, 0, (SOCKADDR*)& RecvAddr, sizeof(RecvAddr));
			iResult = sendto(SendSocket, reinterpret_cast<const char*>(&poppedData), sizeof(poppedData), 0, (SOCKADDR*)&RecvAddr, sizeof(RecvAddr));
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

//PUT GEOM PACKET ON SEND QUEUE
bool GeometryUDPFeed::udpGeom_putDataToSend(CoreGeomCalculationPacket dataToSend)
{
	std::unique_lock<std::mutex> l(udpGeomOutMutex);
	udpGeomOutQueue.push(dataToSend);
	l.unlock();
	udpGeomOutQueue_condVar.notify_one();
	return true;
}

//PUT EXC PACKET ON SEND QUEUE
bool GeometryUDPFeed::udpExc_putDataToSend(WTGS_Exception dataToSend)
{
	std::unique_lock<std::mutex> l(udpExcOutMutex);
	udpExcOutQueue.push(dataToSend);
	l.unlock();
	udpExcOutQueue_condVar.notify_one();
	return true;
}

//SHUTDOWN THE SENDING LOOPS
bool GeometryUDPFeed::udpGeomExc_shutdownSend() 
{
	GEOM_UDP_SEND_RUNNING = false;
	EXC_UDP_SEND_RUNNING = false;
	return true;
}