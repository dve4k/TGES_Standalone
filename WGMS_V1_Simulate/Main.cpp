#include "stdafx.h"
#include "Main.h"

#define MAX_LOADSTRING 100
#define MM2INCH 0.0393701
//#pragma optimize("", off)

struct AnalysisResults
{
	void* scannerObj;
	ProfilePoints analyzedPoints;
	int scannerNumber;
};
struct Scanners 
{
	void* scanner_1;
	void* scanner_2;
};

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

unsigned __stdcall	workerThread(void* args);
void				displayProfilePoints(HDC &hdc, ProcessedPoints displayPoints, int sensor);
void				displaySystemStatus(HDC &hdc);
unsigned __stdcall	getProfile(void* args);
unsigned __stdcall	setupScanners(void* args);
unsigned __stdcall	writeProfile(void* args);
unsigned __stdcall	controllerThread(void* args);
unsigned __stdcall	networkInterfaceController(void* args);
void				resetScannersForOperation(Scanners &scanners);
void				startScannerAcq(Scanners &scanners);
void				stopScannersAcq(Scanners &scanners);
void				insertText(std::string text);
void				checkScannerConnections(Scanners &scanners);
void				loadIniFile();
void				displayProfilePointsCombined(HDC& hdc, int sensor, bool& screenPaintOnce, long &pictureCnt);
//long int			readCounter();
//void				startCounter();
//void				stopCounter();


//MORE GLOBALS
HWND winHandle;
ProcessedPoints displayPoints1;
ProcessedPoints displayPoints2;

TemplatePoints lastBestPoint1;
TemplatePoints lastBestPoint2;

ImageProcessing imageProcessor1;
ImageProcessing imageProcessor2;

ProfileLogging logRecordObj;

NetworkInterface netInterface;

std::mutex displayPoints1_mutex;
std::mutex displayPoints2_mutex;

bool workerThreadRun = false;

long globalPacketCnt = 0;
int globalSystemStatus = -1;

bool programRun = true;
double CALIBRATION_GAGE_OFFSET = 0;
bool LOG_PROFILE_POINTS = false;
double gaugeOffset = CALIBRATION_GAGE_OFFSET;
double CALIBRATION_LEFT_VERTICAL_OFFSET = 0.0;
double CALIBRATION_RIGHT_VERTICAL_OFFSET = 0.0;
//int CALIBRATION_ANGLE_1 = 0;
//int CALIBRATION_ANGLE_2 = 0;

bool LOG_PROFILE_PONTS = false;

//##------SCREEN UPDATE GLOBALS-------##//
bool screenPaintOnce = true;
std::string infoLine1;
std::string infoLine2;
std::string infoLine3;
std::string infoLine4;
std::string sensor1Connected = "LEFT DISCONNECTED";
std::string sensor2Connected = "RIGHT DISCONNECTED";

//Automation::BDaq::UdCounterCtrl* advantechCiCtrl = Automation::BDaq::UdCounterCtrl::Create();
//Automation::BDaq::InstantDiCtrl* advantechDiCtrl = Automation::BDaq::InstantDiCtrl::Create();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_WENGLOR03, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WENGLOR03));

    MSG msg;	

	//INITIALIZE IMAGE PROCESSING VARIABLES
	imageProcessor1 = ImageProcessing();
	imageProcessor2 = ImageProcessing();

	//INITIALIZE SOME DISPLAY VARIABLES
	displayPoints1.fullGageMissedReadingCnt = 0;
	displayPoints1.fullGageReadingCnt = 0;
	displayPoints1.halfGageMissedReadingCnt = 0;
	displayPoints1.halfGageReadingCnt = 0;
	displayPoints2.fullGageMissedReadingCnt = 0;
	displayPoints2.fullGageReadingCnt = 0;
	displayPoints2.halfGageMissedReadingCnt = 0;
	displayPoints2.halfGageReadingCnt = 0;

	//SETUP SIMLUATION & LOGGING OBJECT
	loadIniFile();

	logRecordObj = ProfileLogging(true, false, SIMULATION_FILE_NAME_1, SIMULATION_FILE_NAME_2);

	//START THE WORKER THREAD
	HANDLE workerHandle;
	unsigned workerId;
	workerThreadRun = true;
	workerHandle = (HANDLE)_beginthreadex(NULL, 0, &workerThread, NULL, NULL, &workerId);

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
	programRun = false;

    return (int) msg.wParam;
}


