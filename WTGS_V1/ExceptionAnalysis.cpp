#include "stdafx.h"
#include "ExceptionAnalysis.h"


//PROTOTYPES
void addToExcpetionArray(WTGS_Exception newException, WTGS_Exception* exceptionArray, int &count);
void compareTwist11_Normal(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt);
void compareTwist11_MT(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt);
void compareTwist11_MC(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt);
void compareVertBounce(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt);
void compareCombinationSurface(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt);
void compareOpenGage(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt, bool useRawGage);
void compareTightGage(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt, bool useRawGage);
void compareVariationGage(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt, bool useRawGage);
void compareVaritionCurvature(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt);
void compareProvCurvature(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt);
void compareCrosslevelStandard(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt);
void compareCrosslevelHighSpeed(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt);
void compareCrosslevelVariation(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt);
void compareReverseCrosslevel_Norm(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt);
void compareReverseCrosslevel_25(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt);
void compareFRA62Alignment(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt);
void compareFRA31Alignment(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt);
void compareFRA31Runoff(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt);
void compareFRA62Profile(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt);
void compareFRAVar0Crosslevel(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt);
void compareFRAWarp62(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt);
void compareFRAReverseCrosslevel(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt);
void compareFRAHarmonicRockoff(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int& exceptionsCnt);
void compareFRAExcessElevation(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int& exceptionsCnt);

ExceptionAnalysis::ExceptionAnalysis()
{
	//int buffSize = exceptionQue.size();
	//for (int i = 0; i < buffSize; i++)
	//{
	//	exceptionQue.pop();
	//}
}

ExceptionAnalysis::~ExceptionAnalysis()
{

}

//PERFORM EXCEPTION ANALYSIS ON A CORE GEOMETRY CALCULATION PACKET -- COMPARE THRESHOLDS
void ExceptionAnalysis::PerformExceptionAnalysis(CoreGeomCalculationPacket packet, bool shortVersion, WTGS_Exception* exceptions, int &gotExceptionCount)
{

	WTGS_Exception tmpExceptions[100];
	int tmpExceptionsCnt = 0;

	//SHORT VERSION IF FOR COMPARISIONS WHEN INITIAL CHORD BUFFERS ARE NOT FULL YET
	if (!shortVersion)
	{
		compareOpenGage(packet, tmpExceptions, tmpExceptionsCnt, false);
		compareTightGage(packet, tmpExceptions, tmpExceptionsCnt, false);
		//compareTwist11_Normal(packet, tmpExceptions, tmpExceptionsCnt);
		//compareTwist11_MT(packet, tmpExceptions, tmpExceptionsCnt);
		//compareTwist11_MC(packet, tmpExceptions, tmpExceptionsCnt);
		//compareVertBounce(packet, tmpExceptions, tmpExceptionsCnt);
		//compareCombinationSurface(packet, tmpExceptions, tmpExceptionsCnt);
		//compareVariationGage(packet, tmpExceptions, tmpExceptionsCnt, false);
		//compareVaritionCurvature(packet, tmpExceptions, tmpExceptionsCnt);
		//compareProvCurvature(packet, tmpExceptions, tmpExceptionsCnt);
		//compareCrosslevelStandard(packet, tmpExceptions, tmpExceptionsCnt);
		//compareCrosslevelHighSpeed(packet, tmpExceptions, tmpExceptionsCnt);
		//compareCrosslevelVariation(packet, tmpExceptions, tmpExceptionsCnt);
		//compareReverseCrosslevel_Norm(packet, tmpExceptions, tmpExceptionsCnt);
		//compareReverseCrosslevel_25(packet, tmpExceptions, tmpExceptionsCnt);
		compareFRA62Alignment(packet, tmpExceptions, tmpExceptionsCnt);
		compareFRA31Alignment(packet, tmpExceptions, tmpExceptionsCnt);
		compareFRA31Runoff(packet, tmpExceptions, tmpExceptionsCnt);
		compareFRA62Profile(packet, tmpExceptions, tmpExceptionsCnt);
		compareFRAVar0Crosslevel(packet, tmpExceptions, tmpExceptionsCnt);
		compareFRAWarp62(packet, tmpExceptions, tmpExceptionsCnt);
		compareFRAReverseCrosslevel(packet, tmpExceptions, tmpExceptionsCnt);
		compareFRAHarmonicRockoff(packet, tmpExceptions, tmpExceptionsCnt);
		compareFRAExcessElevation(packet, tmpExceptions, tmpExceptionsCnt);
	}
	else
	{
		//SHORT VERSION -- ONLY LOOK AT RAW GAGE
		compareOpenGage(packet, tmpExceptions, tmpExceptionsCnt, true);
		compareTightGage(packet, tmpExceptions, tmpExceptionsCnt, true);
	}
	gotExceptionCount = tmpExceptionsCnt;
	memcpy(&exceptions[0], &tmpExceptions[0], sizeof((WTGS_Exception())) * 100);
}

//ADDS AN EXCEPTION TO THE EXCEPTION ARRAY AND INCREMENTS THE COUNT
void addToExcpetionArray(WTGS_Exception newException, WTGS_Exception* exceptionArray, int &count)
{
	if (count < 100)
	{
		exceptionArray[count] = newException;
		count++;
	}
}

//DETERMINE TWIST11 NORMAL EXCEPTION
void compareTwist11_Normal(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt)
{
	double twist11 = packet.twist11;
	double excRLimit = -99;
	double excPLimit = -99;
	switch (packet.location.trackClass)
	{
		case 1:
			excPLimit = CLASS_1_PRI_11TWIST_NORMAL;
			excRLimit = CLASS_1_RED_11TWIST_NORMAL;
			break;
		case 2:
			excPLimit = CLASS_2_PRI_11TWIST_NORMAL;
			excRLimit = CLASS_2_RED_11TWIST_NORMAL;
			break;
		case 3:
			excPLimit = CLASS_3_PRI_11TWIST_NORMAL;
			excRLimit = CLASS_3_RED_11TWIST_NORMAL;
			break;
		case 4:
			excPLimit = CLASS_4_PRI_11TWIST_NORMAL;
			excRLimit = CLASS_4_RED_11TWIST_NORMAL;
			break;
		default:
			excPLimit = CLASS_4_PRI_11TWIST_NORMAL;
			excRLimit = CLASS_4_RED_11TWIST_NORMAL;
			break;
	}
	if (std::abs(twist11) >= excRLimit)
	{
		//RED LETTER EXCEPTION
		WTGS_Exception rException = WTGS_Exception();
		rException.exceptionAmplitude = twist11;
		rException.exceptionLength = 1;
		rException.exceptionBaseLength = 11;
		rException.exceptionSeverity = RED_EXCEPTION;
		rException.exceptionType = TWIST11_NORMAL;
		rException.max_location = packet.location;
		rException.latitude = packet.latitude;
		rException.longitude = packet.longitude;
		rException.altitude = packet.altitude;
		rException.speedTraveling = packet.speed;
		rException.secValidity = packet.secValidity;
		memcpy(rException.vehicleName, packet.vehicleID, 10);
		addToExcpetionArray(rException, exceptions, exceptionsCnt);

	}
	else
	{
		if (std::abs(twist11) >= excPLimit)
		{
			//PRIORITY EXCEPTION
			WTGS_Exception pException = WTGS_Exception();
			pException.exceptionAmplitude = twist11;
			pException.exceptionLength = 1;
			pException.exceptionBaseLength = 11;
			pException.exceptionSeverity = PRIORITY_EXCEPTION;
			pException.exceptionType = TWIST11_NORMAL;
			pException.max_location = packet.location;
			pException.latitude = packet.latitude;
			pException.longitude = packet.longitude;
			pException.altitude = packet.altitude;
			pException.speedTraveling = packet.speed;
			pException.secValidity = packet.secValidity;
			memcpy(pException.vehicleName, packet.vehicleID, 10);
			addToExcpetionArray(pException, exceptions, exceptionsCnt);
		}
	}
}

