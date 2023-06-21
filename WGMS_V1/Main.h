#pragma once
#include "resource.h"
#include "windows.h"
#include "EthernetScannerSDK.h"
#include "EthernetScannerSDKDefine.h"
#include <mutex>
#include <ctime>
#include <vector>
#include <iostream>
#include <fstream>
#include "atlstr.h"
#include "Settings.h"
#include "ImageProcessing.h"
#include "ProfileLogging.h"
#include "NetworkInterface.h"
#include "advantech/inc/bdaqctrl.h"
#include "stdio.h"
#include "stdlib.h"
//#include "advantech/inc/compatibility.h"

struct ProcessedPoints
{
	double xCoords[2048];
	double yCoords[2048];
	EdgePoint tor;
	int picCounter;
	double y5_8;
	double x5_8;
	double torAvgY;
	double fifoPercentage;
	double halfGageReading;
	double fullGageReading;
	long halfGageReadingCnt;
	long halfGageMissedReadingCnt;
	long fullGageReadingCnt;
	long fullGageMissedReadingCnt;
	TemplatePoints templatePoint[RAIL_TEMPLATE_COUNT];
	bool foundHalfGage;
	bool foundFullGage;

};
