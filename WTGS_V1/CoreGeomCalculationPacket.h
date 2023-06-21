#pragma once
#include "TGC_Location.h"

#pragma pack(1)
struct CoreGeomCalculationPacket
{
	//--POPULATED BY CORE GEOMETRY--

	//TIME OF VALIDITY -- THE CENTER POINT
	double secValidity;

	//GPS COORDS
	double latitude;
	double longitude;
	double altitude;
	double speed;

	MP_LOCATION location;

	//CROSSLEVEL
	double crosslevel_NOC;
	double crosslevel_OC;
	double crosslevel_OC_SWA;

	//GRADE
	double grade_raw;
	double grade_filtered;

	//VERTICAL MCOs 11/31/62 FOOT - RIGHT / LEFT RAILS
	//NO OPTICAL COMPENSATION
	double L_VERT_MCO_11_NOC;
	double R_VERT_MCO_11_NOC;
	double L_VERT_MCO_31_NOC;
	double R_VERT_MCO_31_NOC;
	double L_VERT_MCO_62_NOC;
	double R_VERT_MCO_62_NOC;
	//OPTICAL COMMPENSATION
	double L_VERT_MCO_11_OC;
	double R_VERT_MCO_11_OC;
	double L_VERT_MCO_31_OC;
	double R_VERT_MCO_31_OC;
	double L_VERT_MCO_62_OC;
	double R_VERT_MCO_62_OC;


	//HORIZONTAL MCOs 11/31/62 FOOT - RIGHT / LEFT RAILS
	//NO OPTICAL COMPENSATION
	double L_HORIZ_MCO_31_NOC;
	double R_HORIZ_MCO_31_NOC;
	double L_HORIZ_MCO_62_NOC;
	double R_HORIZ_MCO_62_NOC;
	double L_HORIZ_MCO_124_NOC;
	double R_HORIZ_MCO_124_NOC;
	//OPTICAL COMPENSATION
	double L_HORIZ_MCO_31_OC;
	double R_HORIZ_MCO_31_OC;
	double L_HORIZ_MCO_62_OC;
	double R_HORIZ_MCO_62_OC;
	double L_HORIZ_MCO_124_OC;
	double R_HORIZ_MCO_124_OC;

	//CURVATURE - 140FT (PRE-FIR FILTERED)
	double rawCurvature;
	double filteredCurvature;

	//GAGE
	double fullGage_raw;
	double fullGage_filtered;
	double rHalfGage;
	double rHalfGage_filtered;
	double lHalfGage;
	double lHalfGage_filtered;
	double rVertGage;
	double rVertGage_filtered;
	double lVertGage;
	double lVertGage_filtered;
	double variationGage;

	//POPULATED BY EXCEPTION ANALYSIS
	bool exceptionAnalysisComplete = false;

	//--POPULATED BY GEOCALCULATIONS--
	bool secondCalcsComplete = false;

	double twist11;
	double twist11_SWA;
	double twist31;
	double multipleTwist11 = 0;
	double verticalBounce;
	double variationCrosslevel[43];
	double variationCrosslevel_SWA[43];
	double fraRunoff;
	double fraWarp62;
	double fraWarp62_SWA;
	double fraWarp62_BaseLength;
	double combinationLeft;
	double combinationRight;
	double variationCurvature;
	double variationCurvature_BaseLength;
	double relAlignmentRight62_NOC;
	double relAlignmentRight62_OC;
	double relAlignmentLeft62_NOC;
	double relAlignmentLeft62_OC;
	double relAlignmentRight31_NOC;
	double relAlignmentRight31_OC;
	double relAlignmentLeft31_NOC;
	double relAlignmentLeft31_OC;
	double fraHarmonic = 0;
	char vehicleID[10];
	int measuringDirection = 0;

};
