#include "stdafx.h"
#include "CoreGeometry.h"

//PROTOTYPES
bool search512FtBuffer(double sec, int &index, bool &packetTooOld);
double calcCrosslevel1FT(int geomPacketIndex);
double calcGrade1FT(int geomPacketIndex);
bool calcHorizRailAlignmentMCO(int geomPacketIndex, int half_length, int full_length, double &leftRailMCO, double &rightRailMCO);
bool calcVertRailAlignmentMCO(int geomPacketIndex, int half_length, int full_length, double &leftRailMCO, double &rightRailMCO);
bool calcCurvature(int geomPacketIndex, double &curvature);
bool insertNewCalcPacket(CoreGeomCalculationPacket p);
CoreGeomPacket createCoreGeomPacket(MARKPVA dataStruct, MP_LOCATION location);//I ADDED THIS??? 


//SUMMARY - 1 FOOT VALUES FOR 512 FEET
//## INPUT BUFFER -- KIND OF ##//
CoreGeomPacket geomPacket_512Buffer[256];
std::mutex geomPacket_512Mutex;
int geomPacket_512Cnt;
std::condition_variable condVar_512BufferNewFoot;

//CALCULATED DATA PACKET FOR EACH 1 FT -- MCO / CURVATURE / GRADE / CROSSLEVEL
//## THIS IS THE OUTPUT BUFFER ##//
std::queue<CoreGeomCalculationPacket> calcPacket_buffer;
std::mutex calcPacket_Mutex;
int calcPacket_512Cnt;
std::condition_variable condVar_calcPacketBuffer;


//FOR GEOMETRY FILE OUTPUT
//OUTPUT FILE
std::ofstream geometryFileStream;
long geometryFSize = 0;
long lineCnt = 0;





CoreGeometry::CoreGeometry()
{
	geomPacket_512Cnt = 0;
}

CoreGeometry::~CoreGeometry()
{
}

