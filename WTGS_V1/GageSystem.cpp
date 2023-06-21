#include "stdafx.h"
#include "GageSystem.h"

//#pragma optimize("", off)

//PROTOTYPES
unsigned __stdcall	getProfile(void* args);
unsigned __stdcall	setupScanners(void* args);
void				resetScannersForOperation(Scanners &scanners);
void				stopScannersAcq(Scanners &scanners);
unsigned __stdcall	writeProfile(void* args);
void				loadGageIniFile();
unsigned __stdcall	gageCollectingThread(void* args);
bool putGageOutput(GageGeometry_Data packet);


//GLOBALS
bool gageSystemCollecting = false;
bool gageSystemEnabled = true;
HANDLE gageCaptureHandle;
unsigned gageCaptureID;
long globalPacketCount;

//IMAGE PROCESSING STUFF
double gs_WINDOW_CENTER_X_S1 = 491.0; // NS7538480.0 //CENTER X
double gs_WINDOW_CENTER_Y_S1 = 325.0; //NS7538 291.0 //CENTER Y
double gs_CV2_IMAGE_MIN_X_1 = gs_WINDOW_CENTER_X_S1 - WINDOW_WIDTH_S1 / 2.0;
double gs_CV2_IMAGE_MIN_Y_1 = gs_WINDOW_CENTER_Y_S1 - WINDOW_HEIGHT_S1 / 2.0;
double gs_CV2_IMAGE_MAX_X_1 = gs_WINDOW_CENTER_X_S1 + WINDOW_WIDTH_S1 / 2.0;
double gs_CV2_IMAGE_MAX_Y_1 = gs_WINDOW_CENTER_Y_S1 + WINDOW_HEIGHT_S1 / 2.0;
double gs_WINDOW_CENTER_X_S2 = 448.0; //ns7538 446.0 //CENTER X
double gs_WINDOW_CENTER_Y_S2 = 338.0; //ns7538 297.0 //CENTER Y
double gs_CV2_IMAGE_MIN_X_2 = gs_WINDOW_CENTER_X_S2 - WINDOW_WIDTH_S2 / 2.0;
double gs_CV2_IMAGE_MIN_Y_2 = gs_WINDOW_CENTER_Y_S2 - WINDOW_HEIGHT_S2 / 2.0;
double gs_CV2_IMAGE_MAX_X_2 = gs_WINDOW_CENTER_X_S2 + WINDOW_WIDTH_S2 / 2.0;
double gs_CV2_IMAGE_MAX_Y_2 = gs_WINDOW_CENTER_Y_S2 + WINDOW_HEIGHT_S2 / 2.0;

//--FROM INI FILE--//
bool LOG_PROFILE_POINTS = false;
double CALIBRATION_GAGE_OFFSET = 0;
double CALIBRATION_LEFT_VERTICAL_OFFSET = 0.0;
double CALIBRATION_RIGHT_VERTICAL_OFFSET = 0.0;
double gaugeOffset = CALIBRATION_GAGE_OFFSET;
int TACH_PULSE_PER_FOOT = 100.0;
double CALIBRATION_LEFT_CORRECTION_ANGLE = 50.9;
double CALIBRATION_RIGHT_CORRECTION_ANGLE = 50.4;

//--IMAGE PROCESSING STUFF--//
ProcessedPoints displayPoints1;
ProcessedPoints displayPoints2;
//DISPLAY POINTS 1 / 2 MUTEX//
std::mutex displayPoints1_mutex;
std::mutex displayPoints2_mutex;

TemplatePoints lastBestPoint1;
TemplatePoints lastBestPoint2;

ImageProcessing imageProcessor1;
ImageProcessing imageProcessor2;

ProfileLogging logRecordObj;

//-- OUTPUT GAGE QUEUE --//
std::queue<GageGeometry_Data> gageOut_buffer;
std::mutex gageOut_mutex;
std::condition_variable gageOut_condVar;


//CONSTRUCTORS
GageSystem::GageSystem()
{

}

GageSystem::GageSystem(bool fullConstructor)
{
	//LOAD THE CONFIGURATION FILE
	loadGageIniFile();

	//INITIALIZE IMAGE PROCESSING VARIABLES
	imageProcessor1 = ImageProcessing(gs_CV2_IMAGE_MAX_X_1, gs_CV2_IMAGE_MAX_Y_1, gs_CV2_IMAGE_MIN_X_1, gs_CV2_IMAGE_MIN_Y_1, gs_CV2_IMAGE_MAX_X_2, gs_CV2_IMAGE_MAX_Y_2, gs_CV2_IMAGE_MIN_X_2, gs_CV2_IMAGE_MIN_Y_2);
	imageProcessor2 = ImageProcessing(gs_CV2_IMAGE_MAX_X_1, gs_CV2_IMAGE_MAX_Y_1, gs_CV2_IMAGE_MIN_X_1, gs_CV2_IMAGE_MIN_Y_1, gs_CV2_IMAGE_MAX_X_2, gs_CV2_IMAGE_MAX_Y_2, gs_CV2_IMAGE_MIN_X_2, gs_CV2_IMAGE_MIN_Y_2);

	imageProcessor1.setLaserCorrectionAngles(CALIBRATION_LEFT_CORRECTION_ANGLE, CALIBRATION_RIGHT_CORRECTION_ANGLE);
	imageProcessor2.setLaserCorrectionAngles(CALIBRATION_LEFT_CORRECTION_ANGLE, CALIBRATION_RIGHT_CORRECTION_ANGLE);


	//INITIALIZE SOME DISPLAY VARIABLES
	std::unique_lock<std::mutex> l1(displayPoints1_mutex);
	std::unique_lock<std::mutex> l2(displayPoints2_mutex);
	displayPoints1.fullGageMissedReadingCnt = 0;
	displayPoints1.fullGageReadingCnt = 0;
	displayPoints1.halfGageMissedReadingCnt = 0;
	displayPoints1.halfGageReadingCnt = 0;
	displayPoints2.fullGageMissedReadingCnt = 0;
	displayPoints2.fullGageReadingCnt = 0;
	displayPoints2.halfGageMissedReadingCnt = 0;
	displayPoints2.halfGageReadingCnt = 0;

	l1.unlock();
	l2.unlock();

	//SETUP SIMLUATION & LOGGING OBJECT
	if (LOG_PROFILE_POINTS)
	{
		//GET CURRENT TIME
		std::chrono::time_point<std::chrono::system_clock> timePoint = std::chrono::system_clock::now();
		time_t now = time(0);
		struct tm * timeinfo;
		timeinfo = localtime(&now);
		char dt[80];// = ctime(&now);
		strftime(dt, 80, "%G%m%d%H%M%S", timeinfo);
		CString cstringDt1 = LOG_DIRECTORY + CString(dt) + "_1.csv";
		CString cstringDt2 = LOG_DIRECTORY + CString(dt) + "_2.csv";
		logRecordObj = ProfileLogging(false, true, cstringDt1, cstringDt2);
	}
	//SETUP THE SCANNERS
	globalScanners = Scanners();
	setupScanners(&globalScanners);	
}

GageSystem::~GageSystem()
{

}