//DETERMINE TWIST11 MULTIPLE TWIST TANGENT EXCEPTION
void compareTwist11_MT(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt)
{
	double twist11_mt = packet.multipleTwist11;
	double excPLimit = -99;
	switch (packet.location.trackClass)
	{
	case 1:
		excPLimit = CLASS_1_PRI_11TWIST_MT;
		break;
	case 2:
		excPLimit = CLASS_2_PRI_11TWIST_MT;
		break;
	case 3:
		excPLimit = CLASS_3_PRI_11TWIST_MT;
		break;
	case 4:
		excPLimit = CLASS_4_PRI_11TWIST_MT;
		break;
	default:
		excPLimit = CLASS_4_PRI_11TWIST_MT;
		break;
	}

	if (std::abs(twist11_mt) >= excPLimit)
	{
		//PRIORITY EXCEPTION
		WTGS_Exception pException = WTGS_Exception();
		pException.exceptionAmplitude = twist11_mt;
		pException.exceptionLength = 1;
		pException.exceptionSeverity = PRIORITY_EXCEPTION;
		pException.exceptionType = TWIST11_MT;
		pException.max_location = packet.location;
		pException.latitude = packet.latitude;
		pException.longitude = packet.longitude;
		pException.altitude = packet.altitude;
		pException.speedTraveling = packet.speed;
		pException.secValidity = packet.secValidity;
		pException.exceptionBaseLength = 1;
		memcpy(pException.vehicleName, packet.vehicleID, 10);
		addToExcpetionArray(pException, exceptions, exceptionsCnt);
	}
}

//DETERMINE TWIST11 MULTIPLE TWIST CURVE EXCEPTION
void compareTwist11_MC(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt)
{
	double twist11_mc = packet.multipleTwist11;
	double excRLimit = -99;
	double excPLimit = -99;
	switch (packet.location.trackClass)
	{
	case 1:
		excPLimit = CLASS_1_PRI_11TWIST_MC;
		excRLimit = CLASS_1_RED_11TWIST_MC;
		break;
	case 2:
		excPLimit = CLASS_2_PRI_11TWIST_MC;
		excRLimit = CLASS_2_RED_11TWIST_MC;
		break;
	case 3:
		excPLimit = CLASS_3_PRI_11TWIST_MC;
		excRLimit = CLASS_3_RED_11TWIST_MC;
		break;
	case 4:
		excPLimit = CLASS_4_PRI_11TWIST_MC;
		excRLimit = CLASS_4_RED_11TWIST_MC;
		break;
	default:
		excPLimit = CLASS_4_PRI_11TWIST_MC;
		excRLimit = CLASS_4_RED_11TWIST_MC;
		break;
	}
	if (std::abs(twist11_mc) >= excRLimit)
	{
		//RED LETTER EXCEPTION
		WTGS_Exception rException = WTGS_Exception();
		rException.exceptionAmplitude = twist11_mc;
		rException.exceptionLength = 1;
		rException.exceptionSeverity = RED_EXCEPTION;
		rException.exceptionType = TWIST11_MC;
		rException.max_location = packet.location;
		rException.latitude = packet.latitude;
		rException.longitude = packet.longitude;
		rException.altitude = packet.altitude;
		rException.speedTraveling = packet.speed;
		rException.secValidity = packet.secValidity;
		rException.exceptionBaseLength = 1;
		memcpy(rException.vehicleName, packet.vehicleID, 10);
		addToExcpetionArray(rException, exceptions, exceptionsCnt);

	}
	else
	{
		if (std::abs(twist11_mc) >= excPLimit)
		{
			//PRIORITY EXCEPTION
			WTGS_Exception pException = WTGS_Exception();
			pException.exceptionAmplitude = twist11_mc;
			pException.exceptionLength = 1;
			pException.exceptionSeverity = PRIORITY_EXCEPTION;
			pException.exceptionType = TWIST11_MC;
			pException.max_location = packet.location;
			pException.latitude = packet.latitude;
			pException.longitude = packet.longitude;
			pException.altitude = packet.altitude;
			pException.speedTraveling = packet.speed;
			pException.secValidity = packet.secValidity;
			pException.exceptionBaseLength = 1;
			memcpy(pException.vehicleName, packet.vehicleID, 10);
			addToExcpetionArray(pException, exceptions, exceptionsCnt);
		}
	}
}

//DETERMINE VERTICAL BOUNCE EXCEPTION
void compareVertBounce(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt)
{
	double vertBnc = packet.verticalBounce;
	double excRLimit = -99;
	double excPLimit = -99;
	switch (packet.location.trackClass)
	{
	case 4:
		excPLimit = CLASS_4_PRI_VERTBOUNCE;
		excRLimit = CLASS_4_RED_VERTBOUNCE;
		break;
	default:
		excPLimit = CLASS_4_PRI_VERTBOUNCE;
		excRLimit = CLASS_4_RED_VERTBOUNCE;
		break;
	}
	if (std::abs(vertBnc) >= excRLimit)
	{
		//RED LETTER EXCEPTION
		WTGS_Exception rException = WTGS_Exception();
		rException.exceptionAmplitude = vertBnc;
		rException.exceptionLength = 1;
		rException.exceptionSeverity = RED_EXCEPTION;
		rException.exceptionType = VERTICAL_BOUNCE;
		rException.max_location = packet.location;
		rException.latitude = packet.latitude;
		rException.longitude = packet.longitude;
		rException.altitude = packet.altitude;
		rException.speedTraveling = packet.speed;
		rException.secValidity = packet.secValidity;
		rException.exceptionBaseLength = 1;
		memcpy(rException.vehicleName, packet.vehicleID, 10);
		addToExcpetionArray(rException, exceptions, exceptionsCnt);

	}
	else
	{
		if (std::abs(vertBnc) >= excPLimit)
		{
			//PRIORITY EXCEPTION
			WTGS_Exception pException = WTGS_Exception();
			pException.exceptionAmplitude = vertBnc;
			pException.exceptionLength = 1;
			pException.exceptionSeverity = PRIORITY_EXCEPTION;
			pException.exceptionType = VERTICAL_BOUNCE;
			pException.max_location = packet.location;
			pException.latitude = packet.latitude;
			pException.longitude = packet.longitude;
			pException.altitude = packet.altitude;
			pException.speedTraveling = packet.speed;
			pException.secValidity = packet.secValidity;
			pException.exceptionBaseLength = 1;
			memcpy(pException.vehicleName, packet.vehicleID, 10);
			addToExcpetionArray(pException, exceptions, exceptionsCnt);
		}
	}
}

