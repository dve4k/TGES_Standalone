#include "stdafx.h"
#include "Novatel.h"

//PROTOTYPES
unsigned long				crc32Value(int i);
unsigned long				calculateBlockCRC32(unsigned long ulCount, unsigned char *ucBuffer);
unsigned long				calcCRC(unsigned char* data, unsigned long count);

//GLOBALS

Novatel::Novatel()
{

}

Novatel::~Novatel()
{


}

//PARSES CORRIMUDATAS PACKET. RETURN BOOL WITH GOOD/BAD CRC
bool Novatel::parse_CORRIMUDATAS(unsigned char* data, CORRIMUDATAS &dataStruct, unsigned long length)
{
	CORRIMUDATAS* tmp = reinterpret_cast<CORRIMUDATAS*>(data);
	dataStruct = *tmp;

	unsigned long CRCrev = calcCRC(data, length);
	bool goodCrc = false;

	if (CRCrev == dataStruct.crc || dataStruct.crc == 9876)//VALUE 9876 IS FOR MY INS SIMULATOR -- INSTEAD OF CALCULATING CRC32 IN C#
		goodCrc = true;

	return goodCrc;
}

//PARSES INSSPDS PACKET. RETURN BOOL WITH GOOD/BAD CRC
bool Novatel::parse_INSSPDS(unsigned char* data, INSSPDS &dataStruct, unsigned long length)
{
	INSSPDS* tmp = reinterpret_cast<INSSPDS*>(data);
	dataStruct = *tmp;

	unsigned long CRCrev = calcCRC(data, length);
	bool goodCrc = false;

	if (CRCrev == dataStruct.crc)
		goodCrc = true;

	return goodCrc;
}

//PARSES MARKPVA PACKET. RETURN BOOL WITH GOOD/BAD CRC
bool Novatel::parse_MARKPVA(unsigned char* data, MARKPVA &dataStruct, unsigned long length)
{
	MARKPVA* tmp = reinterpret_cast<MARKPVA*>(data);
	dataStruct = *tmp;

	unsigned long CRCrev = calcCRC(data, length);
	bool goodCrc = false;

	if (CRCrev == dataStruct.crc)
		goodCrc = true;

	return goodCrc;
}

//PARSES INSPVAS PACKET. RETURN BOOL WITH GOOD/BAD CRC
bool Novatel::parse_INSPVAS(unsigned char* data, INSPVAS &dataStruct, unsigned long length)
{
	INSPVAS* tmp = reinterpret_cast<INSPVAS*>(data);
	dataStruct = *tmp;

	unsigned long CRCrev = calcCRC(data, length);
	bool goodCrc = false;

	if (CRCrev == dataStruct.crc)
		goodCrc = true;

	return goodCrc;
}

//PARSES BESTPOS PACKET. RETURN BOOL WITH GOOD/BAD CRC
bool Novatel::parse_BESTPOS(unsigned char* data, BESTPOS &dataStruct, unsigned long length)
{
	BESTPOS* tmp = reinterpret_cast<BESTPOS*>(data);
	dataStruct = *tmp;

	unsigned long CRCrev = calcCRC(data, length);
	bool goodCrc = false;

	if (CRCrev == dataStruct.crc)
		goodCrc = true;

	return goodCrc;
}

//PARSES SOME INFO FROM THE HEADER -- RETURNS FALSE IF UNABLE TO PARSE
bool Novatel::parse_header(unsigned char* data, HeaderType &headerType, MessageID &messageID, unsigned short &messageLength, unsigned short &weekNumber, long &milliseconds, unsigned short &headerLength)
{
	//DETERMINE HEADER TYPE -- SHORT / FULL LENGTH
	//LOOK AT BYTE INDEX 2 (ZERO INDEXED)
	//0x12 = FULL
	//0x13 = SHORT
	unsigned char* byteIndex0 = reinterpret_cast<unsigned char*>(data);
	unsigned char* byteIndex1 = reinterpret_cast<unsigned char*>(data + 1);
	unsigned char* byteIndex2 = reinterpret_cast<unsigned char*>(data + 2);

	//CHECK FOR VALID SYNC BYTES
	if (byteIndex0[0] != 0xAA && byteIndex1[0] != 0x44)
	{
		//IF HEADER DOES NOT START WITH THE CORRECT 2 SYNC BYTES...THROW AWAY--INVALID HEADER
		//byte0 = 0xAA
		//byte1 = 0x44
		return false;
	}

	//DETERMINE MESSAGE ID, MESSAGE LENGTH, WEEK NUMBER, MILLISECONDS
	HEADER* tmpHeaderFull;
	SHEADER* tmpHeaderShort;
	switch (byteIndex2[0])
	{
	case 0x12:
		headerType = FULL_HEADER;
		tmpHeaderFull = reinterpret_cast<HEADER*>(data);
		messageID = static_cast<MessageID>(tmpHeaderFull->msgID);
		weekNumber = tmpHeaderFull->week;
		milliseconds = tmpHeaderFull->milliseconds;
		messageLength = tmpHeaderFull->msgLength;
		headerLength = tmpHeaderFull->headerLength;
		break;
	case 0x13:
		headerType = SHORT_HEADER;
		tmpHeaderShort = reinterpret_cast<SHEADER*>(data);
		messageID = static_cast<MessageID>(tmpHeaderShort->msgID);
		weekNumber = tmpHeaderShort->week;
		milliseconds = tmpHeaderShort->milliseconds;
		messageLength = tmpHeaderShort->msgLength;
		headerLength = 12;//KNOWN VALUE
		break;
	default:
		headerType = UNKNOWN_HEADER;
		break;
	}

	if (headerType != UNKNOWN_HEADER)
	{
		return true;
	}
	else
	{
		return false;
	}
}

