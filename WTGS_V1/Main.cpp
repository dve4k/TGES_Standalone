#include "stdafx.h"
#include "Main.h"

#define MAX_LOADSTRING 100

#define ENABLE_WENGLORS true
//#pragma optimize("", off)

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

void				displaySystemStatus(HDC &hdc);
unsigned __stdcall	controllerThread(void* args);
unsigned __stdcall	networkInterfaceController(void* args);
void				insertText(std::string text);
unsigned __stdcall	intertialSysSendingPlasserThread(void* args);
unsigned __stdcall	inertialSysRunThread(void* args);
unsigned __stdcall	gageSysRunThread(void* args);
unsigned __stdcall	gageTransferThread(void* args);
unsigned __stdcall  niUDPSendInterfaceThread(void* args);
unsigned __stdcall  niUDPRecvInterfaceThread(void* args);
unsigned __stdcall locomotiveControllerThread(void* args);
unsigned __stdcall backOfficeMonitorCollectThread(void* args);
unsigned __stdcall backOfficeSendingThread(void* args);

//MORE GLOBALS
HWND winHandle;

//-- PLASSER NETWORK INTERFACE --//
NetworkInterface netInterface;

//-- INERTIAL SYSTEM --//
InertialSystem inertialSystem;

//SENDING DATA FROM INTERTIAL TO PLASSER FLAG
bool sendingDataToPlasser = false;
long dataBreakCount = 0;

//-- GAGE SYSTEM --//
GageSystem gageSystem;
double CALIBRATION_GAGE_OFFSET_DISP = 0;
bool LOG_PROFILE_POINTS_DISP = false;
//END GAGE SYSTEM

//NI BOARD UDP COMM
NI_UDP_Interface niInterface;

//BACK OFFICE MONITOR COMMS
BackOfficeUDP backOfficeInterface;

std::mutex displayInfoMutex;
int globalSystemStatus = -1;
int gageSystemCounter_main = 0;
int inertialSystemCounter_main = 0;
int gageInertial_diff = 0;

bool programRun = true;
//FLAG FOR TRANSFERING GAGE DATA TO INERTIAL SYSTEM
bool gageTransferEnable = true;

//##------SCREEN UPDATE GLOBALS-------##//
bool screenPaintOnce = true;
std::string infoLine1;
std::string infoLine2;
std::string infoLine3;
std::string infoLine4;
std::string sensor1Connected = "LEFT DISCONNECTED";
std::string sensor2Connected = "RIGHT DISCONNECTED";

std::mutex tachSimulatorParams_mutex;
bool tachSimulator_Enable = false;
int tachSimulator_Speed = 30;
int tachSimulator_Direction = 1;
int tachSimulator_SendFoot = 0;
int tachSimulator_resetFtCount = 0;

bool BACK_OFFICE_COLLECTING_THREAD_ENABLE = true;
TGES_BACKOFFICE_PACKET lastBackOfficePacketSent;

bool enabledGeomExcUdpSending = false;

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

	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS); //REALTIME_PRIORITY_CLASS IS HIGHEST
	//COMMAND LINE ARGUMENTS -sendudp WILL ENABLE THE UDP SENDING
	wchar_t* cmp = wcsstr(lpCmdLine, L"-sendudp");
	if (lstrlenW(cmp) > 0)
	{
		enabledGeomExcUdpSending = true;
	}



	//TEST WENGLOR CODE
	//Scanners tmpScanners;
	//gageSystem.prepareForCapture(gageSystem.globalScanners);
	//gageSystem.startGageCapture(gageSystem.globalScanners);
	//inertialSystem = InertialSystem();
	//inertialSysRunThread(NULL);


	HANDLE inertialThreadHandle;
	unsigned int inertialThreadID;
	HANDLE plasserControllerHandle;
	unsigned int plasserControllerID;
	HANDLE locomotiveControllerHandle;
	unsigned int locomotiveControllerID;

	HANDLE gageWorkerhandle_constant;
	HANDLE inertialWorkerHandle_constant;
	HANDLE gageTransferHandle_constant;
	unsigned int gageWorkerID_constant;
	unsigned int inertialWorkerId_constant;
	unsigned int gageTransferID_constant;
	

	HANDLE niUdpSendThread;
	unsigned int niUdpSendThreadID;
	niInterface = NI_UDP_Interface();
	niUdpSendThread = (HANDLE) _beginthreadex(NULL, 0, &niUDPSendInterfaceThread, NULL, NULL, &niUdpSendThreadID);

	HANDLE niUdpRecvThread;
	unsigned int niUdpRecvThreadID;
	niUdpRecvThread = (HANDLE)_beginthreadex(NULL, 0, &niUDPRecvInterfaceThread, NULL, NULL, &niUdpRecvThreadID);

	HANDLE backOfficeSendThread;
	unsigned int backOfficeSendThreadID;
	backOfficeInterface = BackOfficeUDP();
	lastBackOfficePacketSent = TGES_BACKOFFICE_PACKET();
	lastBackOfficePacketSent.dataBreakCount = 0;
	lastBackOfficePacketSent.directiontravel = 0;
	lastBackOfficePacketSent.estMilepost = 0;
	lastBackOfficePacketSent.estTrack = 0;
	lastBackOfficePacketSent.gageCount = 0;
	lastBackOfficePacketSent.inertialCount = 0;
	lastBackOfficePacketSent.gageCount = 0;
	lastBackOfficePacketSent.g_i_difference = 0;
	lastBackOfficePacketSent.programState = -1;
	backOfficeSendThread = (HANDLE)_beginthreadex(NULL, 0, &backOfficeSendingThread, NULL, NULL, &backOfficeSendThreadID);

	HANDLE backOfficeCollectThread;
	unsigned int backOfficeCollectThreadID;
	backOfficeCollectThread = (HANDLE)_beginthreadex(NULL, 0, &backOfficeMonitorCollectThread, NULL, NULL, &backOfficeCollectThreadID);



	if (ENABLE_PLASSER_SEND)
	{
		plasserControllerHandle = (HANDLE)_beginthreadex(NULL, 0, &controllerThread, NULL, NULL, &plasserControllerID);
	}
	else
	{
		if (ENABLE_LOCOMOTIVE_CRIO_CONTROL)
		{
			locomotiveControllerHandle = (HANDLE)_beginthreadex(NULL, 0, &locomotiveControllerThread, NULL, NULL, &locomotiveControllerID);
		}
		else
		{
			displayInfoMutex.lock();
			dataBreakCount = 0;
			displayInfoMutex.unlock();

			if (!ACCEPT_APPLANIX_GAGE)
			{
				gageSystem.prepareForCapture(gageSystem.globalScanners);
				gageWorkerhandle_constant = (HANDLE)_beginthreadex(NULL, 0, &gageSysRunThread, NULL, NULL, &gageWorkerID_constant);
			}

			inertialSystem = InertialSystem();
			inertialWorkerHandle_constant = (HANDLE)_beginthreadex(NULL, 0, &inertialSysRunThread, NULL, NULL, &inertialWorkerId_constant);
			if (!ACCEPT_APPLANIX_GAGE)
			{
				gageTransferHandle_constant = (HANDLE)_beginthreadex(NULL, 0, &gageTransferThread, NULL, NULL, &gageTransferID_constant);
			}
		}
	}


    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

	//GATING PLASSER PULSES
	//niInterface.ni_udp_putDataToSend(0);
	NI_SEND_DATA sendDataTmp;
	int setup_pulsePerFoot = gageSystem.getPulsePerFoot();
	sendDataTmp.FOOT_ENABLE = 0; //DISABLE THE FOOT PULSE OUTPUT
	sendDataTmp.MEASURE_DIRECTION = 1; //ASSIGN THE MEASURING DIRECTION ... FORWARD/REVERSE
	sendDataTmp.RELAY_0 = 0;
	sendDataTmp.RELAY_1 = 0;
	sendDataTmp.RELAY_2 = 0;
	sendDataTmp.RELAY_3 = 0;
	sendDataTmp.RELAY_4 = 0;
	sendDataTmp.RELAY_5 = 0;
	sendDataTmp.RELAY_6 = 0;
	sendDataTmp.RELAY_7 = 0;
	sendDataTmp.RESET_FT = 1; //RESET FOOT COUNT TO 0
	sendDataTmp.SCALE_FACTOR = setup_pulsePerFoot; //ASSIGN THE NUMBER OF TACH PULSES PER FOOT
	sendDataTmp.SIM_ENABLE = 0; //DISABLE SIMULATE
	sendDataTmp.SIM_SPEED = 30; //ROUGHLY 60 MPH...30uS PER QUARTER PULSE
	niInterface.ni_udp_putDataToSend(sendDataTmp);
	Sleep(2000);
	//if (ENABLE_PLASSER_SEND)
	//{
	//	WaitForSingleObject(plasserControllerHandle, INFINITE);
	//	CloseHandle(plasserControllerHandle);
	//}
	//else
	//{
	//	WaitForSingleObject(inertialThreadHandle, INFINITE);
	//	CloseHandle(inertialThreadHandle);
	//}
    return (int) msg.wParam;
}


