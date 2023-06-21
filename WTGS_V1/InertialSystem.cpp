#include "stdafx.h"
#include "InertialSystem.h"

//#pragma optimize("", off)

//PROTOTYPES
unsigned __stdcall geoLoggingWriteThread(void* args);
unsigned __stdcall geom2ndCalcProcessorThread(void* args);
unsigned __stdcall geom2ndCalcGetThread(void* args);
unsigned __stdcall geom2ndCalcPutThread(void* args);
unsigned __stdcall geomCoreCalcManagerThread(void* args);
unsigned __stdcall appNetworkPacketParsingController_Group5(void* args);
unsigned __stdcall appNetworkInterfaceController(void* args);
bool			   putDataPacketOutputBuffer(CoreGeomCalculationPacket packet);
unsigned __stdcall udp1CurrentLocationUpdateThread(void* args);
unsigned __stdcall udp1RecieverThread(void* args);
unsigned __stdcall geomCoreGageMatchingThread(void* args);
unsigned __stdcall exceptionSavingTransmittingThread(void* args);
void			   loadInertialIniFile();
bool			   updateInertialPulseGatingStatus(bool newStatus);
unsigned __stdcall udpGeometrySendingThread(void* args);
unsigned __stdcall udpExceptionSendingThread(void* args);

//GLOBALS
//APPLANIX STUFF
ApplanixConnectionClient appConCli;
CoreGeometry_A coreGeom;
GeoCalculations calcs2Geom;
GeometryLogging geoLogger;
Calculation_Simulation sim1;
PlasserUDP udpRecv;
GeometryUDPFeed outDataUDPFeed;

//CURRENT MILEPOST LOCATION STUFF
TGC_Location currentTgcLocation;

//DIRECTION OF TRAVEL FROM THE CRIO STARTUP
int setupMeasureDirection = 0;

//GEO CALCULATIONS FOR DISPLAY
CoreGeomCalculationPacket dispGeo[5280];
std::mutex dispGeo_mutex;
std::condition_variable dispGeo_condVar;
int displayGeoCnt = 0;

//QUEUE OF DATAPACKETS FOR SENDING TO EM1
std::queue<CoreGeomCalculationPacket> dataPacket_outputBuffer;
std::mutex dataPacket_outputBuffer_mutex;
std::condition_variable dataPacket_outputBuffer_condVar;


//END APPLANIX STUFF

//FOOT PULSE GATING
std::mutex inertialPulseGatingMutex;
bool inertialPulseGatingEnabled = true;

//COUNTER FOR APPLANIX MARKS
long applanixMarkerCount = 0;
std::mutex inertialInfoMutex;

//INFINITE LOOP FLAGS//

bool rawApplanixLoggingEnable = false;
bool inertialSystemEnabled = false;
bool geometryLoggingEnable = true;
bool plasserUdpUpdateEnable = true;
char vehicleID[10];
bool udpGeomExcDataOutputEnable = false;

//CONSTRUCTORS
InertialSystem::InertialSystem()
{
	updateInertialPulseGatingStatus(true);
}
InertialSystem::InertialSystem(bool geometryLogging, bool rawApplanixLogging, bool udpGeomDataOutput)
{
	udpGeomExcDataOutputEnable = udpGeomDataOutput;

	updateInertialPulseGatingStatus(true);


	std::unique_lock<std::mutex> l(inertialInfoMutex);
	applanixMarkerCount = 0;
	l.unlock();

	int buffSize = dataPacket_outputBuffer.size();
	for (int i = 0; i < buffSize; i++)
	{
		dataPacket_outputBuffer.pop();
	}
	displayGeoCnt = 0;

	geometryLoggingEnable = geometryLogging;
	rawApplanixLoggingEnable = rawApplanixLogging;

	//LOAD THE INI FILE
	loadInertialIniFile();

	//RUN CALCULATION SIMULATOR
	sim1 = Calculation_Simulation();//CAN I REMOVE THIS?????
	currentTgcLocation = TGC_Location();
	appConCli = ApplanixConnectionClient();
	coreGeom = CoreGeometry_A();
	coreGeom.setVehicleID(vehicleID);
	calcs2Geom = GeoCalculations();
	geoLogger = GeometryLogging(vehicleID);
	outDataUDPFeed = GeometryUDPFeed();
	if (geometryLoggingEnable)
	{
		geoLogger.openFile();
	}
}
InertialSystem::~InertialSystem()
{
}

//GEOMETRY LOGGING THREADS -- PUTS ITEMS ON LOGGING QUE
unsigned __stdcall geoLoggingWriteThread(void* args)
{
	geoLogger.geoLoggingProcessor();
	return 0;
}

//NON-CORE GEOMETRY CALCULATIONS - GEOCALCULATIONS.CPP CONTROLLER LOOP -- PERFORMS THE CALCULATIONS TO COMPLETE RAW PACKETS
unsigned __stdcall geom2ndCalcProcessorThread(void* args)
{
	calcs2Geom.calculationProcessor();

	return 0;
}

//NON-CORE GEOMETRY CALCULATIONS - GEOCALCULATIONS.CPP //REMOVES GEOCALCULATION DATA FROM QUE -- PLACES INTO DISPLAY AND ANALYSIS BUFFERS
unsigned __stdcall geom2ndCalcGetThread(void* args)
{
	while (inertialSystemEnabled)
	{
		CoreGeomCalculationPacket tmpGCP;
		bool gotPacket = calcs2Geom.getCompleteCalcPacket_outputBuffer(tmpGCP);

		if (gotPacket)
		{
			//PLACE PACKET INTO WRITE BUFFER QUEUE
			if (geometryLoggingEnable)
			{
				geoLogger.putGeoLoggingQue(tmpGCP);
			}

			//PLACE PACKET INTO UDP OUTPUT BUFFER QUEUE -- IF SEND IS ENABLED
			if (udpGeomExcDataOutputEnable)
			{
				outDataUDPFeed.udpGeom_putDataToSend(tmpGCP);
			}


			//PLACE THE PACKET INTO DISPLAY PACKET
			//##SHIFT THE DISPLAY BUFFER
			dispGeo_mutex.lock();
			memmove(&dispGeo[1], &dispGeo[0], (sizeof CoreGeomCalculationPacket()) * 5279);

			//WRITE IN THE NEW DATA
			dispGeo[0] = tmpGCP;

			if (displayGeoCnt < 5280)
			{
				displayGeoCnt++;
			}
			dispGeo_mutex.unlock();

			dispGeo_condVar.notify_one();

			if (ENABLE_PLASSER_SEND)
			{
				//PLACE PACKET INTO PLASSER DATAPACKET OUTPUT IF NEEDED
				putDataPacketOutputBuffer(tmpGCP);
			}
		}
	}
	return 0;
}