//PRIMARY 'RECORDING' LOOP
unsigned __stdcall workerThread(void* args)
{
	//Scanners scanners = *(Scanners*)args;
	//void *scanner1 = scanners.scanner_1;
	//void *scanner2 = scanners.scanner_2;

	int status1 = 0;
	int status2 = 0;


	int oldPicCounter1 = 0;
	int oldPicCounter2 = 0;
	long packetCount = 0;

	while (workerThreadRun)
	{
		HANDLE logThreads[2];
		unsigned logThread1ID;
		unsigned logThread2ID;
		unsigned analyzeThread1ID;
		unsigned analyzeThread2ID;
		HANDLE analyzeThreads[2];

		AnalysisResults scanner1Results;
		//scanner1Results.scannerObj = scanner1;
		scanner1Results.scannerNumber = 1;

		AnalysisResults scanner2Results;
		//scanner2Results.scannerObj = scanner2;
		scanner2Results.scannerNumber = 2;


		//SPIN THE THREAD TO GET SENSOR 
		analyzeThreads[0] = (HANDLE)_beginthreadex(NULL, 0, getProfile, &scanner1Results, NULL, &analyzeThread1ID);

		//SPIN THE THREAD TO GET SENSOR 
		analyzeThreads[1] = (HANDLE)_beginthreadex(NULL, 0, getProfile, &scanner2Results, NULL, &analyzeThread2ID);

		//WAIT FOR PROFILES TO BE GATHERED & PROCESSED -- THEN JOIN THREAD
		WaitForMultipleObjects(2, analyzeThreads, true, INFINITE);
		CloseHandle(analyzeThreads[0]);
		CloseHandle(analyzeThreads[1]);

		if (LOG_PROFILE_PONTS)
		{
			//CHECK TO SEE IF LOG FILE NEEDS TO BE ROLLED OER - DUE TO SIZE
			logRecordObj.checkRollFile();
		}

		if (workerThreadRun)
		{
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
			if (displayPoints1.fullGageReading > 57.5)
			{
				bool breakIt = 1;
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

			//REQUEST THAT WE SEND THE PACKET!
			DataPacket packet;
			packet.hasData = true;
			if(_isnan(scanner2Results.analyzedPoints.halfGage))
			{
				packet.s1_right_filtered = 0x8000;
			}
			else
			{
				packet.s1_right_filtered = scanner2Results.analyzedPoints.halfGage * 256;
			}
			if (_isnan(scanner1Results.analyzedPoints.halfGage))
			{
				packet.s1_left_filtered = 0x8000;
			}
			else
			{
				packet.s1_left_filtered = scanner1Results.analyzedPoints.halfGage * 256;
			}
			if (_isnan(displayPoints1.fullGageReading))
			{
				packet.s1_fullGage_Filtered = 0x8000;
			}
			else
			{
				packet.s1_fullGage_Filtered = (displayPoints1.fullGageReading - 56.5) * 256; //MAYBE CHANGE THIS?????
			}
			
			packetCount++;
			globalPacketCnt = packetCount;
			packet.dataBreak = packetCount;
			netInterface.putSendData(packet);

			//REDRAW THE WINDOW AND CHECK FIFO VALUES
			if (displayPoints1.picCounter % SCREEN_UPDATE_MOD == 0)
			{
				//if (!SIMULATION_MODE)
				//{
				//	int fifoPer1 = EthernetScanner_GetDllFiFoState(scanner1);
				//	displayPoints1.fifoPercentage = fifoPer1;

				//	int fifoPer2 = EthernetScanner_GetDllFiFoState(scanner2);
				//	displayPoints2.fifoPercentage = fifoPer2;
				//}

				InvalidateRect(winHandle, NULL, FALSE);
			}
		}
	}
	//scanners.scanner_1 = scanner1;
	//scanners.scanner_2 = scanner2;
	//*(Scanners*)args = scanners;
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
	CString ipString1 = "192.168.14.100";
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
			//break;
		}
	}
	sensor1Connected = "LEFT CONNECTED";
	InvalidateRect(winHandle, NULL, FALSE);

	//CONNECT TO SENSOR 2
	CString ipString2 = "192.168.13.100";
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
			//break;
		}
	}
	sensor2Connected = "RIGHT CONNECTED";
	InvalidateRect(winHandle, NULL, FALSE);

	//STOP ACQUISITION #1
	CString command = "SetAcquisitionStop";
	int commandLength = strlen(command);
	int resp = EthernetScanner_WriteData(scanners.scanner_1, command.GetBuffer(0), commandLength);
	std::string respString = std::to_string(resp);
	insertText(respString + "-ACQ STOP NUM1");
 
	Sleep(1000);
	//STOP ACQUISITION #2
	command = "SetAcquisitionStop";
	commandLength = strlen(command);
	resp = EthernetScanner_WriteData(scanners.scanner_2, command.GetBuffer(0), commandLength);
	respString = std::to_string(resp);
	insertText(respString + "-ACQ STOP NUM2");
	//OutputDebugString(respString.c_str());
	//OutputDebugString("-ACQ STOP NUM2\n");

	//SET CONFIGURATION SET
	command = "SetSettingsLoad=0";
	commandLength = strlen(command);
	resp = EthernetScanner_WriteData(scanners.scanner_1, command.GetBuffer(0), commandLength);
	respString = std::to_string(resp);
	insertText(respString + "-USER SET LOAD NUM1");
	resp = EthernetScanner_WriteData(scanners.scanner_2, command.GetBuffer(0), commandLength);
	respString = std::to_string(resp);
	insertText(respString + "-USER SET LOAD NUM2");

	Sleep(3000);

	//RESET PICTURE COUNTER #1
	command = "SetResetPictureCounter";
	commandLength = strlen(command);
	resp = EthernetScanner_WriteData(scanners.scanner_1, command.GetBuffer(0), commandLength);
	respString = std::to_string(resp);
	insertText(respString + "-PIC CNTER RESET NUM1");
	//OutputDebugString(respString.c_str());
	//OutputDebugString("-PIC CNTER RESET NUM1\n");


	//RESET PICTURE COUNTER #2
	command = "SetResetPictureCounter";
	commandLength = strlen(command);
	resp = EthernetScanner_WriteData(scanners.scanner_2, command.GetBuffer(0), commandLength);
	respString = std::to_string(resp);
	insertText(respString + "-PIC CNTER RESET NUM2");
	//OutputDebugString(respString.c_str());
	//OutputDebugString("-PIC CNTER RESET NUM2\n");

	Sleep(1000);

	//RESET FIFO #1
	int resetResp1 = EthernetScanner_ResetDllFiFo(scanners.scanner_1);

	//RESET FIFO #2
	int resetResp2 = EthernetScanner_ResetDllFiFo(scanners.scanner_2);

	*((Scanners*)args) = scanners;

	Sleep(1000);

	return 1;
}

