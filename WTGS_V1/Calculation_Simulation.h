#pragma once
#include "stdafx.h"
#include "GeoCalculations.h"
#include "ExceptionLimits_Settings.h"
#include "Settings.h"
#include "CoreGeometry_A.h"

#include <ctime>
#include <vector>
#include <iostream>
#include <fstream>
#include "atlstr.h"



class Calculation_Simulation
{
public:
	Calculation_Simulation();
	~Calculation_Simulation();
	bool calcSimBuffer_Get(CoreGeomCalculationPacket &packet);
	void startSimulation();

private:

};