//NON-CORE GEOMETRY CALCUATIONS - GEOCALCULATIONS.CPP // PLACES CORE GEOMETRY DATA INTO GEOCALCULATIONS QUE
unsigned __stdcall  geom2ndCalcPutThread(void* args)
{
	while (inertialSystemEnabled)
	{
		bool gotPacket = false;
		CoreGeomCalculationPacket tmpGCP;

		gotPacket = coreGeom.getCalcPacket_A(tmpGCP);
		if (gotPacket)
		{
			tmpGCP.measuringDirection = setupMeasureDirection;
			bool placedPacket = calcs2Geom.putRawCalcPacket_512Buffer(tmpGCP);
		}
	}
	return 0;
}

//CORE GEOMETRY - CORE BUFFER MANAGER THREAD -- ALIGNS THE DATA
unsigned __stdcall geomCoreCalcManagerThread(void* args)
{
	//int measureDirectionToSetup = *(int*)args;
	coreGeom.controllerThread_CoreCalcs_A(setupMeasureDirection);
	return 1;
}

//CORE GEOMETRY - GAGE MATCHING THREAD - MATCHES GAGE DATA TO CORE GEOM PACKET
unsigned __stdcall geomCoreGageMatchingThread(void* args)
{
	bool val = coreGeom.gageMatchingThread();
	return val;
}

//APPLANIX NETWORK PACKET MANAGER - GROUP 5
unsigned __stdcall appNetworkPacketParsingController_Group5(void* args)
{
	
	while (inertialSystemEnabled)
	{
		bool gotAPG5 = appConCli.getRcvData_Applanix_Group_5(gotAPG5, coreGeom, currentTgcLocation);
		if (gotAPG5)
		{
			std::unique_lock<std::mutex> l(inertialInfoMutex);
			applanixMarkerCount++;
			l.unlock();
		}
	}
	return true;
}

//START APPLANIX NETWORK INTERFACE CONTROLLER
unsigned __stdcall appNetworkInterfaceController(void* args)
{
	appConCli.enableApplanixGlobalGating();
	appConCli.connectApplanix(rawApplanixLoggingEnable);
	return 1;
}

//GEOMETRY UDP SENDING THREAD
unsigned __stdcall udpGeometrySendingThread(void* args)
{
	return 0;
}

//EXCEPTION UDP SENDING THREAD
unsigned __stdcall udpExceptionSendingThread(void* args)
{
	return 0;
}

