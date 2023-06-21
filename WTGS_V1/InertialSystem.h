#pragma once
#include "stdafx.h"
#include <mutex>
#include "atlstr.h"
#include "Settings.h"
#include "stdio.h"
#include "stdlib.h"
#include "Applanix.h"
#include "ApplanixConnectionClient.h"
#include "CoreGeometry_A.h"
#include "GeoCalculations.h"
#include "GeometryLogging.h"
#include "Calculation_Simulation.h"
#include "ExceptionAnalysis.h"
#include "PlasserUDP.h"
#include "ExceptionAnalysis.h"
#include "GeometryUDPFeed.h"

class InertialSystem
{
public:
	InertialSystem();
	InertialSystem(bool geometryLogging, bool rawApplanixLogging, bool udpGeomDataOutput);
	~InertialSystem();
	bool startInertialSystem(int measureDirection);
	bool putGageInput(GageGeometry_Data packet);
	void displayGeometry(HDC &hdc);
	bool getDataPacketOutputBuffer(CoreGeomCalculationPacket &packet);
	bool shutdownInterialSystem();
	void displayGeometryCombined(HDC &hdc, long &inertialCnt);
	bool enableInertialPulseGatingStatus();
	bool getCurrentInertialPulseGatingStatus();
};