//CONTROLLER LOOP FOR CORE GEOMETRY CALCULATIONS
void CoreGeometry::controllerThread_CoreCalcs()
{
	//KEEP CHECKING FOR 'COMPLETE' 512 packets to run CORE GEOMETRY CALCULATIONS ON
	while (true)
	{
		std::unique_lock<std::mutex> l(geomPacket_512Mutex);

		condVar_512BufferNewFoot.wait_for(l, std::chrono::milliseconds(1));

		for (int i = 62; (i < 256 - 62) && (i < geomPacket_512Cnt) && (i + 62 < geomPacket_512Cnt); i++)
		{
			if (!geomPacket_512Buffer[i].CoreGeom_complete)
			{
				if (i > 70)
				{
					int breakHere = 1;
				}
				bool allINSPVAS_Complete = true;
				bool allINSSPDS_Complete = true;
				for (int j = i - 62; j < i + 62; j++)
				{
					if (!geomPacket_512Buffer[j].INSPVAS_complete)
					{
						allINSPVAS_Complete = false;
					}

					if (!geomPacket_512Buffer[j].INSSPDS_complete)
					{
						allINSSPDS_Complete = true;
					}
				}
				if (allINSPVAS_Complete && allINSSPDS_Complete)
				{
					//THE GEO PACKET IS WITHIN OUR 1/2 calculation widths &&
					//THE GEO PACKET HASN'T HAD CALCULATIONS PERFORMED ON IT YET &&
					//THE GEO PACKET HAS COMPLETED RECEPTION OF INSSPDS / INSPVAS PACKETS
					geomPacket_512Buffer[i].CoreGeom_complete = true;

					double crosslevel = calcCrosslevel1FT(i);
					double grade = calcGrade1FT(i);

					double lVert11;
					double lVert31;
					double lVert62;
					double rVert11;
					double rVert31;
					double rVert62;

					double lHoriz124;
					double lHoriz31;
					double lHoriz62;
					double rHoriz124;
					double rHoriz31;
					double rHoriz62;

					double curvature;


					bool goodHorizMCO31 = calcHorizRailAlignmentMCO(i, 15, 31, lHoriz31, rHoriz31);
					bool goodHorizMCO62 = calcHorizRailAlignmentMCO(i, 31, 63, lHoriz62, rHoriz62);//ACTUALLY DOING 63' ALIGNMENT...NOT 62
					bool goodHorizMCO124 = calcHorizRailAlignmentMCO(i, 62, 125, lHoriz124, rHoriz124);//ACTUALLY DOING 125' ALIGNMENT..NOT 124

					bool goodVertMCO11 = calcVertRailAlignmentMCO(i, 5, 10, lVert11, rVert11);
					bool goodVertMCO31 = calcVertRailAlignmentMCO(i, 15, 30, lVert31, rVert31);
					bool goodVertMCO62 = calcVertRailAlignmentMCO(i, 31, 62, lVert62, rVert62);//ACTUALLY DOING 61' ALIGNMENT...NOT 62

					bool goodCurvature = calcCurvature(i, curvature);//THIS IS PRE-140' FIR FILTER

					CoreGeomCalculationPacket coreCalcs = CoreGeomCalculationPacket();
					coreCalcs.latitude = geomPacket_512Buffer[i].oneFootPacket.lat;
					coreCalcs.longitude = geomPacket_512Buffer[i].oneFootPacket.lon;
					coreCalcs.altitude = geomPacket_512Buffer[i].oneFootPacket.height;
					coreCalcs.location = geomPacket_512Buffer[i].location;
					if (geomPacket_512Buffer[i].INSSPDS_BufferIndex > 0)
					{
						coreCalcs.speed = geomPacket_512Buffer[i].buffer_INSSPDS[0].horizontalSpeed;
					}
					else
					{
						coreCalcs.speed = -99;
					}

					coreCalcs.secValidity = geomPacket_512Buffer[i].oneFootPacket.seconds;

					coreCalcs.crosslevel_raw = crosslevel;
					coreCalcs.grade_raw = grade;
					coreCalcs.L_HORIZ_MCO_31_raw = lHoriz31;
					coreCalcs.R_HORIZ_MCO_31_raw = rHoriz31;
					coreCalcs.L_HORIZ_MCO_62_raw = lHoriz62;
					coreCalcs.R_HORIZ_MCO_62_raw = rHoriz62;
					coreCalcs.L_HORIZ_MCO_124_raw = lHoriz124;
					coreCalcs.R_HORIZ_MCO_124_raw = rHoriz124;

					coreCalcs.L_VERT_MCO_11_raw = lVert11;
					coreCalcs.R_VERT_MCO_11_raw = rVert11;
					coreCalcs.L_VERT_MCO_31_raw = lVert31;
					coreCalcs.R_VERT_MCO_31_raw = rVert31;
					coreCalcs.L_VERT_MCO_62_raw = lVert62;
					coreCalcs.R_VERT_MCO_62_raw = rVert62;

					coreCalcs.rawCurvature = curvature;

					insertNewCalcPacket(coreCalcs);
				}
			}
		}
		l.unlock();
	}
}

//CREATE NEW CoreGeomPacket
CoreGeomPacket createCoreGeomPacket(MARKPVA dataStruct, MP_LOCATION location)
{
	CoreGeomPacket packet = CoreGeomPacket();
	packet.oneFootPacket = dataStruct;
	packet.seconds = dataStruct.seconds;
	packet.CORRIMUDATAS_BufferIndex = 0;
	packet.INSSPDS_BufferIndex = 0;
	packet.INSPVAS_BufferIndex = 0;
	packet.CORRIMUDATAS_complete = false;
	packet.INSPVAS_complete = false;
	packet.INSSPDS_complete = false;
	packet.CoreGeom_complete = false;
	packet.location = location;
	return packet;
}

