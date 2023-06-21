#include "stdafx.h"
#include "CoreGeometry_A.h"

//#pragma optimize("", off)

//PROTOTYPES
double calcCrosslevel1FT_A(int geomPacketIndex, bool enableOC, double &crosslevel_SWA);
double calcGrade1FT_A(int geomPacketIndex);
bool calcHorizRailAlignmentMCO_A_gyro(int geomPacketIndex, int half_length, int full_length, double &leftRailMCO, double &rightRailMCO, bool enableOC);
bool calcVertRailAlignmentMCO_A_gyro(int geomPacketIndex, int half_length, int full_length, double &leftRailMCO, double &rightRailMCO, bool enableOC);
bool calcVertRailAlignmentMCO_A_accel(int geomPacketIndex, int half_length, int full_length, double &leftRailMCO, double &rightRailMCO, bool enableOC);
bool calcFilteredGage(int geomPacketIndex, double &filteredGage);
bool calcFilteredHG_and_verticals(int geomPacketIndex, double& leftHG, double& rightHG, double& leftVG, double& rightVG);
bool calcCurvature_A(int geomPacketIndex, double &curvature);
bool insertNewCalcPacket_A(CoreGeomCalculationPacket p);
bool insertGageData(GageGeometry_Data data);
bool getAndInsertGageData();

//GLOBAL INFORMATION
char cgVehicleID[10];

//SUMMARY - 1 FOOT VALUES FOR 256 FEET
//## INPUT BUFFER -- KIND OF ##//
CoreGeomPacket_A geomPacket_256Buffer[512];
std::mutex geomPacket_256Mutex;
int geomPacket_256Cnt;
std::condition_variable condVar_256BufferNewFoot;

//CALCULATED DATA PACKET FOR EACH 1 FT -- MCO / CURVATURE / GRADE / CROSSLEVEL
//## THIS IS THE OUTPUT BUFFER ##//
std::queue<CoreGeomCalculationPacket> calcPacket_buffer;
std::mutex calcPacket_Mutex;
int calcPacket_512Cnt;
std::condition_variable condVar_calcPacketBuffer;

//INCOMING GAGE DATA QUEUE
std::queue<GageGeometry_Data> gagePacket_buffer;
std::mutex gagePacket_mutex;
std::condition_variable gagePacket_condVar;

//FLAGS FOR INFINITE LOOPS
bool coreGeometryEnable = true;

//CONSTRUCTORS
CoreGeometry_A::CoreGeometry_A()
{
	geomPacket_256Cnt = 0;
	memset(geomPacket_256Buffer, 0, sizeof(geomPacket_256Buffer));
	int buffSize = calcPacket_buffer.size();
	for (int i = 0; i < buffSize; i++)
	{
		calcPacket_buffer.pop();
	}
	buffSize = gagePacket_buffer.size();
	for (int i = 0; i < buffSize; i++)
	{
		gagePacket_buffer.pop();
	}
	coreGeometryEnable = true;
}

CoreGeometry_A::~CoreGeometry_A()
{
}