//PROFILE WRITING/LOGGING THREAD
unsigned __stdcall writeProfile(void* args)
{
	ProfilePoints points = *(ProfilePoints*)args;
	logRecordObj.writeProfilePoints(points);
	return 1;
}

//DRAWING THE PROFILES AND DISPLAYING INFO
void displayProfilePoints(HDC &hdc, ProcessedPoints displayPoints, int sensor)
{
	double scale = 1.5;

	//PENS
	HPEN purplePen = CreatePen(PS_SOLID, 3, RGB(255, 0, 255));
	HPEN redPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
	HPEN thickRedPen = CreatePen(PS_SOLID, 4, RGB(255, 0, 0));
	HPEN bluePen = CreatePen(PS_SOLID, 1, RGB(0, 0, 255));
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
		imageMinX = CV2_IMAGE_MIN_X_1;
		imageMaxX = CV2_IMAGE_MAX_X_1;
		imageMinY = CV2_IMAGE_MIN_Y_1;
		imageMaxY = CV2_IMAGE_MAX_Y_1;
		xOffset = 0;
		yOffset = 0;
		break;

	case 2:
		imageMinX = CV2_IMAGE_MIN_X_2;
		imageMaxX = CV2_IMAGE_MAX_X_2;
		imageMinY = CV2_IMAGE_MIN_Y_2;
		imageMaxY = CV2_IMAGE_MAX_Y_2;
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
			if (displayPoints.xCoords[i] >= MIN_PROFILE_DISPLAY_X && displayPoints.xCoords[i] <= MAX_PROFILE_DISPLAY_X
				&& displayPoints.yCoords[i] * -1.0 >= MIN_PROFILE_DISPLAY_Y && displayPoints.yCoords[i] * -1.0 <= MAX_PROFILE_DISPLAY_Y)
					Ellipse(hdc, xOffset + displayPoints.xCoords[i] * scale - 2, -1.0 * displayPoints.yCoords[i] * scale - 2, xOffset + displayPoints.xCoords[i] * scale + 2, -1.0 * displayPoints.yCoords[i] * scale + 2);
		}

		//PLOT MY DETECTED TOR POINT
		SelectObject(hdc, blueBrush);
		SelectObject(hdc, bluePen);

		if (displayPoints.tor.xValue > 0)
			Ellipse(hdc, displayPoints.tor.xValue* scale + xOffset - 3, -1.0 * displayPoints.tor.yValue * scale - 3, displayPoints.tor.xValue * scale + xOffset + 3, -1.0*displayPoints.tor.yValue * scale + 3);

		//DRAW TOR HORIZONTAL LINE
		if (displayPoints.torAvgY > -1000)
		{
			MoveToEx(hdc, 0 + xOffset, displayPoints.torAvgY * -1.0, NULL);
			Ellipse(hdc, xOffset + displayPoints.torAvgY_Xval * scale - 3, displayPoints.torAvgY * scale * -1.0 - 3, xOffset + displayPoints.torAvgY_Xval * scale + 3, displayPoints.torAvgY * scale * -1.0 + 3);
			//LineTo(hdc, displayPoints.tor.xValue + xOffset+400, displayPoints.torAvgY * -1.0);
			//LineTo(hdc, xOffset + displayPoints.torAvgY_Xval, displayPoints.torAvgY * -1.0);

		}

		SelectObject(hdc, greenPen);
		if (displayPoints.y5_8 > -1000)
		{
			MoveToEx(hdc, 0 + xOffset, displayPoints.y5_8 * -1.0, NULL);
			//LineTo(hdc, displayPoints.tor.xValue + xOffset, displayPoints.y5_8 * -1.0);
			Ellipse(hdc, displayPoints.x5_8* scale + xOffset - 2, displayPoints.y5_8 * scale * -1.0 - 2, displayPoints.x5_8 * scale + xOffset + 2, displayPoints.y5_8 * scale * -1.0 + 2);
		}

		//PLOT TEMPLATE MATCHES
		SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
		SelectObject(hdc, fineAuqaPen);

		for (int j = 0; j < RAIL_TEMPLATE_COUNT; j++)
		{
			Rectangle(hdc, displayPoints.templatePoint[j].leftX* scale + xOffset, displayPoints.templatePoint[j].topY* scale, displayPoints.templatePoint[j].rightX* scale + xOffset, displayPoints.templatePoint[j].bottomY* scale);
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

		if (/*displayPoints.fullGageReading > 57.5 ||*/ _isnan(displayPoints.fullGageReading))
		{
			bool breakOne = 1;
		}
		if (displayPoints.fullGageMissedReadingCnt > 0)
		{
			int checkToo = 1;
		}
	}

	//DRAW BOX OF INTEREST
	SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
	SelectObject(hdc, redPen);
	Rectangle(hdc, imageMinX* scale + xOffset, imageMinY* scale, imageMaxX* scale + xOffset, imageMaxY* scale);


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

