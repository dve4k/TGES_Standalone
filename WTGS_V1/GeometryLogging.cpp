#include "stdafx.h"
#include "GeometryLogging.h"

//PROTOTYPES
void openGeoFile(CString fileName1);
void checkGeoRollFile();
void writeGeomData(CoreGeomCalculationPacket p);
bool getGeoLoggingQue(CoreGeomCalculationPacket &p);

//GLOBALS
//TRACKING FILE SIZE
long geoFSize = 0;
int geoLineCnt = 0;

long calcFSize = 0;
int calcLineCnt = 0;

long exceptionNumber = 0;

//GLOBAL FLAG FOR LOGGING WRITE LOOP
bool geoLoggingLoopEnable = false;

char vehicleIDGlobal[10];

//OUTPUT FILE
std::ofstream geoFileStream;
std::ofstream calcFileStream;

//INCOMING DATA TO LOG QUE
std::queue<CoreGeomCalculationPacket> geoLogging_que;
std::mutex geoLogging_mutex;
std::condition_variable geoLogging_condVar;

//CONSTRUCTORS
GeometryLogging::GeometryLogging()
{
	geoLoggingLoopEnable = true;
	int buffSize = geoLogging_que.size();
	for (int i = 0; i < buffSize; i++)
	{
		geoLogging_que.pop();
	}
	exceptionNumber = 0;
}

GeometryLogging::GeometryLogging(char* vehicleID)
{
	memcpy(vehicleIDGlobal, vehicleID, 10);

	geoLoggingLoopEnable = true;
	int buffSize = geoLogging_que.size();
	for (int i = 0; i < buffSize; i++)
	{
		geoLogging_que.pop();
	}
	exceptionNumber = 0;
}
GeometryLogging::~GeometryLogging()
{

}

//WRTIE A GEOMETRY PACKET TO FILE
void writeGeomData(CoreGeomCalculationPacket p)
{
	//GET CURRENT DATE/TIME
	std::chrono::time_point<std::chrono::system_clock> timePoint = std::chrono::system_clock::now();
	time_t now = time(0);
	struct tm* timeinfo;
	timeinfo = localtime(&now);
	char dt[80];// = ctime(&now);
	strftime(dt, 80, "%Y-%m-%d %H:%M:%S", timeinfo);

	//ASSIGN LOCOMOTIVE ORIENTATION FROM THE STARTUP DIRECTION
	//1 = F
	//-1 = R
	char locoOrientation = 'F';
	if (p.measuringDirection == -1)
	{
		locoOrientation = 'R';
	}

	std::string	aLine = "";
	char tLine[800];
	sprintf(tLine, "%d;%f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%11.8f;%11.8f;%6.3f;%6.3f;%d;%d;%d;%s;%c;%c;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%s;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%s",
		geoLineCnt, p.secValidity,
		p.crosslevel_NOC, p.grade_raw,
		p.L_VERT_MCO_11_NOC, p.R_VERT_MCO_11_NOC,
		p.L_VERT_MCO_31_NOC, p.R_VERT_MCO_31_NOC,
		p.L_VERT_MCO_62_NOC, p.R_VERT_MCO_62_NOC,
		p.L_HORIZ_MCO_31_NOC, p.R_HORIZ_MCO_31_NOC,
		p.L_HORIZ_MCO_62_NOC, p.R_HORIZ_MCO_62_NOC,
		p.L_HORIZ_MCO_124_NOC, p.R_HORIZ_MCO_124_NOC,
		p.rawCurvature, p.filteredCurvature,
		p.latitude, p.longitude,
		p.altitude, p.speed, 
		p.location.MP, p.location.FTCnt,
		p.location.trackNumber, p.location.RID,
		p.location.mpSequence, /*p.location.carOrientation*/locoOrientation,
		p.combinationLeft, p.combinationRight,
		p.fraRunoff, p.fraWarp62, 
		p.fullGage_raw, p.lHalfGage,
		p.rHalfGage, p.twist11,
		p.variationCurvature, p.variationGage,
		p.verticalBounce, /*p.multipleTwist11*/0.0f,
		p.L_VERT_MCO_11_OC, p.R_VERT_MCO_11_OC,
		p.L_VERT_MCO_31_OC, p.R_VERT_MCO_31_OC,
		p.L_VERT_MCO_62_OC, p.R_VERT_MCO_62_OC,
		p.L_HORIZ_MCO_31_OC, p.R_HORIZ_MCO_31_OC,
		p.L_HORIZ_MCO_62_OC, p.R_HORIZ_MCO_62_OC,
		p.L_HORIZ_MCO_124_OC, p.R_HORIZ_MCO_124_OC,
		p.crosslevel_OC, p.lVertGage,
		p.rVertGage, p.fullGage_filtered, 
		dt, p.crosslevel_OC_SWA, 
		p.lHalfGage_filtered, p.rHalfGage_filtered,
		p.lVertGage_filtered, p.rVertGage_filtered,
		p.twist11_SWA, p.fraWarp62_SWA, 
		p.fraHarmonic, p.vehicleID);

	aLine += tLine;

	//OUPUT VXL (Normal)
	for (int i = 0; i < 43; i++)
	{
		char someXLData[10];
		sprintf(someXLData, ";%6.3f", p.variationCrosslevel[i]);
		aLine += someXLData;
	}

	//OUTPUT VXL (SWA)
	for (int i = 0; i < 43; i++)
	{
		char someXLData[10];
		sprintf(someXLData, ";%6.3f", p.variationCrosslevel_SWA[i]);
		aLine += someXLData;
	}

	geoFileStream << aLine;
	geoFileStream << '\n';

	geoFSize = aLine.length() + geoFSize;
	geoLineCnt++;
}