//DISPLAY GEOMETRY GRAPH - ONLY GEOM VIEW
void InertialSystem::displayGeometry(HDC &hdc)
{
	//PENS
	HPEN purplePen = CreatePen(PS_SOLID, 3, RGB(255, 0, 255));
	HPEN redPen = CreatePen(PS_SOLID, 3, RGB(255, 0, 0));
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

	SelectObject(hdc, smallFont);
	TextOut(hdc, 280, 0, "MP", sizeof("MP"));
	TextOut(hdc, 370, 0, "FT", sizeof("FT"));
	TextOut(hdc, 1090, 0, "MP SEQ", sizeof("MPSEQ"));
	TextOut(hdc, 1180, 0, "RID", sizeof("RID"));
	TextOut(hdc, 1270, 0, "TRACK", sizeof("TRACK"));


	SelectObject(hdc, bigFont);

	//DRAW RECTANGLES FOR MEASUREMENT BOXES
	SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
	SelectObject(hdc, redPen);

	double horizInc = 1260.0 / 5280.0;
	double x = 0;
	double alignmentYOffset = 85;
	double profileYOffset = 255;
	double curvatureYOffset = 425;
	double xlYOffset = 595;
	double gradeYOffset = 765;


	//ALIGNMENT
	Rectangle(hdc, 0, 0, 1260, 170);
	//PROFILE
	Rectangle(hdc, 0, 170, 1260, 340);
	//CURVATURE
	Rectangle(hdc, 0, 340, 1260, 510);
	//CROSSLEVEL
	Rectangle(hdc, 0, 510, 1260, 680);
	//GRADE
	Rectangle(hdc, 0, 680, 1260, 850);

	double alignmentScale = 85 / 5;
	double profileScale = 85 / 5;
	double curvatureScale = 85 / 8;
	double xlScale = 85 / 5;
	double gradeScale = 85 / 3;

	//DRAW "zero" and scale lines
	SelectObject(hdc, blackBrush);
	SelectObject(hdc, GetStockObject(BLACK_PEN));
	for (double i = -4; i <= 4; i = i + 1)
	{
		if (i == 0)
		{
			SelectObject(hdc, purplePen);
			Rectangle(hdc, 0, alignmentYOffset + i * alignmentScale, 1260, alignmentYOffset + i * alignmentScale + 1);
			Rectangle(hdc, 0, profileYOffset + i * profileScale, 1260, profileYOffset + i * profileScale + 1);
			Rectangle(hdc, 0, xlYOffset + i * xlScale, 1260, xlYOffset + i * xlScale + 1);
		}
		else
		{
			SelectObject(hdc, GetStockObject(BLACK_PEN));
			Rectangle(hdc, 0, alignmentYOffset + i * alignmentScale, 1260, alignmentYOffset + i * alignmentScale + 1);
			Rectangle(hdc, 0, profileYOffset + i * profileScale, 1260, profileYOffset + i * profileScale + 1);
			Rectangle(hdc, 0, xlYOffset + i * xlScale, 1260, xlYOffset + i * xlScale + 1);
			SelectObject(hdc, smallFont);
			char scalechars[10];
			sprintf(scalechars, "%.1f", i*-1.0);
			TextOut(hdc, 1264, alignmentYOffset + i * alignmentScale - 5, scalechars, strlen(scalechars));
			TextOut(hdc, 1264, profileYOffset + i * profileScale - 5, scalechars, strlen(scalechars));
			TextOut(hdc, 1264, xlYOffset + i * xlScale - 5, scalechars, strlen(scalechars));
		}
	}

	for (double i = -6; i <= 6; i = i + 2)
	{
		if (i == 0)
		{
			SelectObject(hdc, purplePen);
			Rectangle(hdc, 0, curvatureYOffset + i * curvatureScale, 1260, curvatureYOffset + i * curvatureScale + 1);
		}
		else
		{
			SelectObject(hdc, GetStockObject(BLACK_PEN));
			Rectangle(hdc, 0, curvatureYOffset + i * curvatureScale, 1260, curvatureYOffset + i * curvatureScale + 1);

			SelectObject(hdc, smallFont);
			char scalechars2[10];
			sprintf(scalechars2, "%.1f", i*-1.0);
			TextOut(hdc, 1264, curvatureYOffset + i * curvatureScale - 5, scalechars2, strlen(scalechars2));
		}
	}

	for (double i = -2.5; i <= 2.5; i = i + 0.5)
	{
		if (i == 0)
		{
			SelectObject(hdc, purplePen);
			Rectangle(hdc, 0, gradeYOffset + i * gradeScale, 1260, gradeYOffset + i * gradeScale + 1);
		}
		else
		{
			SelectObject(hdc, GetStockObject(BLACK_PEN));
			Rectangle(hdc, 0, gradeYOffset + i * gradeScale, 1260, gradeYOffset + i * gradeScale + 1);

			SelectObject(hdc, smallFont);
			char scalechars3[10];
			sprintf(scalechars3, "%.1f", i*-1.0);
			TextOut(hdc, 1264, gradeYOffset + i * gradeScale - 5, scalechars3, strlen(scalechars3));
		}
	}

	Rectangle(hdc, 0, curvatureYOffset, 1260, curvatureYOffset + 1);
	Rectangle(hdc, 0, gradeYOffset, 1260, gradeYOffset + 1);



	dispGeo_mutex.lock();

	CoreGeomCalculationPacket tmpGeo[5280];
	memcpy(tmpGeo, dispGeo, sizeof(dispGeo));
	int tmpGeoCnt = displayGeoCnt;

	dispGeo_mutex.unlock();


	//## PLOT ALIGNMENT ##//
	//--PLOT LEFT RAIL--//
	SelectObject(hdc, bigFont);

	char locationChars[200];
	MP_LOCATION tmpLoc = currentTgcLocation.getLocation();
	sprintf(locationChars, "MP: %d , %d  RID: %s TRACK: %d SEQ: %c ORIEN: %c",
		tmpLoc.MP, tmpLoc.FTCnt, tmpLoc.RID, tmpLoc.trackNumber,
		tmpLoc.mpSequence, tmpLoc.carOrientation);
	TextOut(hdc, 600, 0, locationChars, strlen(locationChars));

	////DISPLAY THE TEXT
	//if (tmpGeoCnt > 0)
	//{
	//	SelectObject(hdc, bigFont);

	//	char alignChars[20];
	//	sprintf(alignChars, "ALIGN: %6.3f", tmpGeo[0].L_HORIZ_MCO_62_raw*-1.0);
	//	TextOut(hdc, 1300, alignmentYOffset, alignChars, strlen(alignChars));

	//	char profileChars[20];
	//	sprintf(profileChars, "PROF: %6.3f", tmpGeo[0].L_VERT_MCO_62_raw*-1.0);
	//	TextOut(hdc, 1300, profileYOffset, profileChars, strlen(profileChars));

	//	char curvChars[20];
	//	sprintf(curvChars, "CURV: %6.3f", tmpGeo[0].filteredCurvature*-1.0);
	//	TextOut(hdc, 1300, curvatureYOffset, curvChars, strlen(curvChars));

	//	char XLChars[20];
	//	sprintf(XLChars, "XL: %6.3f", tmpGeo[0].crosslevel_raw*-1.0);
	//	TextOut(hdc, 1300, xlYOffset, XLChars, strlen(XLChars));

	//	char gradeChars[20];
	//	sprintf(gradeChars, "GRADE: %6.3f", tmpGeo[0].grade_raw*-1.0);
	//	TextOut(hdc, 1300, gradeYOffset, gradeChars, strlen(gradeChars));
	//}



	SelectObject(hdc, purplePen);

	x = 0;

	SelectObject(hdc, blackBrush);
	SelectObject(hdc, GetStockObject(BLACK_PEN));
	x = 0;

	for (int i = 0; i < tmpGeoCnt; i++)//PLOT 1 MILE OF DATA
	{
		Ellipse(hdc, x - 1, alignmentYOffset + alignmentScale * tmpGeo[i].L_HORIZ_MCO_62_OC*-1.0 - 1, x + 1, alignmentYOffset + alignmentScale * tmpGeo[i].L_HORIZ_MCO_62_OC*-1.0 + 1);
		x += horizInc;
	}

	//--PLOT RIGHT RAIL--//
	SelectObject(hdc, blueBrush);
	SelectObject(hdc, bluePen);
	x = 0;
	for (int i = 0; i < tmpGeoCnt; i++)//PLOT 1 MILE OF DATA
	{
		Ellipse(hdc, x - 1, alignmentYOffset + alignmentScale * tmpGeo[i].R_HORIZ_MCO_62_OC*-1.0 - 1, x + 1, alignmentYOffset + alignmentScale * tmpGeo[i].R_HORIZ_MCO_62_OC*-1.0 + 1);
		x += horizInc;
	}


	//## PLOT PROFILE ##//

	//--PLOT LEFT RAIL--//
	SelectObject(hdc, blackBrush);
	SelectObject(hdc, GetStockObject(BLACK_PEN));

	x = 0;
	for (int i = 0; i < tmpGeoCnt; i++)//PLOT 1 MILE OF DATA
	{
		Ellipse(hdc, x - 1, profileYOffset + profileScale * tmpGeo[i].L_VERT_MCO_62_OC*-1.0 - 1, x + 1, profileYOffset + profileScale * tmpGeo[i].L_VERT_MCO_62_OC*-1.0 + 1);
		x += horizInc;
	}

	//--PLOT RIGHT RAIL--//
	SelectObject(hdc, blueBrush);
	SelectObject(hdc, bluePen);
	x = 0;
	for (int i = 0; i < tmpGeoCnt; i++)//PLOT 1 MILE OF DATA
	{
		Ellipse(hdc, x - 1, profileYOffset + profileScale * tmpGeo[i].R_VERT_MCO_62_OC*-1.0 - 1, x + 1, profileYOffset + profileScale * tmpGeo[i].R_VERT_MCO_62_OC*-1.0 + 1);
		x += horizInc;
	}

	//## PLOT CURVATURE ##//
	x = 0;

	SelectObject(hdc, blackBrush);
	SelectObject(hdc, GetStockObject(BLACK_PEN));


	for (int i = 0; i < tmpGeoCnt; i++)//PLOT 1 MILE OF DATA
	{
		Ellipse(hdc, x - 1, curvatureYOffset + curvatureScale * tmpGeo[i].filteredCurvature*-1.0 - 1, x + 1, curvatureYOffset + curvatureScale * tmpGeo[i].filteredCurvature*-1.0 + 1);
		x += horizInc;
	}

	//## PLOT CROSSLEVEL ##//
	SelectObject(hdc, blackBrush);
	SelectObject(hdc, GetStockObject(BLACK_PEN));

	x = 0;
	for (int i = 0; i < tmpGeoCnt; i++)//PLOT 1 MILE OF DATA
	{
		Ellipse(hdc, x - 1, xlYOffset + xlScale * tmpGeo[i].crosslevel_OC*-1.0 - 1, x + 1, xlYOffset + xlScale * tmpGeo[i].crosslevel_OC*-1.0 + 1);
		x += horizInc;
	}

	//## PLOT GRADE ##//
	SelectObject(hdc, blackBrush);
	SelectObject(hdc, GetStockObject(BLACK_PEN));

	x = 0;
	for (int i = 0; i < tmpGeoCnt; i++)//PLOT 1 MILE OF DATA
	{
		Ellipse(hdc, x - 1, gradeYOffset + gradeScale * tmpGeo[i].grade_raw*-1.0 - 1, x + 1, gradeYOffset + gradeScale * tmpGeo[i].grade_raw*-1.0 + 1);
		x += horizInc;
	}

	//dispGeo_mutex.unlock();


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

//DISPLAY GEOMETRY GRAPH -- COMBINED GAGE/GEOM VIEW
void InertialSystem::displayGeometryCombined(HDC &hdc, long &inertialCnt)
{
	//PENS
	HPEN purplePen = CreatePen(PS_SOLID, 3, RGB(255, 0, 255));
	HPEN redPen = CreatePen(PS_SOLID, 3, RGB(255, 0, 0));
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

	//DRAW RECTANGLES FOR MEASUREMENT BOXES
	SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
	SelectObject(hdc, redPen);

	double horizInc = 550.0 / 2640.0;
	double x = 0;
	double alignmentYOffset = 50;
	double profileYOffset = 150;
	double curvatureYOffset = 250;
	double xlYOffset = 350;
	double gageYOffset = 450;


	//ALIGNMENT
	Rectangle(hdc, 0, 0, 550, 100);
	//PROFILE
	Rectangle(hdc, 0, 100, 550, 200);
	//CURVATURE
	Rectangle(hdc, 0, 200, 550, 300);
	//CROSSLEVEL
	Rectangle(hdc, 0, 300, 550, 400);
	//GRADE
	Rectangle(hdc, 0, 400, 550, 500);

	double alignmentScale = 50 / 5;
	double profileScale = 50 / 5;
	double curvatureScale = 50 / 8;
	double xlScale = 50 / 5;
	double gageScale = 50 / 2;

	//DRAW "zero" and scale lines
	SelectObject(hdc, blackBrush);
	SelectObject(hdc, GetStockObject(BLACK_PEN));
	for (double i = -4; i <= 4; i = i + 1)
	{
		if (i == 0)
		{
			SelectObject(hdc, purplePen);
			Rectangle(hdc, 0, alignmentYOffset + i * alignmentScale, 550, alignmentYOffset + i * alignmentScale + 1);
			Rectangle(hdc, 0, profileYOffset + i * profileScale, 550, profileYOffset + i * profileScale + 1);
			Rectangle(hdc, 0, xlYOffset + i * xlScale, 550, xlYOffset + i * xlScale + 1);
		}
		else
		{
			SelectObject(hdc, GetStockObject(BLACK_PEN));
			Rectangle(hdc, 0, alignmentYOffset + i * alignmentScale, 550, alignmentYOffset + i * alignmentScale + 1);
			Rectangle(hdc, 0, profileYOffset + i * profileScale, 550, profileYOffset + i * profileScale + 1);
			Rectangle(hdc, 0, xlYOffset + i * xlScale, 550, xlYOffset + i * xlScale + 1);
			SelectObject(hdc, smallFont);
			char scalechars[10];
			sprintf(scalechars, "%.1f", i*-1.0);
			TextOut(hdc, 555, alignmentYOffset + i * alignmentScale - 5, scalechars, strlen(scalechars));
			TextOut(hdc, 555, profileYOffset + i * profileScale - 5, scalechars, strlen(scalechars));
			TextOut(hdc, 555, xlYOffset + i * xlScale - 5, scalechars, strlen(scalechars));
		}
	}

	for (double i = -6; i <= 6; i = i + 2)
	{
		if (i == 0)
		{
			SelectObject(hdc, purplePen);
			Rectangle(hdc, 0, curvatureYOffset + i * curvatureScale, 555, curvatureYOffset + i * curvatureScale + 1);
		}
		else
		{
			SelectObject(hdc, GetStockObject(BLACK_PEN));
			Rectangle(hdc, 0, curvatureYOffset + i * curvatureScale, 555, curvatureYOffset + i * curvatureScale + 1);

			SelectObject(hdc, smallFont);
			char scalechars2[10];
			sprintf(scalechars2, "%.1f", i*-1.0);
			TextOut(hdc, 555, curvatureYOffset + i * curvatureScale - 5, scalechars2, strlen(scalechars2));
		}
	}

	for (double i = -1.5; i <= 1.5; i = i + 0.5)
	{
		if (i == 0)
		{
			SelectObject(hdc, purplePen);
			Rectangle(hdc, 0, gageYOffset + i * gageScale, 555, gageYOffset + i * gageScale + 1);
		}
		else
		{
			SelectObject(hdc, GetStockObject(BLACK_PEN));
			Rectangle(hdc, 0, gageYOffset + i * gageScale, 555, gageYOffset + i * gageScale + 1);

			SelectObject(hdc, smallFont);
			char scalechars3[10];
			sprintf(scalechars3, "%.1f", i*-1.0);
			TextOut(hdc, 555, gageYOffset + i * gageScale - 5, scalechars3, strlen(scalechars3));
		}
	}

	Rectangle(hdc, 0, curvatureYOffset, 550, curvatureYOffset + 1);
	Rectangle(hdc, 0, gageYOffset, 550, gageYOffset + 1);



	dispGeo_mutex.lock();

	CoreGeomCalculationPacket tmpGeo[5280];
	memcpy(tmpGeo, dispGeo, sizeof(dispGeo));
	int tmpGeoCnt = displayGeoCnt;

	dispGeo_mutex.unlock();

	//## PLOT ALIGNMENT ##//
//--PLOT LEFT RAIL--//
	SelectObject(hdc, bigFont);

	char locationChars[200];
	MP_LOCATION tmpLoc = currentTgcLocation.getLocation();
	sprintf(locationChars, "MP: %d , %d  RID: %s TRACK: %d SEQ: %c ORIEN: %c",
		tmpLoc.MP, tmpLoc.FTCnt, tmpLoc.RID, tmpLoc.trackNumber,
		tmpLoc.mpSequence, tmpLoc.carOrientation);
	TextOut(hdc, 10, 510, locationChars, strlen(locationChars));
	std::unique_lock<std::mutex> l(inertialInfoMutex);
	char markCountChars[50];
	sprintf(markCountChars, "MARK CNT: %d", applanixMarkerCount);
	inertialCnt = applanixMarkerCount;
	l.unlock();
	TextOut(hdc, 450, 510, markCountChars, strlen(markCountChars));

	//DISPLAY THE TEXT
	if (tmpGeoCnt > 0)
	{
		SelectObject(hdc, bigFont);

		char alignChars[20];
		sprintf(alignChars, "ALIGN: %6.3f", tmpGeo[0].L_HORIZ_MCO_62_OC * -1.0);
		TextOut(hdc, 400, alignmentYOffset - 30, alignChars, strlen(alignChars));

		char profileChars[20];
		sprintf(profileChars, "PROF: %6.3f", tmpGeo[0].L_VERT_MCO_62_OC * -1.0);
		TextOut(hdc, 400, profileYOffset - 30, profileChars, strlen(profileChars));

		char curvChars[20];
		sprintf(curvChars, "CURV: %6.3f", tmpGeo[0].filteredCurvature * -1.0);
		TextOut(hdc, 400, curvatureYOffset - 30, curvChars, strlen(curvChars));

		char XLCharsNOC[20];
		sprintf(XLCharsNOC, "XL NOC: %6.3f", tmpGeo[0].crosslevel_NOC * -1.0);
		TextOut(hdc, 400, xlYOffset - 30, XLCharsNOC, strlen(XLCharsNOC));

		char XLCharsOC[20];
		sprintf(XLCharsOC, "XL OC: %6.3f", tmpGeo[0].crosslevel_OC * -1.0);
		TextOut(hdc, 400, xlYOffset + 30, XLCharsOC, strlen(XLCharsOC));


		char gradeChars[20];
		sprintf(gradeChars, "GAGE: %6.3f", tmpGeo[0].fullGage_filtered - 56.5);
		TextOut(hdc, 400, gageYOffset - 30, gradeChars, strlen(gradeChars));
	}

	SelectObject(hdc, purplePen);

	x = 0;

	SelectObject(hdc, blackBrush);
	SelectObject(hdc, GetStockObject(BLACK_PEN));
	x = 0;

	for (int i = 0; i < tmpGeoCnt && i < 2640; i++)//PLOT 1 MILE OF DATA
	{
		Ellipse(hdc, x - 1, alignmentYOffset + alignmentScale * tmpGeo[i].R_HORIZ_MCO_62_NOC*-1.0 - 1, x + 1, alignmentYOffset + alignmentScale * tmpGeo[i].L_HORIZ_MCO_62_OC*-1.0 + 1);
		x += horizInc;
	}

	//--PLOT RIGHT RAIL--//
	SelectObject(hdc, blueBrush);
	SelectObject(hdc, bluePen);
	x = 0;
	for (int i = 0; i < tmpGeoCnt && i < 2640; i++)//PLOT 1 MILE OF DATA
	{
		Ellipse(hdc, x - 1, alignmentYOffset + alignmentScale * tmpGeo[i].R_HORIZ_MCO_62_OC*-1.0 - 1, x + 1, alignmentYOffset + alignmentScale * tmpGeo[i].R_HORIZ_MCO_62_OC*-1.0 + 1);
		x += horizInc;
	}


	//## PLOT PROFILE ##//

	//--PLOT LEFT RAIL--//
	SelectObject(hdc, blackBrush);
	SelectObject(hdc, GetStockObject(BLACK_PEN));

	x = 0;
	for (int i = 0; i < tmpGeoCnt && i < 2640; i++)//PLOT 1 MILE OF DATA
	{
		Ellipse(hdc, x - 1, profileYOffset + profileScale * tmpGeo[i].L_VERT_MCO_62_OC*-1.0 - 1, x + 1, profileYOffset + profileScale * tmpGeo[i].L_VERT_MCO_62_OC*-1.0 + 1);
		x += horizInc;
	}

	//--PLOT RIGHT RAIL--//
	SelectObject(hdc, blueBrush);
	SelectObject(hdc, bluePen);
	x = 0;
	for (int i = 0; i < tmpGeoCnt && i < 2640; i++)//PLOT 1 MILE OF DATA
	{
		Ellipse(hdc, x - 1, profileYOffset + profileScale * tmpGeo[i].R_VERT_MCO_62_OC*-1.0 - 1, x + 1, profileYOffset + profileScale * tmpGeo[i].R_VERT_MCO_62_OC*-1.0 + 1);
		x += horizInc;
	}

	//## PLOT CURVATURE ##//
	x = 0;

	SelectObject(hdc, blackBrush);
	SelectObject(hdc, GetStockObject(BLACK_PEN));


	for (int i = 0; i < tmpGeoCnt && i < 2640; i++)//PLOT 1 MILE OF DATA
	{
		Ellipse(hdc, x - 1, curvatureYOffset + curvatureScale * tmpGeo[i].filteredCurvature*-1.0 - 1, x + 1, curvatureYOffset + curvatureScale * tmpGeo[i].filteredCurvature*-1.0 + 1);
		x += horizInc;
	}

	//## PLOT CROSSLEVEL ##//
	SelectObject(hdc, blackBrush);
	SelectObject(hdc, GetStockObject(BLACK_PEN));

	x = 0;
	for (int i = 0; i < tmpGeoCnt && i < 2640; i++)//PLOT 1 MILE OF DATA
	{
		Ellipse(hdc, x - 1, xlYOffset + xlScale * tmpGeo[i].crosslevel_OC*-1.0 - 1, x + 1, xlYOffset + xlScale * tmpGeo[i].crosslevel_OC*-1.0 + 1);
		x += horizInc;
	}

	//## PLOT GAGE ##//
	SelectObject(hdc, blackBrush);
	SelectObject(hdc, GetStockObject(BLACK_PEN));

	x = 0;
	for (int i = 0; i < tmpGeoCnt && i < 2640; i++)//PLOT 1 MILE OF DATA
	{
		Ellipse(hdc, x - 1, gageYOffset + gageScale * (tmpGeo[i].fullGage_filtered - 56.5)*-1.0 - 1, x + 1, gageYOffset + gageScale * (tmpGeo[i].fullGage_filtered - 56.5)*-1.0 + 1);
		x += horizInc;
	}

	//dispGeo_mutex.unlock();


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

//START COMMUNICATION WITH APPLANIX
bool InertialSystem::startInertialSystem(int measureDirection)
{
	//ASSIGN THE DIRECTION THAT WE ARE MEASURING -- 1 = FORWARD; -1 = REVERSE
	setupMeasureDirection = measureDirection;

	updateInertialPulseGatingStatus(true);


	inertialSystemEnabled = true;

	unsigned plaUdpRecieveThreadID;
	HANDLE plaUdpRecieveThreadHandle;
	unsigned plaUdpUpdateThreadID;
	HANDLE plaUdpUpdateThreadHandle;
	//-- THREAD FOR RECIEVING PLASSER UDP1
	if (ENABLE_PLASSER_UDP1)
	{
		plaUdpRecieveThreadHandle = (HANDLE)_beginthreadex(NULL, 0, &udp1RecieverThread, NULL, NULL, &plaUdpRecieveThreadID);
		plaUdpUpdateThreadHandle = (HANDLE)_beginthreadex(NULL, 0, &udp1CurrentLocationUpdateThread, NULL, NULL, &plaUdpUpdateThreadID);
	}

	//-- THREAD FOR SENDING GEOMETRY AND EXCEPTION DATA OVER UDP --//
	unsigned udpGeometrySendingThreadID;
	HANDLE udpGeometrySendingThreadHandle;
	unsigned udpExceptionSendingThreadID;
	HANDLE udpExceptionSendingThreadHandle;
	if (udpGeomExcDataOutputEnable)
	{
		udpGeometrySendingThreadHandle = (HANDLE)_beginthreadex(NULL, 0, &udpGeometrySendingThread, NULL, NULL, &udpGeometrySendingThreadID);
		udpExceptionSendingThreadHandle = (HANDLE)_beginthreadex(NULL, 0, &udpExceptionSendingThread, NULL, NULL, &udpExceptionSendingThreadID);
	}

	//-- THREAD FOR ALIGNING CORE GEOMETRY DATA --//
	unsigned geomCalcMangerThreadID;
	HANDLE geomCalcMangerThreadHandle;

	coreGeom.enableCoreGeometry();
	geomCalcMangerThreadHandle = (HANDLE)_beginthreadex(NULL, 0, &geomCoreCalcManagerThread, (void*)setupMeasureDirection, NULL, &geomCalcMangerThreadID);

	//-- THREAD FOR PUTTING INFORMATION PACKETS INTO GEO CALCULATIONS INPUT QUE --//
	unsigned geoCalc2ThreadID1;
	HANDLE geoCalc2ThreadHandle1;

	geoCalc2ThreadHandle1 = (HANDLE)_beginthreadex(NULL, 0, &geom2ndCalcPutThread, NULL, NULL, &geoCalc2ThreadID1);

	//-- THREAD FOR TAKING INFORMATION PACKETS FROM GEO CALCULATIONS OUTPUT QUE --//
	unsigned geoCalc2ThreadID2;
	HANDLE geoCalc2ThreadHandle2;

	geoCalc2ThreadHandle2 = (HANDLE)_beginthreadex(NULL, 0, &geom2ndCalcGetThread, NULL, NULL, &geoCalc2ThreadID2);

	//-- THREAD FOR DOING THE GEO CALCULATIONS CALCULATIONS ON DATA --//
	unsigned geoCalc2ThreadID3;
	HANDLE geoCalc2ThreadHandle3;

	geoCalc2ThreadHandle3 = (HANDLE)_beginthreadex(NULL, 0, &geom2ndCalcProcessorThread, NULL, NULL, &geoCalc2ThreadID3);

	//-- THREAD FOR LOGGING GEOMETRY DATA TO HDD --//
	unsigned geoLoggingThreadID1;
	HANDLE geoLoggingThreadHandle1;

	if (geometryLoggingEnable)
	{
		geoLoggingThreadHandle1 = (HANDLE)_beginthreadex(NULL, 0, &geoLoggingWriteThread, NULL, NULL, &geoLoggingThreadID1);
	}

	//-- THREAD FOR WRITING EXCEPTIONS TO FILE --//
	unsigned geoExceptionWriterThreadID;
	HANDLE geoExceptionWriterThreadHandle;

	geoExceptionWriterThreadHandle = (HANDLE)_beginthreadex(NULL, 0, &exceptionSavingTransmittingThread, NULL, NULL, &geoExceptionWriterThreadID);

	//-- THREAD FOR MATCHING GAGE DATA WITH CORE GEOMETRY PACKETS --//
	unsigned coreGeomGageMatchingID;
	HANDLE coreGeomGageMatchingHandle;
	coreGeomGageMatchingHandle = (HANDLE)_beginthreadex(NULL, 0, &geomCoreGageMatchingThread, NULL, NULL, &coreGeomGageMatchingID);

	//-- THREAD FOR RECEIVING NETWORK INFO FROM APPLANIX --//
	unsigned appConCliThreadID;
	HANDLE appConCliThreadHandle;
	appConCliThreadHandle = (HANDLE)_beginthreadex(NULL, 0, &appNetworkInterfaceController, NULL, NULL, &appConCliThreadID);

	//-- THREAD FOR PARSING APPLANIX DATA PACKETS RECIEVED VIA TCP/IP --//
	//Applanix PACKET PARSING CONTROLLER -- GROUP 1 AND GROUP 5
	unsigned appPacketParse_G5_ThreadID;
	HANDLE appPacketParse_G5_Handle;

	appPacketParse_G5_Handle = (HANDLE)_beginthreadex(NULL, 0, &appNetworkPacketParsingController_Group5, NULL, NULL, &appPacketParse_G5_ThreadID);

	//WAIT FOR APPLANIX STARTUP
	while (appConCli.checkApplanixGlobalGating() == true)
	{
		//WAIT FOR CONNECTION TO APPLANIX
		Sleep(5);
	}
	updateInertialPulseGatingStatus(false);



	//WAITS FOR APPLANIX TCP/IP CONNECTION TO CLOSE
	WaitForSingleObject(appConCliThreadHandle, INFINITE);
	
	//FLAG THE REMAINING PROCESSES TO STOP
	coreGeom.shutdownCoreGeometry();
	inertialSystemEnabled = false;

	//WAIT FOR EXCEPTION WRITER/SENDING THREAD TO STOP
	WaitForSingleObject(geoExceptionWriterThreadHandle, INFINITE);

	calcs2Geom.shutdownGeoCalculations();

	//WAIT FOR REMAINING PROCESSES TO STOP
	WaitForSingleObject(geomCalcMangerThreadHandle, INFINITE);
	CloseHandle(geomCalcMangerThreadHandle);

	WaitForSingleObject(coreGeomGageMatchingHandle, INFINITE);
	CloseHandle(coreGeomGageMatchingHandle);

	geoLogger.shutdownGeometryLogging();



	WaitForSingleObject(geoCalc2ThreadHandle1, INFINITE);
	CloseHandle(geoCalc2ThreadHandle1);
	WaitForSingleObject(geoCalc2ThreadHandle2, INFINITE);
	CloseHandle(geoCalc2ThreadHandle2);
	WaitForSingleObject(geoCalc2ThreadHandle3, INFINITE);
	CloseHandle(geoCalc2ThreadHandle3);

	WaitForSingleObject(appPacketParse_G5_Handle, INFINITE);
	CloseHandle(appPacketParse_G5_Handle);


	if (geometryLoggingEnable)
	{
		WaitForSingleObject(geoLoggingThreadHandle1, INFINITE);
	}

	//SHUTDOWN THE UDP GEOMETRY/EXCEPTION DATA SENDING THREAD
	if (udpGeomExcDataOutputEnable)
	{
		outDataUDPFeed.udpGeomExc_shutdownSend();
		WaitForSingleObject(udpGeometrySendingThreadHandle, INFINITE);
		WaitForSingleObject(udpExceptionSendingThreadHandle, INFINITE);
	}


	if (ENABLE_PLASSER_UDP1)
	{
		plasserUdpUpdateEnable = false;
		udpRecv.shutdownUdp1Recv();
		WaitForSingleObject(plaUdpUpdateThreadHandle, INFINITE);
		CloseHandle(plaUdpUpdateThreadHandle);
		WaitForSingleObject(plaUdpRecieveThreadHandle, INFINITE);
		CloseHandle(plaUdpRecieveThreadHandle);
	}

	return true;
}

//PLACE A GAGE PACKET INTO THE INPUT GAGE BUFFER
bool InertialSystem::putGageInput(GageGeometry_Data packet)
{
	coreGeom.putGageData(packet);
	//IF LOGGING - INSERT INTO APPLANIX CON CLI LOGGING
	if (rawApplanixLoggingEnable)
	{
		APPLANIX_GROUP_10304 gData;
		APPLANIX_HEADER gHeader;
		APPLANIX_FOOTER gFooter;
		gHeader.groupStart[0] = '$';
		gHeader.groupStart[1] = 'G';
		gHeader.groupStart[2] = 'R';
		gHeader.groupStart[3] = 'P';
		gHeader.groupNumber = 10304;
		gHeader.byteCount = 56;
		gHeader.time1 = 0;
		gHeader.time2 = 0;
		gHeader.distance = 0;
		gHeader.timeType = 0x00;
		gHeader.distanceType = 0x00;
		gFooter.checksum = 0;
		gFooter.groupEnd[0] = '$';
		gFooter.groupEnd[1] = '#';
		gData.header = gHeader;
		gData.leftGage = packet.left_width;
		gData.rightGage = packet.right_width;
		gData.fullGage = packet.full_gage;
		gData.leftVertical = packet.left_height;
		gData.rightVertical = packet.right_height;
		gData.spare = 0;
		gData.footer = gFooter;
		appConCli.putApplanixGageLoggingData(gData);
	}
	return true;
}

//PUT DATAPACKET INTO OUTPUT BUFFER
bool putDataPacketOutputBuffer(CoreGeomCalculationPacket packet)
{
	std::unique_lock<std::mutex> l(dataPacket_outputBuffer_mutex);
	dataPacket_outputBuffer.push(packet);
	l.unlock();
	dataPacket_outputBuffer_condVar.notify_one();
	return true;
}

//GET DATAPACKET FROM OUTPUT BUFFER
bool InertialSystem::getDataPacketOutputBuffer(CoreGeomCalculationPacket &packet)
{
	std::unique_lock<std::mutex> l(dataPacket_outputBuffer_mutex);
	dataPacket_outputBuffer_condVar.wait_for(l, std::chrono::milliseconds(100), [] {return dataPacket_outputBuffer.size() > 0; });
	if (dataPacket_outputBuffer.size() > 0)
	{
		CoreGeomCalculationPacket tmp;
		tmp = dataPacket_outputBuffer.front();
		dataPacket_outputBuffer.pop();
		l.unlock();

		packet = tmp;
		if (packet.filteredCurvature >= -50 && packet.filteredCurvature <= 50)
		{
			int good = 1;
		}
		else
		{
			int problme = 1;
		}
		return true;
	}
	l.unlock();
	return false;
}

//SHUTDOWN / STOP INERTIAL SYSTEM
bool InertialSystem::shutdownInterialSystem()
{
	inertialSystemEnabled = false;
	geometryLoggingEnable = true;
	appConCli.shutdownApplanix();
	return true;

}

//PLASSER UDP1 CURRENT LOCATION UPDATE THREAD
unsigned __stdcall udp1CurrentLocationUpdateThread(void* args)
{
	plasserUdpUpdateEnable = true;
	while (plasserUdpUpdateEnable)
	{
		UDP1 p;
		bool success = udpRecv.getUDP1Packet(p);
		if (success)
		{
			int track = std::stoi(p.szTrack);
			char seq = 'U';
			if (p.iSequence == -1)
				seq = 'D';
			char direction = 'F';
			if (p.iDirection == 0)
				direction = 'R';
			int ft = (int)p.fSubUnit;
			int mp = (int)p.lPOST;
			
			currentTgcLocation.updateAll(mp, ft, seq, direction, track, p.szCode);
		}
	}

	return 0;
}

//PLASSER UDP1 RECIEVER THREAD
unsigned __stdcall udp1RecieverThread(void* args)
{
	plasserUdpUpdateEnable = true;
	udpRecv = PlasserUDP();
	udpRecv.udp1RecieveThread();
	return 0;
}

//EXCEPTION GETTING / SAVING / SENDING THREAD -- GETS EXCEPTIONS FORM GEOCALCULATIONS.CPP
unsigned __stdcall exceptionSavingTransmittingThread(void* args)
{
	while (inertialSystemEnabled)
	{
		WTGS_Exception tmpException;
		bool gotException = calcs2Geom.getExceptionOutputQue(tmpException);
		if (gotException)
		{
			//GET +/- 1300 FEET OF DATA AROUND THE POINT 
			//GET FOOTAGE OF THE EXCEPTION
			int excFootage = tmpException.max_location.FTCnt;
			//CHECK IF THE GRAPHICAL OUTPUT BUFFER HAS DATA FOR excFootage - 250 //CHECK AT POSITION 0
			dispGeo_mutex.lock();
			if (dispGeo[0].location.FTCnt >= excFootage + 260 && excFootage > 260 )
			{
				int foundGeoIndex = -9;
				//FIND DISPLAY INDEX OF THE EXCEPTION VALUE
				for (int l = 0; l < displayGeoCnt; l++)
				{
					if (dispGeo[l].location.FTCnt == excFootage)
					{
						foundGeoIndex = l;
						l = 10000;
					}
				}
				CoreGeomCalculationPacket exceptionGeomData[501];
				//memmove(&dispGeo[1], &dispGeo[0], (sizeof CoreGeomCalculationPacket()) * 5279);
				memcpy(&exceptionGeomData[0], &dispGeo[foundGeoIndex - 250], (sizeof CoreGeomCalculationPacket()) * 501);
				geoLogger.createWriteException(tmpException, exceptionGeomData);
				if (udpGeomExcDataOutputEnable)
				{
					outDataUDPFeed.udpExc_putDataToSend(tmpException);
				}

				calcs2Geom.popExceptionOutputQue();
			}
			else
			{
				if (excFootage < 260)
				{
					calcs2Geom.popExceptionOutputQue();
				}
			}
			dispGeo_mutex.unlock();
		}
	}
	return 0;
}

//LOAD INI CONFIGURATION FILE
void loadInertialIniFile()
{
	std::ifstream configFile;
	configFile = std::ifstream(InertialIniConfigFile);
	bool isOpen = configFile.is_open();
	std::string aLine;
	std::string defID = "UNK";
	strcpy(vehicleID, defID.c_str());
	while (!std::getline(configFile, aLine).eof())
	{
		std::string logFileString = "LOG_RAW";
		std::string vehicleIDString = "VEHICLE_ID";
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
					rawApplanixLoggingEnable = TRUE;
				}
				else
				{
					rawApplanixLoggingEnable = FALSE;
				}
			}
			if (vehicleIDString.compare(setting) == 0)
			{
				std::string tmpHolder = std::string(value);
				strcpy(vehicleID, tmpHolder.c_str());
				//SETS VEHICLEID GLOBAL
			}
		}
	}
}

//GET THE CURRENT INERTIAL FOOT PULSE GATING STATUS
bool InertialSystem::getCurrentInertialPulseGatingStatus()
{
	std::unique_lock<std::mutex> l(inertialPulseGatingMutex);
	bool currentStatus = inertialPulseGatingEnabled;
	l.unlock();
	return currentStatus;
}

//UPDATE THE CURRENT INERTIAL FOOT PULSE GATING STATUS -- REMOTE
bool InertialSystem::enableInertialPulseGatingStatus()
{
	std::unique_lock<std::mutex> l(inertialPulseGatingMutex);
	inertialPulseGatingEnabled = true;
	l.unlock();
	return true;
}

//UPDATE THE CURRENT INERTIAL FOOT PULSE GATING STATUS -- LOCAL
bool updateInertialPulseGatingStatus(bool newStatus)
{
	std::unique_lock<std::mutex> l(inertialPulseGatingMutex);
	inertialPulseGatingEnabled = newStatus;
	l.unlock();
	return true;
}