//LOAD INI CONFIGURATION FILE
void loadGageIniFile()
{
	std::ifstream configFile;
	configFile = std::ifstream(GageIniConfigFile);
	bool isOpen = configFile.is_open();
	std::string aLine;
	//std::getline(configFile, aLine);
	while (!std::getline(configFile, aLine).eof())
	{
		std::string logFileString = "LOG_PROFILE";
		std::string gageOffset = "GAGE_OFFSET";
		std::string leftVerticalOffset = "LEFT_VERTICAL_OFFSET";
		std::string rightVerticalOffset = "RIGHT_VERTICAL_OFFSET";
		std::string pulsePerFoot = "TACH_PULSE_PER_FOOT";
		std::string rightCalibrationAngle = "RIGHT_CALIBRATION_ANGLE";
		std::string leftCalibrationAngle = "LEFT_CALIBRATION_ANGLE";
		std::string leftWindowCenterX = "LEFT_WINDOW_CENTER_X";
		std::string leftWindowCenterY = "LEFT_WINDOW_CENTER_Y";
		std::string rightWindowCenterX = "RIGHT_WINDOW_CENTER_X";
		std::string rightWindowCenterY = "RIGHT_WINDOW_CENTER_Y";


		int loc = aLine.find('#');
		if (loc < 0)
		{
			//CONFIRM THAT ITS NOT A COMMENT LINE
			//TOKENIZE THE THE STRING BY =
			char* aLine_1 = new char[aLine.length() + 1];
			char* toks_1;
			char* miniTok_1;
			strcpy(aLine_1, aLine.c_str());
			toks_1 = std::strtok(aLine_1, "=");
			char* setting(toks_1);
			toks_1 = std::strtok(NULL, "\n");
			char* value(toks_1);
			if (logFileString.compare(setting) == 0)
			{
				if (std::string(value).compare("TRUE") == 0)
				{
					LOG_PROFILE_POINTS = TRUE;
				}
				else
				{
					LOG_PROFILE_POINTS = FALSE;
				}
			}
			if (gageOffset.compare(setting) == 0)
			{
				CALIBRATION_GAGE_OFFSET = std::stod(value);
				gaugeOffset = std::stod(value);
			}
			if (leftVerticalOffset.compare(setting) == 0)
			{
				CALIBRATION_LEFT_VERTICAL_OFFSET = std::stod(value);
			}
			if (rightVerticalOffset.compare(setting) == 0)
			{
				CALIBRATION_RIGHT_VERTICAL_OFFSET = std::stod(value);
			}
			if (pulsePerFoot.compare(setting) == 0)
			{
				TACH_PULSE_PER_FOOT = std::stoi(value);
			}
			if (rightCalibrationAngle.compare(setting) == 0)
			{
				CALIBRATION_RIGHT_CORRECTION_ANGLE = std::stod(value);
			}
			if (leftCalibrationAngle.compare(setting) == 0)
			{
				CALIBRATION_LEFT_CORRECTION_ANGLE = std::stod(value);
			}
			if (leftWindowCenterX.compare(setting) == 0)
			{
				gs_WINDOW_CENTER_X_S1 = std::stod(value);
				gs_CV2_IMAGE_MIN_X_1 = gs_WINDOW_CENTER_X_S1 - WINDOW_WIDTH_S1 / 2.0;
				gs_CV2_IMAGE_MAX_X_1 = gs_WINDOW_CENTER_X_S1 + WINDOW_WIDTH_S1 / 2.0;
			}
			if (leftWindowCenterY.compare(setting) == 0)
			{
				gs_WINDOW_CENTER_Y_S1 = std::stod(value);
				gs_CV2_IMAGE_MIN_Y_1 = gs_WINDOW_CENTER_Y_S1 - WINDOW_HEIGHT_S1 / 2.0;
				gs_CV2_IMAGE_MAX_Y_1 = gs_WINDOW_CENTER_Y_S1 + WINDOW_HEIGHT_S1 / 2.0;
			}
			if (rightWindowCenterX.compare(setting) == 0)
			{
				gs_WINDOW_CENTER_X_S2 = std::stod(value);
				gs_CV2_IMAGE_MIN_X_2 = gs_WINDOW_CENTER_X_S2 - WINDOW_WIDTH_S2 / 2.0;
				gs_CV2_IMAGE_MAX_X_2 = gs_WINDOW_CENTER_X_S2 + WINDOW_WIDTH_S2 / 2.0;
			}
			if (rightWindowCenterY.compare(setting) == 0)
			{
				gs_WINDOW_CENTER_Y_S2 = std::stod(value);
				gs_CV2_IMAGE_MIN_Y_2 = gs_WINDOW_CENTER_Y_S2 - WINDOW_HEIGHT_S2 / 2.0;
				gs_CV2_IMAGE_MAX_Y_2 = gs_WINDOW_CENTER_Y_S2 + WINDOW_HEIGHT_S2 / 2.0;
			}

			//REF
			//#define WINDOW_CENTER_X_S1 491.0 // NS7538480.0 //CENTER X		--HYRAIL=175.0
			//#define WINDOW_CENTER_Y_S1 325.0 //NS7538 291.0 //CENTER Y		--HYRAIL=275.0
			//#define CV2_IMAGE_MIN_X_1 WINDOW_CENTER_X_S1-WINDOW_WIDTH_S1/2
			//#define CV2_IMAGE_MIN_Y_1 WINDOW_CENTER_Y_S1-WINDOW_HEIGHT_S1/2
			//#define CV2_IMAGE_MAX_X_1 WINDOW_CENTER_X_S1+WINDOW_WIDTH_S1/2
			//#define CV2_IMAGE_MAX_Y_1 WINDOW_CENTER_Y_S1+WINDOW_HEIGHT_S1/2
			//#define WINDOW_CENTER_X_S2 448.0 //ns7538 446.0 //CENTER X		--HYRAIL=170.0
			//#define WINDOW_CENTER_Y_S2 338.0 //ns7538 297.0 //CENTER Y		--HYRAIL=260.0
			//#define CV2_IMAGE_MIN_X_2 WINDOW_CENTER_X_S2-WINDOW_WIDTH_S2/2 
			//#define CV2_IMAGE_MIN_Y_2 WINDOW_CENTER_Y_S2-WINDOW_HEIGHT_S2/2 
			//#define CV2_IMAGE_MAX_X_2 WINDOW_CENTER_X_S2+WINDOW_WIDTH_S2/2 
			//#define CV2_IMAGE_MAX_Y_2 WINDOW_CENTER_Y_S2+WINDOW_HEIGHT_S2/2 
		}
	}
}

//RESET SCANNERS FOR OPERATION
void resetScannersForOperation(Scanners &scanners)
{

	CString command = "SetResetPictureCounter";
	int commandLength = strlen(command);
	int resp = EthernetScanner_WriteData(scanners.scanner_1, command.GetBuffer(0), commandLength);

	//RESET PICTURE COUNTER #2
	command = "SetResetPictureCounter";
	commandLength = strlen(command);
	resp = EthernetScanner_WriteData(scanners.scanner_2, command.GetBuffer(0), commandLength);
	Sleep(200);

	//RESET FIFO #1
	int resetResp1 = EthernetScanner_ResetDllFiFo(scanners.scanner_1);

	//RESET FIFO #2
	int resetResp2 = EthernetScanner_ResetDllFiFo(scanners.scanner_2);

	Sleep(500);
}

//START SCANNERS ACQUISITION
void GageSystem::startScannerAcq(Scanners &scanners)
{
	bool s1Connected, s2Connected;
	checkScannerConnections(scanners, s1Connected, s2Connected);

	//START ACQUISITION #1
	bool sens1Started = false;
	bool sens2Started = false;
	do
	{
		CString command = "SetAcquisitionStart";
		int commandLength = strlen(command);
		int resp = EthernetScanner_WriteData(scanners.scanner_1, command.GetBuffer(0), commandLength);
		if (resp > 0)
		{
			sens1Started = true;
		}
	} while (!sens1Started);

	do
	{

		//START ACQUISITION #2
		CString command = "SetAcquisitionStart";
		int commandLength = strlen(command);
		int resp = EthernetScanner_WriteData(scanners.scanner_2, command.GetBuffer(0), commandLength);
		if (resp > 0)
		{
			sens2Started = true;
		}
	} while (!sens2Started);
}

//STOP CAMERA AQ
void stopScannersAcq(Scanners &scanners)
{
	//STOP ACQUISITION #2 -- #2 IS MASTER
	CString command = "SetAcquisitionStop";
	int commandLength = strlen(command);
	int resp2 = EthernetScanner_WriteData(scanners.scanner_2, command.GetBuffer(0), commandLength);

	//STOP ACQUISITION #1 -- #1 IS SLAVE
	int resp = EthernetScanner_WriteData(scanners.scanner_1, command.GetBuffer(0), commandLength);
}

//CHECK WENGLOR SENSOR CONNECTIONS
void GageSystem::checkScannerConnections(Scanners &scanners, bool &g1Connected, bool &g2Connected)
{
	int s1Connected, s2Connected;
	EthernetScanner_GetConnectStatus(scanners.scanner_1, &s1Connected);
	EthernetScanner_GetConnectStatus(scanners.scanner_2, &s2Connected);
	if (s1Connected == 3)
	{
		g1Connected = true;
	}
	else
	{
		g1Connected = false;
	}
	if (s2Connected == 3)
	{
		g2Connected = true;
	}
	else
	{
		g2Connected = false;
	}
}

