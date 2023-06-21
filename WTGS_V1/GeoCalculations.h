#pragma once
#include "Settings.h"
#include "CoreGeometry_A.h"
#include "windows.h"
#include "atlstr.h"
#include <math.h>
#include "ExceptionAnalysis.h"


struct AnalysisCalcPacket
{

};

class GeoCalculations
{
public:
	GeoCalculations();
	~GeoCalculations();
	bool putRawCalcPacket_512Buffer(CoreGeomCalculationPacket p);
	bool getCompleteCalcPacket_outputBuffer(CoreGeomCalculationPacket &p);
	bool calculationProcessor();
	bool shutdownGeoCalculations();
	bool getExceptionOutputQue(WTGS_Exception &exception);
	bool popExceptionOutputQue();

private:

};