//DRAW SYSTEM STATUS AND PACKET NUMBER
void displaySystemStatus(HDC &hdc)
{
	//FONT
	HFONT smallFont = CreateFont(10, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, "SYSTEM_FIXED_FONT");
	HFONT bigFont = CreateFont(20, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, "SYSTEM_FIXED_FONT");

	SelectObject(hdc, bigFont);

	char packetChar[20];
	sprintf(packetChar, "PACKET: %d", globalPacketCnt);
	TextOut(hdc, 550, 350, packetChar, strlen(packetChar));

	char statusChar[20];
	switch (globalSystemStatus)
	{
	case 1:
		sprintf(statusChar, "STATUS: ARMED", strlen(statusChar));
		break;
	case 2:
		sprintf(statusChar, "STATUS: START WORKER", strlen(statusChar));
		break;
	case 3:
		sprintf(statusChar, "STATUS: WORKER RUN", strlen(statusChar));
		break;
	case 4:
		sprintf(statusChar, "STATUS: STOPPING", strlen(statusChar));
	default:
		sprintf(statusChar, "STATUS: N/A", strlen(statusChar));
		break;
	}
	TextOut(hdc, 550, 370, statusChar, strlen(statusChar));

	SelectObject(hdc, smallFont);
	TextOut( hdc, 550, 450, infoLine1.c_str(), strlen(infoLine1.c_str()));
	TextOut(hdc, 550, 470, infoLine2.c_str(), strlen(infoLine2.c_str()));
	TextOut(hdc, 550, 490, infoLine3.c_str(), strlen(infoLine3.c_str()));
	TextOut(hdc, 550, 510, infoLine4.c_str(), strlen(infoLine4.c_str()));
	TextOut(hdc, 550, 530, sensor1Connected.c_str(), strlen(sensor1Connected.c_str()));
	TextOut(hdc, 550, 550, sensor2Connected.c_str(), strlen(sensor2Connected.c_str()));
	if (LOG_PROFILE_PONTS)
	{
		TextOut(hdc, 550, 570, "LOGGING PROFILES", 17);
	}
	else
	{
		TextOut(hdc, 550, 570, "NOT LOGGING PROFILES", 21);
	}
	char offsetChar[40];
	sprintf(offsetChar, "GAGE OFFSET: %f", CALIBRATION_GAGE_OFFSET);
	TextOut(hdc, 550, 590, offsetChar, 22);

	//DELETE FONT
	DeleteObject(smallFont);
	DeleteObject(bigFont);
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

	logRecordObj.readSimulationLine(scannerNumber, rawPoints);


	//SPIN THREAD TO LOG PROFILE POINTS -- IF REQUESTED
	unsigned logThreadID;
	HANDLE logThread;
	if (LOG_PROFILE_PONTS)
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
	return 1;
}