//CAPUTRING A PROFILE AND SPINNING A THREAD TO ANALYZE IT
unsigned __stdcall getProfile(void* args)
{
	//UNPACK ARGUMENTS
	AnalysisResults argRes = *(AnalysisResults*)args;
	int scannerNumber = argRes.scannerNumber;
	void* scannerObj = argRes.scannerObj;

	//VARS TO HOLD RECIEVED IMAGE DATA
	ProfilePoints rawPoints;
	rawPoints.sensorNumber = scannerNumber;

	//WAIT FOR THE SCANNER TO RECIEVE A PROFILE
	bool recData = false;
	while (!recData && gageSystemCollecting)
	{
		//WAIT UNTIL WE HAVE RECIEVED A VALID PROFILE FROM SENSOR
		int scanRet = EthernetScanner_GetXZIExtended(scannerObj, rawPoints.yVals, rawPoints.xVals, rawPoints.intensity, rawPoints.peak, 2048, &rawPoints.encoder, &rawPoints.roi, 5000, NULL, 0, &rawPoints.picCounter);
		if (scanRet != ETHERNETSCANNER_GETXZINONEWSCAN && scanRet != ETHERNETSCANNER_GETXZIINVALIDLINDATA && scanRet != ETHERNETSCANNER_READDATASMALLBUFFER)
		{
			recData = true;
		}
	}

	//SPIN THREAD TO LOG PROFILE POINTS -- IF REQUESTED
	unsigned logThreadID;
	HANDLE logThread;
	if (LOG_PROFILE_POINTS)
	{
		ProfilePoints pointsToWrite = rawPoints;
		ProfilePoints * PointsToWrite = &pointsToWrite;
		logThread = (HANDLE)_beginthreadex(NULL, 0, writeProfile, (void *)PointsToWrite, NULL, &logThreadID);
	}

	//ANLAYZE THE PROFILE
	switch (scannerNumber)
	{
	case 1:
		imageProcessor1.analyzeProfile(&rawPoints);
		argRes.analyzedPoints = rawPoints;
		*((AnalysisResults*)args) = argRes;
		break;
	case 2:
		imageProcessor2.analyzeProfile(&rawPoints);
		argRes.analyzedPoints = rawPoints;
		*((AnalysisResults*)args) = argRes;
		break;
	}

	if (LOG_PROFILE_POINTS)
	{
		//WAIT TO JOIN LOGGING THREAD AND CLOSE OUT HANDLE
		WaitForSingleObject(logThread, INFINITE);
		CloseHandle(logThread);
	}
	return 1;
}

//PROFILE WRITING/LOGGING THREAD
unsigned __stdcall writeProfile(void* args)
{
	ProfilePoints points = *(ProfilePoints*)args;
	logRecordObj.writeProfilePoints(points);
	return 1;
}

//INITIAL CONNECTION AND CONFIG OF THE SCANNERS
unsigned __stdcall setupScanners(void* args)
{
	Scanners scanners = *(Scanners*)args;
	int status1 = 0;
	int status2 = 0;

	//####----SETUP EACH SCANNER----####

	//CONNECT TO SCANNER 1
	CString ipString1 = "10.255.255.3"; //LEFT
	CString portString1 = "32001";

	scanners.scanner_1 = EthernetScanner_Connect(ipString1.GetBuffer(0), portString1.GetBuffer(0), 0);

	//WAIT FOR GOOD CONNECTION	
	DWORD dwTimeout1 = GetTickCount();
	while (status1 != ETHERNETSCANNER_TCPSCANNERCONNECTED && !SIMULATION_MODE)
	{
		//current state of the connection
		EthernetScanner_GetConnectStatus(scanners.scanner_1, &status1);

		//Detect the timeout
		if (GetTickCount() - dwTimeout1 > 3000)
		{
			scanners.scanner_1 = EthernetScanner_Disconnect(scanners.scanner_1);
			scanners.scanner_1 = EthernetScanner_Connect(ipString1.GetBuffer(0), portString1.GetBuffer(0), 0);
			dwTimeout1 = GetTickCount();
		}
	}

	//CONNECT TO SENSOR 2
	CString ipString2 = "10.255.255.4"; //RIGHT
	CString portString2 = "32001";

	scanners.scanner_2 = EthernetScanner_Connect(ipString2.GetBuffer(0), portString2.GetBuffer(0), 0);

	//WAIT FOR GOOD CONNECTION
	DWORD dwTimeout2 = GetTickCount();
	while (status2 != ETHERNETSCANNER_TCPSCANNERCONNECTED && !SIMULATION_MODE)
	{
		//current state of the connection
		EthernetScanner_GetConnectStatus(scanners.scanner_2, &status2);
		//Detect the timeout
		if (GetTickCount() - dwTimeout2 > 3000)
		{
			scanners.scanner_2 = EthernetScanner_Disconnect(scanners.scanner_2);
			scanners.scanner_2 = EthernetScanner_Connect(ipString2.GetBuffer(0), portString2.GetBuffer(0), 0);
			dwTimeout2 = GetTickCount();
		}
	}

	//STOP ACQUISITION #1
	CString command = "SetAcquisitionStop";
	int commandLength = strlen(command);
	int resp = EthernetScanner_WriteData(scanners.scanner_1, command.GetBuffer(0), commandLength);

	Sleep(1000);
	//STOP ACQUISITION #2
	command = "SetAcquisitionStop";
	commandLength = strlen(command);
	resp = EthernetScanner_WriteData(scanners.scanner_2, command.GetBuffer(0), commandLength);

	//SET CONFIGURATION SET
	command = "SetSettingsLoad=0";
	commandLength = strlen(command);
	resp = EthernetScanner_WriteData(scanners.scanner_1, command.GetBuffer(0), commandLength);
	resp = EthernetScanner_WriteData(scanners.scanner_2, command.GetBuffer(0), commandLength);
	Sleep(3000);

	//NOW DISCONNECT FROM SENSORS
	EthernetScanner_Disconnect(scanners.scanner_1);
	EthernetScanner_Disconnect(scanners.scanner_2);
	Sleep(1000);

	//RECONNECT TO THE SCANNERS
	scanners.scanner_1 = EthernetScanner_Connect(ipString1.GetBuffer(0), portString1.GetBuffer(0), 0);

	//WAIT FOR GOOD CONNECTION	
	dwTimeout1 = GetTickCount();
	while (status1 != ETHERNETSCANNER_TCPSCANNERCONNECTED && !SIMULATION_MODE)
	{
		//current state of the connection
		EthernetScanner_GetConnectStatus(scanners.scanner_1, &status1);

		//Detect the timeout
		if (GetTickCount() - dwTimeout1 > 3000)
		{
			scanners.scanner_1 = EthernetScanner_Disconnect(scanners.scanner_1);
			scanners.scanner_1 = EthernetScanner_Connect(ipString1.GetBuffer(0), portString1.GetBuffer(0), 0);
			dwTimeout1 = GetTickCount();
		}
	}

	//CONNECT TO SENSOR 2
	scanners.scanner_2 = EthernetScanner_Connect(ipString2.GetBuffer(0), portString2.GetBuffer(0), 0);

	//WAIT FOR GOOD CONNECTION
	dwTimeout2 = GetTickCount();
	while (status2 != ETHERNETSCANNER_TCPSCANNERCONNECTED && !SIMULATION_MODE)
	{
		//current state of the connection
		EthernetScanner_GetConnectStatus(scanners.scanner_2, &status2);
		//Detect the timeout
		if (GetTickCount() - dwTimeout2 > 3000)
		{
			scanners.scanner_2 = EthernetScanner_Disconnect(scanners.scanner_2);
			scanners.scanner_2 = EthernetScanner_Connect(ipString2.GetBuffer(0), portString2.GetBuffer(0), 0);
			dwTimeout2 = GetTickCount();
		}
	}


	//RESET PICTURE COUNTER #1
	command = "SetResetPictureCounter";
	commandLength = strlen(command);
	resp = EthernetScanner_WriteData(scanners.scanner_1, command.GetBuffer(0), commandLength);

	//RESET PICTURE COUNTER #2
	command = "SetResetPictureCounter";
	commandLength = strlen(command);
	resp = EthernetScanner_WriteData(scanners.scanner_2, command.GetBuffer(0), commandLength);

	Sleep(1000);

	//RESET FIFO #1
	int resetResp1 = EthernetScanner_ResetDllFiFo(scanners.scanner_1);

	//RESET FIFO #2
	int resetResp2 = EthernetScanner_ResetDllFiFo(scanners.scanner_2);

	*((Scanners*)args) = scanners;

	Sleep(1000);

	return 1;
}