//CONTROLLER LOOP FOR CORE GEOMETRY CALCULATIONS
void CoreGeometry_A::controllerThread_CoreCalcs_A(int setupMeasureDirection)
{
	//KEEP CHECKING FOR 'COMPLETE' 512 packets to run CORE GEOMETRY CALCULATIONS ON
	double coreCalcMeasureDirection = 1.0;
	if (setupMeasureDirection == -1)
	{
		coreCalcMeasureDirection = -1.0;
	}
	while (coreGeometryEnable)
	{
		std::unique_lock<std::mutex> l(geomPacket_256Mutex);
		condVar_256BufferNewFoot.wait_for(l, std::chrono::milliseconds(100));
		double needToUpdateTimes[512];
		int needToUpdateCnt = 0;

		//CALCULATIONS FOR FIRST 62 DATA POINTS -- ONLY WAIT FOR GAGE
		if (geomPacket_256Cnt < 512)
		{
			for (int i = geomPacket_256Cnt - 1; i > geomPacket_256Cnt - 67 - 1 && i >= 0; i--)
			{
				if (!geomPacket_256Buffer[i].CoreGeom_complete && geomPacket_256Buffer[i].gageData_complete)
				{
					geomPacket_256Buffer[i].CoreGeom_complete = true;

					double notused = 0.0;
					double crosslevel_NOC = calcCrosslevel1FT_A(i, false, notused);
					double crosslevel_OC;
					double crosslevel_OC_SWA;

					CoreGeomCalculationPacket coreCalcs = CoreGeomCalculationPacket();
					//ASSIGN THE VEHICLE ID
					memcpy(coreCalcs.vehicleID, cgVehicleID, 10);

					coreCalcs.fullGage_raw = geomPacket_256Buffer[i].gageData.full_gage;
					coreCalcs.lHalfGage = geomPacket_256Buffer[i].gageData.left_width;
					coreCalcs.rHalfGage = geomPacket_256Buffer[i].gageData.right_width;
					coreCalcs.lVertGage = geomPacket_256Buffer[i].gageData.left_height;
					coreCalcs.rVertGage = geomPacket_256Buffer[i].gageData.right_height;
					coreCalcs.fullGage_filtered = 0;
					crosslevel_OC = calcCrosslevel1FT_A(i, false, crosslevel_OC_SWA);

					double grade = calcGrade1FT_A(i);

					coreCalcs.latitude = geomPacket_256Buffer[i].buffer_AG1[0].latitude;
					coreCalcs.longitude = geomPacket_256Buffer[i].buffer_AG1[0].longitude;
					coreCalcs.altitude = geomPacket_256Buffer[i].buffer_AG1[0].altitude;
					coreCalcs.location = geomPacket_256Buffer[i].location;
					coreCalcs.speed = geomPacket_256Buffer[i].buffer_AG1[0].speed;

					coreCalcs.secValidity = geomPacket_256Buffer[i].oneFootPacket.header.time1;

					coreCalcs.crosslevel_NOC = crosslevel_NOC;
					coreCalcs.grade_raw = grade;
					coreCalcs.L_HORIZ_MCO_31_NOC = 0.0;
					coreCalcs.R_HORIZ_MCO_31_NOC = 0.0;
					coreCalcs.L_HORIZ_MCO_62_NOC = 0.0;
					coreCalcs.R_HORIZ_MCO_62_NOC = 0.0;
					coreCalcs.L_HORIZ_MCO_124_NOC = 0.0;
					coreCalcs.R_HORIZ_MCO_124_NOC = 0.0;

					coreCalcs.L_VERT_MCO_11_NOC = 0.0;
					coreCalcs.R_VERT_MCO_11_NOC = 0.0;
					coreCalcs.L_VERT_MCO_31_NOC = 0.0;
					coreCalcs.R_VERT_MCO_31_NOC = 0.0;
					coreCalcs.L_VERT_MCO_62_NOC = 0.0;
					coreCalcs.R_VERT_MCO_62_NOC = 0.0;

					coreCalcs.rawCurvature = 0.0;

					insertNewCalcPacket_A(coreCalcs);
				}
			}
		}

		for(int i = geomPacket_256Cnt - 67 - 1; i >= 67; i--)
		{
			if (!geomPacket_256Buffer[i].CoreGeom_complete)
			{
				bool allAG1_Complete = true;
				for (int j = i - 67; j < i + 67; j++)
				{
					if (!geomPacket_256Buffer[j].AG1_complete || !geomPacket_256Buffer[j].gageData_complete )
					{
						allAG1_Complete = false;
						j = 9999;
					}
				}
				if (allAG1_Complete)
				{
					//THE GEO PACKET IS WITHIN OUR 1/2 calculation widths &&
					//THE GEO PACKET HASN'T HAD CALCULATIONS PERFORMED ON IT YET &&
					//THE GEO PACKET HAS COMPLETED RECEPTION OF INSSPDS / INSPVAS PACKETS
					geomPacket_256Buffer[i].CoreGeom_complete = true;

					double notusedswa;
					double crosslevel_NOC = calcCrosslevel1FT_A(i, false, notusedswa);
					double grade = calcGrade1FT_A(i);

					double crosslevel_OC;
					double crosslevel_OC_SWA;
					//NO OPTICAL COMP
					double lVert11_NOC;
					double lVert31_NOC;
					double lVert62_NOC;
					double rVert11_NOC;
					double rVert31_NOC;
					double rVert62_NOC;

					//OPTICAL COMP
					double lVert11_OC;
					double lVert31_OC;
					double lVert62_OC;
					double rVert11_OC;
					double rVert31_OC;
					double rVert62_OC;

					//NO OPTICAL COMP
					double lHoriz124_NOC;
					double lHoriz31_NOC;
					double lHoriz62_NOC;
					double rHoriz124_NOC;
					double rHoriz31_NOC;
					double rHoriz62_NOC;

					//OPTICAL COMP
					double lHoriz124_OC;
					double lHoriz31_OC;
					double lHoriz62_OC;
					double rHoriz124_OC;
					double rHoriz31_OC;
					double rHoriz62_OC;

					double curvature;

					//FILTERED GAGE
					double filteredGage;

					//FILTERED HGs
					double left_hg_filt;
					double right_hg_filt;
					double left_vert_filt;
					double right_vert_filt;


					bool goodHorizMCO31_NOC = calcHorizRailAlignmentMCO_A_gyro(i, 15, 31, lHoriz31_NOC, rHoriz31_NOC, false);
					lHoriz31_NOC = lHoriz31_NOC * coreCalcMeasureDirection;
					rHoriz31_NOC = rHoriz31_NOC * coreCalcMeasureDirection;
					bool goodHorizMCO62_NOC = calcHorizRailAlignmentMCO_A_gyro(i, 31, 63, lHoriz62_NOC, rHoriz62_NOC, false) * coreCalcMeasureDirection;//ACTUALLY DOING 63' ALIGNMENT...NOT 62
					lHoriz62_NOC = lHoriz62_NOC * coreCalcMeasureDirection;
					rHoriz62_NOC = rHoriz62_NOC * coreCalcMeasureDirection;
					bool goodHorizMCO124_NOC = calcHorizRailAlignmentMCO_A_gyro(i, 62, 125, lHoriz124_NOC, rHoriz124_NOC, false) * coreCalcMeasureDirection;//ACTUALLY DOING 125' ALIGNMENT..NOT 124
					lHoriz124_NOC = lHoriz124_NOC * coreCalcMeasureDirection;
					rHoriz124_NOC = rHoriz124_NOC * coreCalcMeasureDirection;

					bool goodVertMCO11_NOC = calcVertRailAlignmentMCO_A_accel(i, 5, 10, lVert11_NOC, rVert11_NOC, false);
					bool goodVertMCO31_NOC = calcVertRailAlignmentMCO_A_accel(i, 15, 30, lVert31_NOC, rVert31_NOC, false);
					bool goodVertMCO62_NOC = calcVertRailAlignmentMCO_A_accel(i, 31, 62, lVert62_NOC, rVert62_NOC, false);//ACTUALLY DOING 61' ALIGNMENT...NOT 62

					bool goodCurvature = calcCurvature_A(i, curvature);//THIS IS PRE-140' FIR FILTER
					curvature = curvature * coreCalcMeasureDirection;

					CoreGeomCalculationPacket coreCalcs = CoreGeomCalculationPacket();
					//ASSIGN THE VEHICLE ID
					memcpy(coreCalcs.vehicleID, cgVehicleID, 10);

					coreCalcs.fullGage_raw = geomPacket_256Buffer[i].gageData.full_gage;
					coreCalcs.lHalfGage = geomPacket_256Buffer[i].gageData.left_width;
					coreCalcs.rHalfGage = geomPacket_256Buffer[i].gageData.right_width;
					coreCalcs.lVertGage = geomPacket_256Buffer[i].gageData.left_height;
					coreCalcs.rVertGage = geomPacket_256Buffer[i].gageData.right_height;

					bool goodFilteredGage = calcFilteredGage(i, filteredGage);
					coreCalcs.fullGage_filtered = filteredGage;

					bool gootHGFiltering = calcFilteredHG_and_verticals(i, left_hg_filt, right_hg_filt, left_vert_filt, right_vert_filt);
					coreCalcs.lHalfGage_filtered = left_hg_filt;
					coreCalcs.rHalfGage_filtered = right_hg_filt;
					coreCalcs.lVertGage_filtered = left_vert_filt;
					coreCalcs.rVertGage_filtered = right_vert_filt;

					crosslevel_OC = calcCrosslevel1FT_A(i, true, crosslevel_OC_SWA);
					bool goodHorizMCO31_OC = calcHorizRailAlignmentMCO_A_gyro(i, 15, 31, lHoriz31_OC, rHoriz31_OC, true) * coreCalcMeasureDirection;
					lHoriz31_OC = lHoriz31_OC * coreCalcMeasureDirection;
					rHoriz31_OC = rHoriz31_OC * coreCalcMeasureDirection;
					bool goodHorizMCO62_OC = calcHorizRailAlignmentMCO_A_gyro(i, 31, 63, lHoriz62_OC, rHoriz62_OC, true) * coreCalcMeasureDirection;//ACTUALLY DOING 63' ALIGNMENT...NOT 62
					lHoriz62_OC = lHoriz62_OC * coreCalcMeasureDirection;
					rHoriz62_OC = rHoriz62_OC * coreCalcMeasureDirection;
					bool goodHorizMCO124_OC = calcHorizRailAlignmentMCO_A_gyro(i, 62, 125, lHoriz124_OC, rHoriz124_OC, true) * coreCalcMeasureDirection;//ACTUALLY DOING 125' ALIGNMENT..NOT 124
					lHoriz124_OC = lHoriz124_OC * coreCalcMeasureDirection;
					rHoriz124_OC = rHoriz124_OC * coreCalcMeasureDirection;

					bool goodVertMCO11_OC = calcVertRailAlignmentMCO_A_accel(i, 5, 10, lVert11_OC, rVert11_OC, true);
					bool goodVertMCO31_OC = calcVertRailAlignmentMCO_A_accel(i, 15, 30, lVert31_OC, rVert31_OC, true);
					bool goodVertMCO62_OC = calcVertRailAlignmentMCO_A_accel(i, 31, 62, lVert62_OC, rVert62_OC, true);//ACTUALLY DOING 61' ALIGNMENT...NOT 62

					coreCalcs.latitude = geomPacket_256Buffer[i].buffer_AG1[0].latitude;
					coreCalcs.longitude = geomPacket_256Buffer[i].buffer_AG1[0].longitude;
					coreCalcs.altitude = geomPacket_256Buffer[i].buffer_AG1[0].altitude;
					coreCalcs.location = geomPacket_256Buffer[i].location;
					coreCalcs.speed = geomPacket_256Buffer[i].buffer_AG1[0].speed;

					coreCalcs.secValidity = geomPacket_256Buffer[i].oneFootPacket.header.time1;

					coreCalcs.crosslevel_NOC = crosslevel_NOC;
					coreCalcs.crosslevel_OC = crosslevel_OC;
					coreCalcs.crosslevel_OC_SWA = crosslevel_OC_SWA;
					coreCalcs.grade_raw = grade;
					coreCalcs.L_HORIZ_MCO_31_NOC = lHoriz31_NOC;
					coreCalcs.L_HORIZ_MCO_31_OC = lHoriz31_OC;
					coreCalcs.R_HORIZ_MCO_31_NOC = rHoriz31_NOC;
					coreCalcs.R_HORIZ_MCO_31_OC = rHoriz31_OC;
					coreCalcs.L_HORIZ_MCO_62_NOC = lHoriz62_NOC;
					coreCalcs.L_HORIZ_MCO_62_OC =  lHoriz62_OC;
					coreCalcs.R_HORIZ_MCO_62_NOC = rHoriz62_NOC;
					coreCalcs.R_HORIZ_MCO_62_OC =  rHoriz62_OC;
					coreCalcs.L_HORIZ_MCO_124_NOC = lHoriz124_NOC;
					coreCalcs.L_HORIZ_MCO_124_OC = lHoriz124_OC;
					coreCalcs.R_HORIZ_MCO_124_NOC = rHoriz124_NOC;
					coreCalcs.R_HORIZ_MCO_124_OC = rHoriz124_OC;

					coreCalcs.L_VERT_MCO_11_NOC = lVert11_NOC;
					coreCalcs.L_VERT_MCO_11_OC = lVert11_OC;
					coreCalcs.R_VERT_MCO_11_NOC = rVert11_NOC;
					coreCalcs.R_VERT_MCO_11_OC = rVert11_OC;
					coreCalcs.L_VERT_MCO_31_NOC = lVert31_NOC;
					coreCalcs.L_VERT_MCO_31_OC = lVert31_OC;
					coreCalcs.R_VERT_MCO_31_NOC = rVert31_NOC;
					coreCalcs.R_VERT_MCO_31_OC = rVert31_OC;
					coreCalcs.L_VERT_MCO_62_NOC = lVert62_NOC;
					coreCalcs.L_VERT_MCO_62_OC = lVert62_OC;
					coreCalcs.R_VERT_MCO_62_NOC = rVert62_NOC;
					coreCalcs.R_VERT_MCO_62_OC = rVert62_OC;

					coreCalcs.rawCurvature = curvature;

					insertNewCalcPacket_A(coreCalcs);
				}
			}
			else
			{
				//THE REMAINING PACKETS SHOULD BE 'COMPLETE'
				//i = 9999;
			}
		}
		l.unlock();
	}
}

