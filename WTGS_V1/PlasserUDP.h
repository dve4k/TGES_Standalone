#pragma once
#include "stdafx.h"
#include <thread>
#include "windows.h"
#include <mutex>
#include "atlstr.h"
#include <cstdlib>
#include <queue>
#include <condition_variable>
//TCP/IP SERVER
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#define UDP1_LEN 1024
#define UDP1_PORT "6005"
#define UDP1_ADDRESS "192.168.233.255"

#pragma pack(2)
typedef struct
{
	unsigned long  		lDB;			// Data break # adjusted for 							//offset
	char			szDate[12];	// Date DD.MM.YYYY
	char			szTime[10];	// Time HH:MM:SS
	char			szText[62];	// Text
	char			szLine[20]; 	// Line
	char   			szCode[10];	// Code    
	short  	   iTrack;	// Track indicator for single character
				// track identifiers.  If a single character track
				// is used in the range of A to I or a to i, the
				// iTrack will be between 1 and 9.
				//  0 = Default track	                           
				//  1 = "A"
				//  2 = "B"
				//  3 = "C"
				//  4 = "D"
				//  5 = "E"
				//  6 = "F"
				//  7 = "G"
				//  8 = "H"
				//  9 = "I"
	long			lUnit;			// actual position (kilometers 							//with one digit * 10)
	long			lSubunit;		// actual position (kilometers 							//with two digits * 100)
	short			iSequence;		// KM sequence
							//  1 = Ascending
							// -1 = Descending
	short			iDirection;		// Measuring Direction
							//  1 = Forward
							//  0 = Reverse
	short			iRecord;			// Recording
							//  0 = Inactive
							//  1 = Recording
	short			iKey;			// Protocol identifier
//  0 = normal, no changes //other than location
//  1 = Other elements than //location changed or
//  error position, like kilometer //synchronization
//set every 100m in metric //systems or 220yard in UK
							//      or change of header
//  2 = triggered by event
//  @B162
	char		szDivision[18];			//Division
	char		szSubdivision[18];		//Subdivision
	char		szTrack[10];			//Track
	long		lPOST;				//Kilometer / Mile / Station
	float		fSubUnit;			//meter (normally 1000 per 						//KM) or
	//feet (normally 5280 per Mile, 
	//100 per Station) or
	//yard (normally 1760 per 	//Mile) 
	unsigned long	uEventWord;			//Set bit for active event 								//according to EMServer Event 							//list;
	float		fAmbientTemperature;		//ambient temperature with one 							//decimal
	short		iTravelingSpeed;			//current speed of recording car
	float		fLatitude;			//Latitude, invalid = -999.0
	float		fLongitude;			//Longitude, invalid = -999.0
	float		fAltitude;			//Altitude, invalid = -999.0
	float		fData0;				//Railroad Specific Data
	float		fData1;				//in units millimeters
	float		fData2;				//or inches
	float		fData3;				//Data channels are setup 
	float		fData4;				//hard coded per railroad
	float		fData5;				//invalid data is indicated 
	float		fData6;				//with the number
	float		fData7;				// -99999.00
	float		fData8;
	unsigned long	ulFlags;				//This value provides 							//information about the used 						//location measuring system.
	//The following numbers apply:
		//	0x00 : US English, Miles, feet, relative, meas. unit inches
		//	0x01: US English, Miles, feet, absolute, meas. unit inches
		//	0x10 : Metric, Kilometer, Meter, relative, meas. unit mm
		//	0x11 : Metric, Kilometer, Meter, absolute, meas. unit mm
		//	0x20 : Stationing, Station, feet, relative, meas. unit inches
		//	0x21: Stationing, Station, feet, absolute, meas. unit inches
		//	0x30 : UK English, Miles, yard, relative, meas. unit mm
		//	0x31: UK English, Miles, yard, absolute, meas. unit mm


} UDP1;


class PlasserUDP
{
public:
	PlasserUDP();
	~PlasserUDP();
	void udp1RecieveThread();
	bool getUDP1Packet(UDP1 &p);
	void shutdownUdp1Recv();
};
