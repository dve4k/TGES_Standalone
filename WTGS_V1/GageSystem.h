#pragma once
#include "stdafx.h"
#include "EthernetScannerSDK.h"
#include "EthernetScannerSDKDefine.h"
#include <mutex>
#include <ctime>
#include <vector>
#include "atlstr.h"
#include "Settings.h"
#include "ImageProcessing.h"
#include "ProfileLogging.h"
#include "stdio.h"
#include "stdlib.h"
#include "CoreGeometry_A.h"
#include <condition_variable>
#include <queue>

#pragma pack(1)
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
	double torAvgY_Xval;
	double webXVal;
	double webYVal;
};

struct AnalysisResults
{
	void* scannerObj;
	ProfilePoints analyzedPoints;
	int scannerNumber;

	////IMAGE PROCESSING STUFF
	//double WINDOW_CENTER_X_S1;
	//double WINDOW_CENTER_Y_S1;
	//double CV2_IMAGE_MIN_X_1;
	//double CV2_IMAGE_MIN_Y_1;
	//double CV2_IMAGE_MAX_X_1;
	//double CV2_IMAGE_MAX_Y_1;
	//double CV2_IMAGE_MIN_X_2;
	//double CV2_IMAGE_MIN_Y_2;
	//double CV2_IMAGE_MAX_X_2;
	//double CV2_IMAGE_MAX_Y_2;
};

struct Scanners
{
	void* scanner_1;
	void* scanner_2;
};

class GageSystem
{
public:
	GageSystem(bool fullConstructor);
	GageSystem();
	~GageSystem();
	bool prepareForCapture(Scanners &scanners);
	bool startGageCapture(Scanners &scanners);
	bool stopGageCapture(Scanners &scanners);
	bool getGageOutput(GageGeometry_Data &packet);
	double updateGageOffset(double gageReading);
	void displayProfilePoints(HDC &hdc, int sensor, bool &screenPaintOnce);
	void displayProfilePointsCombined(HDC &hdc, int sensor, bool &screenPaintOnce, long &pictureCount);
	void checkScannerConnections(Scanners &scanners, bool &g1Connected, bool &g2Connected);
	Scanners globalScanners;
	int getPulsePerFoot();
private:
	void startScannerAcq(Scanners &scanners);
};