//PLACE THE COMPLETED 'NEW FOOT' INTO THE 256 FOOT BUFFER AND EMPTY THE CURRENT FOOT BUFFER
void CoreGeometry_A::newFoot_A(CoreGeomPacket_A dataStruct, MP_LOCATION location)
{
	//POSITION 0 IS THE NEWEST DATA

	//SHIFT BUFFER BY 1 FOOT -- THROWING LAST POSITION OUT
	std::unique_lock<std::mutex> l(geomPacket_256Mutex);

	//SHIFT BUFFER
	memmove(&geomPacket_256Buffer[1], &geomPacket_256Buffer, (sizeof CoreGeomPacket_A())*511);

	//WRITE IN THE NEW DATA
	dataStruct.location = location;
	geomPacket_256Buffer[0] = dataStruct;

	if (geomPacket_256Cnt < 512)
	{
		geomPacket_256Cnt++;
	}

	////geomPacket_256Mutex.unlock();
	//if (geomPacket_256Cnt > 124)
	//{
	condVar_256BufferNewFoot.notify_one();
	//}
	l.unlock();
	
	//condVar_256BufferNewFoot.notify_one();
}

//CALCULATE CROSSLEVEL FOR 1 FT
double calcCrosslevel1FT_A(int geomPacketIndex, bool enableOC, double &crosslevel_SWA)
{
	double roll = geomPacket_256Buffer[geomPacketIndex].buffer_AG1[0].roll;

	if (enableOC)
	{
		//GET AVERAGE OPTICAL HEIGHTS
		double vert_L = 0.0;
		double vert_R = 0.0;
		double horz_L = 0.0;
		double horz_R = 0.0;

		//FILTER THE HG / VG
		calcFilteredHG_and_verticals(geomPacketIndex, horz_L, horz_R, vert_L, vert_R);

		//RIGHT RAIL HEIGHT - LEFT RAIL HEIGHT = DELTA HEIGHT
		double opticalHeightDiff = vert_R - vert_L;
		opticalHeightDiff = opticalHeightDiff * cos(roll * DEG2RAD);
		double opticalHeightDiff_SWA = (vert_R - vert_L) * 1.075;
		//double rollOptical = std::atan(opticalHeightDiff / TRACKWIDTH) * RAD2DEG;
		//double xl_OC = std::sin((rollOptical + roll) * DEG2RAD) * TRACKWIDTH;
		double xl_OC = std::sin(roll * DEG2RAD) * TRACKWIDTH * 1.01 + opticalHeightDiff;
		crosslevel_SWA = std::sin(roll * DEG2RAD) * TRACKWIDTH * 1.01 + opticalHeightDiff_SWA;
		return xl_OC;
	}
	else
	{
		double xl_NOC = std::sin(DEG2RAD * roll) * TRACKWIDTH * 1.01;
		return xl_NOC;
	}
}

//CALCULATE GRADE FOR 1 FT
double calcGrade1FT_A(int geomPacketIndex)
{
	double pitch = geomPacket_256Buffer[geomPacketIndex].buffer_AG1[0].pitch;
	double grade = std::tan(DEG2RAD * pitch) * 100;
	return grade;
}

//CALCULATE HORIZ RAIL ALIGNMENT # GYRO VERSION # -- PACKET INDEX OF THE 'CENTER' INDEX , R/L RAIL, ALIGNMENT LENGTH
bool calcHorizRailAlignmentMCO_A_gyro(int geomPacketIndex, int half_length, int full_length, double &leftRailMCO, double &rightRailMCO, bool enableOC)
{
	////NON-COMPENSATED HORIZONTAL ALIGNMENT IS THE SAME FOR LEFT AND RIGHT RAILS

	int midIndex = geomPacketIndex;
	int startIndex = midIndex - half_length;
	int endIndex = midIndex + half_length;

	double MCO = -999;
	double cumY = 0;
	double cumHeading = 0;
	double priorHeadingVal = -999;
	double yMid = -999;
	double yEnd = -999;

	for (int i = startIndex; i <= endIndex; i++) //CYCLE THROUGH 512 FT BUFFER
	{
		//LOOP THROUGH SUB-BUFFERS FOR ATTITUDE PACKETS
		for (int j = 0; j < geomPacket_256Buffer[i].AG1_BufferIndex; j++)
		{
			//DETERMINE THE CUM-Y VALUE FOR THE MID POINT FOOT
			if (yMid == -999)
			{
				if (i >= midIndex)
				{
					yMid = cumY;
				}
			}

			//IF WE DON'T HAVE A VALID LAST-HEADING, USE THE CURRENT HEADING
			if (priorHeadingVal == -999)
			{
				priorHeadingVal = geomPacket_256Buffer[i].buffer_AG1[j].heading;
			}

			//CALCULATE THE DELTA HEADING FROM LAST PACKET
			double deltaHeading = geomPacket_256Buffer[i].buffer_AG1[j].heading - priorHeadingVal;
			priorHeadingVal = geomPacket_256Buffer[i].buffer_AG1[j].heading;

			//ATTEMPTING TO DEAL WITH DELTAS OVER 359 DEG and 1 DEG HEADING
			if (deltaHeading >= 180.0)
			{
				deltaHeading = deltaHeading - 360.0;
			}
			else
			{
				if (deltaHeading <= -180.0)
				{
					deltaHeading = deltaHeading + 360.0;
				}
			}

			//ADD DELTA HEADING TO THE RELATIVE RUNNING HEADING
			cumHeading += deltaHeading;
			//CALC THE DELTA IN Y-DIRECTION ASSOCIATED WITH THE CUM. HEADING
			double deltaDistnace = geomPacket_256Buffer[i].buffer_AG1[j].speed * INSSMAPLEPERIOD * METERS_SECOND2IN_SECOND;
			cumY += std::sin(DEG2RAD * cumHeading) * deltaDistnace; //DISTANCE IN INCHES
		}
	}

	//Y END POINT = THE LAST cumY
	yEnd = cumY;

	//ASSURE THAT WE DID FIND A yMid POINT
	if (yMid != -999)
	{
		if (enableOC)
		{
			double gageStart_L = 0;
			double gageStart_R = 0;
			double gageMiddle_L = 0;
			double gageMiddle_R = 0;
			double gageEnd_L = 0;
			double gageEnd_R = 0;


			gageStart_L = geomPacket_256Buffer[startIndex].gageData.left_width;
			gageStart_R = geomPacket_256Buffer[startIndex].gageData.right_width;
			gageMiddle_L = geomPacket_256Buffer[midIndex].gageData.left_width;
			gageMiddle_R = geomPacket_256Buffer[midIndex].gageData.right_width;
			gageEnd_L = geomPacket_256Buffer[endIndex].gageData.left_width;
			gageEnd_R = geomPacket_256Buffer[endIndex].gageData.right_width;


			if (full_length < 100)
			{
				//DO SOME HALF GAGE FILTERING
							//FOR STARTING VALUE
				if (gageStart_L == 0.0)
				{
					//CALCULATE AVERAGE OVER +/- 2 values that are not 0
					double sum1 = 0.0;
					double cnt1Low = 0.0;
					double cnt1High = 0.0;
					int tries = 0;
					while ((cnt1Low < 1.0 || cnt1High < 1.0) && tries < 10)
					{
						cnt1Low = 0.0;
						cnt1High = 0.0;
						sum1 = 0.0;
						tries++;
						for (int o = -1 * tries; o <= 1 * tries; o++)
						{
							if (geomPacket_256Buffer[startIndex + o].gageData.left_width != 0.0)
							{
								sum1 += geomPacket_256Buffer[startIndex + o].gageData.left_width;
								if (o < 0)
								{
									cnt1Low += 1.0;
								}
								if (o > 0)
								{
									cnt1High += 1.0;
								}
							}
						}
					}
					if ((cnt1Low + cnt1High) >= 1.0)
					{
						gageStart_L = sum1 / (cnt1Low + cnt1High);
					}
					else
					{
						gageStart_L = 0.0;
					}
				}

				if (gageStart_R == 0.0)
				{
					//CALCULATE AVERAGE OVER +/- 2 values that are not 0
					double sum1 = 0.0;
					double cnt1Low = 0.0;
					double cnt1High = 0.0;
					int tries = 0;
					while ((cnt1Low < 1.0 || cnt1High < 1.0) && tries < 10)
					{
						cnt1Low = 0.0;
						cnt1High = 0.0;
						sum1 = 0.0;
						tries++;
						for (int o = -1 * tries; o <= 1 * tries; o++)
						{
							if (geomPacket_256Buffer[startIndex + o].gageData.right_width != 0.0)
							{
								sum1 += geomPacket_256Buffer[startIndex + o].gageData.right_width;
								if (o < 0)
								{
									cnt1Low += 1.0;
								}
								if (o > 0)
								{
									cnt1High += 1.0;
								}
							}
						}
					}
					if ((cnt1Low + cnt1High) >= 1.0)
					{
						gageStart_R = sum1 / (cnt1Low + cnt1High);
					}
					else
					{
						gageStart_R = 0.0;
					}
				}

				//FOR MIDDLE VALUES
				if (gageMiddle_L == 0.0)
				{
					//CALCULATE AVERAGE OVER +/- 2 values that are not 0
					double sum1 = 0.0;
					double cnt1Low = 0.0;
					double cnt1High = 0.0;
					int tries = 0;
					while ((cnt1Low < 1.0 || cnt1High < 1.0) && tries < 10)
					{
						cnt1Low = 0.0;
						cnt1High = 0.0;
						sum1 = 0.0;
						tries++;
						for (int o = -1 * tries; o <= 1 * tries; o++)
						{
							if (geomPacket_256Buffer[midIndex + o].gageData.left_width != 0.0)
							{
								sum1 += geomPacket_256Buffer[midIndex + o].gageData.left_width;
								if (o < 0)
								{
									cnt1Low += 1.0;
								}
								if (o > 0)
								{
									cnt1High += 1.0;
								}
							}
						}
					}
					if ((cnt1Low + cnt1High) >= 1.0)
					{
						gageMiddle_L = sum1 / (cnt1Low + cnt1High);
					}
					else
					{
						gageMiddle_L = 0.0;
					}
				}

				if (gageMiddle_R == 0.0)
				{
					//CALCULATE AVERAGE OVER +/- 2 values that are not 0
					double sum1 = 0.0;
					double cnt1Low = 0.0;
					double cnt1High = 0.0;
					int tries = 0;
					while ((cnt1Low < 1.0 || cnt1High < 1.0) && tries < 10)
					{
						cnt1Low = 0.0;
						cnt1High = 0.0;
						sum1 = 0.0;
						tries++;
						for (int o = -1 * tries; o <= 1 * tries; o++)
						{
							if (geomPacket_256Buffer[midIndex + o].gageData.right_width != 0.0)
							{
								sum1 += geomPacket_256Buffer[midIndex + o].gageData.right_width;
								if (o < 0)
								{
									cnt1Low += 1.0;
								}
								if (o > 0)
								{
									cnt1High += 1.0;
								}
							}
						}
					}
					if ((cnt1Low + cnt1High) >= 1.0)
					{
						gageMiddle_R = sum1 / (cnt1Low + cnt1High);
					}
					else
					{
						gageMiddle_R = 0.0;
					}
				}

				//FOR END VALUES
				if (gageEnd_L == 0.0)
				{
					//CALCULATE AVERAGE OVER +/- 2 values that are not 0
					double sum1 = 0.0;
					double cnt1Low = 0.0;
					double cnt1High = 0.0;
					int tries = 0;
					while ((cnt1Low < 1.0 || cnt1High < 1.0) && tries < 10)
					{
						cnt1Low = 0.0;
						cnt1High = 0.0;
						sum1 = 0.0;
						tries++;
						for (int o = -1 * tries; o <= 1 * tries; o++)
						{
							if (geomPacket_256Buffer[endIndex + o].gageData.left_width != 0.0)
							{
								sum1 += geomPacket_256Buffer[endIndex + o].gageData.left_width;
								if (o < 0)
								{
									cnt1Low += 1.0;
								}
								if (o > 0)
								{
									cnt1High += 1.0;
								}
							}
						}
					}
					if ((cnt1Low + cnt1High) >= 1.0)
					{
						gageEnd_L = sum1 / (cnt1Low + cnt1High);
					}
					else
					{
						gageEnd_L = 0.0;
					}
				}

				if (gageEnd_R == 0.0)
				{
					//CALCULATE AVERAGE OVER +/- 2 values that are not 0
					double sum1 = 0.0;
					double cnt1Low = 0.0;
					double cnt1High = 0.0;
					int tries = 0;
					while ((cnt1Low < 1.0 || cnt1High < 1.0) && tries < 10)
					{
						cnt1Low = 0.0;
						cnt1High = 0.0;
						sum1 = 0.0;
						tries++;
						for (int o = -1 * tries; o <= 1 * tries; o++)
						{
							if (geomPacket_256Buffer[endIndex + o].gageData.right_width != 0.0)
							{
								sum1 += geomPacket_256Buffer[endIndex + o].gageData.right_width;
								if (o < 0)
								{
									cnt1Low += 1.0;
								}
								if (o > 0)
								{
									cnt1High += 1.0;
								}
							}
						}
					}
					if ((cnt1Low + cnt1High) >= 1.0)
					{
						gageEnd_R = sum1 / (cnt1Low + cnt1High);
					}
					else
					{
						gageEnd_R = 0.0;
					}
				}
			}


			//MCO = (0.0 - yEnd) / 2.0 - (-yMid);
			double left_MCO = ((-gageStart_L - 0.0) + (-gageEnd_L - yEnd)) / 2.0 - (-gageMiddle_L - yMid);
			double right_MCO = ((gageStart_R - 0.0) + (gageEnd_R - yEnd)) / 2.0 - (gageMiddle_R - yMid);
			leftRailMCO = left_MCO;
			rightRailMCO = right_MCO;
			return true;
		}
		else
		{
			//CALCULATE THE MCO WITH EQUATION
			//MCO = [ gC(START) + gC(END) ] / 2 - gC(MID) 
			MCO = (0.0 - yEnd) / 2.0 - (-yMid);
			leftRailMCO = MCO;
			rightRailMCO = MCO;
			return true;
		}
	}
	return false;
}