//OPEN A FILE FOR GEOMETRY LOGGING -- SIMULATION OUTPUT
void openGeoFile(CString fileName1)
{
	geoFSize = 0;
	if (geoFileStream.is_open())
	{
		geoFileStream.close();
	}
	geoFileStream.open(fileName1);
	//GET DATE-TIME
	// current date/time based on current system
	time_t now = time(0);

	geoFileStream << "INDEX;SEC_VALIDITY;CROSSLEVEL_NOC;GRADE;L_VERT_MCO_11_NOC;R_VERT_MCO_11_NOC;L_VERT_MCO_31_NOC;R_VERT_MCO_31_NOC;L_VERT_MCO_62_NOC;R_VERT_MCO_62_NOC;L_HORIZ_MCO_31_NOC;R_HORIZ_MCO_31_NOC;L_HORIZ_MCO_62_NOC;R_HORIZ_MCO_62_NOC;L_HORIZ_MCO_124_NOC;R_HORIZ_MCO_124_NOC;CURV_140_RAW_NOC;CURV_140_FIR;LAT;LON;ALT;SPEED;MP;FT;TRACK;RID;MP_SEQ;CAR_ORIEN;COMB_LEFT;COMB_RIGHT;FRA_RUNOFF;FRA_WARP62;FULL_GAGE_RAW;L_HALFGAGE;R_HALFGAGE;TWIST11;VARIATION_CURV;VARIATION_GAGE;VERT_BOUNCE;MULT_TWIST;L_VERT_MCO_11_OC;R_VERT_MCO_11_OC;L_VERT_MCO_31_OC;R_VERT_MCO_31_OC;L_VERT_MCO_62_OC;R_VERT_MCO_62_OC;L_HORIZ_MCO_31_OC;R_HORIZ_MCO_31_OC;L_HORIZ_MCO_62_OC;R_HORIZ_MCO_62_OC;L_HORIZ_MCO_124_OC;R_HORIZ_MCO_124_OC;CROSSLEVEL_OC;L_VERTGAGE;R_VERTGAGE;FULL_GAGE_FILT;LINE_DT;CROSSLEVEL_OC_SWA;L_HALFGAGE_FILT;R_HALFGAGE_FILT;L_VERTGAGE_FILT;R_VERTGAGE_FILT;TWIST11_SWA;FRA_WARP62_SWA;FRA_HARMONIC;VEHICLE_NAME;VXL_20;VXL_21;VXL_22;VXL_23;VXL_24;VXL_25;VXL_26;VXL_27;VXL_28;VXL_29;VXL_30;VXL_31;VXL_32;VXL_33;VXL_34;VXL_35;VXL_36;VXL_37;VXL_38;VXL_39;VXL_40;VXL_41;VXL_42;VXL_43;VXL_44;VXL_45;VXL_46;VXL_47;VXL_48;VXL_49;VXL_50;VXL_51;VXL_52;VXL_53;VXL_54;VXL_55;VXL_56;VXL_57;VXL_58;VXL_59;VXL_60;VXL_61;VXL_62;VXL_SWA_20;VXL_SWA_21;VXL_SWA_22;VXL_SWA_23;VXL_SWA_24;VXL_SWA_25;VXL_SWA_26;VXL_SWA_27;VXL_SWA_28;VXL_SWA_29;VXL_SWA_30;VXL_SWA_31;VXL_SWA_32;VXL_SWA_33;VXL_SWA_34;VXL_SWA_35;VXL_SWA_36;VXL_SWA_37;VXL_SWA_38;VXL_SWA_39;VXL_SWA_40;VXL_SWA_41;VXL_SWA_42;VXL_SWA_43;VXL_SWA_44;VXL_SWA_45;VXL_SWA_46;VXL_SWA_47;VXL_SWA_48;VXL_SWA_49;VXL_SWA_50;VXL_SWA_51;VXL_SWA_52;VXL_SWA_53;VXL_SWA_54;VXL_SWA_55;VXL_SWA_56;VXL_SWA_57;VXL_SWA_58;VXL_SWA_59;VXL_SWA_60;VXL_SWA_61;VXL_SWA_62\n";
}

