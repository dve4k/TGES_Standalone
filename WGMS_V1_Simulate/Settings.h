﻿#pragma once
//CALIBRATION AND SENSOR SETUP
#define CALIBRATION_ANGLE_1 39.8	//				--HYRAIL=33
#define CALIBRATION_ANGLE_2 39.9//34.0	//				--HYRAIL=34
//#define CALIBRATION_GAGE_OFFSET 47.0137 //		--HYRAIL=47.0137
#define FLIP_IMAGE_1 false				//		--HYRAIL=TRUE
#define FLIP_IMAGE_2 true//true				//		--HYRAIL=FALSE

//SENSOR # 1
#define WINDOW_CENTER_X_S1 348.0 //CENTER X		--HYRAIL=175.0
#define WINDOW_CENTER_Y_S1 283.0 //CENTER Y		--HYRAIL=275.0
#define WINDOW_WIDTH_S1  122.0 //252.0
#define WINDOW_HEIGHT_S1 101.0 //202.0
#define CV2_IMAGE_MIN_X_1 WINDOW_CENTER_X_S1-WINDOW_WIDTH_S1/2
#define CV2_IMAGE_MIN_Y_1 WINDOW_CENTER_Y_S1-WINDOW_HEIGHT_S1/2
#define CV2_IMAGE_MAX_X_1 WINDOW_CENTER_X_S1+WINDOW_WIDTH_S1/2
#define CV2_IMAGE_MAX_Y_1 WINDOW_CENTER_Y_S1+WINDOW_HEIGHT_S1/2
#define CV2_IMAGE_SIZE_X_1 122//252
#define CV2_IMAGE_SIZE_Y_1 101//202

//SENSOR # 2
#define WINDOW_CENTER_X_S2 367.0 //CENTER X		--HYRAIL=170.0
#define WINDOW_CENTER_Y_S2 280.0 //CENTER Y		--HYRAIL=260.0
#define WINDOW_WIDTH_S2 122.0 //252.0
#define WINDOW_HEIGHT_S2 101.0 //202.0
#define CV2_IMAGE_MIN_X_2 WINDOW_CENTER_X_S2-WINDOW_WIDTH_S2/2 
#define CV2_IMAGE_MIN_Y_2 WINDOW_CENTER_Y_S2-WINDOW_HEIGHT_S2/2 
#define CV2_IMAGE_MAX_X_2 WINDOW_CENTER_X_S2+WINDOW_WIDTH_S2/2 
#define CV2_IMAGE_MAX_Y_2 WINDOW_CENTER_Y_S2+WINDOW_HEIGHT_S2/2 
#define CV2_IMAGE_SIZE_X_2 CV2_IMAGE_SIZE_X_1 // ----NOTE!!!  BOTH SIZE FOR SENS 1 AND 2 MUST BE THE SAME -- ACCOUNT FIXED ARRAY SIZES
#define CV2_IMAGE_SIZE_Y_2 CV2_IMAGE_SIZE_Y_1 // ----NOTE!!!  BOTH SIZE FOR SENS 1 AND 2 MUST BE THE SAME -- ACCOUNT FIXED ARRAY SIZES

//RAIL TEMPLATE ARGUMENTS
#define RAIL_TEMPLATE_COUNT 1

//HOUGH LINE AND FEATURE DETECTION
#define TOR_CIRCLE_MATCH_RADIUS 20.0//20.0
#define SMOOTHING_WINDOW 21 //ODD NUMBER B/C INCLUDING CENTER VALUE
#define HALF_SMOOTHING_WINDOW 10
#define SMOOTHING_MAX_JUMP 10.0
#define ENABLE_SMOOTHED_POINTS true	
#define ENABLE_CUDA_SMOOTHING true
#define IDEAL_CORNER_ANGLE 50.0
#define IDEAL_CORNER_GRADIENT -80.0

//SCREEN UPDATE MODULUS
#define SCREEN_UPDATE_MOD 1//7

//DEBUGGING
//#define LOG_PROFILE_PONTS true
#define LOG_DIRECTORY "D:/"
#define SIMULATION_MODE true
#define SIMULATION_FILE_NAME_1 "C:/TGC/WenglorSim/20190708082309_1.csv" //"C:/TGC/20180530153233.csv"
#define SIMULATION_FILE_NAME_2 "C:/TGC/WenglorSim/20190708082309_2.csv"
#define DRAW_GRID_LINES false
#define BIG_PROFILE_DISPLAY true

#define MIN_PROFILE_DISPLAY_X 225.0
#define MAX_PROFILE_DISPLAY_X 475.0
#define MIN_PROFILE_DISPLAY_Y 225.0
#define MAX_PROFILE_DISPLAY_Y 425.0

//PCIE DIO CARD
#define advantechCardDesc L"PCIE-1760,BID#0"
#define advantechProfile L"advantech/profile.xml"

//CONFIGURATION FILE
#define iniConfigFile L"C:/TGC/gage.ini"