//CALCULATE VERT RAIL ALIGNMENT # GYRO VERSION # -- PACKET INDEX OF THE 'CENTER' INDEX, R/L RAIL, ALIGNMENT LENGTH
bool calcVertRailAlignmentMCO_A_gyro(int geomPacketIndex, int half_length, int full_length, double &leftRailMCO, double &rightRailMCO, bool enableOC)
{
	int midIndex = geomPacketIndex;
	int startIndex = midIndex + half_length;
	int endIndex = midIndex - half_length;

	double MCO_L = -999;
	double MCO_R = -999;
	double cumZ_L = 0.0;
	double cumZ_R = 0.0;
	double cumPitch = 0.0;
	double cumRoll = 0.0;
	double priorPitchVal = -999;
	double priorRollVal = -999;
	double zMid_L = -999;
	double zMid_R = -999;
	double zEnd_L = -999;
	double zEnd_R = -999;

	for (int i = startIndex; i >= endIndex; i--) //CYCLE THROUGH 512 FT BUFFER
	{
		//LOOP THROUGH SUB-BUFFERS FOR ATTITUDE PACKETS
		for (int j = geomPacket_256Buffer[i].AG1_BufferIndex - 1; j >= 0; j--)
		{
			//DETERMINE THE CUM-Y VALUE FOR THE MID POINT FOOT
			if (zMid_R == -999)
			{
				if (i <= midIndex)
				{
					zMid_R = cumZ_R;
					zMid_L = cumZ_L;
				}
			}

			//IF WE DON'T HAVE A VALID LAST-PITCH, USE THE CURRENT PITCH
			if (priorPitchVal == -999)
			{
				priorPitchVal = geomPacket_256Buffer[i].buffer_AG1[j].pitch;
			}

			//IF WE DON'T HAVE A VALID LAST-ROLL, USE THE CURRENT ROLL
			if (priorRollVal == -999)
			{
				priorRollVal = geomPacket_256Buffer[i].buffer_AG1[j].roll;
			}


			//CALCULATE THE DELTA PITCH FROM LAST PACKET
			double deltaPitch = geomPacket_256Buffer[i].buffer_AG1[j].pitch - priorPitchVal;
			priorPitchVal = geomPacket_256Buffer[i].buffer_AG1[j].pitch;
			double deltaRoll = geomPacket_256Buffer[i].buffer_AG1[j].roll - priorRollVal;
			priorRollVal = geomPacket_256Buffer[i].buffer_AG1[j].roll;

			//ADD DELTA PITCH TO THE RELATIVE RUNNING PITCH
			cumPitch += deltaPitch;
			//ADD DELTA ROLL TO THE RELATIVE RUNNING ROLL
			cumRoll += deltaRoll;
			//CALCULATE HORIZONTAL DISTANCE TRAVELED
			if (j > 512)
			{
				int problem = 1;
			}
			if (geomPacket_256Buffer[j].AG1_BufferIndex <= 0)
			{
				int problem2 = 1;
			}
			double lastDeltaDist = geomPacket_256Buffer[j].buffer_AG1[0].speed * INSSMAPLEPERIOD * METERS_SECOND2IN_SECOND;
			//CALC THE DELTA IN Y-DIRECTION ASSOCIATED WITH THE CUM. HEADING
			cumZ_R += std::sin(DEG2RAD * cumPitch) * lastDeltaDist + std::sin(-DEG2RAD * deltaRoll) * RIGHT_RAIL_LEVER_ARM; //DISTANCE IN INCHES
			cumZ_L += std::sin(DEG2RAD * cumPitch) * lastDeltaDist + std::sin(-DEG2RAD * deltaRoll) * LEFT_RAIL_LEVER_ARM; //DISTANCE IN INCHES
		}
	}

	//Z END POINT = THE LAST cumZ
	zEnd_R = cumZ_R;
	zEnd_L = cumZ_L;

	//ASSURE THAT WE DID FIND A zMid POINT
	if (zMid_L != -999 && zMid_R != -999)
	{
		if (enableOC)
		{
			//GET AVERAGE OPTICAL HEIGHTS
			double vertStart_L = 0;
			double vertStart_R = 0;
			double vertMiddle_L = 0;
			double vertMiddle_R = 0;
			double vertEnd_L = 0;
			double vertEnd_R = 0;
			for (int o = -5; o <= 5; o++)
			{
				vertStart_L += (geomPacket_256Buffer[startIndex + o].gageData.left_height * std::cos(-DEG2RAD * geomPacket_256Buffer[startIndex + o].buffer_AG1[0].roll)) / 11;
				vertStart_R += (geomPacket_256Buffer[startIndex + o].gageData.right_height * std::cos(-DEG2RAD * geomPacket_256Buffer[startIndex + 0].buffer_AG1[0].roll)) / 11;
				vertMiddle_L += (geomPacket_256Buffer[midIndex + o].gageData.left_height  * std::cos(-DEG2RAD * geomPacket_256Buffer[midIndex + o].buffer_AG1[0].roll)) / 11;
				vertMiddle_R += (geomPacket_256Buffer[midIndex + o].gageData.right_height * std::cos(-DEG2RAD * geomPacket_256Buffer[midIndex + o].buffer_AG1[0].roll)) / 11;
				vertEnd_L += (geomPacket_256Buffer[endIndex + o].gageData.left_height * std::cos(-DEG2RAD * geomPacket_256Buffer[endIndex + o].buffer_AG1[0].roll)) / 11;
				vertEnd_R += (geomPacket_256Buffer[endIndex + o].gageData.right_height * std::cos(-DEG2RAD * geomPacket_256Buffer[endIndex + o].buffer_AG1[0].roll)) / 11;
			}
			//CALCULATE THE MCO WITH EQUATION
			//MCO = [ gC(START) + gC(END) ] / 2 - gC(MID)
			MCO_L = ((vertStart_L - 0) + (vertEnd_L - zEnd_L)) / 2.0 - (vertMiddle_L - zMid_L);
			MCO_R = ((vertStart_R - 0) + (vertEnd_R - zEnd_R)) / 2.0 - (vertMiddle_R - zMid_R);
			leftRailMCO = MCO_L;
			rightRailMCO = MCO_R;
			return true;
		}
		else
		{
			//CALCULATE THE MCO WITH EQUATION
			//MCO = [ gC(START) + gC(END) ] / 2 - gC(MID) 
			MCO_L = -zEnd_L / 2.0 + zMid_L;
			MCO_R = -zEnd_R / 2.0 + zMid_R;
			leftRailMCO = MCO_L;
			rightRailMCO = MCO_R;
			return true;
		}
	}

	return false;
}