//CHECK TO SEE IF NEED TO ROLL GEOMETRY FILE
void checkGeoRollFile()
{
	if (geoFSize > GEOMETRY_MAX_LOG_SIZE) //IN KB? 500,000KB = 500MB -- I THINK ITS IN BYTES....500MB = 500000000
	{
		//ROLL TO A NEW FILE
		//GET CURRENT TIME
		std::chrono::time_point<std::chrono::system_clock> timePoint = std::chrono::system_clock::now();
		time_t now = time(0);
		struct tm * timeinfo;
		timeinfo = localtime(&now);
		char dt[80];// = ctime(&now);
		strftime(dt, 80, "%Y%m%d%H%M%S", timeinfo);
		CString cstringDt1 = LOG_DIRECTORY + CString(dt) + "_" + CString(vehicleIDGlobal) + "_GEO.csv";

		//CLOSE THE EXISTING FILES
		geoFileStream.close();

		//OPEN NEW FILES
		openGeoFile(cstringDt1);
	}
}

//PUT A GEOMETRY PACKET ON QUE TO BE LOGGED
void GeometryLogging::putGeoLoggingQue(CoreGeomCalculationPacket p)
{
	std::unique_lock<std::mutex> l(geoLogging_mutex);
	geoLogging_que.push(p);

	l.unlock();
	geoLogging_condVar.notify_one();
}

std::vector<CoreGeomCalculationPacket> next15Packets = {};
bool firstTime = true;
//GET A GEOMETRY PACKET FROM LOGGING QUE
bool getGeoLoggingQue(CoreGeomCalculationPacket &p)
{
	std::unique_lock<std::mutex> l(geoLogging_mutex);

	if(firstTime)
		geoLogging_condVar.wait_for(l, std::chrono::milliseconds(100), [] {return geoLogging_que.size() > 15; });
	else
		geoLogging_condVar.wait_for(l, std::chrono::milliseconds(100), [] {return geoLogging_que.size() > 0; });

	int bufferSize = geoLogging_que.size();
	if (firstTime && bufferSize > 15)
	{
		firstTime = false;
		if (bufferSize > 20)
		{
			int breakHere = 1;
		}
		p = geoLogging_que.front();
		if (p.measuringDirection == -1)
		{
			for (int i = 0; i < 15; i++)
			{
				p = geoLogging_que.front();
				next15Packets.push_back(p);
				geoLogging_que.pop();
			}
			next15Packets[0].latitude = next15Packets[14].latitude;
			next15Packets[0].longitude = next15Packets[14].longitude;
			next15Packets[0].altitude = next15Packets[14].altitude;
			p = next15Packets[0];
			
			for (int i = 0; i < 14; i++)
			{
				next15Packets[i] = next15Packets[i + 1];
			}

			next15Packets.pop_back();
		}
		else
		{
			geoLogging_que.pop();
		}
		l.unlock();
		return true;
	}
	else if(!firstTime && bufferSize > 0)
	{
		if (bufferSize > 20)
		{
			int breakHere = 1;
		}
		p = geoLogging_que.front();
		if (p.measuringDirection == -1 && next15Packets.size() >= 14)
		{
			next15Packets.push_back(p);
			geoLogging_que.pop();
			next15Packets[0].latitude = next15Packets[14].latitude;
			next15Packets[0].longitude = next15Packets[14].longitude;
			next15Packets[0].altitude = next15Packets[14].altitude;
			p = next15Packets[0];

			for (int i = 0; i < 14; i++)
			{
				next15Packets[i] = next15Packets[i + 1];
			}

			next15Packets.pop_back();
		}
		else if (p.measuringDirection == -1)
		{
			next15Packets.push_back(p);
			l.unlock();
			return false;
		}
		else
		{
			geoLogging_que.pop();
		}
		l.unlock();
		return true;
	}
	

	l.unlock();
	return false;
}