//PREPARE THE GAGE SENSORS FOR CAPTURE
bool GageSystem::prepareForCapture(Scanners &scanners)
{
	//CLEAR QUEUE
	int queSize = gageOut_buffer.size();
	for (int i = 0; i < queSize; i++)
	{
		gageOut_buffer.pop();
	}
	//Disconnect Scanners
	EthernetScanner_Disconnect(scanners.scanner_1);
	EthernetScanner_Disconnect(scanners.scanner_2);
	//Reconnect to Scanners
	setupScanners(&scanners);
	//RESET THE SCANNERS
	resetScannersForOperation(scanners);
	//START THE AQUIRE
	startScannerAcq(scanners);
	return true;
}

//START THE GAGE CAPTURE
bool GageSystem::startGageCapture(Scanners &scanners)
{
	gageSystemCollecting = true;
	Sleep(3000);
	gageCaptureHandle = (HANDLE)_beginthreadex(NULL, 0, &gageCollectingThread, &scanners, NULL, &gageCaptureID);
	//MAY WANT TO REMOVE THIS AND REDO IT
	WaitForSingleObject(gageCaptureHandle, INFINITE);
	return true;
}

//STOP THE GAGE CAPTURE
bool GageSystem::stopGageCapture(Scanners &scanners)
{
	gageSystemCollecting = false;
	WaitForSingleObject(gageCaptureHandle, INFINITE);
	CloseHandle(gageCaptureHandle);
	if (!SIMULATION_MODE)
	{
		stopScannersAcq(scanners);
	}
	return true;
}

//LOOP FOR COLLECTING / PROCESSING / REPORTING GAGE DATA
unsigned __stdcall gageCollectingThread(void* args)
{
	Scanners scanners = *(Scanners*)args;
	void *scanner1 = scanners.scanner_1;
	void *scanner2 = scanners.scanner_2;

	int status1 = 0;
	int status2 = 0;

	int oldPicCounter1 = 0;
	int oldPicCounter2 = 0;
	long packetCount = 0;

	displayPoints1.halfGageReadingCnt = 0;
	displayPoints2.halfGageReadingCnt = 0;
	displayPoints1.halfGageMissedReadingCnt = 0;
	displayPoints2.halfGageMissedReadingCnt = 0;
	displayPoints1.fullGageReadingCnt = 0;
	displayPoints2.fullGageReadingCnt = 0;
	displayPoints1.fullGageMissedReadingCnt = 0;
	displayPoints2.fullGageMissedReadingCnt = 0;

	GageGeometry_Data prevPacket;
	bool firstPacket = true;
	bool onStraightL = false;
	bool onStraightR = false;
	bool onStraightF = false;

	while (gageSystemCollecting)
	{
		HANDLE logThreads[2];
		unsigned logThread1ID;
		unsigned logThread2ID;
		unsigned analyzeThread1ID;
		unsigned analyzeThread2ID;
		HANDLE analyzeThreads[2];

		AnalysisResults scanner1Results;
		scanner1Results.scannerObj = scanner1;
		scanner1Results.scannerNumber = 1;
		//scanner1Results.CV2_IMAGE_MAX_X_1 = gs_CV2_IMAGE_MAX_X_1;
		//scanner1Results.CV2_IMAGE_MAX_X_2 = gs_CV2_IMAGE_MAX_X_2;
		//scanner1Results.CV2_IMAGE_MAX_Y_1 = gs_CV2_IMAGE_MAX_Y_1;
		//scanner1Results.CV2_IMAGE_MAX_Y_2 = gs_CV2_IMAGE_MAX_Y_2;
		//scanner1Results.CV2_IMAGE_MIN_X_1 = gs_CV2_IMAGE_MIN_X_1;
		//scanner1Results.CV2_IMAGE_MIN_X_2 = gs_CV2_IMAGE_MIN_X_2;
		//scanner1Results.CV2_IMAGE_MIN_Y_1 = gs_CV2_IMAGE_MIN_Y_1;
		//scanner1Results.CV2_IMAGE_MIN_Y_2 = gs_CV2_IMAGE_MIN_Y_2;

		AnalysisResults scanner2Results;
		scanner2Results.scannerObj = scanner2;
		scanner2Results.scannerNumber = 2;
		//scanner2Results.CV2_IMAGE_MAX_X_1 = gs_CV2_IMAGE_MAX_X_1;
		//scanner2Results.CV2_IMAGE_MAX_X_2 = gs_CV2_IMAGE_MAX_X_2;
		//scanner2Results.CV2_IMAGE_MAX_Y_1 = gs_CV2_IMAGE_MAX_Y_1;
		//scanner2Results.CV2_IMAGE_MAX_Y_2 = gs_CV2_IMAGE_MAX_Y_2;
		//scanner2Results.CV2_IMAGE_MIN_X_1 = gs_CV2_IMAGE_MIN_X_1;
		//scanner2Results.CV2_IMAGE_MIN_X_2 = gs_CV2_IMAGE_MIN_X_2;
		//scanner2Results.CV2_IMAGE_MIN_Y_1 = gs_CV2_IMAGE_MIN_Y_1;
		//scanner2Results.CV2_IMAGE_MIN_Y_2 = gs_CV2_IMAGE_MIN_Y_2;


		//SPIN THE THREAD TO GET SENSOR 
		analyzeThreads[0] = (HANDLE)_beginthreadex(NULL, 0, getProfile, &scanner1Results, NULL, &analyzeThread1ID);

		//SPIN THE THREAD TO GET SENSOR 
		analyzeThreads[1] = (HANDLE)_beginthreadex(NULL, 0, getProfile, &scanner2Results, NULL, &analyzeThread2ID);

		//WAIT FOR PROFILES TO BE GATHERED & PROCESSED -- THEN JOIN THREAD
		WaitForMultipleObjects(2, analyzeThreads, true, INFINITE);
		CloseHandle(analyzeThreads[0]);
		CloseHandle(analyzeThreads[1]);

		if (LOG_PROFILE_POINTS)
		{
			//CHECK TO SEE IF LOG FILE NEEDS TO BE ROLLED OVER - DUE TO SIZE
			logRecordObj.checkRollFile();
		}

		if (gageSystemCollecting)
		{
			std::unique_lock<std::mutex> l1(displayPoints1_mutex);
			std::unique_lock<std::mutex> l2(displayPoints2_mutex);
			//CALCULATE FULL GAGE READING
			if (scanner1Results.analyzedPoints.foundHalfGage && scanner2Results.analyzedPoints.foundHalfGage)
			{
				double fg = scanner1Results.analyzedPoints.halfGage + scanner2Results.analyzedPoints.halfGage + gaugeOffset;
				displayPoints1.fullGageReading = fg;
				displayPoints1.foundHalfGage = true;
				displayPoints1.fullGageReadingCnt++;
				displayPoints1.halfGageReadingCnt++;
				displayPoints2.halfGageReadingCnt++;
			}
			else
			{
				displayPoints1.fullGageReading = nan("n");
				displayPoints1.fullGageMissedReadingCnt++;
				if (!scanner1Results.analyzedPoints.foundHalfGage)
				{
					displayPoints1.halfGageMissedReadingCnt++;
				}
				else
				{
					displayPoints1.halfGageReadingCnt++;
				}
				if (!scanner2Results.analyzedPoints.foundHalfGage)
				{
					displayPoints2.halfGageMissedReadingCnt++;
				}
				else
				{
					displayPoints2.halfGageReadingCnt++;
				}
			}


			//COPY VALUES OVER TO DISPLAYPOINT GLOBALS
			memcpy(displayPoints1.xCoords, scanner1Results.analyzedPoints.xVals, sizeof(scanner1Results.analyzedPoints.xVals));
			memcpy(displayPoints1.yCoords, scanner1Results.analyzedPoints.yVals, sizeof(scanner1Results.analyzedPoints.yVals));
			displayPoints1.picCounter = scanner1Results.analyzedPoints.picCounter;
			displayPoints1.tor = scanner1Results.analyzedPoints.tor;
			memcpy(displayPoints1.templatePoint, scanner1Results.analyzedPoints.templatePoint, sizeof(scanner1Results.analyzedPoints.templatePoint));
			displayPoints1.torAvgY = scanner1Results.analyzedPoints.torAvgY;
			displayPoints1.torAvgY_Xval = scanner1Results.analyzedPoints.torAvgY_Xval;
			displayPoints1.x5_8 = scanner1Results.analyzedPoints.x5_8;
			displayPoints1.y5_8 = scanner1Results.analyzedPoints.y5_8;
			displayPoints1.halfGageReading = scanner1Results.analyzedPoints.halfGage;
			displayPoints1.webXVal = scanner1Results.analyzedPoints.webXVal;
			displayPoints1.webYVal = scanner1Results.analyzedPoints.webYVal;

			memcpy(displayPoints2.xCoords, scanner2Results.analyzedPoints.xVals, sizeof(scanner2Results.analyzedPoints.xVals));
			memcpy(displayPoints2.yCoords, scanner2Results.analyzedPoints.yVals, sizeof(scanner2Results.analyzedPoints.yVals));
			displayPoints2.picCounter = scanner2Results.analyzedPoints.picCounter;
			displayPoints2.tor = scanner2Results.analyzedPoints.tor;
			memcpy(displayPoints2.templatePoint, scanner2Results.analyzedPoints.templatePoint, sizeof(scanner2Results.analyzedPoints.templatePoint));
			displayPoints2.torAvgY = scanner2Results.analyzedPoints.torAvgY;
			displayPoints2.torAvgY_Xval = scanner2Results.analyzedPoints.torAvgY_Xval;
			displayPoints2.x5_8 = scanner2Results.analyzedPoints.x5_8;
			displayPoints2.y5_8 = scanner2Results.analyzedPoints.y5_8;
			displayPoints2.halfGageReading = scanner2Results.analyzedPoints.halfGage;
			displayPoints2.webXVal = scanner2Results.analyzedPoints.webXVal;
			displayPoints2.webYVal = scanner2Results.analyzedPoints.webYVal;


			//PACKAGE THE GAGE DATA AND INSERT INTO GLOBAL QUEUE
			GageGeometry_Data packet;
		
			packet.hasData = true;
			packet.hasAllGoodData = true;
			if (_isnan(scanner2Results.analyzedPoints.halfGage))
			{
				packet.right_width = 0.0;
				packet.right_height = 0.0;
				packet.hasAllGoodData = true;
				onStraightR = true;
			}
			else if (!firstPacket && ((abs(prevPacket.right_width - scanner2Results.analyzedPoints.halfGage) >= 0.5) || (abs(prevPacket.right_height - (scanner2Results.analyzedPoints.torAvgY * (-MM2INCH) - CALIBRATION_RIGHT_VERTICAL_OFFSET)) >= 0.5)) && (abs(prevPacket.packetNumber - packetCount) <= 10))
			{
				packet.right_height = 0;
				packet.right_width = 0;
				packet.hasAllGoodData = true;
				onStraightR = true;
			}
			else
			{
				packet.right_width = scanner2Results.analyzedPoints.halfGage;
				//packet.right_height = /*scanner2Results.analyzedPoints.torAvgY*/-scanner2Results.analyzedPoints.templatePoint[0].topY * (-MM2INCH) - CALIBRATION_RIGHT_VERTICAL_OFFSET;
				packet.right_height = scanner2Results.analyzedPoints.torAvgY * (-MM2INCH) - CALIBRATION_RIGHT_VERTICAL_OFFSET;

				packet.hasAllGoodData = true;
				onStraightR = false;
			}


			if (_isnan(scanner1Results.analyzedPoints.halfGage))
			{
				packet.left_width = 0.0;
				packet.left_height = 0.0;
				packet.hasAllGoodData = true;
				onStraightL = true;
			}
			else if (!firstPacket && ((abs(prevPacket.left_width - scanner1Results.analyzedPoints.halfGage) >= 0.5) || (abs(prevPacket.left_height - (scanner1Results.analyzedPoints.torAvgY * (-MM2INCH) - CALIBRATION_LEFT_VERTICAL_OFFSET)) >= 0.5)) && (abs(prevPacket.packetNumber - packetCount) <= 10))
			{
				packet.left_width = 0;
				packet.left_height = 0;
				packet.hasAllGoodData = true;
				onStraightL = true;
			}
			else
			{
				packet.left_width = scanner1Results.analyzedPoints.halfGage;
				//packet.left_height = /*scanner1Results.analyzedPoints.torAvgY*/-scanner2Results.analyzedPoints.templatePoint[0].topY * (-MM2INCH) - CALIBRATION_LEFT_VERTICAL_OFFSET;
				packet.left_height = scanner1Results.analyzedPoints.torAvgY * (-MM2INCH) - CALIBRATION_LEFT_VERTICAL_OFFSET;
				packet.hasAllGoodData = true;
				onStraightL = false;

			}


			if (_isnan(displayPoints1.fullGageReading))
			{
				packet.full_gage = 0.0;
				packet.hasAllGoodData = true;
				onStraightF = true;
			}
			else if (!firstPacket && (abs(prevPacket.full_gage - displayPoints1.fullGageReading) >= 0.75) && (abs(prevPacket.packetNumber - packetCount) <= 3))
			{
				packet.full_gage = 0;
				onStraightF = true;
			}
			else
			{
				packet.full_gage = displayPoints1.fullGageReading;
				onStraightF = false;
			}

			l1.unlock();
			l2.unlock();

			packetCount++;
			globalPacketCount = packetCount;
			packet.packetNumber = packetCount;
			if (!onStraightL && !onStraightR && !onStraightF)
			{
				prevPacket = packet;
				firstPacket = false;
			}

			//PLACE THE GAGE DATA PACKET ONTO THE OUTPUT QUEUE
			putGageOutput(packet);

			//REDRAW THE WINDOW AND CHECK FIFO VALUES
			l1.lock();
			l2.lock();
			if (displayPoints1.picCounter % SCREEN_UPDATE_MOD == 0)
			{
				if (!SIMULATION_MODE)
				{
					int fifoPer1 = EthernetScanner_GetDllFiFoState(scanner1);
					displayPoints1.fifoPercentage = fifoPer1;
					

					int fifoPer2 = EthernetScanner_GetDllFiFoState(scanner2);
					displayPoints2.fifoPercentage = fifoPer2;
				}
			}
			l1.unlock();
			l2.unlock();
		}
	}
	scanners.scanner_1 = scanner1;
	scanners.scanner_2 = scanner2;
	*(Scanners*)args = scanners;
	return 1;
}