//DETERMINE COMBINATION LEFT / RIGHT EXCEPTONS
void compareCombinationSurface(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt)
{
	double combL = packet.combinationLeft;
	double combR = packet.combinationRight;
	double excPLimit = -99;
	switch (packet.location.trackClass)
	{
	case 1:
		excPLimit = CLASS_1_PRI_COMB;
		break;
	case 2:
		excPLimit = CLASS_2_PRI_COMB;
		break;
	case 3:
		excPLimit = CLASS_3_PRI_COMB;
		break;
	case 4:
		excPLimit = CLASS_4_PRI_COMB;
		break;
	default:
		excPLimit = CLASS_4_PRI_COMB;
		break;
	}

	//LEFT RAIL
	if (std::abs(combL) >= excPLimit)
	{
		//PRIORITY EXCEPTION
		WTGS_Exception pException = WTGS_Exception();
		pException.exceptionAmplitude = combL;
		pException.exceptionLength = 1;
		pException.exceptionSeverity = PRIORITY_EXCEPTION;
		pException.exceptionType = COMBINATION_LEFT;
		pException.max_location = packet.location;
		pException.latitude = packet.latitude;
		pException.longitude = packet.longitude;
		pException.altitude = packet.altitude;
		pException.speedTraveling = packet.speed;
		pException.secValidity = packet.secValidity;
		pException.exceptionBaseLength = 1;
		memcpy(pException.vehicleName, packet.vehicleID, 10);
		addToExcpetionArray(pException, exceptions, exceptionsCnt);
	}

	//RIGHT RAIL
	if (std::abs(combR) >= excPLimit)
	{
		WTGS_Exception pException = WTGS_Exception();
		pException.exceptionAmplitude = combR;
		pException.exceptionLength = 1;
		pException.exceptionSeverity = PRIORITY_EXCEPTION;
		pException.exceptionType = COMBINATION_RIGHT;
		pException.max_location = packet.location;
		pException.latitude = packet.latitude;
		pException.longitude = packet.longitude;
		pException.altitude = packet.altitude;
		pException.speedTraveling = packet.speed;
		pException.secValidity = packet.secValidity;
		pException.exceptionBaseLength = 1;
		memcpy(pException.vehicleName, packet.vehicleID, 10);
		addToExcpetionArray(pException, exceptions, exceptionsCnt);
	}
}

//DETERMINE OPEN GAGE
void compareOpenGage(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt, bool useRawGage)
{
	double gage;
	if (useRawGage)
	{
		gage = packet.fullGage_raw - 56.5;
	}
	else
	{
		gage = packet.fullGage_filtered - 56.5;
	}
	
	double excRLimit = -99;
	double excPLimit = -99;
	switch (packet.location.trackClass)
	{
	case 1:
		excPLimit = CLASS_1_PRI_GAGE_OPEN;
		excRLimit = CLASS_1_RED_GAGE_OPEN;
		break;
	case 2:
		excPLimit = CLASS_2_PRI_GAGE_OPEN;
		excRLimit = CLASS_2_RED_GAGE_OPEN;
		break;
	case 3:
		excPLimit = CLASS_3_PRI_GAGE_OPEN;
		excRLimit = CLASS_3_RED_GAGE_OPEN;
		break;
	case 4:
		excPLimit = CLASS_4_PRI_GAGE_OPEN;
		excRLimit = CLASS_4_RED_GAGE_OPEN;
		break;
	default:
		excPLimit = CLASS_4_PRI_GAGE_OPEN;
		excRLimit = CLASS_4_RED_GAGE_OPEN;
		break;
	}
	if (gage >= excRLimit)
	{
		//RED LETTER EXCEPTION
		WTGS_Exception rException = WTGS_Exception();
		rException.exceptionAmplitude = gage;
		rException.exceptionLength = 1;
		rException.exceptionSeverity = RED_EXCEPTION;
		rException.exceptionType = GAGE_OPEN;
		rException.max_location = packet.location;
		rException.latitude = packet.latitude;
		rException.longitude = packet.longitude;
		rException.altitude = packet.altitude;
		rException.speedTraveling = packet.speed;
		rException.secValidity = packet.secValidity;
		rException.exceptionBaseLength = 1;
		memcpy(rException.vehicleName, packet.vehicleID, 10);
		addToExcpetionArray(rException, exceptions, exceptionsCnt);

	}
	else
	{
		if (gage >= excPLimit)
		{
			//PRIORITY EXCEPTION
			WTGS_Exception pException = WTGS_Exception();
			pException.exceptionAmplitude = gage;
			pException.exceptionLength = 1;
			pException.exceptionSeverity = PRIORITY_EXCEPTION;
			pException.exceptionType = GAGE_OPEN;
			pException.max_location = packet.location;
			pException.latitude = packet.latitude;
			pException.longitude = packet.longitude;
			pException.altitude = packet.altitude;
			pException.speedTraveling = packet.speed;
			pException.secValidity = packet.secValidity;
			pException.exceptionBaseLength = 1;
			memcpy(pException.vehicleName, packet.vehicleID, 10);
			addToExcpetionArray(pException, exceptions, exceptionsCnt);
		}
	}
}

//DETERMINE TIGHT GAGE
void compareTightGage(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt, bool useRawGage)
{
	double gage;
	if (useRawGage)
	{
		gage = packet.fullGage_raw - 56.5;
	}
	else
	{
		gage = packet.fullGage_filtered - 56.5;
	}
	double excPLimit = -99;
	switch (packet.location.trackClass)
	{
	case 1:
		excPLimit = CLASS_1_PRI_GAGE_TIGHT;
		break;
	case 2:
		excPLimit = CLASS_2_PRI_GAGE_TIGHT;
		break;
	case 3:
		excPLimit = CLASS_3_PRI_GAGE_TIGHT;
		break;
	case 4:
		excPLimit = CLASS_4_PRI_GAGE_TIGHT;
		break;
	default:
		excPLimit = CLASS_4_PRI_GAGE_TIGHT;
		break;
	}

	if (gage <= -excPLimit && gage != -56.5)
	{
		//PRIORITY EXCEPTION
		WTGS_Exception pException = WTGS_Exception();
		pException.exceptionAmplitude = gage;
		pException.exceptionLength = 1;
		pException.exceptionSeverity = PRIORITY_EXCEPTION;
		pException.exceptionType = GAGE_TIGHT;
		pException.max_location = packet.location;
		pException.latitude = packet.latitude;
		pException.longitude = packet.longitude;
		pException.altitude = packet.altitude;
		pException.speedTraveling = packet.speed;
		pException.secValidity = packet.secValidity;
		pException.exceptionBaseLength = 1;
		memcpy(pException.vehicleName, packet.vehicleID, 10);
		addToExcpetionArray(pException, exceptions, exceptionsCnt);
	}
}

