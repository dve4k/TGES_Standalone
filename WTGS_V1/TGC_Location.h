#pragma once
#include "stdafx.h"
#include <mutex>

#pragma pack(1)

struct MP_LOCATION
{
	int MP;
	int FTCnt;
	char mpSequence;
	char carOrientation;
	char RID[6];
	int trackNumber;
	int trackClass = 4;
};

class TGC_Location
{
public:
	TGC_Location();
	~TGC_Location();
	MP_LOCATION getLocation();
	MP_LOCATION getLocation_Increment();
	void updateFt(int ft);
	void updateMP(int mp, int ft);
	void updateRID(char* RID);
	void updateTrack(int trackNum);
	void updateMpSequence(char mpSeq);
	void updateAll(int mp, int ft, char mpSeq, char carOrientation, int trackNum, char* rid);
	void decrementMP();
	void incrementMP();
};


