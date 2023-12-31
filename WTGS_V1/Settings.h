#pragma once
//CALIBRATION AND SENSOR SETUP
//#define CALIBRATION_ANGLE_1 50.9	//				--HYRAIL=33
//#define CALIBRATION_ANGLE_2 50.4	//				--HYRAIL=34
//#define CALIBRATION_GAGE_OFFSET 47.0137 //		--HYRAIL=47.0137
#define FLIP_IMAGE_1 true				//		--HYRAIL=TRUE
#define FLIP_IMAGE_2 false		//NS38 NEED FLIP=TRUE		//		--HYRAIL=FALSE

//SENSOR # 1
//#define WINDOW_CENTER_X_S1 491.0 // NS7538 480.0 //CENTER X		--HYRAIL=175.0
//#define WINDOW_CENTER_Y_S1 325.0 //NS7538 291.0 //CENTER Y		--HYRAIL=275.0
#define WINDOW_WIDTH_S1  152.0//122.0 //152.0
#define WINDOW_HEIGHT_S1 101.0 //102.0
//#define CV2_IMAGE_MIN_X_1 WINDOW_CENTER_X_S1-WINDOW_WIDTH_S1/2
//#define CV2_IMAGE_MIN_Y_1 WINDOW_CENTER_Y_S1-WINDOW_HEIGHT_S1/2
//#define CV2_IMAGE_MAX_X_1 WINDOW_CENTER_X_S1+WINDOW_WIDTH_S1/2
//#define CV2_IMAGE_MAX_Y_1 WINDOW_CENTER_Y_S1+WINDOW_HEIGHT_S1/2
#define CV2_IMAGE_SIZE_X_1 152 //122//152
#define CV2_IMAGE_SIZE_Y_1 101//102

//SENSOR # 2
//#define WINDOW_CENTER_X_S2 448.0 //ns7538 446.0 //CENTER X		--HYRAIL=170.0
//#define WINDOW_CENTER_Y_S2 338.0 //ns7538 297.0 //CENTER Y		--HYRAIL=260.0
#define WINDOW_WIDTH_S2 152.0 //122.0 //152.0
#define WINDOW_HEIGHT_S2 101.0 //102.0
//#define CV2_IMAGE_MIN_X_2 WINDOW_CENTER_X_S2-WINDOW_WIDTH_S2/2 
//#define CV2_IMAGE_MIN_Y_2 WINDOW_CENTER_Y_S2-WINDOW_HEIGHT_S2/2 
//#define CV2_IMAGE_MAX_X_2 WINDOW_CENTER_X_S2+WINDOW_WIDTH_S2/2 
//#define CV2_IMAGE_MAX_Y_2 WINDOW_CENTER_Y_S2+WINDOW_HEIGHT_S2/2 
#define CV2_IMAGE_SIZE_X_2 CV2_IMAGE_SIZE_X_1 // ----NOTE!!!  BOTH SIZE FOR SENS 1 AND 2 MUST BE THE SAME -- ACCOUNT FIXED ARRAY SIZES
#define CV2_IMAGE_SIZE_Y_2 CV2_IMAGE_SIZE_Y_1 // ----NOTE!!!  BOTH SIZE FOR SENS 1 AND 2 MUST BE THE SAME -- ACCOUNT FIXED ARRAY SIZES

//COORDINATES FOR WHAT PROFILE POINTS ARE DISPLAYED
#define MIN_PROFILE_DISPLAY_X 225.0
#define MAX_PROFILE_DISPLAY_X 650.0
#define MIN_PROFILE_DISPLAY_Y 225.0
#define MAX_PROFILE_DISPLAY_Y 550.0

//RAIL TEMPLATE ARGUMENTS
#define RAIL_TEMPLATE_COUNT 1

//HOUGH LINE AND FEATURE DETECTION
#define TOR_CIRCLE_MATCH_RADIUS 20.0
#define HALF_SMOOTHING_WINDOW 10
#define SMOOTHING_MAX_JUMP 10.0
#define IDEAL_CORNER_ANGLE 50.0
#define IDEAL_CORNER_GRADIENT -80.0

//SCREEN UPDATE MODULUS
#define SCREEN_UPDATE_MOD 7//7
#define DISPLAY_PROFILE_ENABLE true
#define DISPLAY_INERTIAL_ENABLE true

//INERTIAL SYSTEM CONFIGS
#define LEFT_RAIL_LEVER_ARM -29.5276//66.9291 //1.7 METERS IN INCHES //NS32 = -29.5276
#define LEFT_LASER_LEVER_ARM -29.5276//-16.0
#define RIGHT_RAIL_LEVER_ARM 29.5276//7.874 //0.2 METERS IN INCHES  //NS32 = 29.5276
#define RIGHT_LASER_LEVER_ARM 29.5276//16.0
#define PI 3.14159265359
#define TRACKWIDTH 59.0551
#define DEG2RAD 0.0174532925
#define RAD2DEG 57.29577951308
#define MM2INCH 0.0393701
#define INSSAMPLERATE 200 //100
#define INSSMAPLEPERIOD 0.005 //0.01
#define METERS_SECOND2IN_SECOND 39.3701
#define INS_IP_ADDRESS "10.255.255.100"

#define COREGEOMPACKET_PACKETLEN 50

//DEBUGGING
#define LOG_DIRECTORY "D:\\WTGS_LOG\\" //"C:\\TGC\\WTGS_LOG\\"
#define EXCEPTION_DIRECTORY "D:\\EXCEPTIONS\\" //"C:\\TGC\\EXCEPTIONS\\"
#define SIMULATION_MODE false
#define SIMULATION_FILE_NAME_1 "C:/TGC/WenglorSim/20180926084033_1.csv" //"C:/TGC/20180530153233.csv"
#define SIMULATION_FILE_NAME_2 "C:/TGC/WenglorSim/20180926084033_2.csv"
#define DRAW_GRID_LINES true

//CONFIGURATION FILE
#define GageIniConfigFile L"C:/TGC/gage.ini"
#define InertialIniConfigFile L"C:/TGC/inertial.ini"

//MAX FILE SIZE THAT TRIGGERS 'ROLL'
#define RAW_APPLANIX_MAX_LOG_SIZE 100000000
#define GEOMETRY_MAX_LOG_SIZE 50000000//100000000
#define GAGE_LASER_MAX_LOG_SIZE 500000

//PLACE DATA INTO PLASSER SEND QUE
#define ENABLE_PLASSER_SEND false

//UDP1 LOCATION UPDATES
#define ENABLE_PLASSER_UDP1 true

//LOCOMOTIVE CRIO SETUP
#define ENABLE_LOCOMOTIVE_CRIO_CONTROL true

#define ACCEPT_APPLANIX_GAGE false

#define fullSimulation false

#if fullSimulation == true
	#undef LOG_DIRECTORY
	#define LOG_DIRECTORY "C:\\TGC\\WTGS_LOG\\"
	#undef EXCEPTION_DIRECTORY
	#define EXCEPTION_DIRECTORY "C:\\TGC\\EXCEPTIONS\\"
	#undef INS_IP_ADDRESS	
	#define INS_IP_ADDRESS "127.0.0.1"
	#undef ENABLE_PLASSER_SEND
	#define ENABLE_PLASSER_SEND false
	#undef ENABLE_PLASSER_UDP1
	#define ENABLE_PLASSER_UDP1 false
	#undef GAGE_SYSTEM_ENABLE
	#define GAGE_SYSTEM_ENABLE true
	#undef ACCEPT_APPLANIX_GAGE
	#define ACCEPT_APPLANIX_GAGE true
#endif