//DETERIMINE VARIATION GAGE
void compareVariationGage(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt, bool useRawGage)
{
	double varGage = packet.variationGage;
	double excPLimit = -99;
	switch (packet.location.trackClass)
	{
	case 1:
		excPLimit = CLASS_1_PRI_GAGE_VAR;
		break;
	case 2:
		excPLimit = CLASS_2_PRI_GAGE_VAR;
		break;
	case 3:
		excPLimit = CLASS_3_PRI_GAGE_VAR;
		break;
	case 4:
		excPLimit = CLASS_4_PRI_GAGE_VAR;
		break;
	default:
		excPLimit = CLASS_4_PRI_GAGE_VAR
;
		break;
	}
	if (std::abs(varGage) >= excPLimit)
	{
		//PRIORITY EXCEPTION
		WTGS_Exception pException = WTGS_Exception();
		pException.exceptionAmplitude = varGage;
		pException.exceptionLength = 1;
		pException.exceptionSeverity = PRIORITY_EXCEPTION;
		pException.exceptionType = GAGE_VARIATION;
		pException.max_location = packet.location;
		pException.latitude = packet.latitude;
		pException.longitude = packet.longitude;
		pException.altitude = packet.altitude;
		pException.speedTraveling = packet.speed;
		pException.secValidity = packet.secValidity;
		pException.exceptionBaseLength = 1;
		memcpy(pException.vehicleName, packet.vehicleID, 10);
		addToExcpetionArray(pException, exceptions, exceptionsCnt);
	}
}

//DETERMINE VARIATION CURVATURE
void compareVaritionCurvature(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt)
{
	double varCurv = packet.variationCurvature;
	double excRLimit = -99;
	double excPLimit = -99;
	switch (packet.location.trackClass)
	{
	case 1:
		excPLimit = CLASS_1_PRI_CURV_VAR;
		excRLimit = CLASS_1_RED_CURV_VAR;
		break;
	case 2:
		excPLimit = CLASS_2_PRI_CURV_VAR;
		excRLimit = CLASS_2_RED_CURV_VAR;
		break;
	case 3:
		excPLimit = CLASS_3_PRI_CURV_VAR;
		excRLimit = CLASS_3_RED_CURV_VAR;
		break;
	case 4:
		excPLimit = CLASS_4_PRI_CURV_VAR;
		excRLimit = CLASS_4_RED_CURV_VAR;
		break;
	default:
		excPLimit = CLASS_4_PRI_CURV_VAR;
		excRLimit = CLASS_4_RED_CURV_VAR;
		break;
	}
	if (std::abs(varCurv) >= excRLimit)
	{
		//RED LETTER EXCEPTION
		WTGS_Exception rException = WTGS_Exception();
		rException.exceptionAmplitude = varCurv;
		rException.exceptionLength = 1;
		rException.exceptionSeverity = RED_EXCEPTION;
		rException.exceptionType = CURVATURE_VARIATION;
		rException.max_location = packet.location;
		rException.latitude = packet.latitude;
		rException.longitude = packet.longitude;
		rException.altitude = packet.altitude;
		rException.speedTraveling = packet.speed;
		rException.secValidity = packet.secValidity;
		rException.exceptionBaseLength = 1;
		memcpy(rException.vehicleName, packet.vehicleID, 10);
		addToExcpetionArray(rException, exceptions, exceptionsCnt);

	}
	else
	{
		if (std::abs(varCurv) >= excPLimit)
		{
			//PRIORITY EXCEPTION
			WTGS_Exception pException = WTGS_Exception();
			pException.exceptionAmplitude = varCurv;
			pException.exceptionLength = 1;
			pException.exceptionSeverity = PRIORITY_EXCEPTION;
			pException.exceptionType = CURVATURE_VARIATION;
			pException.max_location = packet.location;
			pException.latitude = packet.latitude;
			pException.longitude = packet.longitude;
			pException.altitude = packet.altitude;
			pException.speedTraveling = packet.speed;
			pException.secValidity = packet.secValidity;
			pException.exceptionBaseLength = 1;
			memcpy(pException.vehicleName, packet.vehicleID, 10);
			addToExcpetionArray(pException, exceptions, exceptionsCnt);
		}
	}
}

//DETERMINE PROVISIONAL CURVATURE
void compareProvCurvature(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt)
{
	double provCurv = packet.variationCurvature;
	double excRLimit = -99;
	double excPLimit = -99;
	switch (packet.location.trackClass)
	{
	case 1:
		excPLimit = CLASS_1_PRI_CURV_PROV;
		excRLimit = CLASS_1_RED_CURV_PROV;
		break;
	case 2:
		excPLimit = CLASS_2_PRI_CURV_PROV;
		excRLimit = CLASS_2_RED_CURV_PROV;
		break;
	case 3:
		excPLimit = CLASS_3_PRI_CURV_PROV;
		excRLimit = CLASS_3_RED_CURV_PROV;
		break;
	default:
		excPLimit = CLASS_3_PRI_CURV_PROV;
		excRLimit = CLASS_3_RED_CURV_PROV;
		break;
	}
	if (std::abs(provCurv) >= excRLimit)
	{
		//RED LETTER EXCEPTION
		WTGS_Exception rException = WTGS_Exception();
		rException.exceptionAmplitude = provCurv;
		rException.exceptionLength = 1;
		rException.exceptionSeverity = RED_EXCEPTION;
		rException.exceptionType = CURVATURE_PROV;
		rException.max_location = packet.location;
		rException.latitude = packet.latitude;
		rException.longitude = packet.longitude;
		rException.altitude = packet.altitude;
		rException.speedTraveling = packet.speed;
		rException.secValidity = packet.secValidity;
		rException.exceptionBaseLength = 1;
		memcpy(rException.vehicleName, packet.vehicleID, 10);
		addToExcpetionArray(rException, exceptions, exceptionsCnt);

	}
	else
	{
		if (std::abs(provCurv) >= excPLimit)
		{
			//PRIORITY EXCEPTION
			WTGS_Exception pException = WTGS_Exception();
			pException.exceptionAmplitude = provCurv;
			pException.exceptionLength = 1;
			pException.exceptionSeverity = PRIORITY_EXCEPTION;
			pException.exceptionType = CURVATURE_PROV;
			pException.max_location = packet.location;
			pException.latitude = packet.latitude;
			pException.longitude = packet.longitude;
			pException.altitude = packet.altitude;
			pException.speedTraveling = packet.speed;
			pException.secValidity = packet.secValidity;
			pException.exceptionBaseLength = 1;
			memcpy(pException.vehicleName, packet.vehicleID, 10);
			addToExcpetionArray(pException, exceptions, exceptionsCnt);
		}
	}
}

//DETERMINE CROSSLEVEL - STANDARD
void compareCrosslevelStandard(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt)
{
	double xlStandard = packet.crosslevel_OC;
	double excRLimit = -99;
	double excPLimit = -99;
	switch (packet.location.trackClass)
	{
	case 1:
		excPLimit = CLASS_1_PRI_CL_STANDARD;
		excRLimit = CLASS_1_RED_CL_STANDARD;
		break;
	case 2:
		excPLimit = CLASS_2_PRI_CL_STANDARD;
		excRLimit = CLASS_2_RED_CL_STANDARD;
		break;
	case 3:
		excPLimit = CLASS_3_PRI_CL_STANDARD;
		excRLimit = CLASS_3_RED_CL_STANDARD;
		break;
	case 4:
		excPLimit = CLASS_4_PRI_CL_STANDARD;
		excRLimit = CLASS_4_RED_CL_STANDARD;
		break;
	default:
		excPLimit = CLASS_4_PRI_CL_STANDARD;
		excRLimit = CLASS_4_RED_CL_STANDARD;
		break;
	}
	if (std::abs(xlStandard) >= excRLimit)
	{
		//RED LETTER EXCEPTION
		WTGS_Exception rException = WTGS_Exception();
		rException.exceptionAmplitude = xlStandard;
		rException.exceptionLength = 1;
		rException.exceptionSeverity = RED_EXCEPTION;
		rException.exceptionType = XL_STANDARD;
		rException.max_location = packet.location;
		rException.latitude = packet.latitude;
		rException.longitude = packet.longitude;
		rException.altitude = packet.altitude;
		rException.speedTraveling = packet.speed;
		rException.secValidity = packet.secValidity;
		rException.exceptionBaseLength = 1;
		memcpy(rException.vehicleName, packet.vehicleID, 10);
		addToExcpetionArray(rException, exceptions, exceptionsCnt);

	}
	else
	{
		if (std::abs(xlStandard) >= excPLimit)
		{
			//PRIORITY EXCEPTION
			WTGS_Exception pException = WTGS_Exception();
			pException.exceptionAmplitude = xlStandard;
			pException.exceptionLength = 1;
			pException.exceptionSeverity = PRIORITY_EXCEPTION;
			pException.exceptionType = XL_STANDARD;
			pException.max_location = packet.location;
			pException.latitude = packet.latitude;
			pException.longitude = packet.longitude;
			pException.altitude = packet.altitude;
			pException.speedTraveling = packet.speed;
			pException.secValidity = packet.secValidity;
			pException.exceptionBaseLength = 1;
			memcpy(pException.vehicleName, packet.vehicleID, 10);
			addToExcpetionArray(pException, exceptions, exceptionsCnt);
		}
	}
}