//PLACE MESSAGE INTO MATCHED-FOOT BUFFER : INSPVAS
bool CoreGeometry::putMsgIntoCoreGeomPacket_INSPVAS(INSPVAS dataStruct, bool &popIt)
{
	popIt = false;
	int matched512Index;
	bool matchedPacketTooOld;

	geomPacket_512Mutex.lock();//LOCK 512 FT BUFFER
	bool foundMatch = search512FtBuffer(dataStruct.seconds, matched512Index, matchedPacketTooOld);

	//PACKET IS TOO OLD -- ORDER IT POPPED
	if (matchedPacketTooOld)
	{
		geomPacket_512Mutex.unlock(); //UNLOCK 512 FT BUFFER
		popIt = true;
		return false;
	}
	//WE FOUND A MATCH -- PLACE THE PACKET INTO 512 BUFFER
	if (foundMatch && matched512Index != -1)
	{
		//ORDER THE PACKET POPPED!
		popIt = true;

		//CLOSE OUT THE LAST 512 FT PACKET's INSPVAS
		if (matched512Index + 1 < 256)
		{
			geomPacket_512Buffer[matched512Index + 1].INSPVAS_complete = true;
		}

		//INDEX 0 IS THE NEWEST
		//SHIFT THE PACKETS IN ARRAY BY 1 INDEX
		for (int i = geomPacket_512Buffer[matched512Index].INSPVAS_BufferIndex; i > 0; i--)
		{
			geomPacket_512Buffer[matched512Index].buffer_INSPVAS[i] = geomPacket_512Buffer[matched512Index].buffer_INSPVAS[i - 1];
		}
		//WRITE IN THE NEW DATA
		geomPacket_512Buffer[matched512Index].buffer_INSPVAS[0] = dataStruct;
		if (geomPacket_512Buffer[matched512Index].INSPVAS_BufferIndex < COREGEOMPACKET_PACKETLEN - 1)
		{
			geomPacket_512Buffer[matched512Index].INSPVAS_BufferIndex++;//INCREMENT THE COUNTER
		}
	}
	geomPacket_512Mutex.unlock(); //UNLOCK 512 FT BUFFER
	return foundMatch;
}

//PLACE MESSAGE INTO MATCHED-FOOT BUFFER : INSSPDS
bool CoreGeometry::putMsgIntoCoreGeomPacket_INSSPDS(INSSPDS dataStruct, bool &popIt)
{
	popIt = false;
	int matched512Index;
	bool matchedPacketTooOld;

	geomPacket_512Mutex.lock();//LOCK 512 FT BUFFER
	bool foundMatch = search512FtBuffer(dataStruct.seconds, matched512Index, matchedPacketTooOld);

	//PACKET IS TOO OLD -- ORDER IT POPPED
	if (matchedPacketTooOld)
	{
		geomPacket_512Mutex.unlock(); //UNLOCK 512 FT BUFFER
		popIt = true;
		return false;
	}
	//WE FOUND A MATCH -- PLACE THE PACKET INTO 512 BUFFER
	if (foundMatch && matched512Index != -1)
	{
		//ORDER THE PACKET POPPED!
		popIt = true;

		//CLOSE OUT THE LAST 512 FT PACKET's INSSPDS
		if (matched512Index + 1 < 256)
		{
			geomPacket_512Buffer[matched512Index + 1].INSSPDS_complete = true;
		}

		//INDEX 0 IS THE NEWEST
		//SHIFT THE PACKETS IN ARRAY BY 1 INDEX
		for (int i = geomPacket_512Buffer[matched512Index].INSSPDS_BufferIndex; i > 0; i--)
		{
			geomPacket_512Buffer[matched512Index].buffer_INSSPDS[i] = geomPacket_512Buffer[matched512Index].buffer_INSSPDS[i - 1];
		}
		//WRITE IN THE NEW DATA
		geomPacket_512Buffer[matched512Index].buffer_INSSPDS[0] = dataStruct;
		if (geomPacket_512Buffer[matched512Index].INSSPDS_BufferIndex < COREGEOMPACKET_PACKETLEN - 1)
		{
			geomPacket_512Buffer[matched512Index].INSSPDS_BufferIndex++;//INCREMENT THE COUNTER
		}
	}
	geomPacket_512Mutex.unlock(); //UNLOCK 512 FT BUFFER
	return foundMatch;
}