//CONTROLLER THREAD
unsigned __stdcall controllerThread(void* args)
{
	Scanners scanners = *(Scanners*)args;

	//SETUP THE NETWORK INTERFACE WITH EM1
	unsigned netInterfaceControllerID;
	HANDLE netInterfaceHandle = (HANDLE)_beginthreadex(NULL, 0, &networkInterfaceController, NULL, NULL, &netInterfaceControllerID);
	//MY PROGRAM STATES ARE:
	//(1) WAITING FOR A START COMMAND -- JUST READING
	//(2) TRANSITION TO COLLECTING/SENDING
	//(3) COLLECTING/SENDING DATA -- WHILE CHECKING FOR READS
	//(4) SHUTTING DOWN
	int state = 1;
	globalSystemStatus = 1;
	HANDLE workerHandle = NULL;
	unsigned workerId;
	while (true)
	{
		//CONFIRM THAT SCANNERS ARE STILL CONNECTED
		checkScannerConnections(scanners);
		if (state == 1 || state == 3)
		{
			EM1Command em1Command = netInterface.getRcvData();
			DataPacket packet;

			if (em1Command.commandName != -99)
			{
				int viperBreak = 1;
			}
			switch (em1Command.commandName)
			{
			case FORWARD_CMD:
				packet.sendAckInstead = true;
				netInterface.putSendData(packet);
				break;
			case REVERSE_CMD:
				packet.sendAckInstead = true;
				netInterface.putSendData(packet);
				break;
			case FLUSHBUFFER_CMD:
				packet.sendAckInstead = true;
				netInterface.putSendData(packet);
				break;
			case SETUP_CMD:
				packet.sendAckInstead = true;
				netInterface.putSendData(packet);
				break;
			case PREPARESEND_CMD:
				packet.sendAckInstead = true;
				resetScannersForOperation(scanners);
				startScannerAcq(scanners);
				netInterface.putSendData(packet);
				break;
			case PREPARESENDASC_CMD:
				packet.sendAckInstead = true;
				resetScannersForOperation(scanners);
				startScannerAcq(scanners);
				netInterface.putSendData(packet);
				break;
			case PREPARESENDDSC_CMD:
				packet.sendAckInstead = true;
				resetScannersForOperation(scanners);
				startScannerAcq(scanners);
				netInterface.putSendData(packet);
				break;
			case STOP_CMD:
				//STOP THE RECORD
				state = 4;
				break;
			case SYNC_CMD:
				//START THE RECORD
				state = 2;
				break;
			}
		}
		if (state == 2)
		{
			//SPINUP WORKER THREAD
			globalSystemStatus = 2;
			workerThreadRun = true;
			workerHandle = (HANDLE)_beginthreadex(NULL, 0, &workerThread, &scanners, NULL, &workerId);
			state = 3;	
		}
		if (state == 3)
		{
			globalSystemStatus = 3;
			//CHECK THAT EVERYTHING IS STILL OK!
			//WHAT ELSE?????
			if (!netInterface.checkSocketConnection())
			{
				state = 4;
			}
		}
		if (state == 4)
		{
			globalSystemStatus = 4;
			workerThreadRun = false;
			WaitForSingleObject(workerHandle, INFINITE);
			CloseHandle(workerHandle);
			if (!SIMULATION_MODE)
			{
				stopScannersAcq(scanners);
			}
			state = 1;
			globalSystemStatus = 1;
		}
	}
	return 1;
}