//PUT PACKET ON GAGE OUTPUT QUEUE
bool putGageOutput(GageGeometry_Data packet)
{
	std::unique_lock<std::mutex> l(gageOut_mutex);
	gageOut_buffer.push(packet);
	l.unlock();
	gageOut_condVar.notify_one();
	return true;
}

//GET PACKET FROM GAGE OUTPUT BUFFER
bool GageSystem::getGageOutput(GageGeometry_Data &packet)
{
	std::unique_lock<std::mutex> l(gageOut_mutex);
	gageOut_condVar.wait_for(l, std::chrono::milliseconds(100), [] {return gageOut_buffer.size() > 0; });

	if (gageOut_buffer.size() > 0)
	{
		GageGeometry_Data tmp = gageOut_buffer.front();
		gageOut_buffer.pop();
		l.unlock();
		packet = tmp;
		return true;
	}
	l.unlock();
	return false;
}

//UPDATE GAGE OFFSET
double GageSystem::updateGageOffset(double gageReading)
{
	std::unique_lock<std::mutex> l1(displayPoints1_mutex);
	std::unique_lock<std::mutex> l2(displayPoints2_mutex);
	gaugeOffset = gageReading - displayPoints1.halfGageReading - displayPoints2.halfGageReading;
	l1.unlock();
	l2.unlock();
	return gaugeOffset;
}

