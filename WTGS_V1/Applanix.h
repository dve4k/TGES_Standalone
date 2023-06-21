#pragma once
#include "stdafx.h"
#include <thread>
#include "windows.h"
#include <mutex>
#include "atlstr.h"
#include <cstdlib>
#include <intrin.h>

#define APPLANIX_GROUP_START "$GRP"
#define APPLANIX_GROUP_END "$#"

enum APPLANIX_TIME_TYPE 
{
	APPLANIX_TIME_POS = 0,
	APPLANIX_TIME_GPS = 1,
	APPLANIX_TIME_UTC = 2,
	APPLANIX_TIME_USER = 3
};

enum APPLANIX_DISTANCE_TYPE 
{
	APPLANIX_DISTANCE_NA = 0,
	APPLANIX_DISTANCE_POS = 1,
	APPLANIX_DISTANCE_DMI = 2
};

enum APPLANIX_ALIGNMENT_STATUS
{
	APPLANIX_STATUS_FULL_NAVIATION = 0,
	APPLANIX_STATUS_FINE_ALIGNMENT = 1,
	APPLANIX_STATUS_GCCHI2 = 2,
	APPLANIX_STATUS_PCCHI2 = 3,
	APPLANIX_STATUS_GCCHI1 = 4,
	APPLANIX_STATUS_PCCHI1 = 5,
	APPLANIX_STATUS_COARSLEVELING = 6,
	APPLANIX_STATUS_INITSOLUTIONASSIGNED = 7,
	APPLANIX_STATUS_NOSOLUTION = 8
};

enum APPLANIX_DMI_STATUS
{
	APPLANIX_DMI_STATUS_INVALID = 0,
	APPLANIX_DMI_STATUS_VALID = 1,
	APPLANIX_DMI_STATUS_SCALECHANGED = 2
};

enum APPLANIX_DMI_TYPE
{
	APPLANIX_DMI_TYPE_NONE = 0,
	APPLANIX_DMI_TYPE_PULSEDIRECTION = 1,
	APPLANIX_DMI_TYPE_QUADRATURE = 2
};

enum APPLANIX_DMI_RATE
{
	APPLANIX_DMI_RATE_50 = 0,
	APPLANIX_DMI_RATE_100 = 1,
	APPLANIX_DMI_RATE_200 = 2,
	APPLANIX_DMI_RATE_400 = 3,
	APPLANIX_DMI_RATE_125 = 4
};
#pragma pack(1)
struct APPLANIX_HEADER 
{
	char groupStart[4];
	unsigned short groupNumber;
	unsigned short byteCount;
	double time1;
	double time2;
	double distance;
	unsigned char timeType;
	unsigned char distanceType;
};

#pragma pack(1)
struct APPLANIX_FOOTER
{
	unsigned short checksum;
	char groupEnd[2];
};


#pragma pack(1)
struct APPLANIX_GROUP_1 
{
	APPLANIX_HEADER header;
	double latitude;
	double longitude;
	double altitude;
	float northVelocity;
	float eastVelocity;
	float downVelocity;
	double roll;
	double pitch;
	double heading;
	double wanderAngle;
	float trackAngle;
	float speed;
	float lonAngularRate;
	float travAngularRate;
	float downAngularRate;
	float lonAcceleration;
	float travAcceleration;
	float downAcceleration;
	unsigned char alignmentStatus;
	unsigned char padding;
	APPLANIX_FOOTER footer;
};

#pragma pack(1)
struct APPLANIX_GROUP_2
{
	APPLANIX_HEADER header;
	float northPositionError;
	float eastPositionError;
	float downPositionError;
	float northVelocityError;
	float eastVelocityError;
	float downVelocityError;
	float rollError;
	float pitchError;
	float headingError;
	float ellipsoi_semiMajorError;
	float ellipsoid_semiMinorError;
	float ellipsoid_orientationError;
	unsigned char padding[2];
	APPLANIX_FOOTER footer;
};

#pragma pack(1)
struct APPLANIX_GROUP_5
{
	APPLANIX_HEADER header;
	unsigned long pulseNumber;
	unsigned char padding[2];
	APPLANIX_FOOTER footer;
};

#pragma pack(1)
struct APPLANIX_GROUP_15
{
	APPLANIX_HEADER header;
	double signedDistanceTraveled;
	double unsignedDistanceTraveled;
	unsigned short dmiScale;
	unsigned char dataStatus;
	unsigned char dmiType;
	unsigned char dmiRate;
	unsigned char padding;
	APPLANIX_FOOTER footer;
};

#pragma pack(1)
struct APPLANIX_GROUP_331
{
	APPLANIX_HEADER header;
	float superelevation;
	char pad[2];
	APPLANIX_FOOTER footer;
};

#pragma pack(1)
struct APPLANIX_GROUP_10304
{
	APPLANIX_HEADER header;
	float leftGage;
	float rightGage;
	float fullGage;
	float leftVertical;
	float rightVertical;
	float spare;
	char pad[2];
	APPLANIX_FOOTER footer;
};

#pragma pack(1)
struct APPLANIX_MSG_4
{
	char msgStart[4];
	unsigned short msgID;
	unsigned short byteCnt;
	unsigned short transactionNumber;
	unsigned short groupID1;
	unsigned short groupID2;
	unsigned short loggingRate;
	unsigned char autoLog;
	unsigned char diskControl;
	char fileName[7];
	char reserved[52];
	char pad[2];///??????
	unsigned short checksum;
	char msgEnd[2];
};

class Applanix
{
public:
	Applanix();
	~Applanix();
	bool parse_Applanix_Group_1(APPLANIX_GROUP_1 &dataStruct, unsigned char* data);
	bool parse_Applanix_Group_2(APPLANIX_GROUP_2 &dataStruct, unsigned char* data);
	bool parse_Applanix_Group_5(APPLANIX_GROUP_5 &dataStruct, unsigned char* data);
	bool parse_Applanix_Group_15(APPLANIX_GROUP_15 &dataStruct, unsigned char* data);
	bool parse_Applanix_Group_10304(APPLANIX_GROUP_10304 &dataStruct, unsigned char* data);
	bool parse_Applanix_Header(APPLANIX_HEADER &header, unsigned char* data);
};