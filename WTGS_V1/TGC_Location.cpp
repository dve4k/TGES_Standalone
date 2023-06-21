#include "stdafx.h"
#include "TGC_Location.h"

//GLOBALS
MP_LOCATION currentLocation;
std::mutex currentLocation_mutex;



TGC_Location::TGC_Location()
{
	currentLocation = MP_LOCATION();
	currentLocation.carOrientation = 'F';
	currentLocation.mpSequence = 'U';
	currentLocation.FTCnt = 0;
	currentLocation.MP = 0;
	for (int i = 0; i < 6; i++)
	{
		currentLocation.RID[i] = '-';
	}
	currentLocation.trackNumber = 0;
}

TGC_Location::~TGC_Location()
{
	currentLocation = MP_LOCATION();
	currentLocation.carOrientation = 'F';
	currentLocation.mpSequence = 'U';
	currentLocation.FTCnt = 0;
	currentLocation.MP = 0;
	for (int i = 0; i < 6; i++)
	{
		currentLocation.RID[i] = '-';
	}
	currentLocation.trackNumber = 0;
}

//GET THE CURRENT MP LOCATION
MP_LOCATION TGC_Location::getLocation()
{
	currentLocation_mutex.lock();
	MP_LOCATION toReturn = currentLocation;

	currentLocation_mutex.unlock();
	return toReturn;
}

//GET THE CURRENT MP LOCATION -- INCREMENT/DECREMENT IT AFTERWARDS
MP_LOCATION TGC_Location::getLocation_Increment()
{
	currentLocation_mutex.lock();
	MP_LOCATION toReturn = currentLocation;
	if (currentLocation.mpSequence == 'U' || currentLocation.mpSequence == 'u')
	{
		currentLocation.FTCnt++;
	}

	else
	{
		currentLocation.FTCnt--;
	}
	currentLocation_mutex.unlock();
	return toReturn;
}

//UPDATE THE CURRENT FT
void TGC_Location::updateFt(int ft)
{
	currentLocation_mutex.lock();
	currentLocation.FTCnt = ft;
	currentLocation_mutex.unlock();
}

//UPDATE THE MILEPOST / FT
void TGC_Location::updateMP(int mp, int ft)
{
	currentLocation_mutex.lock();
	currentLocation.FTCnt = ft;
	currentLocation.MP = mp;
	currentLocation_mutex.unlock();
}

//UPDATE THE RID
void TGC_Location::updateRID(char* RID)
{
	currentLocation_mutex.lock();
	memcpy(currentLocation.RID, RID, sizeof(currentLocation.RID));
	currentLocation_mutex.unlock();
}

//UPDATE THE TRACK NUMBER
void TGC_Location::updateTrack(int trackNum)
{
	currentLocation_mutex.lock();
	currentLocation.trackNumber = trackNum;
	currentLocation_mutex.unlock();
}

//UPDATE TRACK MP SEQUENCE
void TGC_Location::updateMpSequence(char mpSeq)
{
	currentLocation_mutex.lock();
	currentLocation.mpSequence = mpSeq;
	currentLocation_mutex.unlock();
}

//UPDATE ALL LOCATION INFORMATION
void TGC_Location::updateAll(int mp, int ft, char mpSeq, char carOrientation, int trackNum, char* rid)
{
	currentLocation_mutex.lock();
	currentLocation.MP = mp;
	currentLocation.FTCnt = ft;
	currentLocation.carOrientation = carOrientation;
	currentLocation.mpSequence = mpSeq;
	currentLocation.trackNumber = trackNum;
	memcpy(currentLocation.RID, rid, sizeof(currentLocation.RID));

	currentLocation_mutex.unlock();
}

//DECREMENT MILEPOST
void TGC_Location::decrementMP()
{
	currentLocation_mutex.lock();
	currentLocation.FTCnt = 5280;
	currentLocation.MP--;
	currentLocation_mutex.unlock();
}

//INCREMENT MILEPOST
void TGC_Location::incrementMP()
{
	currentLocation_mutex.lock();
	currentLocation.FTCnt = 0;
	currentLocation.MP++;
	currentLocation_mutex.unlock();
}