//DRAWING THE PROFILES AND DISPLAYING INFO -- ONLY GAGE PROFILES
void GageSystem::displayProfilePoints(HDC &hdc, int sensor, bool &screenPaintOnce)
{
	ProcessedPoints displayPoints;
	if (sensor == 1)
	{
		std::unique_lock<std::mutex> l1(displayPoints1_mutex);
		displayPoints = displayPoints1;
		l1.unlock();
	}
	else
	{
		std::unique_lock<std::mutex> l2(displayPoints2_mutex);
		displayPoints = displayPoints2;
		l2.unlock();
	}


	//PENS
	HPEN purplePen = CreatePen(PS_SOLID, 3, RGB(255, 0, 255));
	HPEN redPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
	HPEN thickRedPen = CreatePen(PS_SOLID, 4, RGB(255, 0, 0));
	HPEN bluePen = CreatePen(PS_SOLID, 3, RGB(0, 0, 255));
	HPEN greenPen = CreatePen(PS_SOLID, 2, RGB(0, 255, 0));
	HPEN blackPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
	HPEN fineGreenPen = CreatePen(PS_SOLID, 1, RGB(0, 255, 0));
	HPEN fineAuqaPen = CreatePen(PS_SOLID, 2, RGB(0, 255, 255));

	//BRUSHES
	HBRUSH redBrush = CreateSolidBrush(RGB(0, 255, 0));
	HBRUSH blueBrush = CreateSolidBrush(RGB(0, 0, 255));
	HBRUSH blackBrush = CreateSolidBrush(RGB(255, 255, 255));

	//FONT
	HFONT smallFont = CreateFont(10, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, "SYSTEM_FIXED_FONT");
	HFONT bigFont = CreateFont(20, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, "SYSTEM_FIXED_FONT");


	SelectObject(hdc, bigFont);

	int imageMinX = 0;
	int imageMinY = 0;
	int imageMaxX = 500;
	int imageMaxY = 500;
	int xOffset = 0;
	int yOffset = 0;
	switch (sensor)
	{
	case 1:
		imageMinX = gs_CV2_IMAGE_MIN_X_1;
		imageMaxX = gs_CV2_IMAGE_MAX_X_1;
		imageMinY = gs_CV2_IMAGE_MIN_Y_1;
		imageMaxY = gs_CV2_IMAGE_MAX_Y_1;
		xOffset = 0;
		yOffset = 0;
		break;

	case 2:
		imageMinX = gs_CV2_IMAGE_MIN_X_2;
		imageMaxX = gs_CV2_IMAGE_MAX_X_2;
		imageMinY = gs_CV2_IMAGE_MIN_Y_2;
		imageMaxY = gs_CV2_IMAGE_MAX_Y_2;
		xOffset = 750;
		yOffset = 120;
		break;
	}

	if (screenPaintOnce || true)
	{
		//DRAW GRIDLINES IF REQUESTED
		if (DRAW_GRID_LINES)
		{
			SelectObject(hdc, fineGreenPen);
			//FOR Y AXIS
			for (int y = 0; y < 500; y = y + 50)
			{
				MoveToEx(hdc, 0 + xOffset, y, NULL);
				LineTo(hdc, 500 + xOffset, y);
				std::string val = std::to_string(y);
				TextOut(hdc, 0 + xOffset, y, val.c_str(), strlen(val.c_str()));
			}
			//FOR X AXIS
			for (int x = 0; x < 500; x = x + 50)
			{
				MoveToEx(hdc, x + xOffset, 0, NULL);
				LineTo(hdc, x + xOffset, 500);
				std::string val = std::to_string(x);
				TextOut(hdc, x + xOffset, 500, val.c_str(), strlen(val.c_str()));
			}
		}

		//OUTPUT PLACEHOLDER LABELS
		if (sensor == 1)
		{
			TextOut(hdc, 600, 5 + yOffset, "LEFT SENSOR", strlen("LEFT SENSOR"));
		}
		else
		{
			TextOut(hdc, 600, 5 + yOffset, "RIGHT SENSOR", strlen("RIGHT SENSOR"));
		}

		TextOut(hdc, 550, 25 + yOffset, "PIC CNT: ", strlen("PIC CNT: "));
		TextOut(hdc, 550, 45 + yOffset, "FIFO: ", strlen("FIFO: "));
		TextOut(hdc, 550, 65 + yOffset, "HG CNT: ", strlen("HG CNT: "));
		TextOut(hdc, 550, 85 + yOffset, "MISS HG: ", strlen("MISS HG: "));
		TextOut(hdc, 550, 105 + yOffset, "HG: ", strlen("HG: "));
		TextOut(hdc, 600, 250, "SUMMARY", strlen("SUMMARY"));
		TextOut(hdc, 550, 270, "G: ", strlen("G: "));
		TextOut(hdc, 550, 290, "G (+/-): ", strlen("G (+/-): "));
		TextOut(hdc, 550, 310, "MISS: ", strlen("MISS: "));
		TextOut(hdc, 550, 330, "READ: ", strlen("READ: "));

		screenPaintOnce = false;
	}


	if (displayPoints.picCounter > 0)//IF THERE IS A NEW PROFILE -- AND IT'S A VALID PROFILE
	{

		//PLOT THE PROFILE POINTS
		SelectObject(hdc, blackBrush);
		SelectObject(hdc, GetStockObject(BLACK_PEN));
		for (int i = 0; i < 2048; i++)
		{
			Ellipse(hdc, xOffset + displayPoints.xCoords[i] - 2, -1.0 * displayPoints.yCoords[i] - 2, xOffset + displayPoints.xCoords[i] + 2, -1.0 * displayPoints.yCoords[i] + 2);
		}

		//PLOT MY DETECTED TOR POINT
		SelectObject(hdc, blueBrush);
		SelectObject(hdc, bluePen);

		if (displayPoints.tor.xValue > 0)
			Ellipse(hdc, displayPoints.tor.xValue + xOffset - 5, -1.0 * displayPoints.tor.yValue - 5, displayPoints.tor.xValue + xOffset + 5, -1.0*displayPoints.tor.yValue + 5);

		//DRAW TOR HORIZONTAL LINE
		if (displayPoints.torAvgY > -1000)
		{
			MoveToEx(hdc, 0 + xOffset, displayPoints.torAvgY * -1.0, NULL);
			//LineTo(hdc, displayPoints.tor.xValue + xOffset, displayPoints.torAvgY * -1.0);
			Ellipse(hdc, displayPoints.torAvgY_Xval - 5, displayPoints.torAvgY*-1.0 - 5, displayPoints.torAvgY_Xval + 5, displayPoints.torAvgY * -1.0 + 5);
		}

		SelectObject(hdc, greenPen);
		if (displayPoints.y5_8 > -1000)
		{
			MoveToEx(hdc, 0 + xOffset, displayPoints.y5_8 * -1.0, NULL);
			LineTo(hdc, displayPoints.tor.xValue + xOffset, displayPoints.y5_8 * -1.0);
			Ellipse(hdc, displayPoints.x5_8 + xOffset - 6, displayPoints.y5_8 * -1.0 - 6, displayPoints.x5_8 + xOffset + 6, displayPoints.y5_8 * -1.0 + 6);
		}

		//PLOT TEMPLATE MATCHES
		SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
		SelectObject(hdc, fineAuqaPen);

		for (int j = 0; j < RAIL_TEMPLATE_COUNT; j++)
		{
			Rectangle(hdc, displayPoints.templatePoint[j].leftX + xOffset, displayPoints.templatePoint[j].topY, displayPoints.templatePoint[j].rightX + xOffset, displayPoints.templatePoint[j].bottomY);
		}
	}


	std::string picCounterString = std::to_string(displayPoints.picCounter);
	TextOut(hdc, 650, 25 + yOffset, picCounterString.c_str(), strlen(picCounterString.c_str()));
	char fifoChar[40];
	sprintf(fifoChar, "%3.0f%%", displayPoints.fifoPercentage);
	TextOut(hdc, 650, 45 + yOffset, fifoChar, strlen(fifoChar));
	std::string gageReadingCnt = std::to_string(displayPoints.halfGageReadingCnt);
	std::string missedGageReadingCnt = std::to_string(displayPoints.halfGageMissedReadingCnt);
	std::string gageReading = std::to_string(displayPoints.halfGageReading);
	TextOut(hdc, 650, 65 + yOffset, gageReadingCnt.c_str(), strlen(gageReadingCnt.c_str()));
	TextOut(hdc, 650, 85 + yOffset, missedGageReadingCnt.c_str(), strlen(missedGageReadingCnt.c_str()));
	TextOut(hdc, 650, 105 + yOffset, gageReading.c_str(), strlen(gageReading.c_str()));

	//DISPLAY GAUGE IF INFO IS FROM SENSOR #1
	if (sensor == 1)
	{
		double fg = displayPoints.fullGageReading;
		char fgChar[20];
		sprintf(fgChar, "%3.4f", fg);
		TextOut(hdc, 650, 270, fgChar, strlen(fgChar));

		double fgZ = displayPoints.fullGageReading - 56.5;
		char fgZChar[40];
		sprintf(fgZChar, "%3.4f", fgZ);
		TextOut(hdc, 650, 290, fgZChar, strlen(fgZChar));

		char missedChar[20];
		sprintf(missedChar, "%d", displayPoints.fullGageMissedReadingCnt);
		TextOut(hdc, 650, 310, missedChar, strlen(missedChar));

		char readChar[20];
		sprintf(readChar, "%d", displayPoints.fullGageReadingCnt);
		TextOut(hdc, 650, 330, readChar, strlen(readChar));
	}

	//DRAW BOX OF INTEREST
	SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
	SelectObject(hdc, redPen);
	Rectangle(hdc, imageMinX + xOffset, imageMinY, imageMaxX + xOffset, imageMaxY);


	//CLEANUP AND DELETE OBJECTS
	//PENS
	DeleteObject(purplePen);
	DeleteObject(redPen);
	DeleteObject(thickRedPen);
	DeleteObject(bluePen);
	DeleteObject(greenPen);
	DeleteObject(blackPen);
	DeleteObject(fineGreenPen);
	DeleteObject(fineAuqaPen);

	//BRUSHES
	DeleteObject(redBrush);
	DeleteObject(blueBrush);
	DeleteObject(blackBrush);

	//FONT
	DeleteObject(smallFont);
	DeleteObject(bigFont);

}

