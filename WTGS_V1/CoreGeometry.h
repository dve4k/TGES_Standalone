#pragma once
#include "stdafx.h"
#include "Novatel.h"
#include <thread>
#include "windows.h"
#include "atlstr.h"
#include <mutex>
#include <queue>
#include <math.h>
#include "Settings.h"
#include <fstream>
#include <iostream>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <string>
#include <condition_variable>
#include "TGC_Location.h"
#include "CoreGeomCalculationPacket.h"

struct CoreGeomPacket
{
	double seconds;
	MP_LOCATION location;
	MARKPVA oneFootPacket;
	INSSPDS buffer_INSSPDS[COREGEOMPACKET_PACKETLEN];
	INSPVAS buffer_INSPVAS[COREGEOMPACKET_PACKETLEN];
	CORRIMUDATAS buffer_CORRIMUDATAS[COREGEOMPACKET_PACKETLEN];
	int CORRIMUDATAS_BufferIndex;
	int INSSPDS_BufferIndex;
	int INSPVAS_BufferIndex;

	//FLAGS FOR INDICATING THAT THE PACKET IS 'COMPLETE' IE. PACKETS WITH LATER TIMESTAMPS HAVE BEEN RECIEVED
	bool CORRIMUDATAS_complete;
	bool INSSPDS_complete;
	bool INSPVAS_complete;
	bool CoreGeom_complete;
};


enum Rail
{
	LEFT = 0,
	RIGHT = 1
};



class CoreGeometry
{
public:
	CoreGeometry();
	~CoreGeometry();
	void controllerThread_CoreCalcs();
	void newFoot(MARKPVA dataStruct, MP_LOCATION location);
	bool putMsgIntoCoreGeomPacket_CORRIMUDATAS(CORRIMUDATAS dataStruct, bool &popIt);
	bool putMsgIntoCoreGeomPacket_INSSPDS(INSSPDS dataStruct, bool &popIt);
	bool putMsgIntoCoreGeomPacket_INSPVAS(INSPVAS dataStruct, bool &popIt);
	bool createGeometryFile(CString fileName);
	//bool CoreGeometry::newGeometryFileLine(CoreGeomCalculationPacket p);
	bool getCalcPacket(CoreGeomCalculationPacket &p);
};

