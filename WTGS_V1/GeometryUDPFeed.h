#pragma once
#include "stdafx.h"
#include <thread>
#include "windows.h"
#include "atlstr.h"
#include <cstdlib>
#include "CoreGeomCalculationPacket.h"
#include "ExceptionAnalysis.h"
//TCP/IP SERVER
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <condition_variable>
#include <queue>
#include <mutex>

class GeometryUDPFeed
{
public:
		GeometryUDPFeed();
		~GeometryUDPFeed();
		int setupGeomUdpSend();
		int setupExcUdpSend();
		bool udpGeom_putDataToSend(CoreGeomCalculationPacket dataToSend);
		bool udpExc_putDataToSend(WTGS_Exception dataToSend);
		bool udpGeomExc_shutdownSend();
};
