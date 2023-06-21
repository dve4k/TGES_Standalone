#include "stdafx.h"
#include "PlasserUDP.h"


//PROTOTYPES
void putUDP1Packet(UDP1 p);

//GLOBALS
std::queue<UDP1> udp1_que;
std::mutex udp1_mutex;
std::condition_variable udp1_condVar;
SOCKET RecvSocket;

//FLAG FOR RECIEVING DATA
bool udp_recieve_flag = true;

//CONSTRUCTORS
PlasserUDP::PlasserUDP()
{

}

PlasserUDP::~PlasserUDP()
{

}

//PUT UDP1 INTO QUEUE
void putUDP1Packet(UDP1 p)
{
	std::unique_lock<std::mutex> l(udp1_mutex);
	udp1_que.push(p);
	l.unlock();
	udp1_condVar.notify_one();
}

//GET UDP1 FROM QUEUE
bool PlasserUDP::getUDP1Packet(UDP1 &p)
{
	std::unique_lock<std::mutex> l(udp1_mutex);
	udp1_condVar.wait_for(l, std::chrono::milliseconds(100), [] {return udp1_que.size() > 0; });
	if (udp1_que.size() > 0)
	{
		p = udp1_que.front();
		udp1_que.pop();
		l.unlock();
		return true;
	}
	l.unlock();
	return false;
}

//RECIEVE THREAD FOR UDP1 PACKETS
void PlasserUDP::udp1RecieveThread()
{
	udp_recieve_flag = true;

	int iResult = 0;
	WSADATA wsaData;
	sockaddr_in RecvAddr;
	unsigned short Port = 6005;

	char RecvBuf[1024];
	int BufLen = 1024;

	sockaddr_in SenderAddr;
	int SenderAddrSize = sizeof(SenderAddr);

	//-----------------------------------------------
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		wprintf(L"WSAStartup failed with error %d\n", iResult);
		return ;
	}
	//-----------------------------------------------
	// Create a receiver socket to receive datagrams
	RecvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (RecvSocket == INVALID_SOCKET) {
		wprintf(L"socket failed with error %d\n", WSAGetLastError());
		return ;
	}
	//-----------------------------------------------
	// Bind the socket to any address and the specified port.
	RecvAddr.sin_family = AF_INET;
	RecvAddr.sin_port = htons(Port);
	RecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	iResult = bind(RecvSocket, (SOCKADDR *)& RecvAddr, sizeof(RecvAddr));
	if (iResult != 0) {
		wprintf(L"bind failed with error %d\n", WSAGetLastError());
		return ;
	}

	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 500000;

	int selectRes;
	while (udp_recieve_flag)
	{
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(RecvSocket, &fds);
		////RECIEVE DATA
		selectRes = select(RecvSocket, &fds, NULL, NULL, &timeout);
		if (selectRes > 0)
		{
			iResult = recvfrom(RecvSocket,
				RecvBuf, BufLen, 0, (SOCKADDR *)& SenderAddr, &SenderAddrSize);
			if (iResult > 0)
			{
				UDP1* tmp = reinterpret_cast<UDP1*>(RecvBuf);
				UDP1 udp1Packet = *tmp;
				putUDP1Packet(udp1Packet);
			}
		}
	}

	iResult = closesocket(RecvSocket);
	WSACleanup();
}

//END THREAD
void PlasserUDP::shutdownUdp1Recv()
{
	udp_recieve_flag = false;
	//shutdown(RecvSocket, 2); // 2 = SD_BOTH
	//closesocket(RecvSocket);
}