//START NETWORK INTERFACE CONTROLLER
unsigned __stdcall networkInterfaceController(void* args)
{
	netInterface.controllerThread();
	return 1;
}

//RESET SCANNERS FOR OPERATION
void resetScannersForOperation(Scanners &scanners)
{
	if (!SIMULATION_MODE)
	{
		CString command = "SetResetPictureCounter";
		int commandLength = strlen(command);
		int resp = EthernetScanner_WriteData(scanners.scanner_1, command.GetBuffer(0), commandLength);
		std::string respString = std::to_string(resp);
		insertText(respString + "-PIC CNTER RESET NUM1\n");
		OutputDebugString(respString.c_str());
		OutputDebugString("-PIC CNTER RESET NUM1\n");


		//RESET PICTURE COUNTER #2
		command = "SetResetPictureCounter";
		commandLength = strlen(command);
		resp = EthernetScanner_WriteData(scanners.scanner_2, command.GetBuffer(0), commandLength);
		respString = std::to_string(resp);
		insertText(respString + "-PIC CNTER RESET NUM2");
		OutputDebugString(respString.c_str());
		OutputDebugString("-PIC CNTER RESET NUM2\n");

		Sleep(200);

		//RESET FIFO #1
		int resetResp1 = EthernetScanner_ResetDllFiFo(scanners.scanner_1);

		//RESET FIFO #2
		int resetResp2 = EthernetScanner_ResetDllFiFo(scanners.scanner_2);

		Sleep(500);
	}
}

//START SCANNERS ACQUISITION
void startScannerAcq(Scanners &scanners)
{
	//START ACQUISITION #1
	bool sens1Started = false;
	bool sens2Started = false;
	do
	{
		CString command = "SetAcquisitionStart";
		int commandLength = strlen(command);
		int resp = EthernetScanner_WriteData(scanners.scanner_1, command.GetBuffer(0), commandLength);
		std::string respString = std::to_string(resp);
		insertText(respString + "-ACQ START NUM1");
		OutputDebugString(respString.c_str());
		OutputDebugString("-ACQ START NUM1\n");
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
		std::string respString = std::to_string(resp);
		insertText(respString + "-ACQ START NUM2");
		OutputDebugString(respString.c_str());
		OutputDebugString("-ACQ START NUM2\n");
		if (resp > 0)
		{
			sens2Started = true;
		}
	} while (!sens2Started);

}

