#pragma once
#include "stdafx.h"
#include <thread>
#include "windows.h"
#include <mutex>
#include "atlstr.h"
#include <cstdlib>
#include <intrin.h>

#define CRC32_POLYNOMIAL 0xEDB88320L

enum HeaderType
{
	FULL_HEADER = 1,
	SHORT_HEADER = 2,
	UNKNOWN_HEADER = 3
};

enum MessageID 
{
	BESTPOS_MSG = 42,
	INSSPDS_MSG = 323,
	INSPVAS_MSG = 508,
	CORRIMUDATAS_MSG = 813,
	MARK1PVA_MSG = 1067,
	MARK2PVA_MSG = 1068,
	MARK3PVA_MSG = 1118,
	MARK4PVA_MSG = 1119
};

enum TimeStatus:unsigned char
{
	unknown = 20,
	approximate = 60,
	coarseAdjusting = 80,
	coarse = 100,
	coarseSteering = 120,
	freeWheeling = 130,
	fineAdjusting = 140,
	fine = 160,
	fineBackupSteering = 170,
	fineSteering = 180,
	satTime = 200
};

enum InsStatus
{
	INS_INACTIVE = 0,
	INS_ALIGNING = 1,
	INS_HIGH_VARIANCE = 2,
	INS_SOLUTION_GOOD = 3,
	INS_SOLUTION_FREE = 6,
	INS_ALIGNMENT_COMPLETE = 7,
	DETERMINING_ORIENTATION = 8,
	WAITING_INITIALPOS = 9,
	WAITING_AZIMUTH = 10,
	INITIALIZING_BIASES = 11,
	MOTION_DETECT = 12
};

enum SolutionStatus
{
	SOL_COMPUTED = 0,
	INSUFFICIENT_OBS = 1,
	NO_CONVERGENCE = 2,
	SINGULARITY = 3,
	COV_TRACE = 4,
	TEST_DIST = 5,
	COLD_START = 6,
	V_H_LIMIT = 7,
	VARIANCE = 8,
	RESIDUALS = 9,
	INTERGRITY_WARNING = 13,
	PENDING = 18,
	INVALID_FIX = 19,
	UNAUTHORIZED = 20,
	INVALID_RATE = 22
};

enum Pos_Vel_Type
{
	NONE = 0,
	FIXEDPOS = 1,
	FIXEDHEIGHT = 2,
	FLOATCONV = 4,
	WIDELANE = 5,
	NARROWLANE = 6,
	DOPPLER_VELOCITY = 8,
	SINGLE = 16,
	PSRDIFF = 17,
	WAAS = 18,
	PROPAGATED = 19,
	L1_FLOAT = 32,
	IONOFREE_FLOAT = 33,
	NARROW_FLOAT = 34,
	L1_INT = 48,
	WIDE_INT = 49,
	NARROW_INT = 50,
	RTK_DIRECT_INS = 51,
	INS_SBAS = 52,
	INS_PSRSP = 53,
	INS_PSRDIFF = 54,
	INS_RTKFLOAT = 55,
	INS_RTKFIXED = 56,
	PPP_CONVERGING = 68,
	PPP = 69,
	OPERATIONAL = 70,
	WARNING = 71,
	OUT_OF_BOUNDS = 72, 
	INS_PPP_CONVERGING = 73,
	INS_PPP = 74,
	PPP_BASIC_CONVERGING = 77,
	PPP_BASIC = 78, 
	INS_PPP_BASIC_CONVERGING = 79,
	INS_PPP_BASIC = 80
};

enum DatumID
{
	ADIND = 1, ARC50 = 2, ARC60 = 3, AGD66 = 4, AGD84 = 5, BUKIT = 6, ASTRO = 7, CHATM = 8, CARTH = 9, CAPE = 10,
	DJAKA = 11, EGYPT = 12, ED50 = 13, ED79 = 14, GUNSG = 15, GEO49 = 16, GRB36 = 17, GUAM = 18, HAWAII = 19, KAUAI = 20,
	MAUI = 21, OAHU = 22, HERAT = 23, HJORS = 24, HONGK = 25, HUTZU = 26, INDIA = 27, IRE65 = 28, KERTA = 29, KANDA = 30,
	LIBER = 31, LUZON = 32, MINDA = 33, MERCH = 34, NAHR = 35, NAD83 = 36, CANADA = 37, ALASKA = 38, NAD27 = 39, CARIBB = 40,
	MEXICO = 41, CAMER = 42, MINNA = 43, OMAN = 44, PUERTO = 45, QORNO = 46, ROME = 47, CHUA = 48, SAM56 = 49, SAM69 = 50,
	CAMPO = 51, SACOR = 52, YACAR = 53, TANAN = 54, TIMBA = 55, TOKYO = 56, TRIST = 57, VITI = 58, WAK60 = 59, WGS72 = 60, 
	WGS84 = 61, ZANDE = 62, USER = 63, CSRS = 64, ADIM = 65, ARSM = 66, ENW = 67, HTN = 68, INDB = 69, INDI = 70,
	IRL = 71,  LUZA = 72, LUZB = 73, NAHC = 74, NASP = 75, OGBM = 76, OHAA = 77, OHAB = 78, OHAC = 79, OHAD = 80,
	OHIA = 81, OHIB = 82, OHIC = 83, OHID = 84, TIL = 85, TOYM = 86
};

