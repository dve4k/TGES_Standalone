#include "stdafx.h"
#include "Applanix.h"


//CONSTRUCTORS
Applanix::Applanix()
{

}

Applanix::~Applanix()
{

}

//PARSE AND RETURN STRUCTURES

//PARSE APPLANIX GROUP 1 PACKET
bool Applanix::parse_Applanix_Group_1(APPLANIX_GROUP_1 &dataStruct, unsigned char* data)
{
	APPLANIX_GROUP_1* tmp = reinterpret_cast<APPLANIX_GROUP_1*>(data);
	dataStruct = *tmp;

	if (tmp->header.groupNumber == 1 && std::strncmp(tmp->header.groupStart, APPLANIX_GROUP_START, 4) == 0 && strncmp(tmp->footer.groupEnd, APPLANIX_GROUP_END, 2) == 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

//PARSE APPLANIX GROUP 2 PACKET
bool Applanix::parse_Applanix_Group_2(APPLANIX_GROUP_2 &dataStruct, unsigned char* data)
{
	APPLANIX_GROUP_2* tmp = reinterpret_cast<APPLANIX_GROUP_2*>(data);
	dataStruct = *tmp;
	if (tmp->header.groupNumber == 2 && std::strncmp(tmp->header.groupStart, APPLANIX_GROUP_START, 4) == 0 && strncmp(tmp->footer.groupEnd, APPLANIX_GROUP_END, 2) == 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

//PARSE APPLANIX GROUP 5 PACKET
bool Applanix::parse_Applanix_Group_5(APPLANIX_GROUP_5 &dataStruct, unsigned char* data)
{
	APPLANIX_GROUP_5* tmp = reinterpret_cast<APPLANIX_GROUP_5*>(data);
	dataStruct = *tmp;
	if (tmp->header.groupNumber == 5 && std::strncmp(tmp->header.groupStart, APPLANIX_GROUP_START, 4) == 0 && strncmp(tmp->footer.groupEnd, APPLANIX_GROUP_END, 2) == 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

//PARSE APPLANIX GROUP 15 PACKET
bool Applanix::parse_Applanix_Group_15(APPLANIX_GROUP_15 &dataStruct, unsigned char* data)
{
	APPLANIX_GROUP_15* tmp = reinterpret_cast<APPLANIX_GROUP_15*>(data);
	dataStruct = *tmp;
	if (tmp->header.groupNumber == 15 && std::strncmp(tmp->header.groupStart, APPLANIX_GROUP_START, 4) == 0 && strncmp(tmp->footer.groupEnd, APPLANIX_GROUP_END, 2) == 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

//PARSE APPLANIX GROUP 10304 PACKET - GAGE INFO FOR SIMULATION
bool Applanix::parse_Applanix_Group_10304(APPLANIX_GROUP_10304 &dataStruct, unsigned char* data)
{
	APPLANIX_GROUP_10304* tmp = reinterpret_cast<APPLANIX_GROUP_10304*>(data);
	dataStruct = *tmp;
	if (tmp->header.groupNumber == 10304 && std::strncmp(tmp->header.groupStart, APPLANIX_GROUP_START, 4) == 0 && strncmp(tmp->footer.groupEnd, APPLANIX_GROUP_END, 2) == 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

//PARSE APPLANIX HEADER
bool Applanix::parse_Applanix_Header(APPLANIX_HEADER &dataStruct, unsigned char* data)
{
	APPLANIX_HEADER* tmpHeader = reinterpret_cast<APPLANIX_HEADER*> (data);
	dataStruct = *tmpHeader;
	if (std::strncmp(tmpHeader->groupStart, APPLANIX_GROUP_START, 4) == 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}