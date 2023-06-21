#pragma once
#include "stdafx.h"
#include <thread>
#include "windows.h"
#include "atlstr.h"
#include <mutex>
#include <queue>
#include <math.h>
#include "Settings.h"
#include <string>
#include <condition_variable>
#include "TGC_Location.h"
#include "ExceptionLimits_Settings.h"
#include "CoreGeometry_A.h"

#pragma pack(1)
struct WTGS_Exception
{
	MP_LOCATION max_location;
	double latitude;
	double longitude;
	double altitude;
	int exceptionType;
	double exceptionAmplitude;
	int exceptionSeverity;
	int exceptionBaseLength = 1;
	int exceptionLength = 1;
	int trackClass;
	int trackType;
	bool FRA_Exception;
	double speedTraveling;
	double secValidity;
	char vehicleName[10];
};


class ExceptionAnalysis
{
public:
	ExceptionAnalysis();
	~ExceptionAnalysis();
	void PerformExceptionAnalysis(CoreGeomCalculationPacket packet, bool shortVersion, WTGS_Exception* exceptions, int &gotExceptionCount);
};