//WHILE LOOP FOR WRITING GEOMETRY DATA TO FILE
void GeometryLogging::geoLoggingProcessor()
{
	while (geoLoggingLoopEnable)
	{
		CoreGeomCalculationPacket tmpCGCP;
		bool gotData = getGeoLoggingQue(tmpCGCP);
		if (gotData)
		{
			writeGeomData(tmpCGCP);
			checkGeoRollFile();
		}
	}
}

//FUNCTION FOR CALL FROM OTHER CLASS TO OPEN GEOMETRY FILES 
void GeometryLogging::openFile()
{
	//GET CURRENT TIME
	std::chrono::time_point<std::chrono::system_clock> timePoint = std::chrono::system_clock::now();
	time_t now = time(0);
	struct tm * timeinfo;
	timeinfo = localtime(&now);
	char dt[80];// = ctime(&now);
	strftime(dt, 80, "%Y%m%d%H%M%S", timeinfo);
	CString cstringDt1 = LOG_DIRECTORY + CString(dt) + "_" + CString(vehicleIDGlobal) + "_GEO.csv";
	geoLineCnt = 0;
	openGeoFile(cstringDt1);
}

//SHUTDOWN GEOMETRY LOGGING
bool GeometryLogging::shutdownGeometryLogging()
{
	geoLoggingLoopEnable = false;
	return true;
}