//PLACE MESSAGE INTO MATCHED-FOOT BUFFER : CORRIMUDATAS
bool CoreGeometry::putMsgIntoCoreGeomPacket_CORRIMUDATAS(CORRIMUDATAS dataStruct, bool &popIt)
{
	popIt = false;
	int matched512Index;
	bool matchedPacketTooOld;

	geomPacket_512Mutex.lock();//LOCK 512 FT BUFFER
	bool foundMatch = search512FtBuffer(dataStruct.seconds, matched512Index, matchedPacketTooOld);

	//PACKET IS TOO OLD -- ORDER IT POPPED
	if (matchedPacketTooOld)
	{
		geomPacket_512Mutex.unlock(); //UNLOCK 512 FT BUFFER
		popIt = true;
		return false;
	}
	//WE FOUND A MATCH -- PLACE THE PACKET INTO 512 BUFFER
	if (foundMatch && matched512Index != -1)
	{
		//ORDER THE PACKET POPPED!
		popIt = true;

		//CLOSE OUT THE LAST 512 FT PACKET's CORRIMUDATAS
		if (matched512Index + 1 < 256)
		{
			geomPacket_512Buffer[matched512Index + 1].CORRIMUDATAS_complete = true;
		}

		//INDEX 0 IS THE NEWEST
		//SHIFT THE PACKETS IN ARRAY BY 1 INDEX
		for (int i = geomPacket_512Buffer[matched512Index].CORRIMUDATAS_BufferIndex; i > 0; i--)
		{
			geomPacket_512Buffer[matched512Index].buffer_CORRIMUDATAS[i] = geomPacket_512Buffer[matched512Index].buffer_CORRIMUDATAS[i - 1];
		}
		//WRITE IN THE NEW DATA
		geomPacket_512Buffer[matched512Index].buffer_CORRIMUDATAS[0] = dataStruct;
		if (geomPacket_512Buffer[matched512Index].CORRIMUDATAS_BufferIndex < COREGEOMPACKET_PACKETLEN - 1)
		{
			geomPacket_512Buffer[matched512Index].CORRIMUDATAS_BufferIndex++;//INCREMENT THE COUNTER
		}
	}
	geomPacket_512Mutex.unlock(); //UNLOCK 512 FT BUFFER
	return foundMatch;
}

//SEARCH THE SMALL FEET BUFFER TO DETERMINE IF THE DATA PACKET MATCHES ANY OF THEM
bool search512FtBuffer(double sec, int &index, bool &packetTooOld)
{
	packetTooOld = false;
	index = -1;

	//MAKE SURE WE HAVE AT LEAST 2 FEET POINTS IN GEOMETRY 512 FOOT BUFFER TO MATCH TO -- OTHERWISE EXIT LOOP//
	if (geomPacket_512Cnt < 2)
	{
		return false;
	}

	//DETERMINE IF THE PACKET IS TOO OLD -- IF SO, RETURN FROM LOOP : MAX geomPacket_512 INDEX IS THE OLDEST -- INDEX 0 = NEWEST

	if (geomPacket_512Buffer[geomPacket_512Cnt - 1].seconds >= sec)
	{
		packetTooOld = true;
		return false;
	}

	//SEARCH THE 512 BUFFER FOR MATCHING TIME
	for (int i = 1; i < geomPacket_512Cnt; i++) //NEED TO BE i = 1
	{

		if (sec > geomPacket_512Buffer[i].seconds && sec <= geomPacket_512Buffer[i - 1].seconds)
		{
			//RETURN TRUE
			index = i;
			return true;
		}
		else
		{
			//DO NOTHING
		}
	}
	return false;
}

//PLACE THE COMPLETED 'NEW FOOT' INTO THE 512 FOOT BUFFER AND EMPTY THE CURRENT FOOT BUFFER
void CoreGeometry::newFoot(MARKPVA dataStruct, MP_LOCATION location)
{
	//POSITION 0 IS THE NEWEST DATA

	//SHIFT BUFFER BY 1 FOOT -- THROWING LAST POSITION OUT
	geomPacket_512Mutex.lock();
	for (int i = geomPacket_512Cnt - 1; i > -1; i--) //NEED -1 BECAUSE 512 IS OUT OF BOUNDS!
	{
		geomPacket_512Buffer[i] = geomPacket_512Buffer[i - 1];
	}
	//WRITE IN THE NEW DATA
	geomPacket_512Buffer[0] = createCoreGeomPacket(dataStruct, location);

	if (geomPacket_512Cnt < 256)
	{
		geomPacket_512Cnt++;
	}
	geomPacket_512Mutex.unlock();

	condVar_512BufferNewFoot.notify_one();
}