//CALCULATE VERT RAIL ALIGNMENT # ACCEL VERSION # -- PACKET INDEX OF THE 'CENTER' INDEX, R/L RAIL, ALIGNMENT LENGTH
bool calcVertRailAlignmentMCO_A_accel(int geomPacketIndex, int half_length, int full_length, double &leftRailMCO, double &rightRailMCO, bool enableOC)
{
	int midIndex = geomPacketIndex;
	int startIndex = midIndex + half_length;
	int endIndex = midIndex - half_length;

	double MCO_L = -999;
	double MCO_R = -999;
	double cumZ_IMU = 0.0;
	double zMid_L = -999;
	double zMid_R = -999;
	double zEnd_L = -999;
	double zEnd_R = -999;
	double zStart_L = -999;
	double zStart_R = -999;

	for (int i = startIndex; i >= endIndex; i--) //CYCLE THROUGH 512 FT BUFFER
	{
		//LOOP THROUGH SUB-BUFFERS FOR ATTITUDE PACKETS
		for (int j = geomPacket_256Buffer[i].AG1_BufferIndex - 1; j >= 0; j--)
		{
			if (zStart_R == -999)
			{
				if (enableOC)
				{
					zStart_R = 0 + std::sin(DEG2RAD * geomPacket_256Buffer[i].buffer_AG1[j].roll) * RIGHT_LASER_LEVER_ARM;
					zStart_L = 0 + std::sin(DEG2RAD * geomPacket_256Buffer[i].buffer_AG1[j].roll) * LEFT_LASER_LEVER_ARM;
				}
				else
				{
					zStart_R = 0 + std::sin(DEG2RAD * geomPacket_256Buffer[i].buffer_AG1[j].roll) * RIGHT_RAIL_LEVER_ARM;
					zStart_L = 0 + std::sin(DEG2RAD * geomPacket_256Buffer[i].buffer_AG1[j].roll) * LEFT_RAIL_LEVER_ARM;
				}
			}

			//UPDATE IMU Z POSITION
			cumZ_IMU = cumZ_IMU + INSSMAPLEPERIOD * geomPacket_256Buffer[i].buffer_AG1[j].downVelocity * METERS_SECOND2IN_SECOND; //REMEMBER THAT DOWN VELOCITY POINTS --DOWN--

			//DETERMINE THE CUM-Y VALUE FOR THE MID POINT FOOT
			if (zMid_R == -999)
			{
				if (i <= midIndex)
				{
					if (enableOC)
					{
						zMid_R = cumZ_IMU + std::sin(DEG2RAD * geomPacket_256Buffer[i].buffer_AG1[j].roll) * RIGHT_LASER_LEVER_ARM;
						zMid_L = cumZ_IMU + std::sin(DEG2RAD * geomPacket_256Buffer[i].buffer_AG1[j].roll) * LEFT_LASER_LEVER_ARM;
					}
					else
					{
						zMid_R = cumZ_IMU + std::sin(DEG2RAD * geomPacket_256Buffer[i].buffer_AG1[j].roll) * RIGHT_RAIL_LEVER_ARM;
						zMid_L = cumZ_IMU + std::sin(DEG2RAD * geomPacket_256Buffer[i].buffer_AG1[j].roll) * LEFT_RAIL_LEVER_ARM;
					}
				}
			}

			//Z END POINT = THE LAST cumZ -- UPDATES EVERY TIME, INCLUDING THE LAST TIME
			if (enableOC)
			{
				zEnd_R = cumZ_IMU + std::sin(DEG2RAD * geomPacket_256Buffer[i].buffer_AG1[j].roll) * RIGHT_LASER_LEVER_ARM;
				zEnd_L = cumZ_IMU + std::sin(DEG2RAD * geomPacket_256Buffer[i].buffer_AG1[j].roll) * LEFT_LASER_LEVER_ARM;
			}
			else
			{
				zEnd_R = cumZ_IMU + std::sin(DEG2RAD * geomPacket_256Buffer[i].buffer_AG1[j].roll) * RIGHT_RAIL_LEVER_ARM;
				zEnd_L = cumZ_IMU + std::sin(DEG2RAD * geomPacket_256Buffer[i].buffer_AG1[j].roll) * LEFT_RAIL_LEVER_ARM;
			}
		}
	}



	//ASSURE THAT WE DID FIND A zMid POINT
	if (zMid_L != -999 && zMid_R != -999)
	{
		if (enableOC)
		{
			//GET AVERAGE OPTICAL HEIGHTS
			double vertStart_L = 0.0;
			double vertStart_R = 0.0;
			double vertMiddle_L = 0.0;
			double vertMiddle_R = 0.0;
			double vertEnd_L = 0.0;
			double vertEnd_R = 0.0;

			double rawVertStart_L = geomPacket_256Buffer[startIndex].gageData.left_height;
			double rawVertStart_R = geomPacket_256Buffer[startIndex].gageData.right_height;
			double rawVertMid_L = geomPacket_256Buffer[midIndex].gageData.left_height;
			double rawVertMid_R = geomPacket_256Buffer[midIndex].gageData.right_height;
			double rawVertEnd_L = geomPacket_256Buffer[endIndex].gageData.left_height;
			double rawVertEnd_R = geomPacket_256Buffer[endIndex].gageData.right_height;

			//FOR STARTING VALUE
			//if (rawVertStart_L == 0.0)
			//{
			//	//CALCULATE AVERAGE OVER +/- 2 values that are not 0
			//	double sum1 = 0.0;
			//	double cnt1Low = 0.0;
			//	double cnt1High = 0.0;
			//	int tries = 0;
			//	while ((cnt1Low < 1.0 || cnt1High < 1.0) && tries < 10)
			//	{
			//		cnt1Low = 0.0;
			//		cnt1High = 0.0;
			//		sum1 = 0.0;
			//		tries++;
			//		for (int o = -1 * tries; o <= 1 * tries; o++)
			//		{
			//			if (geomPacket_256Buffer[startIndex + o].gageData.left_height != 0.0)
			//			{
			//				sum1 += geomPacket_256Buffer[startIndex + o].gageData.left_height;
			//				if (o < 0)
			//				{
			//					cnt1Low += 1.0;
			//				}
			//				if (o > 0)
			//				{
			//					cnt1High += 1.0;
			//				}
			//			}
			//		}
			//	}
			//	if ((cnt1Low + cnt1High) >= 1.0)
			//	{
			//		rawVertStart_L = sum1 / (cnt1Low + cnt1High);
			//	}
			//	else
			//	{
			//		rawVertStart_L = 0.0;
			//	}
			//}

			//if (rawVertStart_R == 0.0)
			//{
			//	//CALCULATE AVERAGE OVER +/- 2 values that are not 0
			//	double sum1 = 0.0;
			//	double cnt1Low = 0.0;
			//	double cnt1High = 0.0;
			//	int tries = 0;
			//	while ((cnt1Low < 1.0 || cnt1High < 1.0) && tries < 10)
			//	{
			//		cnt1Low = 0.0;
			//		cnt1High = 0.0;
			//		sum1 = 0.0;
			//		tries++;
			//		for (int o = -1 * tries; o <= 1 * tries; o++)
			//		{
			//			if (geomPacket_256Buffer[startIndex + o].gageData.right_height != 0.0)
			//			{
			//				sum1 += geomPacket_256Buffer[startIndex + o].gageData.right_height;
			//				if (o < 0)
			//				{
			//					cnt1Low += 1.0;
			//				}
			//				if (o > 0)
			//				{
			//					cnt1High += 1.0;
			//				}
			//			}
			//		}
			//	}
			//	if ((cnt1Low + cnt1High) >= 1.0)
			//	{
			//		rawVertStart_R = sum1 / (cnt1Low + cnt1High);
			//	}
			//	else
			//	{
			//		rawVertStart_R = 0.0;
			//	}
			//}

			//FOR MIDDLE VALUES
			//if (rawVertMid_L == 0.0)
			//{
			//	//CALCULATE AVERAGE OVER +/- 2 values that are not 0
			//	double sum1 = 0.0;
			//	double cnt1Low = 0.0;
			//	double cnt1High = 0.0;
			//	int tries = 0;
			//	while ((cnt1Low < 1.0 || cnt1High < 1.0) && tries < 10)
			//	{
			//		cnt1Low = 0.0;
			//		cnt1High = 0.0;
			//		sum1 = 0.0;
			//		tries++;
			//		for (int o = -1 * tries; o <= 1 * tries; o++)
			//		{
			//			if (geomPacket_256Buffer[midIndex + o].gageData.left_height != 0.0)
			//			{
			//				sum1 += geomPacket_256Buffer[midIndex + o].gageData.left_height;
			//				if (o < 0)
			//				{
			//					cnt1Low += 1.0;
			//				}
			//				if (o > 0)
			//				{
			//					cnt1High += 1.0;
			//				}
			//			}
			//		}
			//	}
			//	if ((cnt1Low + cnt1High) >= 1.0)
			//	{
			//		rawVertMid_L = sum1 / (cnt1Low + cnt1High);
			//	}
			//	else
			//	{
			//		rawVertMid_L = 0.0;
			//	}
			//}

			//if (rawVertMid_R == 0.0)
			//{
			//	//CALCULATE AVERAGE OVER +/- 2 values that are not 0
			//	double sum1 = 0.0;
			//	double cnt1Low = 0.0;
			//	double cnt1High = 0.0;
			//	int tries = 0;
			//	while ((cnt1Low < 1.0 || cnt1High < 1.0) && tries < 10)
			//	{
			//		cnt1Low = 0.0;
			//		cnt1High = 0.0;
			//		sum1 = 0.0;
			//		tries++;
			//		for (int o = -1 * tries; o <= 1 * tries; o++)
			//		{
			//			if (geomPacket_256Buffer[midIndex + o].gageData.right_height != 0.0)
			//			{
			//				sum1 += geomPacket_256Buffer[midIndex + o].gageData.right_height;
			//				if (o < 0)
			//				{
			//					cnt1Low += 1.0;
			//				}
			//				if (o > 0)
			//				{
			//					cnt1High += 1.0;
			//				}
			//			}
			//		}
			//	}
			//	if ((cnt1Low + cnt1High) >= 1.0)
			//	{
			//		rawVertMid_R = sum1 / (cnt1Low + cnt1High);
			//	}
			//	else
			//	{
			//		rawVertMid_R = 0.0;
			//	}
			//}

			//FOR END VALUES
			//if (rawVertEnd_L == 0.0)
			//{
			//	//CALCULATE AVERAGE OVER +/- 2 values that are not 0
			//	double sum1 = 0.0;
			//	double cnt1Low = 0.0;
			//	double cnt1High = 0.0;
			//	int tries = 0;
			//	while ((cnt1Low < 1.0 || cnt1High < 1.0) && tries < 10)
			//	{
			//		cnt1Low = 0.0;
			//		cnt1High = 0.0;
			//		sum1 = 0.0;
			//		tries++;
			//		for (int o = -1 * tries; o <= 1 * tries; o++)
			//		{
			//			if (geomPacket_256Buffer[endIndex + o].gageData.left_height != 0.0)
			//			{
			//				sum1 += geomPacket_256Buffer[endIndex + o].gageData.left_height;
			//				if (o < 0)
			//				{
			//					cnt1Low += 1.0;
			//				}
			//				if (o > 0)
			//				{
			//					cnt1High += 1.0;
			//				}
			//			}
			//		}
			//	}
			//	if ((cnt1Low + cnt1High) >= 1.0)
			//	{
			//		rawVertEnd_L = sum1 / (cnt1Low + cnt1High);
			//	}
			//	else
			//	{
			//		rawVertEnd_L = 0.0;
			//	}
			//}

			//if (rawVertEnd_R == 0.0)
			//{
			//	//CALCULATE AVERAGE OVER +/- 2 values that are not 0
			//	double sum1 = 0.0;
			//	double cnt1Low = 0.0;
			//	double cnt1High = 0.0;
			//	int tries = 0;
			//	while ((cnt1Low < 1.0 || cnt1High < 1.0) && tries < 10)
			//	{
			//		cnt1Low = 0.0;
			//		cnt1High = 0.0;
			//		sum1 = 0.0;
			//		tries++;
			//		for (int o = -1 * tries; o <= 1 * tries; o++)
			//		{
			//			if (geomPacket_256Buffer[endIndex + o].gageData.right_height != 0.0)
			//			{
			//				sum1 += geomPacket_256Buffer[endIndex + o].gageData.right_height;
			//				if (o < 0)
			//				{
			//					cnt1Low += 1.0;
			//				}
			//				if (o > 0)
			//				{
			//					cnt1High += 1.0;
			//				}
			//			}
			//		}
			//	}
			//	if ((cnt1Low + cnt1High) >= 1.0)
			//	{
			//		rawVertEnd_R = sum1 / (cnt1Low + cnt1High);
			//	}
			//	else
			//	{
			//		rawVertEnd_R = 0.0;
			//	}
			//}

			double notusedLHG;
			double notusedRHG;
			calcFilteredHG_and_verticals(startIndex, notusedLHG, notusedRHG, rawVertStart_L, rawVertStart_R);
			calcFilteredHG_and_verticals(midIndex, notusedLHG, notusedRHG, rawVertMid_L, rawVertMid_R);
			calcFilteredHG_and_verticals(endIndex, notusedLHG, notusedRHG, rawVertEnd_L, rawVertEnd_R);

			vertStart_L = (rawVertStart_L + 10.5) * std::cos(DEG2RAD * geomPacket_256Buffer[startIndex].buffer_AG1[0].roll);
			vertStart_R = (rawVertStart_R + 10.5) * std::cos(DEG2RAD * geomPacket_256Buffer[startIndex].buffer_AG1[0].roll);
			vertMiddle_L = (rawVertMid_L + 10.5) * std::cos(DEG2RAD * geomPacket_256Buffer[midIndex].buffer_AG1[0].roll);
			vertMiddle_R = (rawVertMid_R + 10.5) * std::cos(DEG2RAD * geomPacket_256Buffer[midIndex].buffer_AG1[0].roll);
			vertEnd_L = (rawVertEnd_L + 10.5) * std::cos(DEG2RAD * geomPacket_256Buffer[endIndex].buffer_AG1[0].roll);
			vertEnd_R = (rawVertEnd_R + 10.5) * std::cos(DEG2RAD * geomPacket_256Buffer[endIndex].buffer_AG1[0].roll);;




			//CALCULATE THE MCO WITH EQUATION
			//MCO = [ gC(START) + gC(END) ] / 2 - gC(MID)
			//MCO_L = ((vertStart_L - zStart_L) + (vertEnd_L - zEnd_L)) / 2.0 - (vertMiddle_L - zMid_L);
			//MCO_R = ((vertStart_R - zStart_R) + (vertEnd_R - zEnd_R)) / 2.0 - (vertMiddle_R - zMid_R);
			MCO_L = ((-vertStart_L - zStart_L) + (-vertEnd_L - zEnd_L)) / 2.0 - (-vertMiddle_L - zMid_L);
			MCO_R = ((-vertStart_R - zStart_R) + (-vertEnd_R - zEnd_R)) / 2.0 - (-vertMiddle_R - zMid_R);
			leftRailMCO = MCO_L;
			rightRailMCO = MCO_R;
			return true;
		}
		else
		{
			//CALCULATE THE MCO WITH EQUATION
			//MCO = [ gC(START) + gC(END) ] / 2 - gC(MID) 
			MCO_L = ((0 - zStart_L) + (0 - zEnd_L)) / 2.0 - (0 - zMid_L);
			MCO_R = ((0 - zStart_R) + (0 - zEnd_R)) / 2.0 - (0 - zMid_R);
			leftRailMCO = MCO_L;
			rightRailMCO = MCO_R;
			return true;
		}
	}

	return false;
}