//CREATE AND WRITE AN EXCEPTION FILE
bool GeometryLogging::createWriteException(WTGS_Exception exception, CoreGeomCalculationPacket *exceptionGeomData)
{
	exceptionNumber++;
	//CREATE A FILE NAME
	std::chrono::time_point<std::chrono::system_clock> timePoint = std::chrono::system_clock::now();
	time_t now = time(0);
	struct tm * timeinfo;
	timeinfo = localtime(&now);
	char dt[80];
	strftime(dt, 80, "%Y%m%d%H%M%S", timeinfo);
	char justDate[20];
	strftime(justDate, 20, "%Y%m%d", timeinfo);
	char justTime[20];
	strftime(justTime, 20, "%H%M%S", timeinfo);
	char excNumberStr[10];
	sprintf(excNumberStr, "_%d", exceptionNumber);
	CString cstringDt1 = EXCEPTION_DIRECTORY + CString(dt) + excNumberStr + "_" + exception.vehicleName + "_EXCEPTION.csv";
	CString cstringDt2 = EXCEPTION_DIRECTORY + CString(dt) + excNumberStr + "_" + exception.vehicleName + "_EXCDATA.csv";
	//OPEN A FILE
	std::ofstream exceptionStream;
	exceptionStream.open(cstringDt1);

	std::ofstream excDataStream;
	excDataStream.open(cstringDt2);

	//WRITE THE FILE HEADER
	exceptionStream << "DATE;TIME;GPSSEC;LAT;LON;ALT;TYPE;AMPLITUDE;SEVERITY;BASELENGTH;LENGTH;TRACK_TYPE;FRA_EXCEPTION;TRAVEL_SPEED;TRACK_CLASS;MP;FT;MP_SEQ;CAR_ORIEN;RID;TRACK_NUMBER;VEHICLE_NAME;FILE_NAME\n";
	
	//WRITE EXCEPTION STRING TO BUFF & FILE
	char charBuff[250];
	sprintf(charBuff, "%s;%s;%f;%11.8f;%11.8f;%5.1f;%2d;%6.3f;%2d;%4d;%5d;%2d;%2d;%5.1f;%2d;%3d;%5d;%c;%c;%s;%3d;%s;%s\n", 
			justDate, justTime, 
			exception.secValidity, exception.latitude,
			exception.longitude, exception.altitude, 
			exception.exceptionType, exception.exceptionAmplitude,
			exception.exceptionSeverity, exception.exceptionBaseLength,
			exception.exceptionLength, exception.trackType, 
			exception.FRA_Exception, exception.speedTraveling,
			exception.trackClass, exception.max_location.MP,
			exception.max_location.FTCnt, exception.max_location.mpSequence,
			exception.max_location.carOrientation, exception.max_location.RID,
			exception.max_location.trackNumber, exception.vehicleName,
			CString(dt) + excNumberStr + "_" + exception.vehicleName + "_EXCEPTION.csv");
	exceptionStream << charBuff;
	//CLOSE THE FILE
	exceptionStream.close();

	//WRITE EXCEPTION GEOMETRY DATA FILE
	excDataStream << "INDEX; SEC_VALIDITY; CROSSLEVEL_NOC; GRADE; L_VERT_MCO_11_NOC; R_VERT_MCO_11_NOC; L_VERT_MCO_31_NOC; R_VERT_MCO_31_NOC; L_VERT_MCO_62_NOC; R_VERT_MCO_62_NOC; L_HORIZ_MCO_31_NOC; R_HORIZ_MCO_31_NOC; L_HORIZ_MCO_62_NOC; R_HORIZ_MCO_62_NOC; L_HORIZ_MCO_124_NOC; R_HORIZ_MCO_124_NOC; CURV_140_RAW_NOC; CURV_140_FIR; LAT; LON; ALT; SPEED; MP; FT; TRACK; RID; MP_SEQ; CAR_ORIEN; COMB_LEFT; COMB_RIGHT; FRA_RUNOFF; FRA_WARP62; FULL_GAGE_RAW; L_HALFGAGE; R_HALFGAGE; TWIST11; VARIATION_CURV; VARIATION_GAGE; VERT_BOUNCE; MULT_TWIST; L_VERT_MCO_11_OC; R_VERT_MCO_11_OC; L_VERT_MCO_31_OC; R_VERT_MCO_31_OC; L_VERT_MCO_62_OC; R_VERT_MCO_62_OC; L_HORIZ_MCO_31_OC; R_HORIZ_MCO_31_OC; L_HORIZ_MCO_62_OC; R_HORIZ_MCO_62_OC; L_HORIZ_MCO_124_OC; R_HORIZ_MCO_124_OC; CROSSLEVEL_OC; L_VERTGAGE; R_VERTGAGE; FULL_GAGE_FILT; LINE_DT; CROSSLEVEL_OC_SWA; L_HALFGAGE_FILT; R_HALFGAGE_FILT; L_VERTGAGE_FILT; R_VERTGAGE_FILT; TWIST11_SWA; FRA_WARP62_SWA; FRA_HARMONIC; VEHICLE_NAME; VXL_20; VXL_21; VXL_22; VXL_23; VXL_24; VXL_25; VXL_26; VXL_27; VXL_28; VXL_29; VXL_30; VXL_31; VXL_32; VXL_33; VXL_34; VXL_35; VXL_36; VXL_37; VXL_38; VXL_39; VXL_40; VXL_41; VXL_42; VXL_43; VXL_44; VXL_45; VXL_46; VXL_47; VXL_48; VXL_49; VXL_50; VXL_51; VXL_52; VXL_53; VXL_54; VXL_55; VXL_56; VXL_57; VXL_58; VXL_59; VXL_60; VXL_61; VXL_62; VXL_SWA_20; VXL_SWA_21; VXL_SWA_22; VXL_SWA_23; VXL_SWA_24; VXL_SWA_25; VXL_SWA_26; VXL_SWA_27; VXL_SWA_28; VXL_SWA_29; VXL_SWA_30; VXL_SWA_31; VXL_SWA_32; VXL_SWA_33; VXL_SWA_34; VXL_SWA_35; VXL_SWA_36; VXL_SWA_37; VXL_SWA_38; VXL_SWA_39; VXL_SWA_40; VXL_SWA_41; VXL_SWA_42; VXL_SWA_43; VXL_SWA_44; VXL_SWA_45; VXL_SWA_46; VXL_SWA_47; VXL_SWA_48; VXL_SWA_49; VXL_SWA_50; VXL_SWA_51; VXL_SWA_52; VXL_SWA_53; VXL_SWA_54; VXL_SWA_55; VXL_SWA_56; VXL_SWA_57; VXL_SWA_58; VXL_SWA_59; VXL_SWA_60; VXL_SWA_61; VXL_SWA_62; EXCEPTION_FILE_NAME\n";

	for (int l = 0; l < 501; l++)
	{
		CoreGeomCalculationPacket p = exceptionGeomData[l];
		std::string	aLine = "";
		char tLine[900];
		//ASSIGN LOCOMOTIVE ORIENTATION FROM THE STARTUP DIRECTION
		//1 = F
		//-1 = R
		char locoOrientation = 'F';
		if (p.measuringDirection == -1)
		{
			locoOrientation = 'R';
		}
		sprintf(tLine, "%d;%f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%11.8f;%11.8f;%6.3f;%6.3f;%d;%d;%d;%s;%c;%c;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%s;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%6.3f;%s",
			geoLineCnt, p.secValidity,
			p.crosslevel_NOC, p.grade_raw,
			p.L_VERT_MCO_11_NOC, p.R_VERT_MCO_11_NOC,
			p.L_VERT_MCO_31_NOC, p.R_VERT_MCO_31_NOC,
			p.L_VERT_MCO_62_NOC, p.R_VERT_MCO_62_NOC,
			p.L_HORIZ_MCO_31_NOC, p.R_HORIZ_MCO_31_NOC,
			p.L_HORIZ_MCO_62_NOC, p.R_HORIZ_MCO_62_NOC,
			p.L_HORIZ_MCO_124_NOC, p.R_HORIZ_MCO_124_NOC,
			p.rawCurvature, p.filteredCurvature,
			p.latitude, p.longitude,
			p.altitude, p.speed,
			p.location.MP, p.location.FTCnt,
			p.location.trackNumber, p.location.RID,
			p.location.mpSequence, /*p.location.carOrientation*/locoOrientation,
			p.combinationLeft, p.combinationRight,
			p.fraRunoff, p.fraWarp62,
			p.fullGage_raw, p.lHalfGage,
			p.rHalfGage, p.twist11,
			p.variationCurvature, p.variationGage,
			p.verticalBounce, /*p.multipleTwist11*/0.0f,
			p.L_VERT_MCO_11_OC, p.R_VERT_MCO_11_OC,
			p.L_VERT_MCO_31_OC, p.R_VERT_MCO_31_OC,
			p.L_VERT_MCO_62_OC, p.R_VERT_MCO_62_OC,
			p.L_HORIZ_MCO_31_OC, p.R_HORIZ_MCO_31_OC,
			p.L_HORIZ_MCO_62_OC, p.R_HORIZ_MCO_62_OC,
			p.L_HORIZ_MCO_124_OC, p.R_HORIZ_MCO_124_OC,
			p.crosslevel_OC, p.lVertGage,
			p.rVertGage, p.fullGage_filtered,
			dt, p.crosslevel_OC_SWA,
			p.lHalfGage_filtered, p.rHalfGage_filtered,
			p.lVertGage_filtered, p.rVertGage_filtered,
			p.twist11_SWA, p.fraWarp62_SWA,
			p.fraHarmonic, p.vehicleID);

		aLine += tLine;
		//OUPUT VXL (Normal)
		for (int i = 0; i < 43; i++)
		{
			char someXLData[10];
			sprintf(someXLData, ";%6.3f", p.variationCrosslevel[i]);
			aLine += someXLData;
		}

		//OUTPUT VXL (SWA)
		for (int i = 0; i < 43; i++)
		{
			char someXLData[10];
			sprintf(someXLData, ";%6.3f", p.variationCrosslevel_SWA[i]);
			aLine += someXLData;
		}

		aLine += ";" + CString(dt) + excNumberStr + "_" + exception.vehicleName + "_EXCEPTION.csv\n";
		excDataStream << aLine;
	}

	excDataStream.close();

	return true;
}