//CALCULATE CROSSLEVEL FOR 1 FT
double calcCrosslevel1FT(int geomPacketIndex)
{
	//double roll = geomPacket_512Buffer[geomPacketIndex].oneFootPacket.roll;
	double roll = geomPacket_512Buffer[geomPacketIndex].buffer_INSPVAS[0].roll;
	double xl = std::sin(DEG2RAD * roll) * TRACKWIDTH;
	return xl;
}

//CALCULATE GRADE FOR 1 FT
double calcGrade1FT(int geomPacketIndex)
{
	//double pitch = geomPacket_512Buffer[geomPacketIndex].oneFootPacket.pitch;
	double pitch = geomPacket_512Buffer[geomPacketIndex].buffer_INSPVAS[0].pitch;
	double grade = std::tan(DEG2RAD * pitch) * 100;
	return grade;
}

//CALCULATE HORIZ RAIL ALIGNMENT -- PACKET INDEX OF THE 'CENTER' INDEX , R/L RAIL, ALIGNMENT LENGTH
bool calcHorizRailAlignmentMCO(int geomPacketIndex, int half_length, int full_length, double &leftRailMCO, double &rightRailMCO)
{
	////NON-COMPENSATED HORIZONTAL ALIGNMENT IS THE SAME FOR LEFT AND RIGHT RAILS

	int midIndex = geomPacketIndex;
	int startIndex = midIndex - half_length;
	int endIndex = midIndex + half_length;

	double MCO = -999;
	double cumY = 0;
	double cumHeading = 0;
	double priorHeadingVal = -999;
	double lastDeltaDist = -999;
	double yMid = -999;
	double yEnd = -999;

	for (int i = startIndex; i <= endIndex; i++) //CYCLE THROUGH 512 FT BUFFER
	{
		//MAKE SURE THAT THE COUNTS OF SUB-BUFFERS FOR SPEED AND ATTITUDES MATCH -- THIS MAKE BE PERM.
		if (geomPacket_512Buffer[i].INSSPDS_BufferIndex != geomPacket_512Buffer[i].INSPVAS_BufferIndex)
		{
			int WE_HAVE_A_SYNC_PROBLEM = 1;
		}

		//FIND A FIRST SPEED IF LAST DELTA DIST = -999;
		if (lastDeltaDist == -999)
		{
			for (int j = startIndex; j <= endIndex; j++)
			{
				if (geomPacket_512Buffer[j].INSSPDS_BufferIndex > 0)
				{
					lastDeltaDist = geomPacket_512Buffer[j].buffer_INSSPDS[0].horizontalSpeed * INSSMAPLEPERIOD * METERS_SECOND2IN_SECOND;
					j = 9999;//BREAK THE FOR LOOP
				}
			}
		}

		//LOOP THROUGH SUB-BUFFERS FOR SPEED AND ATTITUDE PACKETS
		for (int j = 0; j < geomPacket_512Buffer[i].INSPVAS_BufferIndex; j++)
		{
			//IF NEW SPEED EXISTS, UPDATE THE DELTA DIST -- OTHERWISE, USE LAST GOOD CAPTURE
			if (geomPacket_512Buffer[i].INSSPDS_BufferIndex > j)
			{
				double deltaDistance = geomPacket_512Buffer[i].buffer_INSSPDS[j].horizontalSpeed * INSSMAPLEPERIOD * METERS_SECOND2IN_SECOND;
				lastDeltaDist = deltaDistance;
			}

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
				priorHeadingVal = geomPacket_512Buffer[i].buffer_INSPVAS[j].azimuth;
			}

			//CALCULATE THE DELTA HEADING FROM LAST PACKET
			double deltaHeading = geomPacket_512Buffer[i].buffer_INSPVAS[j].azimuth - priorHeadingVal;
			priorHeadingVal = geomPacket_512Buffer[i].buffer_INSPVAS[j].azimuth;

			//ATTEMPTING TO DEAL WITH DELTAS OVER 359 DEG and 1 DEG HEADING
			if (deltaHeading >= 360)
			{
					deltaHeading = deltaHeading - 360.0;
			}
			else
			{
				if (deltaHeading <= -360)
				{
					deltaHeading = deltaHeading + 360.0;
				}
			}

			//ADD DELTA HEADING TO THE RELATIVE RUNNING HEADING
			cumHeading += deltaHeading;
			//CALC THE DELTA IN Y-DIRECTION ASSOCIATED WITH THE CUM. HEADING
			cumY += std::sin(DEG2RAD * cumHeading) * lastDeltaDist; //DISTANCE IN INCHES
		}
	}

	//Y END POINT = THE LAST cumY
	yEnd = cumY;

	//ASSURE THAT WE DID FIND A yMid POINT
	if (yMid != -999)
	{
		//CALCULATE THE MCO WITH EQUATION
		//MCO = [ gC(START) + gC(END) ] / 2 - gC(MID) 
		MCO = (0.0 - yEnd) / 2.0 - (-yMid);
		leftRailMCO = MCO;
		rightRailMCO = MCO;
		return true;
	}

	return false;
}