//CALCULATE CURVATURE -- PACKET INDEX OF THE 'CENTER' INDEX
bool calcCurvature_A(int geomPacketIndex, double &curvature)
{
	int midIndex = geomPacketIndex;
	int startIndex = midIndex - 31;
	int endIndex = midIndex + 31;

	double cumAngle = 0.0;
	double cumSpeed = 0.0;
	double sampleCnt = 0.0;

	double priorHeadingVal = -999.0;

	for (int i = startIndex; i <= endIndex; i++) //CYCLE THROUGH 512 FT BUFFER
	{
		//LOOP THROUGH SUB-BUFFERS FOR ATTITUDE PACKETS
		for (int j = 0; j < geomPacket_256Buffer[i].AG1_BufferIndex; j++)
		{
			//IF WE DON'T HAVE A VALID LAST-HEADING, USE THE CURRENT HEADING
			if (priorHeadingVal == -999)
			{
				priorHeadingVal = geomPacket_256Buffer[i].buffer_AG1[j].heading;
			}

			//CALCULATE THE DELTA HEADING FROM LAST PACKET
			double deltaHeading = geomPacket_256Buffer[i].buffer_AG1[j].heading - priorHeadingVal;
			priorHeadingVal = geomPacket_256Buffer[i].buffer_AG1[j].heading;

			//ATTEMPTING TO DEAL WITH DELTAS OVER 359 DEG and 1 DEG HEADING
			if (deltaHeading >= 180.0)
			{
				deltaHeading = deltaHeading - 360.0;
			}
			else
			{
				if (deltaHeading <= -180.0)
				{
					deltaHeading = deltaHeading + 360.0;
				}
			}

			//ADD DELTA HEADING TO THE RELATIVE RUNNING HEADING
			cumAngle += deltaHeading * DEG2RAD;
			sampleCnt++;
			cumSpeed += geomPacket_256Buffer[i].buffer_AG1[j].speed;
		}
	}


	if (sampleCnt > 0)
	{
		//CALCULATE THE CURVATURE
		//deltaAngle must be in radians
		//k units are 1/meter
		//k = [deltaAngle / deltaTime] / averageSpeed
		double avgSpeed = cumSpeed / sampleCnt;
		double deltaTime = sampleCnt * INSSMAPLEPERIOD;
		double k = (cumAngle / deltaTime) / avgSpeed;
		//CONVERT INTO STANDARD DEGREES PER 100 FT CURVATURE
		double railroadCurvature = k * 1730.103;
		curvature = railroadCurvature * -1.0;
		return true;
	}
	return false;
}