enum Novatel_Port
{
	ICOM1_ALL = 0x17,
	ICOM2_ALL = 0x18,
	ICOM3_ALL = 0x19
};

enum Novatel_Log_Trigger
{
	ON_NEW = 0,
	ON_CHANGED = 1,
	ON_TIME = 2,
	ON_NEXT = 3,
	ONCE = 4,
	ON_MARK = 5
};

enum Novatel_Log_Hold
{
	NO_HOLD = 0,
	HOLD = 1
};

//DO I NEED PRAGMA PACK(1) / PRAGMA POP(1)???????
#pragma pack(1)
struct HEADER
{
	char sync1;
	char sync2;
	char sync3;
	unsigned char headerLength;
	unsigned short msgID;
	char msgType;
	unsigned char portAddress;
	unsigned short msgLength;
	unsigned short sequence;
	unsigned char idleTime;
	TimeStatus timeStatus; //THIS IS ONE BYTE INSTEAD OF TWO
	//unsigned char timeStatus;
	unsigned short week;
	long milliseconds;
	unsigned long recStatus;
	unsigned short reserved1;
	unsigned short recSwVer;
};
#pragma pop(1)

#pragma pack(1)
struct SHEADER
{
	unsigned char sync1;
	unsigned char sync2;
	unsigned char sync3;
	unsigned char msgLength;
	unsigned short msgID;
	unsigned short week;
	long milliseconds;
};
#pragma pop(1)

#pragma pack(1)
struct CORRIMUDATAS
{
	SHEADER header;
	unsigned long week;
	double seconds;
	double pitchRate;
	double rollRate;
	double yawRate;
	double latAccel;
	double longAccel;
	double vertAccel;
	unsigned long crc;
};
#pragma pop(1)

#pragma pack(1)
struct MARKPVA
{
	HEADER header;
	unsigned long week;
	double seconds;
	double lat;
	double lon;
	double height;
	double northVelocity;
	double eastVelocity;
	double upVelocity;
	double roll;
	double pitch;
	double azimuth;
	InsStatus insStatus;
	unsigned long crc;
};
#pragma pop(1)

#pragma pack(1)
struct INSSPDS
{
	SHEADER header;
	unsigned long week;
	double seconds;
	double trackGround;
	double horizontalSpeed;
	double verticalSpeed;
	InsStatus insStatus;
	unsigned long crc;
};
#pragma pop(1)

#pragma pack(1)
struct INSPVAS
{
	SHEADER header;
	unsigned long week;
	double seconds;
	double lat;
	double lon;
	double height;
	double northVelocity;
	double eastVelocity;
	double upVelocity;
	double roll;
	double pitch;
	double azimuth;
	InsStatus insStatus;
	unsigned long crc;
};
#pragma pop(1)

#pragma pack(1)
struct BESTPOS
{
	HEADER header;
	SolutionStatus solutionStatus;
	Pos_Vel_Type posType;
	double lat;
	double lon;
	double height;
	float undulation;
	DatumID datumID;
	float sdLat;
	float sdLon;
	float sdHeight;
	char baseID[4];
	float differentialAge;
	float solutionAge;
	unsigned char satsTracked;
	unsigned char satsInSolution;
	unsigned char satsL1Solution;
	unsigned char satsMfSolution;
	unsigned char res1;
	unsigned char exSolutionStat;
	unsigned char galileoUsed;
	unsigned char glonassUsed;
	unsigned long crc;
};
#pragma pop(1)

struct NovatelSendCommandOb
{
	char data[512];
	int dataLength;
};

struct LOG_CMD
{
	HEADER header;
	Novatel_Port port;
	unsigned short logID;
	char logType;
	char reserved1;
	Novatel_Log_Trigger logTrigger;
	double logPeriod;
	double offsetTime;
	Novatel_Log_Hold holdSetting;
};

class Novatel
{
public:
	Novatel();
	~Novatel();
	void checkThis(unsigned char* data, unsigned long length);
	bool parse_header(unsigned char* data, HeaderType &headerType, MessageID &messageID, unsigned short &messageLength, unsigned short &weekNumber, long &milliseconds, unsigned short &headerLength);
	bool parse_BESTPOS(unsigned char* data, BESTPOS &dataStruct, unsigned long length);
	bool parse_INSPVAS(unsigned char* data, INSPVAS &dataStruct, unsigned long length);
	bool parse_MARKPVA(unsigned char* data, MARKPVA &dataStruct, unsigned long length);
	bool parse_INSSPDS(unsigned char* data, INSSPDS &dataStruct, unsigned long length);
	bool parse_CORRIMUDATAS(unsigned char* data, CORRIMUDATAS &dataStruct, unsigned long length);
	bool build_command_log_PVAS(char* data);

};