//BUILD MESSAGE TO REQUEST LOGGING
bool Novatel::build_command_log_PVAS(char* data)
{
	//char dataH[sizeof(HEADER)];
	//HEADER
	HEADER cH;
	cH.sync1 = 0xAA;
	cH.sync2 = 0x44;
	cH.sync3 = 0x12;
	cH.headerLength = 28;
	cH.msgID = 1; //LOG MESSAGE
	cH.msgType = 0b00000000; //???
	cH.portAddress = 0x17; //ICOM1
	cH.msgLength = 32;
	cH.sequence = 0;
	cH.idleTime = 0x1D;
	cH.timeStatus = unknown;
	cH.week = 0x00;
	cH.milliseconds = 0x00;
	cH.recStatus = 0x00004c00;
	cH.reserved1 = 0x00;
	cH.recSwVer = 0x00;
	//memcpy(dataH, &cH, sizeof(HEADER));

	char dataMsg[60];
	LOG_CMD cmd;
	cmd.header = cH;
	cmd.port = ICOM1_ALL;
	cmd.logID = (unsigned short)INSPVAS_MSG;
	cmd.logType = 0x00;
	cmd.reserved1 = 0x00;
	cmd.logTrigger = ON_TIME;
	cmd.logPeriod = 0.01;
	cmd.offsetTime = 0.0;
	cmd.holdSetting = NO_HOLD;
	memcpy(dataMsg, &cmd, 60);

	memcpy(data, dataMsg, 60);
	
	return true;
}

void Novatel::checkThis(unsigned char *data, unsigned long length)
{
	BESTPOS aPos;
	parse_BESTPOS(data, aPos, length);
	HeaderType ht;
	MessageID msgID;
	unsigned short msgLen;
	unsigned short weekNum;
	long milliseconds;
	unsigned short headerLength;
	bool goodCap = parse_header(data, ht, msgID, msgLen, weekNum, milliseconds, headerLength);
	int a = 1;
}

//USED IN calcCRC
unsigned long crc32Value(int i)
{
	int j;
	unsigned long ulCRC;
	ulCRC = i;
	for (j = 8; j > 0; j--)
	{
		if (ulCRC & 1)
		{
			ulCRC = (ulCRC >> 1) ^ CRC32_POLYNOMIAL;
		}
		else
		{
			ulCRC >>= 1;
		}
	}
	return ulCRC;
}
//USED IN calcCRC
unsigned long calculateBlockCRC32(unsigned long ulCount, unsigned char *ucBuffer)
{
	unsigned long ulTemp1;
	unsigned long ulTemp2;
	unsigned long ulCRC = 0;
	while (ulCount-- != 0)
	{
		ulTemp1 = (ulCRC >> 8) & 0x00FFFFFFL;
		ulTemp2 = crc32Value( ((int)ulCRC ^ *ucBuffer++) & 0xFF);
		ulCRC = ulTemp1 ^ ulTemp2;
	}
	return (ulCRC);
}
//CALCULATE CRC FOR LOG MESSAGE
unsigned long calcCRC(unsigned char* data, unsigned long length)
{
	unsigned long CRCle = calculateBlockCRC32(length - 4, data); //-4, because we are removing the x4 CRC BYTES
	//NEED TO SWAP ENDIAN_NESS?????
	return CRCle;
}