//CALCULATE FILTERED GAGE
bool calcFilteredGage(int geomPacketIndex, double &filteredGage)
{
	//## FILTERED FULL GAGE ##//
	//DETERMINE THE MEDIAN FOR A 5 FOOT WINDOW
	double filtFullGage;
	double medianFullGageVal;
	double medianGageArray[11];

	//FILL THE ARRAY
	for (int o = -5; o <= 5; o++)
	{
		medianGageArray[o + 5] = geomPacket_256Buffer[geomPacketIndex + o].gageData.full_gage;
	}

	//SORT THE ARRAY
	std::sort(std::begin(medianGageArray), std::end(medianGageArray));

	//PULL THE MIDDLE VALUE
	medianFullGageVal = medianGageArray[5];

	//COMPARE THE CURRENT GAGE VALUE TO THE MEDIAN VALUE TO MAX DELTA LIMITS
	if (geomPacket_256Buffer[geomPacketIndex].gageData.full_gage > medianFullGageVal + 1.0 || geomPacket_256Buffer[geomPacketIndex].gageData.full_gage < medianFullGageVal - 1.0 && geomPacket_256Buffer[geomPacketIndex].gageData.full_gage >  54.5 && geomPacket_256Buffer[geomPacketIndex].gageData.full_gage < 58.5)
	{
		double avgWindow = 0.0;
		double avgWindowCnt = 0.0;
		//AVERAGE + / - 2
		for (int o = -5; o <= 5; o++)
		{
			if (o != 0 && geomPacket_256Buffer[geomPacketIndex + o].gageData.full_gage > 54.5 && geomPacket_256Buffer[geomPacketIndex + o].gageData.full_gage < 58.5)
			{
				avgWindow += geomPacket_256Buffer[geomPacketIndex + o].gageData.full_gage;
				avgWindowCnt += 1.0;
			}
		}
		filteredGage = avgWindow / avgWindowCnt;
	}
	else
	{
		filteredGage = geomPacket_256Buffer[geomPacketIndex].gageData.full_gage;
	}
	return true;
}