//DETERMINE CROSSLEVEL - HIGH SPEED
void compareCrosslevelHighSpeed(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt)
{
	double xlHighSpeed = packet.crosslevel_OC;
	double excRLimit = -99;
	double excPLimit = -99;
	switch (packet.location.trackClass)
	{
	case 1:
		excPLimit = CLASS_1_PRI_CL_HS;
		excRLimit = CLASS_1_RED_CL_HS;
		break;
	case 2:
		excPLimit = CLASS_2_PRI_CL_HS;
		excRLimit = CLASS_2_RED_CL_HS;
		break;
	case 3:
		excPLimit = CLASS_3_PRI_CL_HS;
		excRLimit = CLASS_3_RED_CL_HS;
		break;
	case 4:
		excPLimit = CLASS_4_PRI_CL_HS;
		excRLimit = CLASS_4_RED_CL_HS;
		break;
	default:
		excPLimit = CLASS_4_PRI_CL_HS;
		excRLimit = CLASS_4_RED_CL_HS;
		break;
	}
	if (std::abs(xlHighSpeed) >= excRLimit)
	{
		//RED LETTER EXCEPTION
		WTGS_Exception rException = WTGS_Exception();
		rException.exceptionAmplitude = xlHighSpeed;
		rException.exceptionLength = 1;
		rException.exceptionSeverity = RED_EXCEPTION;
		rException.exceptionType = XL_HS;
		rException.max_location = packet.location;
		rException.latitude = packet.latitude;
		rException.longitude = packet.longitude;
		rException.altitude = packet.altitude;
		rException.speedTraveling = packet.speed;
		rException.secValidity = packet.secValidity;
		rException.exceptionBaseLength = 1;
		memcpy(rException.vehicleName, packet.vehicleID, 10);
		addToExcpetionArray(rException, exceptions, exceptionsCnt);

	}
	else
	{
		if (std::abs(xlHighSpeed) >= excPLimit)
		{
			//PRIORITY EXCEPTION
			WTGS_Exception pException = WTGS_Exception();
			pException.exceptionAmplitude = xlHighSpeed;
			pException.exceptionLength = 1;
			pException.exceptionSeverity = PRIORITY_EXCEPTION;
			pException.exceptionType = XL_HS;
			pException.max_location = packet.location;
			pException.latitude = packet.latitude;
			pException.longitude = packet.longitude;
			pException.altitude = packet.altitude;
			pException.speedTraveling = packet.speed;
			pException.secValidity = packet.secValidity;
			pException.exceptionBaseLength = 1;
			memcpy(pException.vehicleName, packet.vehicleID, 10);
			addToExcpetionArray(pException, exceptions, exceptionsCnt);
		}
	}
}

//DETERMINE CROSSLEVEL - VARIATION
void compareCrosslevelVariation(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int& exceptionsCnt)
{
	double excRLimit = -99;
	double excPLimit = -99;
	switch (packet.location.trackClass)
	{
	case 1:
		excPLimit = CLASS_1_PRI_CL_VAR;
		excRLimit = CLASS_1_RED_CL_VAR;
		break;
	case 2:
		excPLimit = CLASS_2_PRI_CL_VAR;
		excRLimit = CLASS_2_RED_CL_VAR;
		break;
	case 3:
		excPLimit = CLASS_3_PRI_CL_VAR;
		excRLimit = CLASS_3_RED_CL_VAR;
		break;
	case 4:
		excPLimit = CLASS_4_PRI_CL_VAR;
		excRLimit = CLASS_4_RED_CL_VAR;
		break;
	default:
		excPLimit = CLASS_4_PRI_CL_VAR;
		excRLimit = CLASS_4_RED_CL_VAR;
		break;
	}

	bool foundARed = false;
	bool foundAPri = false;
	bool foundAException = false;
	double maxValue = 0.0;
	int maxBase = 0;

	for (int i = 0; i < 43; i++)
	{
		double xlVariation = packet.variationCrosslevel[i];
		if (std::abs(xlVariation) >= excRLimit + ((double)(i + 20)) / 62.0)
		{
			if (foundARed)//ALREADY HAVE AT LEAST ONE RED VARIATION XL
			{
				if (std::abs(xlVariation) > std::abs(maxValue))
				{
					maxValue = xlVariation;
					maxBase = i + 20;
				}
			}
			else
			{
				maxValue = xlVariation;
				maxBase = i + 20;
			}

			foundARed = true;
			foundAPri = false;


		}
		else
		{
			if (std::abs(xlVariation) >= excPLimit + ((double)(i + 20)) / 62.0)
			{
				if (!foundARed)//HAVE ALREADY FOUND AT LEAST ONE RED EXC
				{
					if (foundAPri)//HAVE ALREADY FOUND AT LEAST ONE PRIORITY EXC
					{
						if (std::abs(xlVariation) > std::abs(maxValue))
						{
							maxValue = xlVariation;
							maxBase = i + 20;
						}
					}
					else
					{
						maxValue = xlVariation;
						maxBase = i + 20;
					}
					foundAPri = true;
				}
			}
		}
	}

	if (foundARed)
	{
		//RED LETTER EXCEPTION
		WTGS_Exception rException = WTGS_Exception();
		rException.exceptionAmplitude = maxValue;
		rException.exceptionBaseLength = maxBase;
		rException.exceptionLength = 1;
		rException.exceptionSeverity = RED_EXCEPTION;
		rException.exceptionType = XL_VARIATION;
		rException.max_location = packet.location;
		rException.latitude = packet.latitude;
		rException.longitude = packet.longitude;
		rException.altitude = packet.altitude;
		rException.speedTraveling = packet.speed;
		rException.secValidity = packet.secValidity;
		memcpy(rException.vehicleName, packet.vehicleID, 10);
		addToExcpetionArray(rException, exceptions, exceptionsCnt);
	}
	else
	{
		if (foundAPri)
		{
			//PRIORITY EXCEPTION
			WTGS_Exception pException = WTGS_Exception();
			pException.exceptionAmplitude = maxValue;
			pException.exceptionBaseLength = maxBase;
			pException.exceptionLength = 1;
			pException.exceptionSeverity = PRIORITY_EXCEPTION;
			pException.exceptionType = XL_VARIATION;
			pException.max_location = packet.location;
			pException.latitude = packet.latitude;
			pException.longitude = packet.longitude;
			pException.altitude = packet.altitude;
			pException.speedTraveling = packet.speed;
			pException.secValidity = packet.secValidity;
			memcpy(pException.vehicleName, packet.vehicleID, 10);
			addToExcpetionArray(pException, exceptions, exceptionsCnt);
		}
	}
}

