#pragma once
#include "stdafx.h"
#include "Applanix.h"
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

#pragma pack(1)
struct GageGeometry_Data
{
	//LEFT HALF-GAGE INFORMATION
	double left_height;
	double left_width;

	//RIGHT HALF-GAGE INFORMATION
	double right_height;
	double right_width;

	//FULL GAGE INFORMATION
	double full_gage;

	//PACKET NUMBER
	long packetNumber;

	//HAS DATA
	bool hasData;
	bool hasAllGoodData;
};

#pragma pack(1)
struct CoreGeomPacket_A
{
	int AG1_BufferIndex;

	//FLAGS FOR INDICATING THAT THE PACKET IS 'COMPLETE' IE. PACKETS WITH LATER TIMESTAMPS HAVE BEEN RECIEVED
	bool AG1_complete;
	bool CoreGeom_complete;
	bool gageData_complete;
	double seconds;
	MP_LOCATION location;
	APPLANIX_GROUP_5 oneFootPacket;
	APPLANIX_GROUP_1 buffer_AG1[COREGEOMPACKET_PACKETLEN];
	double lastDMI;
	GageGeometry_Data gageData;




};


class CoreGeometry_A
{
public:
	CoreGeometry_A();
	~CoreGeometry_A();
	void controllerThread_CoreCalcs_A(int setupMeasureDirection);
	void newFoot_A(CoreGeomPacket_A dataStruct, MP_LOCATION location);
	bool getCalcPacket_A(CoreGeomCalculationPacket &p);
	bool putGageData(GageGeometry_Data data);
	bool shutdownCoreGeometry();
	bool enableCoreGeometry();
	bool gageMatchingThread();
	void setVehicleID(char* vehicleID);
};