//CALCULATE FILTERED HALF GAGES / VERTICALS
bool calcFilteredHG_and_verticals(int geomPacketIndex, double& leftHG, double& rightHG, double& leftVG, double& rightVG)
{
	//## VERTICALS ##//
	//GET AVERAGE OPTICAL HEIGHTS
	double vert_L = 0.0;
	double vert_R = 0.0;

	vert_L = geomPacket_256Buffer[geomPacketIndex].gageData.left_height;
	vert_R = geomPacket_256Buffer[geomPacketIndex].gageData.right_height;

	if (vert_L == 0.0)
	{
		//CALCULATE AVERAGE OVER +/- 2 values that are not 0
		double sum1 = 0.0;
		double cnt1Low = 0.0;
		double cnt1High = 0.0;
		int tries = 0;
		while ((cnt1Low < 1.0 || cnt1High < 1.0) && tries < 16)
		{
			cnt1Low = 0.0;
			cnt1High = 0.0;
			sum1 = 0.0;
			tries++;
			for (int o = -1 * tries; o < 1 * tries; o++)
			{
				if (geomPacket_256Buffer[geomPacketIndex + o].gageData.left_height != 0.0)
				{
					sum1 += geomPacket_256Buffer[geomPacketIndex + o].gageData.left_height;
					if (o < 0)
					{
						cnt1Low += 1.0;
					}
					if (o > 0)
					{
						cnt1High += 1.0;
					}
				}
			}
		}
		if (cnt1Low + cnt1High >= 1.0)
		{
			vert_L = sum1 / (cnt1Low + cnt1High);
		}
		else
		{
			vert_L = 0.0;
		}
	}
	else
	{
		//CHECK FOR 'JUMP'
		if (geomPacket_256Buffer[geomPacketIndex + 1].gageData.left_height != 0.0 && geomPacket_256Buffer[geomPacketIndex - 1].gageData.left_height != 0.0)
		{
			double windowDelta = std::abs(geomPacket_256Buffer[geomPacketIndex + 1].gageData.left_height - geomPacket_256Buffer[geomPacketIndex - 1].gageData.left_height);
			if (windowDelta < 0.33)
			{
				double windowAvg = (geomPacket_256Buffer[geomPacketIndex + 1].gageData.left_height + geomPacket_256Buffer[geomPacketIndex - 1].gageData.left_height) / 2.0;
				if (geomPacket_256Buffer[geomPacketIndex].gageData.left_height - windowAvg > 0.60)
				{
					vert_L = windowAvg;
				}
			}
		}
	}

	if (vert_R == 0.0)
	{
		//CALCULATE AVERAGE OVER +/- 2 values that are not 0
		double sum1 = 0.0;
		double cnt1Low = 0.0;
		double cnt1High = 0.0;
		int tries = 0;
		while ((cnt1Low < 1.0 || cnt1High < 1.0) && tries < 16)
		{
			cnt1Low = 0.0;
			cnt1High = 0.0;
			sum1 = 0.0;
			tries++;
			for (int o = -1 * tries; o < 1 * tries; o++)
			{
				if (geomPacket_256Buffer[geomPacketIndex + o].gageData.right_height != 0.0)
				{
					sum1 += geomPacket_256Buffer[geomPacketIndex + o].gageData.right_height;
					if (o < 0)
					{
						cnt1Low += 1.0;
					}
					if (o > 0)
					{
						cnt1High += 1.0;
					}
				}
			}
		}
		if ((cnt1Low + cnt1High) >= 1.0)
		{
			vert_R = sum1 / (cnt1Low + cnt1High);
		}
		else
		{
			vert_R = 0.0;
		}
	}else
	{
		//CHECK FOR 'JUMP'
		if (geomPacket_256Buffer[geomPacketIndex + 1].gageData.right_height != 0.0 && geomPacket_256Buffer[geomPacketIndex - 1].gageData.right_height != 0.0)
		{
			double windowDelta = std::abs(geomPacket_256Buffer[geomPacketIndex + 1].gageData.right_height - geomPacket_256Buffer[geomPacketIndex - 1].gageData.right_height);
			if (windowDelta < 0.33)
			{
				double windowAvg = (geomPacket_256Buffer[geomPacketIndex + 1].gageData.right_height + geomPacket_256Buffer[geomPacketIndex - 1].gageData.right_height) / 2.0;
				if (geomPacket_256Buffer[geomPacketIndex].gageData.right_height - windowAvg > 0.60)
				{
					vert_R = windowAvg;
				}
			}
		}
	}


	leftVG = vert_L;
	rightVG = vert_R;


	//## HORIZONTALS ##//
	double gage_L = 0;
	double gage_R = 0;

	gage_L = geomPacket_256Buffer[geomPacketIndex].gageData.left_width;
	gage_R = geomPacket_256Buffer[geomPacketIndex].gageData.right_width;

	//DO SOME HALF GAGE FILTERING
	//FOR STARTING VALUE
	if (gage_L == 0.0)
	{
		//CALCULATE AVERAGE OVER +/- 2 values that are not 0
		double sum1 = 0.0;
		double cnt1Low = 0.0;
		double cnt1High = 0.0;
		int tries = 0;
		while ((cnt1Low < 1.0 || cnt1High < 1.0) && tries < 16)
		{
			cnt1Low = 0.0;
			cnt1High = 0.0;
			sum1 = 0.0;
			tries++;
			for (int o = -1 * tries; o <= 1 * tries; o++)
			{
				if (geomPacket_256Buffer[geomPacketIndex + o].gageData.left_width != 0.0)
				{
					sum1 += geomPacket_256Buffer[geomPacketIndex + o].gageData.left_width;
					if (o < 0)
					{
						cnt1Low += 1.0;
					}
					if (o > 0)
					{
						cnt1High += 1.0;
					}
				}
			}
		}
		if ((cnt1Low + cnt1High) >= 1.0)
		{
			gage_L = sum1 / (cnt1Low + cnt1High);
		}
		else
		{
			gage_L = 0.0;
		}
	}

	if (gage_R == 0.0)
	{
		//CALCULATE AVERAGE OVER +/- 2 values that are not 0
		double sum1 = 0.0;
		double cnt1Low = 0.0;
		double cnt1High = 0.0;
		int tries = 0;
		while ((cnt1Low < 1.0 || cnt1High < 1.0) && tries < 16)
		{
			cnt1Low = 0.0;
			cnt1High = 0.0;
			sum1 = 0.0;
			tries++;
			for (int o = -1 * tries; o <= 1 * tries; o++)
			{
				if (geomPacket_256Buffer[geomPacketIndex + o].gageData.right_width != 0.0)
				{
					sum1 += geomPacket_256Buffer[geomPacketIndex + o].gageData.right_width;
					if (o < 0)
					{
						cnt1Low += 1.0;
					}
					if (o > 0)
					{
						cnt1High += 1.0;
					}
				}
			}
		}
		if ((cnt1Low + cnt1High) >= 1.0)
		{
			gage_R = sum1 / (cnt1Low + cnt1High);
		}
		else
		{
			gage_R = 0.0;
		}
	}

	leftHG = gage_L;
	rightHG = gage_R;

	return true;
}

//INSERT A NEW CALCULATION PACKET INTO THE BUFFER
bool insertNewCalcPacket_A(CoreGeomCalculationPacket p)
{
	std::unique_lock<std::mutex> l(calcPacket_Mutex);
	calcPacket_buffer.push(p);
	l.unlock();
	condVar_calcPacketBuffer.notify_one();

	return true;
}

//REMOVE A CALCULATION PACKET FROM THE BUFFER
bool CoreGeometry_A::getCalcPacket_A(CoreGeomCalculationPacket &p)
{
	std::unique_lock<std::mutex> l(calcPacket_Mutex);
	//condVar_calcPacketBuffer.wait(l, [] {return calcPacket_buffer.size() > 0; });
	condVar_calcPacketBuffer.wait_for(l, std::chrono::milliseconds(1000), [] {return calcPacket_buffer.size() > 0; });
	int bufferSize = calcPacket_buffer.size();
	if (bufferSize > 0)
	{
		p = calcPacket_buffer.front();
		calcPacket_buffer.pop();
		l.unlock();
		return true;
	}
	l.unlock();
	return false;
}

//PLACE A NEW GAGE PACKET ON INCOMING QUEUE
bool CoreGeometry_A::putGageData(GageGeometry_Data data)
{
	std::unique_lock<std::mutex> l(gagePacket_mutex);

	gagePacket_buffer.push(data);
	l.unlock();
	gagePacket_condVar.notify_one();
	return true;
}

//GET A GAGE PACKET FROM INCOMING QUEUE
bool getAndInsertGageData()
{
	std::unique_lock<std::mutex> l(gagePacket_mutex);
	//gagePacket_condVar.wait(l, [] {return gagePacket_buffer.size() > 0; });
	gagePacket_condVar.wait_for(l, std::chrono::milliseconds(100), [] {return gagePacket_buffer.size() > 0; });
	if(gagePacket_buffer.size() > 0)
	{
		GageGeometry_Data tmpData;
		tmpData = gagePacket_buffer.front();
		l.unlock();
		bool insertSuccess = insertGageData(tmpData);
		if (insertSuccess)
		{
			l.lock();
			gagePacket_buffer.pop();
			//cga_popped++;
			//OutputDebugString("GAGE INSERTED\n");
			l.unlock();
		}
		else
		{
			//OutputDebugString("GAGE NOT INSERTED\n");
		}

		return insertSuccess;
	}
	l.unlock();
	return false;
}

//PLACE GAGE DATA PACKET INTO CORE GEOMETRY PACKET
bool insertGageData(GageGeometry_Data data)
{
	//START AT THE HIGHEST POSITION IN BUFFER
	bool foundMatch = false;
	std::unique_lock<std::mutex> l(geomPacket_256Mutex);
	for (int i = geomPacket_256Cnt - 1; i >= 0; i--)
	{
		if (!geomPacket_256Buffer[i].gageData_complete)
		{
			if (i > 511 || i < 0)
			{
				int problem = 1;
			}
			geomPacket_256Buffer[i].gageData = data;
			geomPacket_256Buffer[i].gageData_complete = true;
			l.unlock();
			i = -99;
			foundMatch = true;
			return foundMatch;
		}
	}
	l.unlock();
	return foundMatch;
}

//GAGE - PACKET MATCHING / INSERTING THREAD : RETURN TRUE FOR MATCHED PACKET ; FALSE FOR UNMATCHED PACKET
bool CoreGeometry_A::gageMatchingThread()
{
	while (coreGeometryEnable)
	{
		bool success = getAndInsertGageData();
	}
	return true;
}

//SET CORE GEOMETRY INFINITE LOOP FLAG TO - FALSE
bool CoreGeometry_A::shutdownCoreGeometry()
{
	coreGeometryEnable = false;
	return true;
}

//SET CORE GEOMETRY INFINITE LOOP FLAG TO - TRUE
bool CoreGeometry_A::enableCoreGeometry()
{
	coreGeometryEnable = true;
	return true;
}

//SET THE VEHICLE ID GLOBAL VARIABLE
void CoreGeometry_A::setVehicleID(char* vehicleID)
{
	memcpy(cgVehicleID, vehicleID, 10);
}

