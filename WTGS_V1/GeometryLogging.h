#pragma once
#include <mutex>
#include <ctime>
#include <vector>
#include <iostream>
#include <fstream>
#include "atlstr.h"
#include "Settings.h"
#include "GeoCalculations.h"

class GeometryLogging
{
public:
	GeometryLogging();
	GeometryLogging(char* vehicleID);
	~GeometryLogging();
	void putGeoLoggingQue(CoreGeomCalculationPacket p);
	void geoLoggingProcessor();
	void openFile();
	bool shutdownGeometryLogging();
	bool createWriteException(WTGS_Exception exception, CoreGeomCalculationPacket* exceptionGeomData);
};