//DETERMINE REVERSE CROSSLEVEL - NORMAL TYPE
void compareReverseCrosslevel_Norm(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt)
{
	double crosslevel = packet.crosslevel_OC;
	double curvature = packet.filteredCurvature;

	double excRLimit = -99;
	double excPLimit = -99;
	switch (packet.location.trackClass)
	{
	case 1:
		excPLimit = CLASS_1_PRI_RCL_NML;
		excRLimit = CLASS_1_RED_RCL_NML;
		break;
	case 2:
		excPLimit = CLASS_2_PRI_RCL_NML;
		excRLimit = CLASS_2_RED_RCL_NML;
		break;
	case 3:
		excPLimit = CLASS_3_PRI_RCL_NML;
		excRLimit = CLASS_3_RED_RCL_NML;
		break;
	case 4:
		excPLimit = CLASS_4_PRI_RCL_NML;
		excRLimit = CLASS_4_RED_RCL_NML;
		break;
	default:
		excPLimit = CLASS_4_PRI_RCL_NML;
		excRLimit = CLASS_4_RED_RCL_NML;
		break;
	}
	if (std::abs(crosslevel) >= excRLimit && std::abs(curvature) >= 1.0 && ((crosslevel > 0 && curvature < 0) || crosslevel < 0 && curvature > 0))
	{
		//RED LETTER EXCEPTION
		WTGS_Exception rException = WTGS_Exception();
		rException.exceptionAmplitude = crosslevel;
		rException.exceptionLength = 1;
		rException.exceptionSeverity = RED_EXCEPTION;
		rException.exceptionType = XL_REV_NML;
		rException.max_location = packet.location;
		rException.latitude = packet.latitude;
		rException.longitude = packet.longitude;
		rException.altitude = packet.altitude;
		rException.speedTraveling = packet.speed;
		rException.secValidity = packet.secValidity;
		rException.exceptionBaseLength = 1;
		memcpy(rException.vehicleName, packet.vehicleID, 10);
		addToExcpetionArray(rException, exceptions, exceptionsCnt);

	}
	else
	{
		if (std::abs(crosslevel) >= excPLimit && std::abs(curvature) >= 1.0 && ((crosslevel > 0 && curvature < 0) || crosslevel < 0 && curvature > 0))
		{
			//PRIORITY EXCEPTION
			WTGS_Exception pException = WTGS_Exception();
			pException.exceptionAmplitude = crosslevel;
			pException.exceptionLength = 1;
			pException.exceptionSeverity = PRIORITY_EXCEPTION;
			pException.exceptionType = XL_REV_NML;
			pException.max_location = packet.location;
			pException.latitude = packet.latitude;
			pException.longitude = packet.longitude;
			pException.altitude = packet.altitude;
			pException.speedTraveling = packet.speed;
			pException.secValidity = packet.secValidity;
			pException.exceptionBaseLength = 1;
			memcpy(pException.vehicleName, packet.vehicleID, 10);
			addToExcpetionArray(pException, exceptions, exceptionsCnt);
		}
	}

}

//DETERMINE REVERSE CROSSLEVE - 25FT LEVELS
void compareReverseCrosslevel_25(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt)
{
	double crosslevel = packet.crosslevel_OC;
	double curvature = packet.filteredCurvature;

	double excPLimit = -99;
	switch (packet.location.trackClass)
	{
	case 2:
		excPLimit = CLASS_2_PRI_RCL_25;
		break;
	case 3:
		excPLimit = CLASS_3_PRI_RCL_25;
		break;
	case 4:
		excPLimit = CLASS_4_PRI_RCL_25;
		break;
	default:
		excPLimit = CLASS_4_PRI_RCL_25;
		break;
	}
	if (std::abs(crosslevel) >= excPLimit && std::abs(curvature) >= 1.0 && ((crosslevel > 0 && curvature < 0) || crosslevel < 0 && curvature > 0))
	{
		//PRIORITY EXCEPTION
		WTGS_Exception pException = WTGS_Exception();
		pException.exceptionAmplitude = crosslevel;
		pException.exceptionLength = 1;
		pException.exceptionSeverity = PRIORITY_EXCEPTION;
		pException.exceptionType = XL_REV_25FT;
		pException.max_location = packet.location;
		pException.latitude = packet.latitude;
		pException.longitude = packet.longitude;
		pException.altitude = packet.altitude;
		pException.speedTraveling = packet.speed;
		pException.secValidity = packet.secValidity;
		pException.exceptionBaseLength = 1;
		memcpy(pException.vehicleName, packet.vehicleID, 10);
		addToExcpetionArray(pException, exceptions, exceptionsCnt);
	}
}

//DETERMINE FRA 62 FOOT ALIGNMENT
void compareFRA62Alignment(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt)
{
	double fra62AlignL = packet.relAlignmentLeft62_NOC;
	double fra62AlignR = packet.relAlignmentRight62_NOC;
	double excRLimit = -99;
	switch (packet.location.trackClass)
	{
	case 1:
		excRLimit = CLASS_1_FRA_ALIGN62;
		break;
	case 2:
		excRLimit = CLASS_2_FRA_ALIGN62;
		break;
	case 3:
		excRLimit = CLASS_3_FRA_ALIGN62;
		break;
	case 4:
		excRLimit = CLASS_4_FRA_ALIGN62;
		break;
	default:
		excRLimit = CLASS_4_FRA_ALIGN62;
		break;
	}

	//LEFT RAIL
	if (std::abs(fra62AlignL) >= excRLimit)
	{
		//RED LETTER EXCEPTION
		WTGS_Exception rException = WTGS_Exception();
		rException.exceptionAmplitude = fra62AlignL;
		rException.exceptionLength = 1;
		rException.exceptionSeverity = RED_EXCEPTION;
		rException.exceptionType = FRA62_ALIGNMENT_L;
		rException.max_location = packet.location;
		rException.latitude = packet.latitude;
		rException.longitude = packet.longitude;
		rException.altitude = packet.altitude;
		rException.speedTraveling = packet.speed;
		rException.secValidity = packet.secValidity;
		rException.exceptionBaseLength = 1;
		memcpy(rException.vehicleName, packet.vehicleID, 10);
		addToExcpetionArray(rException, exceptions, exceptionsCnt);
	}

	//RAIL RAIL
	if (std::abs(fra62AlignR) >= excRLimit)
	{
		//RED LETTER EXCEPTION
		WTGS_Exception rException = WTGS_Exception();
		rException.exceptionAmplitude = fra62AlignR;
		rException.exceptionLength = 1;
		rException.exceptionSeverity = RED_EXCEPTION;
		rException.exceptionType = FRA62_ALIGNMENT_R;
		rException.max_location = packet.location;
		rException.latitude = packet.latitude;
		rException.longitude = packet.longitude;
		rException.altitude = packet.altitude;
		rException.speedTraveling = packet.speed;
		rException.secValidity = packet.secValidity;
		rException.exceptionBaseLength = 1;
		memcpy(rException.vehicleName, packet.vehicleID, 10);
		addToExcpetionArray(rException, exceptions, exceptionsCnt);
	}
}