//START NETWORK INTERFACE CONTROLLER
unsigned __stdcall networkInterfaceController(void* args)
{
	netInterface = NetworkInterface();
	netInterface.controllerThread();
	return 1;
}

//DRAW SYSTEM STATUS AND PACKET NUMBER
void displaySystemStatus(HDC &hdc)
{
	//FONT
	HFONT smallFont = CreateFont(10, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, "SYSTEM_FIXED_FONT");
	HFONT bigFont = CreateFont(20, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, "SYSTEM_FIXED_FONT");

	SelectObject(hdc, bigFont);

	char packetChar[20];
	std::unique_lock<std::mutex> l(displayInfoMutex);
	sprintf(packetChar, "PACKET: %d", dataBreakCount);
	TextOut(hdc, 250, 540, packetChar, strlen(packetChar));

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
	l.unlock();
	TextOut(hdc, 10, 540, statusChar, strlen(statusChar));

	SelectObject(hdc, smallFont);
	TextOut( hdc, 10, 560, infoLine1.c_str(), strlen(infoLine1.c_str()));
	TextOut(hdc, 10, 580, infoLine2.c_str(), strlen(infoLine2.c_str()));
	TextOut(hdc, 10, 600, infoLine3.c_str(), strlen(infoLine3.c_str()));
	TextOut(hdc, 10, 620, infoLine4.c_str(), strlen(infoLine4.c_str()));
	TextOut(hdc, 10, 640, sensor1Connected.c_str(), strlen(sensor1Connected.c_str()));
	TextOut(hdc, 10, 660, sensor2Connected.c_str(), strlen(sensor2Connected.c_str()));
	if (LOG_PROFILE_POINTS_DISP)
	{
		TextOut(hdc, 550, 570, "LOGGING PROFILES", 17);
	}
	else
	{
		TextOut(hdc, 550, 570, "NOT LOGGING PROFILES", 21);
	}
	char offsetChar[40];
	sprintf(offsetChar, "GAGE OFFSET: %f", CALIBRATION_GAGE_OFFSET_DISP);
	TextOut(hdc, 550, 590, offsetChar, 22);

	//SIMULATION PARAMETERS
	std::unique_lock<std::mutex> l2(tachSimulatorParams_mutex);
	char SimParamChars[90];
	sprintf(SimParamChars, "Sim Enable: %d Sim Direction: %d Sim Speed: %d Foot Out Enable: %d Sim Reset Ft: %d", tachSimulator_Enable, tachSimulator_Direction, tachSimulator_Speed, tachSimulator_SendFoot, tachSimulator_resetFtCount);
	l2.unlock();
	TextOut(hdc, 100, 590, SimParamChars, 90);

	//GET CURRENT CRIO SETUP
	NI_RECV_DATA recvData = niInterface.getRecentRecvDataNI();
	char crioState[300];
	sprintf(crioState, "Ft Enable: %d Measure Dir: %d Detected Dir: %d R0: %d R1: %d R2: %d R3: %d R4: %d R5: %d R6: %d R7: %d Ft Cnt: %d Scale Fac: %d Sim Enable: %d Sim Speed: %d Applanix Spd: %f Speed Cnt: %d", 
		recvData.OUT_FOOT_ENABLE, recvData.MEASURING_DIRECTION, recvData.DIRECTION_OF_TRAVEL, recvData.RELAY_0, recvData.RELAY_1, recvData.RELAY_2, recvData.RELAY_3,
		recvData.RELAY_4, recvData.RELAY_5, recvData.RELAY_6, recvData.RELAY_7, recvData.FOOT_COUNT, recvData.SCALE_FACTOR,
		recvData.SIM_ENABLE, recvData.SIM_SPEED, recvData.APPLANIX_SPEED * 2.237, recvData.SPEED_CNT);
	TextOut(hdc, 100, 610, crioState, 300);

	//DELETE FONT
	DeleteObject(smallFont);
	DeleteObject(bigFont);
}