//CALCULATE VERT RAIL ALIGNMENT -- PACKET INDEX OF THE 'CENTER' INDEX, R/L RAIL, ALIGNMENT LENGTH
bool calcVertRailAlignmentMCO(int geomPacketIndex, int half_length, int full_length, double &leftRailMCO, double &rightRailMCO)
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
	double lastDeltaDist = -999;
	double zMid_L = -999;
	double zMid_R = -999;
	double zEnd_L = -999;
	double zEnd_R = -999;

	for (int i = startIndex; i >= endIndex; i--) //CYCLE THROUGH 512 FT BUFFER
	{
		//MAKE SURE THAT THE COUNTS OF SUB-BUFFERS FOR SPEED AND ATTITUDES MATCH
		if (geomPacket_512Buffer[i].INSSPDS_BufferIndex != geomPacket_512Buffer[i].INSPVAS_BufferIndex)
		{
			int WE_HAVE_A_SYNC_PROBLEM = 1;
		}

		//FIND A FIRST SPEED IF LAST DELTA DIST = -999;
		if (lastDeltaDist == -999)
		{
			for (int j = startIndex; j <= endIndex; j++)
			{
				if (geomPacket_512Buffer[j].INSSPDS_BufferIndex > 0)
				{
					lastDeltaDist = geomPacket_512Buffer[j].buffer_INSSPDS[0].horizontalSpeed * INSSMAPLEPERIOD * METERS_SECOND2IN_SECOND;
					j = 9999;//BREAK THE FOR LOOP
				}
			}
		}

		//LOOP THROUGH SUB-BUFFERS FOR SPEED AND ATTITUDE PACKETS
		for (int j = geomPacket_512Buffer[i].INSPVAS_BufferIndex - 1; j >= 0; j--)
		{
			//IF NEW SPEED EXISTS, UPDATE THE DELTA DIST -- OTHERWISE, USE LAST GOOD CAPTURE
			if (geomPacket_512Buffer[i].INSSPDS_BufferIndex > j)
			{
				double deltaDistance = geomPacket_512Buffer[i].buffer_INSSPDS[j].horizontalSpeed * INSSMAPLEPERIOD * METERS_SECOND2IN_SECOND;
				lastDeltaDist = deltaDistance;
			}

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
				priorPitchVal = geomPacket_512Buffer[i].buffer_INSPVAS[j].pitch;
			}

			//IF WE DON'T HAVE A VALID LAST-ROLL, USE THE CURRENT ROLL
			if (priorRollVal == -999)
			{
				priorRollVal = geomPacket_512Buffer[i].buffer_INSPVAS[j].roll;
			}


			//CALCULATE THE DELTA PITCH FROM LAST PACKET
			double deltaPitch = geomPacket_512Buffer[i].buffer_INSPVAS[j].pitch - priorPitchVal;
			priorPitchVal = geomPacket_512Buffer[i].buffer_INSPVAS[j].pitch;
			double deltaRoll = geomPacket_512Buffer[i].buffer_INSPVAS[j].roll - priorRollVal;
			priorRollVal = geomPacket_512Buffer[i].buffer_INSPVAS[j].roll;

			//ADD DELTA PITCH TO THE RELATIVE RUNNING PITCH
			cumPitch += deltaPitch;
			//ADD DELTA ROLL TO THE RELATIVE RUNNING ROLL
			cumRoll += deltaRoll;
			//CALC THE DELTA IN Y-DIRECTION ASSOCIATED WITH THE CUM. HEADING
			cumZ_R += std::sin(DEG2RAD * cumPitch) * lastDeltaDist + std::sin(DEG2RAD * deltaRoll) * RIGHT_RAIL_LEVER_ARM; //DISTANCE IN INCHES
			cumZ_L += std::sin(DEG2RAD * cumPitch) * lastDeltaDist + std::sin(DEG2RAD * deltaRoll) * LEFT_RAIL_LEVER_ARM; //DISTANCE IN INCHES
		}
	}

	//Z END POINT = THE LAST cumZ
	zEnd_R = cumZ_R;
	zEnd_L = cumZ_L;

	//ASSURE THAT WE DID FIND A zMid POINT
	if (zMid_L != -999 && zMid_R != -999)
	{
		//CALCULATE THE MCO WITH EQUATION
		//MCO = [ gC(START) + gC(END) ] / 2 - gC(MID) 
		MCO_L = -zEnd_L / 2.0 + zMid_L;
		MCO_R = -zEnd_R / 2.0 + zMid_R;
		leftRailMCO = MCO_L;
		rightRailMCO = MCO_R;
		return true;
	}

	return false;
}

