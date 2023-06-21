#include "stdafx.h"
#include "BackOfficeUDP.h"

#pragma comment(lib,"ws2_32.lib") //Winsock Library

#define BACKOFFICE_SEND_PORT 9989	//The port on which to send data

//PROTOTYPES

//GLOBALS
bool BACKOFFICE_UDP_SEND_RUN = false;
std::mutex backOfficeUdpSending_mutex;
std::queue<TGES_BACKOFFICE_PACKET> backOfficeUdpSending_queue;
std::condition_variable backOfficeUdpSending_conditionVariable;

BackOfficeUDP::BackOfficeUDP()
{

}

//SETUP AND RUN INFINITE SEND LOOP
int BackOfficeUDP::setupBackOfficeUdpSend()
{
	BACKOFFICE_UDP_SEND_RUN = true;
	int iResult;
	WSADATA wsaData;

	SOCKET SendSocket = INVALID_SOCKET;
	sockaddr_in RecvAddr;

	unsigned short Port = BACKOFFICE_SEND_PORT;

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
	RecvAddr.sin_addr.s_addr = inet_addr("10.13.164.151");

	//---------------------------------------------
	// Send a datagram to the receiver
	//wprintf(L"Sending a datagram to the receiver...\n");

	while (BACKOFFICE_UDP_SEND_RUN)
	{
		std::unique_lock<std::mutex> l(backOfficeUdpSending_mutex);
		backOfficeUdpSending_conditionVariable.wait_for(l, std::chrono::milliseconds(500));
		if (backOfficeUdpSending_queue.size() > 0)
		{
			TGES_BACKOFFICE_PACKET poppedData = backOfficeUdpSending_queue.front();
			//int dataLen = 32;
			backOfficeUdpSending_queue.pop();
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

//PLACE DATA ONTO THE SEND QUEUE
bool BackOfficeUDP::backOffice_udp_putDataToSend(TGES_BACKOFFICE_PACKET dataToSend)
{
	std::unique_lock<std::mutex> l(backOfficeUdpSending_mutex);
	backOfficeUdpSending_queue.push(dataToSend);
	l.unlock();
	backOfficeUdpSending_conditionVariable.notify_one();
	return true;
}

//SHUTDOWN THE SENDING LOOP
bool BackOfficeUDP::shutdownBackOfficeUdpSend()
{
	BACKOFFICE_UDP_SEND_RUN = false;
	return true;
}