//DRAWING THE PROFILES AND DISPLAYING INFO -- COMBINED GAGE & INERTIAL VERSION
void GageSystem::displayProfilePointsCombined(HDC& hdc, int sensor, bool& screenPaintOnce, long& pictureCnt)
{
	ProcessedPoints displayPoints;
	if (sensor == 1)
	{
		std::unique_lock<std::mutex> l1(displayPoints1_mutex);
		displayPoints = displayPoints1;
		l1.unlock();
	}
	else
	{
		std::unique_lock<std::mutex> l2(displayPoints2_mutex);
		displayPoints = displayPoints2;
		l2.unlock();
	}

	pictureCnt = displayPoints.halfGageReadingCnt + displayPoints.halfGageMissedReadingCnt;

	//PENS
	HPEN purplePen = CreatePen(PS_SOLID, 3, RGB(255, 0, 255));
	HPEN redPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
	HPEN thickRedPen = CreatePen(PS_SOLID, 4, RGB(255, 0, 0));
	HPEN bluePen = CreatePen(PS_SOLID, 3, RGB(0, 0, 255));
	HPEN greenPen = CreatePen(PS_SOLID, 2, RGB(0, 255, 0));
	HPEN blackPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
	HPEN fineGreenPen = CreatePen(PS_SOLID, 1, RGB(0, 255, 0));
	HPEN fineAuqaPen = CreatePen(PS_SOLID, 2, RGB(0, 255, 255));
	HPEN magentaPen = CreatePen(PS_SOLID, 2, RGB(255, 0, 255));

	//BRUSHES
	HBRUSH redBrush = CreateSolidBrush(RGB(0, 255, 0));
	HBRUSH blueBrush = CreateSolidBrush(RGB(0, 0, 255));
	HBRUSH blackBrush = CreateSolidBrush(RGB(255, 255, 255));
	HBRUSH magentaBrush = CreateSolidBrush(RGB(255, 0, 255));

	//FONT
	HFONT smallFont = CreateFont(10, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, "SYSTEM_FIXED_FONT");
	HFONT bigFont = CreateFont(20, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, "SYSTEM_FIXED_FONT");


	SelectObject(hdc, bigFont);

	double displayScale = 1.0;
	int imageMinX = 0;
	int imageMinY = 0;
	int imageMaxX = 500;
	int imageMaxY = 500;
	int xOffset = 0;
	int yOffset = 0;
	int xOffsetInfo = 0;
	int yOffsetInfo = 0;
	switch (sensor)
	{
	case 1:
		imageMinX = gs_CV2_IMAGE_MIN_X_1;
		imageMaxX = gs_CV2_IMAGE_MAX_X_1;
		imageMinY = gs_CV2_IMAGE_MIN_Y_1;
		imageMaxY = gs_CV2_IMAGE_MAX_Y_1;
		xOffset = 600 - MIN_PROFILE_DISPLAY_X / 2.0;
		yOffset = 0 - MIN_PROFILE_DISPLAY_Y;
		xOffsetInfo = 600;
		yOffsetInfo = 410;
		break;

	case 2:
		imageMinX = gs_CV2_IMAGE_MIN_X_2;
		imageMaxX = gs_CV2_IMAGE_MAX_X_2;
		imageMinY = gs_CV2_IMAGE_MIN_Y_2;
		imageMaxY = gs_CV2_IMAGE_MAX_Y_2;
		xOffset = 600 - MIN_PROFILE_DISPLAY_X / 2.0;
		yOffset = 200 - MIN_PROFILE_DISPLAY_Y;//200;
		xOffsetInfo = 800;
		yOffsetInfo = 410;
		break;
	}

	if (screenPaintOnce || true)
	{
		//DRAW GRIDLINES IF REQUESTED
		if (DRAW_GRID_LINES)
		{
			SelectObject(hdc, smallFont);
			SelectObject(hdc, fineGreenPen);
			//FOR Y AXIS
			for (double y = 0; y < 500; y = y + 50)
			{
				if (y >= MIN_PROFILE_DISPLAY_Y && y <= MAX_PROFILE_DISPLAY_Y)
				{
					MoveToEx(hdc, 0 + xOffset + MIN_PROFILE_DISPLAY_X, y * displayScale + yOffset, NULL);
					LineTo(hdc, MAX_PROFILE_DISPLAY_X + xOffset, y * displayScale + yOffset);
					std::string val = std::to_string((int)(y));
					TextOut(hdc, 0 + xOffset + MIN_PROFILE_DISPLAY_X, y * displayScale + yOffset, val.c_str(), strlen(val.c_str()));
				}
			}
			//FOR X AXIS
			for (double x = 0; x < 900; x = x + 50)
			{
				if (x >= MIN_PROFILE_DISPLAY_X && x <= MAX_PROFILE_DISPLAY_X)
				{
					MoveToEx(hdc, x * displayScale + xOffset, yOffset + MIN_PROFILE_DISPLAY_Y, NULL);
					LineTo(hdc, x * displayScale + xOffset, yOffset + MAX_PROFILE_DISPLAY_Y);
					std::string val = std::to_string((int)(x));
					TextOut(hdc, x * displayScale + xOffset, yOffset + MIN_PROFILE_DISPLAY_Y, val.c_str(), strlen(val.c_str()));
				}
			}
		}
		SelectObject(hdc, bigFont);

		//OUTPUT PLACEHOLDER LABELS
		if (sensor == 1)
		{
			TextOut(hdc, xOffsetInfo, 5 + yOffsetInfo, "LEFT SENSOR", strlen("LEFT SENSOR"));
		}
		else
		{
			TextOut(hdc, xOffsetInfo, 5 + yOffsetInfo, "RIGHT SENSOR", strlen("RIGHT SENSOR"));

			TextOut(hdc, 1000 + 50, yOffsetInfo/*250*/, "SUMMARY", strlen("SUMMARY"));
			TextOut(hdc, 1000, yOffsetInfo + 20/*270*/, "G: ", strlen("G: "));
			TextOut(hdc, 1000, yOffsetInfo + 40/*290*/, "G (+/-): ", strlen("G (+/-): "));
			TextOut(hdc, 1000, yOffsetInfo + 60/*310*/, "MISS: ", strlen("MISS: "));
			TextOut(hdc, 1000, yOffsetInfo + 80/*330*/, "READ: ", strlen("READ: "));

		}

		TextOut(hdc, xOffsetInfo, 25 + yOffsetInfo, "PIC CNT: ", strlen("PIC CNT: "));
		TextOut(hdc, xOffsetInfo, 45 + yOffsetInfo, "FIFO: ", strlen("FIFO: "));
		TextOut(hdc, xOffsetInfo, 65 + yOffsetInfo, "HG CNT: ", strlen("HG CNT: "));
		TextOut(hdc, xOffsetInfo, 85 + yOffsetInfo, "MISS HG: ", strlen("MISS HG: "));
		TextOut(hdc, xOffsetInfo, 105 + yOffsetInfo, "HG: ", strlen("HG: "));
		TextOut(hdc, xOffsetInfo, 125 + yOffsetInfo, "VERT: ", strlen("HG: "));


		screenPaintOnce = false;
	}


	if (displayPoints.picCounter > 0)//IF THERE IS A NEW PROFILE -- AND IT'S A VALID PROFILE
	{
		//PLOT TEMPLATE MATCHES
		SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
		SelectObject(hdc, fineAuqaPen);

		for (int j = 0; j < RAIL_TEMPLATE_COUNT; j++)
		{
			Rectangle(hdc, displayPoints.templatePoint[j].leftX * displayScale + xOffset, displayPoints.templatePoint[j].topY * displayScale + yOffset, displayPoints.templatePoint[j].rightX * displayScale + xOffset, displayPoints.templatePoint[j].bottomY * displayScale + yOffset);
		}

		//PLOT THE PROFILE POINTS
		SelectObject(hdc, blackBrush);
		SelectObject(hdc, GetStockObject(BLACK_PEN));
		for (int i = 0; i < 2048; i++)
		{
			if (displayPoints.xCoords[i] >= MIN_PROFILE_DISPLAY_X && displayPoints.xCoords[i] <= MAX_PROFILE_DISPLAY_X
				&& displayPoints.yCoords[i] * -1.0 >= MIN_PROFILE_DISPLAY_Y && displayPoints.yCoords[i] * -1.0 <= MAX_PROFILE_DISPLAY_Y)
				Ellipse(hdc, xOffset + displayPoints.xCoords[i] * displayScale - 2, yOffset + -1.0 * displayPoints.yCoords[i] * displayScale - 2, xOffset + displayPoints.xCoords[i] * displayScale + 2, yOffset + -1.0 * displayPoints.yCoords[i] * displayScale + 2);
		}

		//PLOT WEB POINT
		SelectObject(hdc, magentaBrush);
		SelectObject(hdc, magentaPen);
		if (displayPoints.webXVal != 0.0 && displayPoints.webYVal != 0.0)
		{
			Ellipse(hdc, xOffset + displayPoints.webXVal * displayScale - 3, yOffset + -1.0 * displayPoints.webYVal * displayScale - 3, xOffset + displayPoints.webXVal * displayScale + 3, yOffset + -1.0 * displayPoints.webYVal * displayScale + 3);
		}

		//PLOT MY DETECTED TOR POINT
		SelectObject(hdc, blueBrush);
		SelectObject(hdc, bluePen);

		if (displayPoints.tor.xValue > 0)
			Ellipse(hdc, displayPoints.tor.xValue * displayScale + xOffset - 2, -1.0 * displayPoints.tor.yValue * displayScale + yOffset - 2, displayPoints.tor.xValue * displayScale + xOffset + 2, -1.0 * displayPoints.tor.yValue * displayScale + yOffset + 2);

		//DRAW TOR HORIZONTAL LINE
		if (displayPoints.torAvgY > -1000)
		{
			MoveToEx(hdc, 0 + xOffset, displayPoints.torAvgY * displayScale * -1.0 + yOffset, NULL);
			//LineTo(hdc, displayPoints.tor.xValue * displayScale + xOffset, displayPoints.torAvgY * displayScale * -1.0 + yOffset);
			Ellipse(hdc, displayPoints.torAvgY_Xval * displayScale - 3 + xOffset, displayPoints.torAvgY * displayScale * -1.0 - 3 + yOffset, displayPoints.torAvgY_Xval * displayScale + 3 + xOffset, displayPoints.torAvgY * displayScale * -1.0 + 3 + yOffset);
		}

		SelectObject(hdc, greenPen);
		if (displayPoints.y5_8 > -1000)
		{
			MoveToEx(hdc, 0 + xOffset, displayPoints.y5_8 * displayScale * -1.0 + yOffset, NULL);
			//LineTo(hdc, displayPoints.tor.xValue * displayScale + xOffset, displayPoints.y5_8 * displayScale * -1.0 + yOffset);
			Ellipse(hdc, displayPoints.x5_8 * displayScale + xOffset - 3, displayPoints.y5_8 * displayScale * -1.0 + yOffset - 3, displayPoints.x5_8 * displayScale + xOffset + 3, displayPoints.y5_8 * displayScale * -1.0 + yOffset + 3);
		}
	}


	std::string picCounterString = std::to_string(displayPoints.picCounter);
	TextOut(hdc, xOffsetInfo + 80, 25 + yOffsetInfo, picCounterString.c_str(), strlen(picCounterString.c_str()));
	char fifoChar[40];
	sprintf(fifoChar, "%3.0f%%", displayPoints.fifoPercentage);
	TextOut(hdc, xOffsetInfo + 80, 45 + yOffsetInfo, fifoChar, strlen(fifoChar));
	std::string gageReadingCnt = std::to_string(displayPoints.halfGageReadingCnt);
	std::string missedGageReadingCnt = std::to_string(displayPoints.halfGageMissedReadingCnt);
	std::string gageReading = std::to_string(displayPoints.halfGageReading);
	std::string vertReading;
	if (sensor == 1)
	{
		vertReading = std::to_string(displayPoints.torAvgY * -MM2INCH - CALIBRATION_LEFT_VERTICAL_OFFSET);
	}
	else
	{
		vertReading = std::to_string(displayPoints.torAvgY * -MM2INCH - CALIBRATION_RIGHT_VERTICAL_OFFSET);
	}
	TextOut(hdc, xOffsetInfo + 80, 65 + yOffsetInfo, gageReadingCnt.c_str(), strlen(gageReadingCnt.c_str()));
	TextOut(hdc, xOffsetInfo + 80, 85 + yOffsetInfo, missedGageReadingCnt.c_str(), strlen(missedGageReadingCnt.c_str()));
	TextOut(hdc, xOffsetInfo + 80, 105 + yOffsetInfo, gageReading.c_str(), strlen(gageReading.c_str()));
	TextOut(hdc, xOffsetInfo + 80, 125 + yOffsetInfo, vertReading.c_str(), strlen(vertReading.c_str()));

	//DISPLAY GAUGE IF INFO IS FROM SENSOR #1
	if (sensor == 1)
	{
		double fg = displayPoints.fullGageReading;
		char fgChar[20];
		sprintf(fgChar, "%3.4f", fg);
		TextOut(hdc, 1080, yOffsetInfo + 20, fgChar, strlen(fgChar));

		double fgZ = displayPoints.fullGageReading - 56.5;
		char fgZChar[40];
		sprintf(fgZChar, "%3.4f", fgZ);
		TextOut(hdc, 1080, yOffsetInfo + 40, fgZChar, strlen(fgZChar));

		char missedChar[20];
		sprintf(missedChar, "%d", displayPoints.fullGageMissedReadingCnt);
		TextOut(hdc, 1080, yOffsetInfo + 60, missedChar, strlen(missedChar));

		char readChar[20];
		sprintf(readChar, "%d", displayPoints.fullGageReadingCnt);
		TextOut(hdc, 1080, yOffsetInfo + 80, readChar, strlen(readChar));
	}

	//DRAW BOX OF INTEREST
	SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
	SelectObject(hdc, redPen);
	Rectangle(hdc, imageMinX * displayScale + xOffset, imageMinY * displayScale + yOffset, imageMaxX * displayScale + xOffset, imageMaxY * displayScale + yOffset);


	//CLEANUP AND DELETE OBJECTS
	//PENS
	DeleteObject(purplePen);
	DeleteObject(redPen);
	DeleteObject(thickRedPen);
	DeleteObject(bluePen);
	DeleteObject(greenPen);
	DeleteObject(blackPen);
	DeleteObject(fineGreenPen);
	DeleteObject(fineAuqaPen);
	DeleteObject(magentaPen);

	//BRUSHES
	DeleteObject(redBrush);
	DeleteObject(blueBrush);
	DeleteObject(blackBrush);
	DeleteObject(magentaBrush);

	//FONT
	DeleteObject(smallFont);
	DeleteObject(bigFont);

}

//GET THE TACH PULSE PER FOOT SETTING THAT'S BEEN READ IN
int GageSystem::getPulsePerFoot()
{
	return TACH_PULSE_PER_FOOT;
}