//CALCULATE CURVATURE -- PACKET INDEX OF THE 'CENTER' INDEX
bool calcCurvature(int geomPacketIndex, double &curvature)
{
	int midIndex = geomPacketIndex;
	int startIndex = midIndex - 31;
	int endIndex = midIndex + 31;

	double cumAngle = 0.0;
	double cumSpeed = 0.0;
	double sampleCnt = 0.0;

	double priorHeadingVal = -999.0;
	double lastSpeed = -999.0;


	for (int i = startIndex; i <= endIndex; i++) //CYCLE THROUGH 512 FT BUFFER
	{
		//MAKE SURE THAT THE COUNTS OF SUB-BUFFERS FOR SPEED AND ATTITUDES MATCH
		if (geomPacket_512Buffer[i].INSSPDS_BufferIndex != geomPacket_512Buffer[i].INSPVAS_BufferIndex)
		{
			int WE_HAVE_A_SYNC_PROBLEM = 1;
		}

		//FIND A FIRST SPEED IF LAST DELTA DIST = -999;
		if (lastSpeed == -999.0)
		{
			for (int j = startIndex; j <= endIndex; j++)
			{
				if (geomPacket_512Buffer[j].INSSPDS_BufferIndex > 0)
				{
					lastSpeed = geomPacket_512Buffer[j].buffer_INSSPDS[0].horizontalSpeed; //DO I NEED TO CONVERT FROM METERS/SECOND????
					j = 9999;//BREAK THE FOR LOOP
				}
			}
		}

		//LOOP THROUGH SUB-BUFFERS FOR SPEED AND ATTITUDE PACKETS
		for (int j = 0; j < geomPacket_512Buffer[i].INSPVAS_BufferIndex; j++)
		{
			//IF NEW SPEED EXISTS, UPDATE THE DELTA DIST -- OTHERWISE, USE LAST GOOD CAPTURE
			if (geomPacket_512Buffer[i].INSSPDS_BufferIndex > j)
			{
				double currentSpeed = geomPacket_512Buffer[i].buffer_INSSPDS[j].horizontalSpeed;
				lastSpeed = currentSpeed;
			}


			//IF WE DON'T HAVE A VALID LAST-HEADING, USE THE CURRENT HEADING
			if (priorHeadingVal == -999)
			{
				priorHeadingVal = geomPacket_512Buffer[i].buffer_INSPVAS[j].azimuth;
			}

			//CALCULATE THE DELTA HEADING FROM LAST PACKET
			double deltaHeading = geomPacket_512Buffer[i].buffer_INSPVAS[j].azimuth - priorHeadingVal;
			priorHeadingVal = geomPacket_512Buffer[i].buffer_INSPVAS[j].azimuth;

			//ATTEMPTING TO DEAL WITH DELTAS OVER 359 DEG and 1 DEG HEADING
			if (deltaHeading >= 360)
			{
				deltaHeading = deltaHeading - 360.0;
			}
			else
			{
				if (deltaHeading <= -360)
				{
					deltaHeading = deltaHeading + 360.0;
				}
			}

			//ADD DELTA HEADING TO THE RELATIVE RUNNING HEADING
			cumAngle += deltaHeading * DEG2RAD;
			sampleCnt++;
			cumSpeed += lastSpeed;
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
		curvature = railroadCurvature;
		return true;
	}
	return false;
}

//CREATE A NEW GEOMETRY FILE
bool CoreGeometry::createGeometryFile(CString fileName)
{
	geometryFSize = 0;
	geometryFileStream.open(fileName);

	//GET DATE-TIME
	// current date/time based on current system
	time_t now = time(0);

	// convert now to string form
	//char* dt = ctime(&now);
	char dt[26];
	ctime_s(dt, sizeof(dt), &now);
	geometryFileStream << dt + '\n';
	geometryFileStream << "INDEX;SEC_VALIDITY;CROSSLEVEL;GRADE;L_VERT_MCO_11;R_VERT_MCO_11;L_VERT_MCO_31;R_VERT_MCO_31;L_VERT_MCO_62;R_VERT_MCO_62;L_HORIZ_MCO_31;R_HORIZ_MCO_31;L_HORIZ_MCO_62;R_HORIZ_MCO_62;L_HORIZ_MCO_124;R_HORIZ_MCO_124;CURV_140\n";
	
	return true;
}

//CREATE A NEW LINE IN GEOMETRY FILE
//bool CoreGeometry::newGeometryFileLine(CoreGeomCalculationPacket p)
//{
//	std::string	aLine = "";
//	char tLine[210];
//	sprintf(tLine, "%d;%f;%f;%f;%f;%f;%f;%f;%f;%f;%f;%f;%f;%f;%f;%f;%f\n",
//							 lineCnt, p.secValidity,
//							 p.crosslevel_raw, p.grade_raw,
//							 p.L_VERT_MCO_11_raw, p.R_VERT_MCO_11_raw, 
//							 p.L_VERT_MCO_31_raw, p.R_VERT_MCO_31_raw, 
//							 p.L_VERT_MCO_62_raw, p.R_VERT_MCO_62_raw,
//							 p.L_HORIZ_MCO_31_raw, p.R_HORIZ_MCO_31_raw,
//							 p.L_HORIZ_MCO_62_raw, p.R_HORIZ_MCO_62_raw,
//							 p.L_HORIZ_MCO_124_raw, p.R_HORIZ_MCO_124_raw,
//							 p.rawCurvature);
//
//	aLine += tLine;
//
//	geometryFileStream << aLine;
//	geometryFSize = sizeof(aLine) + geometryFSize;
//	lineCnt++;
//
//	return true;
//}

//INSERT A NEW CALCULATION PACKET INTO THE BUFFER
bool insertNewCalcPacket(CoreGeomCalculationPacket p)
{
	calcPacket_Mutex.lock();
	calcPacket_buffer.push(p);
	calcPacket_Mutex.unlock();
	condVar_calcPacketBuffer.notify_one();

	return true;
}

//REMOVE A CALCULATION PACKET FROM THE BUFFER
bool CoreGeometry::getCalcPacket(CoreGeomCalculationPacket &p)
{
	std::unique_lock<std::mutex> l(calcPacket_Mutex);
	condVar_calcPacketBuffer.wait(l, [] {return calcPacket_buffer.size() > 0; });
	int bufferSize = calcPacket_buffer.size();
	if(bufferSize > 0)
	{
		p = calcPacket_buffer.front();
		calcPacket_buffer.pop();
		if (bufferSize > 10)
		{
			int breakHere = 1;
		}
	}
	l.unlock();

	return true;
}