//CONTROLLER THREAD FOR PLASSER NETWORK CONNECTION
unsigned __stdcall controllerThread(void* args)
{
	gageSystem = GageSystem(true);


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
	HANDLE inertialWorkerHandle = NULL;
	unsigned inertialWorkerId;
	HANDLE plasserSendingHandle = NULL;
	unsigned plasserSendingID;
	HANDLE gageWorkerhandle = NULL;
	unsigned gageWorkerID;
	HANDLE gageTransferHandle = NULL;
	unsigned gageTransferID;
	while (true)
	{
		//CONFIRM THAT SCANNERS ARE STILL CONNECTED
		if (state == 1 || state == 3)
		{
			EM1Command em1Command;
			bool gotPacket = netInterface.getRcvData(em1Command);
			if (gotPacket)
			{
				DataPacket packet;
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
					sendingDataToPlasser = true;

					//GATING PLASSER PULSES
					//niInterface.ni_udp_putDataToSend(0);

					displayInfoMutex.lock();
					dataBreakCount = 0;
					displayInfoMutex.unlock();

					gageSystem.prepareForCapture(gageSystem.globalScanners);
					gageWorkerhandle = (HANDLE)_beginthreadex(NULL, 0, &gageSysRunThread, NULL, NULL, &gageWorkerID);
					inertialSystem = InertialSystem();
					inertialWorkerHandle = (HANDLE)_beginthreadex(NULL, 0, &inertialSysRunThread, NULL, NULL, &inertialWorkerId);
					plasserSendingHandle = (HANDLE)_beginthreadex(NULL, 0, &intertialSysSendingPlasserThread, NULL, NULL, &plasserSendingID);
					gageTransferHandle = (HANDLE)_beginthreadex(NULL, 0, &gageTransferThread, NULL, NULL, &gageTransferID);
					while (inertialSystem.getCurrentInertialPulseGatingStatus() == true)
					{
						Sleep(5);
					}
					Sleep(1000);
					//niInterface.ni_udp_putDataToSend(1);
					netInterface.putSendData(packet);
					break;
				case PREPARESENDASC_CMD:
					packet.sendAckInstead = true;
					sendingDataToPlasser = true;

					//GATING PLASSER PULSES
					//niInterface.ni_udp_putDataToSend(0);

					displayInfoMutex.lock();
					dataBreakCount = 0;
					displayInfoMutex.unlock();

					gageSystem.prepareForCapture(gageSystem.globalScanners);
					gageWorkerhandle = (HANDLE)_beginthreadex(NULL, 0, &gageSysRunThread, NULL, NULL, &gageWorkerID);

					inertialSystem = InertialSystem();
					inertialWorkerHandle = (HANDLE)_beginthreadex(NULL, 0, &inertialSysRunThread, NULL, NULL, &inertialWorkerId);
					plasserSendingHandle = (HANDLE)_beginthreadex(NULL, 0, &intertialSysSendingPlasserThread, NULL, NULL, &plasserSendingID);
					gageTransferHandle = (HANDLE)_beginthreadex(NULL, 0, &gageTransferThread, NULL, NULL, &gageTransferID);
					while (inertialSystem.getCurrentInertialPulseGatingStatus() == true)
					{
						Sleep(5);
					}
					Sleep(1000);
					//niInterface.ni_udp_putDataToSend(1);
					netInterface.putSendData(packet);
					break;
				case PREPARESENDDSC_CMD:
					packet.sendAckInstead = true;
					sendingDataToPlasser = true;

					//GATING PLASSER PULSES
					//niInterface.ni_udp_putDataToSend(0);

					displayInfoMutex.lock();
					dataBreakCount = 0;
					displayInfoMutex.unlock();

					gageSystem.prepareForCapture(gageSystem.globalScanners);
					gageWorkerhandle = (HANDLE)_beginthreadex(NULL, 0, &gageSysRunThread, NULL, NULL, &gageWorkerID);

					inertialSystem = InertialSystem();
					inertialWorkerHandle = (HANDLE)_beginthreadex(NULL, 0, &inertialSysRunThread, NULL, NULL, &inertialWorkerId);
					plasserSendingHandle = (HANDLE)_beginthreadex(NULL, 0, &intertialSysSendingPlasserThread, NULL, NULL, &plasserSendingID);
					gageTransferHandle = (HANDLE)_beginthreadex(NULL, 0, &gageTransferThread, NULL, NULL, &gageTransferID);
					while (inertialSystem.getCurrentInertialPulseGatingStatus() == true)
					{
						Sleep(5);
					}
					Sleep(1000);
					//niInterface.ni_udp_putDataToSend(1);
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
		}
		if (state == 2)
		{
			//SPINUP WORKER THREAD
			sendingDataToPlasser = true;
			globalSystemStatus = 2;

			//CHANGE STATE
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

			//GATING PLASSER PULSES
			//niInterface.ni_udp_putDataToSend(0);

			//SHUTDOWN INERTIAL THREAD
			sendingDataToPlasser = false;
			inertialSystem.shutdownInterialSystem();
			
			WaitForSingleObject(plasserSendingHandle, INFINITE);
			CloseHandle(plasserSendingHandle);
			WaitForSingleObject(inertialWorkerHandle, INFINITE);
			CloseHandle(inertialWorkerHandle);

			gageTransferEnable = false;
			gageSystem.stopGageCapture(gageSystem.globalScanners);

			WaitForSingleObject(gageWorkerhandle, INFINITE);
			CloseHandle(gageWorkerhandle);
	
			WaitForSingleObject(gageTransferHandle, INFINITE);
			CloseHandle(gageTransferHandle);

			state = 1;
			globalSystemStatus = 1;
		}
	}
	return 1;
}

//CRIO LOCOMOTIVE CONTROLLER THREAD
unsigned __stdcall locomotiveControllerThread(void* args)
{
	//FOR TESTING
	NI_SEND_DATA sendDataTmp1;
	sendDataTmp1.FOOT_ENABLE = 0; //DISABLE THE FOOT PULSE OUTPUT
	sendDataTmp1.MEASURE_DIRECTION = 1; //ASSIGN THE MEASURING DIRECTION ... FORWARD/REVERSE
	sendDataTmp1.RELAY_0 = 0;
	sendDataTmp1.RELAY_1 = 0;
	sendDataTmp1.RELAY_2 = 0;
	sendDataTmp1.RELAY_3 = 0;
	sendDataTmp1.RELAY_4 = 0;
	sendDataTmp1.RELAY_5 = 0;
	sendDataTmp1.RELAY_6 = 0;
	sendDataTmp1.RELAY_7 = 0;
	sendDataTmp1.RESET_FT = 0; //RESET FOOT COUNT TO 0
	sendDataTmp1.SCALE_FACTOR = 90; //ASSIGN THE NUMBER OF TACH PULSES PER FOOT
	std::unique_lock<std::mutex> l(tachSimulatorParams_mutex);
	if (tachSimulator_Enable)
	{
		sendDataTmp1.SIM_ENABLE = 1;
		sendDataTmp1.MEASURE_DIRECTION = tachSimulator_Direction;
	}
	else
	{
		sendDataTmp1.SIM_ENABLE = 0; //DISABLE SIMULATE
	}
	sendDataTmp1.SIM_SPEED = tachSimulator_Speed; //ROUGHLY 60 MPH...30uS PER QUARTER PULSE
	l.unlock();
	niInterface.ni_udp_putDataToSend(sendDataTmp1);
	//END TESTING




	gageSystem = GageSystem(true);

	//SETUP THE CRIO TO GATE PULSES
	NI_SEND_DATA sendDataTmp;
	int setup_pulsePerFoot = gageSystem.getPulsePerFoot();
	sendDataTmp.FOOT_ENABLE = 0; //DISABLE THE FOOT PULSE OUTPUT
	sendDataTmp.MEASURE_DIRECTION = 1; //ASSIGN THE MEASURING DIRECTION ... FORWARD/REVERSE
	sendDataTmp.RELAY_0 = 0;
	sendDataTmp.RELAY_1 = 0;
	sendDataTmp.RELAY_2 = 0;
	sendDataTmp.RELAY_3 = 0;
	sendDataTmp.RELAY_4 = 0;
	sendDataTmp.RELAY_5 = 0;
	sendDataTmp.RELAY_6 = 0;
	sendDataTmp.RELAY_7 = 0;
	sendDataTmp.RESET_FT = 1; //RESET FOOT COUNT TO 0
	sendDataTmp.SCALE_FACTOR = setup_pulsePerFoot; //ASSIGN THE NUMBER OF TACH PULSES PER FOOT
	std::unique_lock<std::mutex> l1(tachSimulatorParams_mutex);
	if (tachSimulator_Enable)
	{
		sendDataTmp.SIM_ENABLE = 1;
		sendDataTmp.MEASURE_DIRECTION = tachSimulator_Direction;
	}
	else
	{
		sendDataTmp.SIM_ENABLE = 0; //DISABLE SIMULATE
	}
	sendDataTmp.SIM_SPEED = tachSimulator_Speed; //ROUGHLY 60 MPH...30uS PER QUARTER PULSE
	l1.unlock();
	niInterface.ni_udp_putDataToSend(sendDataTmp);

	//MY PROGRAM STATES ARE:
	//(1) WAITING FOR A START COMMAND -- JUST READING
	//(2) TRANSITION TO COLLECTING/SENDING
	//(3) COLLECTING/SENDING DATA -- WHILE CHECKING FOR READS
	//(4) SHUTTING DOWN
	int state = 1;
	globalSystemStatus = 1;

	long long startCount = 0;
	long long endCount = 0;
	long long resetCountWithTimer = 0;
	high_resolution_clock resetClock;
	auto resetStart = resetClock.now();
	auto resetEnd = resetClock.now();
	bool startResetClock = true;

	high_resolution_clock fifoClock;
	auto start = fifoClock.now();
	auto end = fifoClock.now();
	bool startFifoCount = true;

	long long startResetFootCount = 0;
	long long endResetFootCount = 0;
	HANDLE inertialWorkerHandle = NULL;
	unsigned inertialWorkerId;
	HANDLE gageWorkerhandle = NULL;
	unsigned gageWorkerID;
	HANDLE gageTransferHandle = NULL;
	unsigned gageTransferID;
	while (true)
	{
		//CONFIRM THAT SCANNERS ARE STILL CONNECTED
		if (state == 1)
		{
			NI_RECV_DATA recentReceive = niInterface.getRecentRecvDataNI();
			bool needToEnable = false;
			int setupMeasureDirection = 0;
			NI_SEND_DATA sendDataTmp;
			if (recentReceive.SPEED_CNT > 50 || (std::abs(recentReceive.APPLANIX_SPEED) * 2.237 >= 10.0 && std::abs(recentReceive.APPLANIX_SPEED) * 2.237 < 100)) //CONVERT APPLANIX SPEED TO MPH AND COMPARE TO 10MPH
			{
				needToEnable = true;
				setupMeasureDirection = recentReceive.DIRECTION_OF_TRAVEL;
				int setup_pulsePerFoot = gageSystem.getPulsePerFoot();
				sendDataTmp.FOOT_ENABLE = 0; //DISABLE THE FOOT PULSE OUTPUT
				sendDataTmp.MEASURE_DIRECTION = setupMeasureDirection; //ASSIGN THE MEASURING DIRECTION ... FORWARD/REVERSE
				sendDataTmp.RELAY_0 = 0;
				sendDataTmp.RELAY_1 = 0;
				sendDataTmp.RELAY_2 = 0;
				sendDataTmp.RELAY_3 = 0;
				sendDataTmp.RELAY_4 = 0;
				sendDataTmp.RELAY_5 = 0;
				sendDataTmp.RELAY_6 = 0;
				sendDataTmp.RELAY_7 = 0;
				sendDataTmp.RESET_FT = 1; //RESET FOOT COUNT TO 0
				sendDataTmp.SCALE_FACTOR = setup_pulsePerFoot; //ASSIGN THE NUMBER OF TACH PULSES PER FOOT
				std::unique_lock<std::mutex> l2(tachSimulatorParams_mutex);
				if (tachSimulator_Enable)
				{
					sendDataTmp.SIM_ENABLE = 1;
					sendDataTmp.MEASURE_DIRECTION = tachSimulator_Direction;
				}
				else
				{
					sendDataTmp.SIM_ENABLE = 0; //DISABLE SIMULATE
				}
				sendDataTmp.SIM_SPEED = tachSimulator_Speed; //ROUGHLY 60 MPH...30uS PER QUARTER PULSE
				l2.unlock();
			}

			if (needToEnable)
			{
				//GATING PLASSER PULSES / ASSIGN PULSE PER FOOT / ASSIGN DIRECTION OF TESTING
				niInterface.ni_udp_putDataToSend(sendDataTmp);

				//KICK OF THE STARTUP
				displayInfoMutex.lock();
				dataBreakCount = 0;
				displayInfoMutex.unlock();

				gageSystem.prepareForCapture(gageSystem.globalScanners);
				gageWorkerhandle = (HANDLE)_beginthreadex(NULL, 0, &gageSysRunThread, NULL, NULL, &gageWorkerID);

				inertialSystem = InertialSystem();
				//PACK MEASURE DIRECTION INTO VOID ARGS
				int* directionForArgs = &setupMeasureDirection;
				inertialWorkerHandle = (HANDLE)_beginthreadex(NULL, 0, &inertialSysRunThread, (void *)directionForArgs, NULL, &inertialWorkerId);
				gageTransferHandle = (HANDLE)_beginthreadex(NULL, 0, &gageTransferThread, NULL, NULL, &gageTransferID);
				while (inertialSystem.getCurrentInertialPulseGatingStatus() == true)
				{
					Sleep(5);
				}
				Sleep(20000);//WAIT 10 SECONDS APPROX

				//UPDATE SEVERAL PORTIONS OF THE UDP CONTROL PACKET AND RE-SEND
				sendDataTmp.FOOT_ENABLE = 1; //DISABLE THE FOOT PULSE OUTPUT
				sendDataTmp.RESET_FT = 1; //RESET FOOT COUNT TO 0

				niInterface.ni_udp_putDataToSend(sendDataTmp);
				state = 2;
			}
			else
			{

			}

			if (startResetClock)
			{
				resetStart = resetClock.now();
				startCount = resetCountWithTimer;
				startResetClock = false;
			}
			resetEnd = resetClock.now();
			endCount = resetCountWithTimer;
			if (duration_cast<minutes>(resetEnd - resetStart).count() <= 3)
			{
				if ((endCount - startCount) >= 2)
				{
					//close the program
					SendMessageA(winHandle, WM_DESTROY, 0, 0);
					exit(0);
					
				}
			}
			else
			{
				startResetClock = true;
			}

		}
		if (state == 2)
		{
			//SPINUP WORKER THREAD
			sendingDataToPlasser = true;
			globalSystemStatus = 2;
			
			//CHANGE STATE
			state = 3;
		}
		if (state == 3)
		{
			globalSystemStatus = 3;
			//CHECK THAT EVERYTHING IS STILL OK!

			//CHECK TO MAKE SURE WE ARE USING THE CORRECT MEASURING DIRECTION
			NI_RECV_DATA recentRecv = niInterface.getRecentRecvDataNI();
			NI_SEND_DATA recentSend = niInterface.getRecentSendDataNI();
			if (recentRecv.SCALE_FACTOR != recentSend.SCALE_FACTOR || recentRecv.MEASURING_DIRECTION != recentSend.MEASURE_DIRECTION)
			{
				//RESEND THE SETUP UDP TO NI
			}

			if (recentRecv.DIRECTION_OF_TRAVEL != recentRecv.MEASURING_DIRECTION)
			{
				if (recentRecv.SPEED_CNT > 50 || (std::abs(recentRecv.APPLANIX_SPEED) * 2.237 >= 8.0 && std::abs(recentRecv.APPLANIX_SPEED) * 2.237 < 100))
				{
					//PERFORM A SHUTDOWN AND RESTART BECAUSE WE ARE MEASURING IN THE WRONG DIRECTION
					state = 4;
				}
			}

			displayInfoMutex.lock();
			//Restart if the speed is 0 and the G-I is not equal to 9
			if (recentRecv.SPEED_CNT == 0 && gageInertial_diff != 9 && gageSystemCounter_main != 0 && inertialSystemCounter_main != 0)
			{
				state = 4;
			}
			displayInfoMutex.unlock();

			//Restart if the FIFO percentage increase past 0%
			displayInfoMutex.lock();
			if (EthernetScanner_GetDllFiFoState(gageSystem.globalScanners.scanner_1) > 0 || EthernetScanner_GetDllFiFoState(gageSystem.globalScanners.scanner_2) > 0)
			{
				if (startFifoCount)
				{
					start = fifoClock.now();
					startFifoCount = false;
				}
				end = fifoClock.now();
				if (duration_cast<seconds>(end - start).count() >= 5)
				{
					state = 4;
				}
			}
			else
			{
				startFifoCount = true;
			}
			displayInfoMutex.unlock();

			//Restart if the G-I is less than 0 or greater than 9 for at least 500 ft
			displayInfoMutex.lock();
			if (gageInertial_diff < 0 || gageInertial_diff > 9)
			{
				if (startResetFootCount == 0)
				{
					startResetFootCount = recentRecv.FOOT_COUNT;
				}
				endResetFootCount = recentRecv.FOOT_COUNT;

				if (endResetFootCount - startResetFootCount >= 500)
				{
					state = 4;
				}
			}
			else if (gageInertial_diff >= 0 && gageInertial_diff <= 9)
			{
				startResetFootCount = 0;
			}
			displayInfoMutex.unlock();

			//WHAT ELSE?????
		}
		if (state == 4)
		{
			globalSystemStatus = 4;

			//GATING PLASSER PULSES
			NI_SEND_DATA sendDataTmp;
			int setup_pulsePerFoot = gageSystem.getPulsePerFoot();
			sendDataTmp.FOOT_ENABLE = 0; //DISABLE THE FOOT PULSE OUTPUT
			sendDataTmp.MEASURE_DIRECTION = 1; //ASSIGN THE MEASURING DIRECTION ... FORWARD/REVERSE
			sendDataTmp.RELAY_0 = 0;
			sendDataTmp.RELAY_1 = 0;
			sendDataTmp.RELAY_2 = 0;
			sendDataTmp.RELAY_3 = 0;
			sendDataTmp.RELAY_4 = 0;
			sendDataTmp.RELAY_5 = 0;
			sendDataTmp.RELAY_6 = 0;
			sendDataTmp.RELAY_7 = 0;
			sendDataTmp.RESET_FT = 1; //RESET FOOT COUNT TO 0
			sendDataTmp.SCALE_FACTOR = setup_pulsePerFoot; //ASSIGN THE NUMBER OF TACH PULSES PER FOOT
			std::unique_lock<std::mutex> l3(tachSimulatorParams_mutex);
			if (tachSimulator_Enable)
			{
				sendDataTmp.SIM_ENABLE = 1;
				sendDataTmp.MEASURE_DIRECTION = tachSimulator_Direction;
			}
			else
			{
				sendDataTmp.SIM_ENABLE = 0; //DISABLE SIMULATE
			}
			sendDataTmp.SIM_SPEED = tachSimulator_Speed; //ROUGHLY 60 MPH...30uS PER QUARTER PULSE
			//SEND THE PACKET TO CRIO
			l3.unlock();
			niInterface.ni_udp_putDataToSend(sendDataTmp);

			//SHUTDOWN INERTIAL THREAD
			sendingDataToPlasser = false;
			inertialSystem.shutdownInterialSystem();

			WaitForSingleObject(inertialWorkerHandle, INFINITE);
			CloseHandle(inertialWorkerHandle);

			gageTransferEnable = false;
			gageSystem.stopGageCapture(gageSystem.globalScanners);

			WaitForSingleObject(gageWorkerhandle, INFINITE);
			CloseHandle(gageWorkerhandle);

			WaitForSingleObject(gageTransferHandle, INFINITE);
			CloseHandle(gageTransferHandle);

			state = 1;
			globalSystemStatus = 1;
			startResetFootCount = 0;
			endResetFootCount = 0;
			resetCountWithTimer++;
			
		}
	}
	return 1;
}

//INERTIAL SYSTEM - SENDING TO PLASSER THREAD
unsigned __stdcall intertialSysSendingPlasserThread(void* args)
{
	while (sendingDataToPlasser)
	{
		CoreGeomCalculationPacket gPacket;
		bool gotPacket = inertialSystem.getDataPacketOutputBuffer(gPacket);
		if (gotPacket)
		{
			DataPacket dPacket = netInterface.packPlasserDataWithCGCalc(gPacket, dataBreakCount);
			netInterface.putSendData(dPacket);
			std::unique_lock<std::mutex> l(displayInfoMutex);
			dataBreakCount++;
			l.unlock();
		}
	}
	return 1;
}

//INERTIAL SYSTEM - START/RUN THREAD
unsigned __stdcall inertialSysRunThread(void* args)
{
	//UNPACK THE DIRECTION
	int measureDirectionToSetup = *(int*)args;
	inertialSystem = InertialSystem(true, true, enabledGeomExcUdpSending);
	bool val = inertialSystem.startInertialSystem(measureDirectionToSetup);
	return val;
}

//GAGE SYSTEM - START/RUN THREAD
unsigned __stdcall gageSysRunThread(void* args)
{
	bool val = gageSystem.startGageCapture(gageSystem.globalScanners);
	return val;
}

//NI COMPACT RIO UDP SENDING THREAD
unsigned __stdcall niUDPSendInterfaceThread(void* args)
{
	niInterface.setupNiUdpSend();
	return 0;
}

//NI COMPACT RIO UDP RECEIVING THREAD
unsigned __stdcall niUDPRecvInterfaceThread(void* args)
{
	niInterface.setupNiUdpRecv();
	return 0;
}

//BACK OFFICE MONITOR DATA COLLECT THREAD
unsigned __stdcall backOfficeMonitorCollectThread(void* args)
{
	while (BACK_OFFICE_COLLECTING_THREAD_ENABLE)
	{
		Sleep(10000);
		TGES_BACKOFFICE_PACKET boPacket = TGES_BACKOFFICE_PACKET();
		NI_RECV_DATA tmpNi = niInterface.getRecentRecvDataNI();
		//GET CURRENT TIME
		std::chrono::time_point<std::chrono::system_clock> timePoint = std::chrono::system_clock::now();
		time_t now = time(0);
		struct tm* timeinfo;
		timeinfo = localtime(&now);
		char dt[15];
		strftime(dt, 15, "%G%m%d%H%M%S", timeinfo);
		memcpy(boPacket.dateTime, dt, 15);
		displayInfoMutex.lock();
		boPacket.dataBreakCount = dataBreakCount;
		boPacket.directiontravel = tmpNi.DIRECTION_OF_TRAVEL;
		boPacket.measureDirection = tmpNi.MEASURING_DIRECTION;
		boPacket.gageCount = gageSystemCounter_main;
		boPacket.inertialCount = inertialSystemCounter_main;
		boPacket.g_i_difference = gageInertial_diff;
		strcpy(boPacket.estRID, "------");
		boPacket.estMilepost = 0;
		boPacket.estTrack = 0;
		boPacket.versionNumber = 6;
		boPacket.programState = globalSystemStatus;
		boPacket.dataBreaksAdded = boPacket.dataBreakCount - lastBackOfficePacketSent.dataBreakCount;
		displayInfoMutex.unlock();
		backOfficeInterface.backOffice_udp_putDataToSend(boPacket);
		memcpy(&lastBackOfficePacketSent, &boPacket, sizeof(boPacket));
	}
	return 1;
}

//BACK OFFICE SENDING THREAD
unsigned __stdcall backOfficeSendingThread(void* args)
{
	backOfficeInterface.setupBackOfficeUdpSend();
	return true;
}


//INSERT TEXT TO INFO LINES
void insertText(std::string text)
{
	infoLine4 = infoLine3;
	infoLine3 = infoLine2;
	infoLine2 = infoLine1;
	infoLine1 = text;
}

//GAGE DATA TRANSFER LOOP -- TRANSFERS FROM GAGE SYSTEM TO INERTIAL SYSTEM
unsigned __stdcall gageTransferThread(void* args)
{
	//NEED TO PUT SOME KIND OF ERROR CATCHING HERE -- IN CASE THE GAGE SYSTEM / INERTIAL SYSTEM FAILS
	gageTransferEnable = true;
	while (gageTransferEnable)
	{
		//GET THE GAGE DATA FROM GAGESYSTEM
		
		GageGeometry_Data gageData;
		bool gotData = gageSystem.getGageOutput(gageData);

		if (gotData)
		{
			//OutputDebugString("HIT TRANSFER-GOT");
			//PUT THE GAGE DATA IN INERTIAL SYSTEM
			inertialSystem.putGageInput(gageData);
		}
		else
		{
			//OutputDebugString("HIT TRANSFER-NO REC");
		}
	}
	return 0;
}

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

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable
	//szTitle

	HMENU hMenu = CreateMenu();
	HMENU hSubMenu = CreatePopupMenu();

	AppendMenu(hSubMenu, MF_STRING, (UINT)207, "Simulate Enable");
	AppendMenu(hSubMenu, MF_STRING, (UINT)208, "Simulate Disable");
	AppendMenu(hSubMenu, MF_STRING, (UINT)209, "Simulate Speed 10");
	AppendMenu(hSubMenu, MF_STRING, (UINT)210, "Simulate Speed 30");
	AppendMenu(hSubMenu, MF_STRING, (UINT)211, "Simulate Speed 60");
	AppendMenu(hSubMenu, MF_STRING, (UINT)212, "Simulate Speed 70");
	AppendMenu(hSubMenu, MF_STRING, (UINT)213, "Simulate Direction FWD");
	AppendMenu(hSubMenu, MF_STRING, (UINT)214, "Simulate Direction REV");
	AppendMenu(hSubMenu, MF_STRING, (UINT)216, "Enable Foot Output");
	AppendMenu(hSubMenu, MF_STRING, (UINT)217, "Disable Foot Output");
	AppendMenu(hSubMenu, MF_STRING, (UINT)218, "XX Foot Output");
	AppendMenu(hSubMenu, MF_STRING, (UINT)219, "Reset Foot Count Enable");
	AppendMenu(hSubMenu, MF_STRING, (UINT)220, "Reset Foot Count Disable");
	AppendMenu(hSubMenu, MF_STRING, (UINT)215, "Force Send CRIO Command");

	AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu, "Simulate Tools");


	HWND hWnd = CreateWindowW(szWindowClass, /*szTitle*/L"WTGMS V10: 03/16/2020", WS_OVERLAPPEDWINDOW,
		0, 0, 1250, 680, nullptr, /*nullptr*/hMenu, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	//#######GAGE CALIBRATION#########//
	if (DISPLAY_PROFILE_ENABLE)
	{
		HWND hWnd_Button_1 = CreateWindowW(L"BUTTON", L"CAL GAUGE",
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
			300, 650, 80, 20, hWnd, (HMENU)201, hInstance, nullptr);

		HWND hWnd_Textbox_1 = CreateWindowW(L"EDIT", L"",
			WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL | ES_WANTRETURN,
			300, 680, 80, 20, hWnd, (HMENU)202, hInstance, nullptr);

	}

	//HWND hWnd_MP_Button_UP = CreateWindowW(L"BUTTON", L"MP+",
	//	WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
	//	10, 10, 80, 20, hWnd, (HMENU)203, hInstance, nullptr);

	//HWND hWnd_MP_Button_DOWN = CreateWindowW(L"BUTTON", L"MP-",
	//	WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
	//	100, 10, 80, 20, hWnd, (HMENU)204, hInstance, nullptr);

	//HWND hWnd_MP_Button_Sync = CreateWindowW(L"BUTTON", L"SYNC",
	//	WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
	//	190, 10, 80, 20, hWnd, (HMENU)205, hInstance, nullptr);

 //  HWND hWnd_setupTGC_Button = CreateWindowW(L"BUTTON", L"SETUP",
	//   WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
	//   1000, 10, 80, 20, hWnd, (HMENU)206, hInstance, nullptr);

 //  HWND hwnd_MPSYNC_Textbox = CreateWindowW(L"EDIT", L"",
	//   WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL | ES_WANTRETURN,
	//   280, 10, 80, 20, hWnd, (HMENU)207, hInstance, nullptr);

 //  HWND hwnd_FTSYNC_Textbox = CreateWindowW(L"EDIT", L"",
	//   WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL | ES_WANTRETURN,
	//   370, 10, 80, 20, hWnd, (HMENU)208, hInstance, nullptr);

 //  HWND hwnd_MPSEQ_Textbox = CreateWindowW(L"EDIT", L"",
	//   WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL | ES_WANTRETURN,
	//   1090, 10, 80, 20, hWnd, (HMENU)209, hInstance, nullptr);

 //  HWND hwnd_RID_Textbox = CreateWindowW(L"EDIT", L"",
	//   WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL | ES_WANTRETURN,
	//   1180, 10, 80, 20, hWnd, (HMENU)210, hInstance, nullptr);

 //  HWND hwnd_TRACK_Textbox = CreateWindowW(L"EDIT", L"",
	//   WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL | ES_WANTRETURN,
	//   1270, 10, 80, 20, hWnd, (HMENU)211, hInstance, nullptr);



   //SETUP 1Hz TIMER
   UINT timer1 = 1;
   SetTimer(hWnd, timer1, 1000, (TIMERPROC)NULL);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

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
					//gaugeOffset = gaugeReading - displayPoints1.halfGageReading - displayPoints2.halfGageReading;	
					double newOffset;
					newOffset = gageSystem.updateGageOffset(gaugeReading);
					MessageBox(NULL, std::to_string(newOffset).c_str(), "NEW GAUGE OFFSET", MB_OK);
					break;

				case 203:
					//INCREMENT MP UP
					//currentTgcLocation.incrementMP();
					break;

				case 204:
					//DECREMENT MP DOWN
					//currentTgcLocation.decrementMP();
					break;

				case 205:
					//SYNC MP
					wchar_t text2[20];
					GetWindowTextW(GetDlgItem(hWnd, 207), text2, 20);
					int mp;
					wchar_t text3[30];
					GetWindowTextW(GetDlgItem(hWnd, 208), text3, 20);
					int ft;
					ft = std::stoi(text3);
					mp = std::stoi(text2);
					//currentTgcLocation.updateMP(mp, ft);

					break;

				case 206:
					//UPDATE TGC INFO
					//MP_LOCATION aLoc = currentTgcLocation.getLocation();
					wchar_t text_mpSeq[20];
					GetWindowTextW(GetDlgItem(hWnd, 209), text_mpSeq, 20);

					wchar_t text_rid[6];
					GetWindowTextW(GetDlgItem(hWnd, 210), text_rid, 6);

					wchar_t text_track[20];
					GetWindowTextW(GetDlgItem(hWnd, 211), text_track, 20);
					int track;
					track = std::stoi(text_track);

					char rid_1[6];
					for (int i = 0; i < 6; i++)
					{
						rid_1[i] = text_rid[i];
					}

					//currentTgcLocation.updateAll(aLoc.MP, aLoc.FTCnt, text_mpSeq[0], 'R', track, rid_1);

					break;

				case 207:
					//SIMULATE ENABLE
					tachSimulatorParams_mutex.lock();
					tachSimulator_Enable = true;
					tachSimulatorParams_mutex.unlock();
					break;
				case 208:
					tachSimulatorParams_mutex.lock();
					//SIMULATE DISABLE
					tachSimulator_Enable = false;
					tachSimulatorParams_mutex.unlock();
					break;
				case 209:
					tachSimulatorParams_mutex.lock();
					//SIMULATE SPEED = 10MPH
					//720uS per quarter pulse = 10 MPH
					tachSimulator_Speed = 720;
					tachSimulatorParams_mutex.unlock();
					break;

				case 210:
					tachSimulatorParams_mutex.lock();
					//SIMULATE SPEED = 30MPH
					//60uS per quarter pulse = 30 MPH
					tachSimulator_Speed = 60;
					tachSimulatorParams_mutex.unlock();
					break;
				case 211:
					tachSimulatorParams_mutex.lock();
					//SIMULATE SPEED = 60MPH
					//30uS per quarter pulse = 60 MPH
					tachSimulator_Speed = 30;
					tachSimulatorParams_mutex.unlock();
					break;
				case 212:
					tachSimulatorParams_mutex.lock();
					//SIMULATE SPEED = 70MPH
					//26uS per quarter pulse = 70 MPH
					tachSimulator_Speed = 26;
					tachSimulatorParams_mutex.unlock();
					break;

				case 213:
					tachSimulatorParams_mutex.lock();
					//SET SIMULATE TO FORWARD DIRECTION
					tachSimulator_Direction = 1;
					tachSimulatorParams_mutex.unlock();
					break;
				case 214:
					tachSimulatorParams_mutex.lock();
					//SET SIMULATE TO REVERSE DIRECTION
					tachSimulator_Direction = -1;
					tachSimulatorParams_mutex.unlock();
					break;

				case 215:
					//SEND A CRIO COMMAND PACKET
					tachSimulatorParams_mutex.lock();
					NI_RECV_DATA lastRecvPacket;
					lastRecvPacket = niInterface.getRecentRecvDataNI();
					NI_SEND_DATA aPacket;
					if (tachSimulator_SendFoot == -1)
					{
						aPacket.FOOT_ENABLE = lastRecvPacket.OUT_FOOT_ENABLE;
					}
					else
					{
						aPacket.FOOT_ENABLE = tachSimulator_SendFoot;
					}
					aPacket.MEASURE_DIRECTION = tachSimulator_Direction;
					aPacket.RELAY_0 = 0;
					aPacket.RELAY_1 = 0;
					aPacket.RELAY_2 = 0;
					aPacket.RELAY_3 = 0;
					aPacket.RELAY_4 = 0;
					aPacket.RELAY_5 = 0;
					aPacket.RELAY_6 = 0;
					aPacket.RELAY_7 = 0;
					aPacket.RESET_FT = tachSimulator_resetFtCount;
					aPacket.SCALE_FACTOR = lastRecvPacket.SCALE_FACTOR;
					aPacket.SIM_ENABLE = tachSimulator_Enable;
					aPacket.SIM_SPEED = tachSimulator_Speed;
					niInterface.ni_udp_putDataToSend(aPacket);
					tachSimulatorParams_mutex.unlock();
					break;

				case 216:
					//ENABLE FOOT OUTPUT
					tachSimulatorParams_mutex.lock();
					tachSimulator_SendFoot = 1;
					tachSimulatorParams_mutex.unlock();
					break;
				case 217:
					//DISABLE FOOT OUTPUT
					tachSimulatorParams_mutex.lock();
					tachSimulator_SendFoot = 0;
					tachSimulatorParams_mutex.unlock();
					break;
				case 218:
					//FOOT OUTPUT STATE - DONT CARE
					tachSimulatorParams_mutex.lock();
					tachSimulator_SendFoot = -1;
					tachSimulatorParams_mutex.unlock();
					break;
				case 219:
					//RESET FOOT COUNT - ENABLE
					tachSimulatorParams_mutex.lock();
					tachSimulator_resetFtCount = 1;
					tachSimulatorParams_mutex.unlock();
					break;
				case 220:
					//RESET FOOT COUNT - DISABLE
					tachSimulatorParams_mutex.lock();
					tachSimulator_resetFtCount = 0;
					tachSimulatorParams_mutex.unlock();
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
			long inertialSystemCount = 0;
			long gageSystemCount1 = 0;
			long gageSystemCount2 = 0;
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

			//DISPLAY PLASSER STATUS
			displaySystemStatus(hdcMem);

			if (DISPLAY_PROFILE_ENABLE)
			{
				//displaySystemStatus(hdcMem);
				gageSystem.displayProfilePointsCombined(hdcMem, 1, screenPaintOnce, gageSystemCount1);
				gageSystem.displayProfilePointsCombined(hdcMem, 2, screenPaintOnce, gageSystemCount2);
			}

			if (DISPLAY_INERTIAL_ENABLE)
			{
				inertialSystem.displayGeometryCombined(hdcMem, inertialSystemCount);
			}
			//DISPLAY DELTA GAGE/INERTIAL COUNT
			
			displayInfoMutex.lock();
			long deltaCount = gageSystemCount1 - inertialSystemCount;
			gageSystemCounter_main = gageSystemCount1;
			inertialSystemCounter_main = inertialSystemCount;
			gageInertial_diff = deltaCount;
			displayInfoMutex.unlock();
			char deltaCountChars[50];
			sprintf(deltaCountChars, "G-I: %d", deltaCount);
			TextOut(hdcMem, 410, 540, deltaCountChars, strlen(deltaCountChars));
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