//DETERMINE FRA 31 FOOT ALIGNMENT
void compareFRA31Alignment(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt)
{
	double fra31AlignL = packet.relAlignmentLeft31_NOC;
	double fra31AlignR = packet.relAlignmentRight31_NOC;
	double excRLimit = -99;
	switch (packet.location.trackClass)
	{
	case 3:
		excRLimit = CLASS_3_FRA_ALIGN31;
		break;
	case 4:
		excRLimit = CLASS_4_FRA_ALIGN31;
		break;
	default:
		excRLimit = CLASS_4_FRA_ALIGN31;
		break;
	}

	//LEFT RAIL
	if (std::abs(fra31AlignL) >= excRLimit)
	{
		//RED LETTER EXCEPTION
		WTGS_Exception rException = WTGS_Exception();
		rException.exceptionAmplitude = fra31AlignL;
		rException.exceptionLength = 1;
		rException.exceptionSeverity = RED_EXCEPTION;
		rException.exceptionType = FRA31_ALIGNMENT_L;
		rException.max_location = packet.location;
		rException.latitude = packet.latitude;
		rException.longitude = packet.longitude;
		rException.altitude = packet.altitude;
		rException.speedTraveling = packet.speed;
		rException.secValidity = packet.secValidity;
		rException.exceptionBaseLength = 1;
		memcpy(rException.vehicleName, packet.vehicleID, 10);
		addToExcpetionArray(rException, exceptions, exceptionsCnt);
	}

	//RAIL RAIL
	if (std::abs(fra31AlignR) >= excRLimit)
	{
		//RED LETTER EXCEPTION
		WTGS_Exception rException = WTGS_Exception();
		rException.exceptionAmplitude = fra31AlignR;
		rException.exceptionLength = 1;
		rException.exceptionSeverity = RED_EXCEPTION;
		rException.exceptionType = FRA31_ALIGNMENT_R;
		rException.max_location = packet.location;
		rException.latitude = packet.latitude;
		rException.longitude = packet.longitude;
		rException.altitude = packet.altitude;
		rException.speedTraveling = packet.speed;
		rException.secValidity = packet.secValidity;
		rException.exceptionBaseLength = 1;
		memcpy(rException.vehicleName, packet.vehicleID, 10);
		addToExcpetionArray(rException, exceptions, exceptionsCnt);
	}
}

//DETERMINE FRA 31 FOOT RUNOFF
void compareFRA31Runoff(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt)
{
	double fra31Runoff = packet.fraRunoff;
	double excRLimit = -99;
	switch (packet.location.trackClass)
	{
	case 1:
		excRLimit = CLASS_1_FRA_RUNOFF;
		break;
	case 2:
		excRLimit = CLASS_2_FRA_RUNOFF;
		break;
	case 3:
		excRLimit = CLASS_3_FRA_RUNOFF;
		break;
	case 4:
		excRLimit = CLASS_4_FRA_RUNOFF;
		break;
	default:
		excRLimit = CLASS_4_FRA_RUNOFF;
		break;
	}

	if (std::abs(fra31Runoff) >= excRLimit)
	{
		//RED LETTER EXCEPTION
		WTGS_Exception rException = WTGS_Exception();
		rException.exceptionAmplitude = fra31Runoff;
		rException.exceptionLength = 1;
		rException.exceptionSeverity = RED_EXCEPTION;
		rException.exceptionType = FRA_RUNOFF;
		rException.max_location = packet.location;
		rException.latitude = packet.latitude;
		rException.longitude = packet.longitude;
		rException.altitude = packet.altitude;
		rException.speedTraveling = packet.speed;
		rException.secValidity = packet.secValidity;
		rException.exceptionBaseLength = 1;
		memcpy(rException.vehicleName, packet.vehicleID, 10);
		addToExcpetionArray(rException, exceptions, exceptionsCnt);
	}
}

//DETERMINE FRA 62 FOOT PROFILE
void compareFRA62Profile(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt)
{
	double fra62Profile_L = packet.L_VERT_MCO_62_OC;
	double fra62Profile_R = packet.R_VERT_MCO_62_OC;
	double excRLimit = -99;
	switch (packet.location.trackClass)
	{
	case 1:
		excRLimit = CLASS_1_FRA_62PROFILE;
		break;
	case 2:
		excRLimit = CLASS_2_FRA_62PROFILE;
		break;
	case 3:
		excRLimit = CLASS_3_FRA_62PROFILE;
		break;
	case 4:
		excRLimit = CLASS_4_FRA_62PROFILE;
		break;
	default:
		excRLimit = CLASS_4_FRA_62PROFILE;
		break;
	}

	//LEFT RAIL
	if (std::abs(fra62Profile_L) >= excRLimit)
	{
		//RED LETTER EXCEPTION
		WTGS_Exception rException = WTGS_Exception();
		rException.exceptionAmplitude = fra62Profile_L;
		rException.exceptionLength = 1;
		rException.exceptionSeverity = RED_EXCEPTION;
		rException.exceptionType = FRA62_PROFILE_L;
		rException.max_location = packet.location;
		rException.latitude = packet.latitude;
		rException.longitude = packet.longitude;
		rException.altitude = packet.altitude;
		rException.speedTraveling = packet.speed;
		rException.secValidity = packet.secValidity;
		rException.exceptionBaseLength = 1;
		memcpy(rException.vehicleName, packet.vehicleID, 10);
		addToExcpetionArray(rException, exceptions, exceptionsCnt);
	}

	//RIGHT RAIL
	if (std::abs(fra62Profile_R) >= excRLimit)
	{
		//RED LETTER EXCEPTION
		WTGS_Exception rException = WTGS_Exception();
		rException.exceptionAmplitude = fra62Profile_R;
		rException.exceptionLength = 1;
		rException.exceptionSeverity = RED_EXCEPTION;
		rException.exceptionType = FRA62_PROFILE_R;
		rException.max_location = packet.location;
		rException.latitude = packet.latitude;
		rException.longitude = packet.longitude;
		rException.altitude = packet.altitude;
		rException.speedTraveling = packet.speed;
		rException.secValidity = packet.secValidity;
		rException.exceptionBaseLength = 1;
		memcpy(rException.vehicleName, packet.vehicleID, 10);
		addToExcpetionArray(rException, exceptions, exceptionsCnt);
	}
}