//START CAMERA AQ
void stopScannersAcq(Scanners &scanners)
{
	//STOP ACQUISITION #2 -- #2 IS MASTER
	CString command = "SetAcquisitionStop";
	int commandLength = strlen(command);
	int resp2 = EthernetScanner_WriteData(scanners.scanner_2, command.GetBuffer(0), commandLength);
	std::string respString2 = std::to_string(resp2);
	insertText(respString2 + "-ACQ STOP NUM2");
	OutputDebugString(respString2.c_str());
	OutputDebugString("-ACQ STOP NUM2\n");

	//STOP ACQUISITION #1 -- #1 IS SLAVE
	int resp = EthernetScanner_WriteData(scanners.scanner_1, command.GetBuffer(0), commandLength);
	std::string respString = std::to_string(resp);
	insertText(respString + "-ACQ STOP NUM1");
	OutputDebugString(respString.c_str());
	OutputDebugString("-ACQ STOP NUM1\n");
}

//INSERT TEXT TO INFO LINES
void insertText(std::string text)
{
	infoLine4 = infoLine3;
	infoLine3 = infoLine2;
	infoLine2 = infoLine1;
	infoLine1 = text;
}

//CHECK WENGLOR SENSOR CONNECTIONS
void checkScannerConnections(Scanners &scanners)
{
	int s1Connected, s2Connected;
	EthernetScanner_GetConnectStatus(scanners.scanner_1, &s1Connected);
	EthernetScanner_GetConnectStatus(scanners.scanner_2, &s2Connected);
	if (s1Connected == 3)
	{
		sensor1Connected = "LEFT CONNECTED";
	}
	else
	{
		sensor1Connected = "LEFT DISCONNECTED";
	}
	if (s2Connected == 3)
	{
		sensor2Connected = "RIGHT CONNECTED";
	}
	else
	{
		sensor2Connected = "RIGHT DISCONNECTED";
	}
}

//LOAD INI CONFIGURATION FILE
void loadIniFile()
{
	std::ifstream configFile;
	configFile = std::ifstream(iniConfigFile);
	bool isOpen = configFile.is_open();
	std::string aLine;
	//std::getline(configFile, aLine);
	while (!std::getline(configFile, aLine).eof())
	{
		std::string logFileString = "LOG_PROFILE";
		std::string gageOffset = "GAGE_OFFSET";
		std::string leftVerticalOffset = "LEFT_VERTICAL_OFFSET";
		std::string rightVerticalOffset = "RIGHT_VERTICAL_OFFSET";
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
		}
	}
}