//DETERMINE FRA VARIATION FROM ZERO CROSS LEVEL
void compareFRAVar0Crosslevel(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt)
{
	double curvature = packet.filteredCurvature;
	double crosslevel = packet.crosslevel_OC;
	double excRLimit = -99;
	switch (packet.location.trackClass)
	{
	case 1:
		excRLimit = CLASS_1_FRA_V0CL;
		break;
	case 2:
		excRLimit = CLASS_2_FRA_V0CL;
		break;
	case 3:
		excRLimit = CLASS_3_FRA_V0CL;
		break;
	case 4:
		excRLimit = CLASS_4_FRA_V0CL;
		break;
	default:
		excRLimit = CLASS_4_FRA_V0CL;
		break;
	}

	if (std::abs(crosslevel) >= excRLimit && std::abs(curvature) < 0.01 && std::abs(curvature) > -0.01)
	{
		//RED LETTER EXCEPTION
		WTGS_Exception rException = WTGS_Exception();
		rException.exceptionAmplitude = crosslevel;
		rException.exceptionLength = 1;
		rException.exceptionSeverity = RED_EXCEPTION;
		rException.exceptionType = FRA_VARIATION0_XL;
		rException.max_location = packet.location;
		rException.latitude = packet.latitude;
		rException.longitude = packet.longitude;
		rException.altitude = packet.altitude;
		rException.speedTraveling = packet.speed;
		rException.secValidity = packet.secValidity;
		rException.exceptionBaseLength = 1;
		memcpy(rException.vehicleName, packet.vehicleID, 10);
		addToExcpetionArray(rException, exceptions, exceptionsCnt);
	}
}

//DETERMINE FRA WARP 62
void compareFRAWarp62(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt)
{
	//double fraWarp62 = packet.fraWarp62;
	double fraWarp62 = packet.fraWarp62;
	double excRLimit = -99;
	switch (packet.location.trackClass)
	{
	case 1:
		excRLimit = CLASS_1_FRA_WARP62;
		break;
	case 2:
		excRLimit = CLASS_2_FRA_WARP62;
		break;
	case 3:
		excRLimit = CLASS_3_FRA_WARP62;
		break;
	case 4:
		excRLimit = CLASS_4_FRA_WARP62;
		break;
	default:
		excRLimit = CLASS_4_FRA_WARP62;
		break;
	}

	if (std::abs(fraWarp62) >= excRLimit)
	{
		//RED LETTER EXCEPTION
		WTGS_Exception rException = WTGS_Exception();
		rException.exceptionAmplitude = fraWarp62;
		rException.exceptionBaseLength = std::abs(packet.fraWarp62_BaseLength);
		rException.exceptionLength = 1;
		rException.exceptionSeverity = RED_EXCEPTION;
		rException.exceptionType = FRA62_WARP;
		rException.max_location = packet.location;
		rException.latitude = packet.latitude;
		rException.longitude = packet.longitude;
		rException.altitude = packet.altitude;
		rException.speedTraveling = packet.speed;
		rException.secValidity = packet.secValidity;
		rException.exceptionBaseLength = 1;
		memcpy(rException.vehicleName, packet.vehicleID, 10);
		addToExcpetionArray(rException, exceptions, exceptionsCnt);
	}
}

//DETERMINE FRA REVERSE CROSS LEVEL
void compareFRAReverseCrosslevel(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int &exceptionsCnt)
{
	double crosslevel = packet.crosslevel_OC;
	double curvature = packet.filteredCurvature;
	double excRLimit = -99;
	switch (packet.location.trackClass)
	{
	case 1:
		excRLimit = CLASS_1_FRA_RCL;
		break;
	case 2:
		excRLimit = CLASS_2_FRA_RCL;
		break;
	case 3:
		excRLimit = CLASS_3_FRA_RCL;
		break;
	case 4:
		excRLimit = CLASS_4_FRA_RCL;
		break;
	default:
		excRLimit = CLASS_4_FRA_RCL;
		break;
	}

	if ( std::abs(curvature) >= 0.2 && std::abs(crosslevel) >= excRLimit && ((crosslevel > 0 && curvature < 0) || crosslevel < 0 && curvature > 0))
	{
		//RED LETTER EXCEPTION
		WTGS_Exception rException = WTGS_Exception();
		rException.exceptionAmplitude = crosslevel;
		rException.exceptionLength = 1;
		rException.exceptionSeverity = RED_EXCEPTION;
		rException.exceptionType = FRA_REV_XL;
		rException.max_location = packet.location;
		rException.latitude = packet.latitude;
		rException.longitude = packet.longitude;
		rException.altitude = packet.altitude;
		rException.speedTraveling = packet.speed;
		rException.secValidity = packet.secValidity;
		rException.exceptionBaseLength = 1;
		memcpy(rException.vehicleName, packet.vehicleID, 10);
		addToExcpetionArray(rException, exceptions, exceptionsCnt);
	}
}

//DETERMINE FRA HARMONIC ROCKOFF
void compareFRAHarmonicRockoff(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int& exceptionsCnt)
{
	double fraHarmonic = packet.fraHarmonic;
	double excRLimit = -99;
	switch (packet.location.trackClass)
	{
	case 1:
		excRLimit = CLASS_1_FRA_HARMONIC;
		break;
	case 2:
		excRLimit = CLASS_2_FRA_HARMONIC;
		break;
	case 3:
		excRLimit = CLASS_3_FRA_HARMONIC;
		break;
	case 4:
		excRLimit = CLASS_4_FRA_HARMONIC;
		break;
	default:
		excRLimit = CLASS_4_FRA_HARMONIC;
		break;
	}

	if (std::abs(fraHarmonic) >= excRLimit )
	{
		//RED LETTER EXCEPTION
		WTGS_Exception rException = WTGS_Exception();
		rException.exceptionAmplitude = fraHarmonic;
		rException.exceptionLength = 1;
		rException.exceptionSeverity = RED_EXCEPTION;
		rException.exceptionType = FRA_HARMONIC;
		rException.max_location = packet.location;
		rException.latitude = packet.latitude;
		rException.longitude = packet.longitude;
		rException.altitude = packet.altitude;
		rException.speedTraveling = packet.speed;
		rException.secValidity = packet.secValidity;
		rException.exceptionBaseLength = 1;
		memcpy(rException.vehicleName, packet.vehicleID, 10);
		addToExcpetionArray(rException, exceptions, exceptionsCnt);
	}
}

//DETER FRA EXCESS ELEVATION
void compareFRAExcessElevation(CoreGeomCalculationPacket packet, WTGS_Exception* exceptions, int& exceptionsCnt)
{
	double crosslevel = packet.crosslevel_OC;
	double excRLimit = -99;
	switch (packet.location.trackClass)
	{
	case 1:
		excRLimit = CLASS_1_FRA_EXCESSELEVATION;
		break;
	case 2:
		excRLimit = CLASS_2_FRA_EXCESSELEVATION;
		break;
	case 3:
		excRLimit = CLASS_3_FRA_EXCESSELEVATION;
		break;
	case 4:
		excRLimit = CLASS_4_FRA_EXCESSELEVATION;
		break;
	default:
		excRLimit = CLASS_4_FRA_EXCESSELEVATION;
		break;
	}

	if (std::abs(crosslevel) >= excRLimit)
	{
		//RED LETTER EXCEPTION
		WTGS_Exception rException = WTGS_Exception();
		rException.exceptionAmplitude = crosslevel;
		rException.exceptionLength = 1;
		rException.exceptionSeverity = RED_EXCEPTION;
		rException.exceptionType = FRA_EXCESS_ELEV;
		rException.max_location = packet.location;
		rException.latitude = packet.latitude;
		rException.longitude = packet.longitude;
		rException.altitude = packet.altitude;
		rException.speedTraveling = packet.speed;
		rException.secValidity = packet.secValidity;
		rException.exceptionBaseLength = 1;
		memcpy(rException.vehicleName, packet.vehicleID, 10);
		addToExcpetionArray(rException, exceptions, exceptionsCnt);
	}
}