//DRAWING THE PROFILES AND DISPLAYING INFO -- COMBINED GAGE & INERTIAL VERSION
void displayProfilePointsCombined(HDC& hdc, int sensor, bool& screenPaintOnce, long& pictureCnt)
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
		imageMinX = CV2_IMAGE_MIN_X_1;
		imageMaxX = CV2_IMAGE_MAX_X_1;
		imageMinY = CV2_IMAGE_MIN_Y_1;
		imageMaxY = CV2_IMAGE_MAX_Y_1;
		xOffset = 600 - MIN_PROFILE_DISPLAY_X / 2.0;
		yOffset = 0 - MIN_PROFILE_DISPLAY_Y;
		xOffsetInfo = 600;
		yOffsetInfo = 410;
		break;

	case 2:
		imageMinX = CV2_IMAGE_MIN_X_2;
		imageMaxX = CV2_IMAGE_MAX_X_2;
		imageMinY = CV2_IMAGE_MIN_Y_2;
		imageMaxY = CV2_IMAGE_MAX_Y_2;
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
////READ THE COUNTER VALUE
//long int readCounter()
//{
//	Automation::BDaq::ErrorCode ret = Automation::BDaq::Success;
//	long int value = 0;
//	ret = advantechCiCtrl->Read(value);
//	if (BioFailed(ret))
//	{
//		int problem3 = 1;
//	}
//	return value;
//}
//
////START THE COUNTER
//void startCounter()
//{
//	Automation::BDaq::ErrorCode ret = Automation::BDaq::Success;
//	
//	ret = advantechCiCtrl->ValueReset();
//
//	if (BioFailed(ret))
//	{
//		int problem3 = 1;
//	}
//
//	ret = advantechCiCtrl->setEnabled(true);
//
//	if (BioFailed(ret))
//	{
//		int problem3 = 1;
//	}
//}
//
////STOP THE COUNTER
//void stopCounter()
//{
//	Automation::BDaq::ErrorCode ret = Automation::BDaq::Success;
//	ret = advantechCiCtrl->setEnabled(false);
//	if (BioFailed(ret))
//	{
//		int problem3 = 1;
//	}
//}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WENGLOR03));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_WENGLOR03);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable
   //szTitle
   HWND hWnd = CreateWindowW(szWindowClass, /*szTitle*/L"WJE SIMULATION V1.0 : 9/28/2018", WS_OVERLAPPEDWINDOW,
      0, 0, 1250, 680, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }
   ////#######GAGE CALIBRATION#########//
   //HWND hWnd_Button_1 = CreateWindowW(L"BUTTON", L"CAL GAUGE",
   // WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
   // 600, 400, 80, 20, hWnd, (HMENU)201, hInstance, nullptr);

   //HWND hWnd_Textbox_1 = CreateWindowW(L"EDIT", L"",
   // WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL | ES_WANTRETURN,
   // 600, 430, 80, 20, hWnd, (HMENU)202, hInstance, nullptr);

   //SETUP 1Hz TIMER
   UINT timer1 = 1;
   SetTimer(hWnd, timer1, 1000, (TIMERPROC)NULL);
  

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
				case 201://GENERATE GAUGE OFFSET
					//int len = GetWindowTextLengthW(GetDlgItem(hWnd, 12)) + 1;
					wchar_t text[20];
					GetWindowTextW(GetDlgItem(hWnd, 202), text, 20);
					double gaugeReading;
					gaugeReading = std::stod(text);
					gaugeOffset = gaugeReading - displayPoints1.halfGageReading - displayPoints2.halfGageReading;			
					MessageBox(NULL, std::to_string(gaugeOffset).c_str(), "NEW GAUGE OFFSET", MB_OK);
					break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

			HDC hdcMem;
			HBITMAP hbmMem, hbmOld;
			HBRUSH hbrBkGnd;

			RECT fullRect;
			GetClientRect(hWnd, &fullRect);
			hdcMem = CreateCompatibleDC(hdc);
			hbmMem = CreateCompatibleBitmap(hdc, fullRect.right - fullRect.left, fullRect.bottom - fullRect.top);
			hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);
			
			hbrBkGnd = CreateSolidBrush(RGB(255, 255, 255));
			SelectObject(hdcMem, hbrBkGnd);
			Rectangle(hdcMem, fullRect.left, fullRect.top, fullRect.right, fullRect.bottom);
			DeleteObject(hbrBkGnd);

			displaySystemStatus(hdcMem);

			long scanner1Cnt = 0;
			long scanner2Cnt = 0;
			bool onScreenOnce1 = false;
			bool onScreenOnce2 = false;

			if (BIG_PROFILE_DISPLAY)
			{
				displayProfilePoints(hdcMem, displayPoints1, 1);
				displayProfilePoints(hdcMem, displayPoints2, 2);
			}
			else
			{
				displayProfilePointsCombined(hdcMem, 1, onScreenOnce1, scanner1Cnt);
				displayProfilePointsCombined(hdcMem, 2, onScreenOnce2, scanner2Cnt);
			}

			SetBkMode(hdcMem, TRANSPARENT);

			BitBlt(ps.hdc,
				fullRect.left, fullRect.top,
				fullRect.right - fullRect.left, fullRect.bottom - fullRect.top,
				hdcMem,
				0, 0,
				SRCCOPY);

			SelectObject(hdcMem, hbmOld);
			DeleteObject(hbmMem);
			DeleteObject(hdcMem);
			
			DeleteDC(hdc);//??????
            EndPaint(hWnd, &ps);
        }
        break;

	case WM_ERASEBKGND:
		{
		return (LRESULT)1;
		}

	case WM_TIMER:
	{
		InvalidateRect(winHandle, NULL, FALSE);
		break;
	}
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hWnd, message, wParam, lParam); //DefWindowProc
    }
    return 0;
}


