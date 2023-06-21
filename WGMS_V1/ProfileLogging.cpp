#include "stdafx.h"
#include "ProfileLogging.h"

//###---GLOBALS---###
//OUTPUT FILE
std::ofstream fileStream1;
std::ofstream fileStream2;

//SIMULATION FILE
std::ifstream inStream_1;
std::ifstream inStream_2;

//SIMULATION HEADER/TIMESTAMP
std::string header_1;
std::string header_2;
std::string timeStamp_1;
std::string timeStamp_2;

//TRACKING FILE SIZE
long fs1Size = 0;
long fs2Size = 0;

bool simluationMode = false;
bool loggingMode = false;

//SETUP THE SIMLUATION OBJECT
ProfileLogging::ProfileLogging(bool simulation, bool logging, CString fileName1, CString fileName2)
{
	if (simulation)
	{
		openSimulationFile(fileName1, fileName2);
		simluationMode = true;
	}
	else
	{
		if (logging)
		{
			openProfileFile(fileName1, fileName2);
			loggingMode = true;
		}
	}
}

//DEFAULT / EMPTY CONSTRUCTOR
ProfileLogging::ProfileLogging()
{

}

//OPEN FILE TO WRTIE
void ProfileLogging::openProfileFile(CString fileName1, CString fileName2)
{
	fs1Size = 0;
	fs2Size = 0;

	fileStream1.open(fileName1);
	fileStream2.open(fileName2);
	//GET DATE-TIME
	// current date/time based on current system
	time_t now = time(0);

	// convert now to string form
	//char* dt = ctime(&now);
	char dt[26];
	ctime_s(dt, sizeof(dt), &now);
	fileStream1 << dt + '\n';
	fileStream2 << dt + '\n';
	fileStream1 << "PIC_CNT,TOR,BASE";
	fileStream2 << "PIC_CNT,TOR,BASE";

	for (int i = 0; i < 2048; i++)
	{
		fileStream1 << ',';
		fileStream2 << ',';
		fileStream1 << std::to_string(i);
		fileStream2 << std::to_string(i);
	}
}

//WRITE TO LOG FILE
void  ProfileLogging::writeProfilePoints(ProfilePoints points)
{
	//BUILD A STRING TO WRITE
	if (points.picCounter > -1)
	{
		std::string	aLine = "";
		aLine += '\n';
		aLine += std::to_string(points.picCounter);
		aLine += ',' + std::to_string(0) + ';' + std::to_string(0);
		aLine += ',' + std::to_string(0) + ';' + std::to_string(0);
		for (int i = 0; i < 2048; i++)
		{
			//aLine += ',' + std::to_string(points.xVals[i]) + ';' + std::to_string(points.yVals[i]);
			char tLine[50];
			sprintf(tLine, ",%f;%f", points.xVals[i], points.yVals[i]);
			aLine += tLine;
		}
		if (points.sensorNumber == 1)
		{
			fileStream1 << aLine;
			fs1Size = sizeof(aLine) + fs1Size;
		}
		else
		{
			if (points.sensorNumber == 2)
			{
				fileStream2 << aLine;
				fs2Size = sizeof(aLine) + fs2Size;
			}
		}
	}
}

//OPEN SIMULATION FILE
void ProfileLogging::openSimulationFile(CString fileName1, CString fileName2)
{
	//OPEN THE SIMULATION FILE
	inStream_1 = std::ifstream(SIMULATION_FILE_NAME_1);
	inStream_2 = std::ifstream(SIMULATION_FILE_NAME_2);
	//READ TIME
	std::getline(inStream_1, timeStamp_1);
	std::getline(inStream_2, timeStamp_2);
	//READ HEADER
	std::getline(inStream_1, header_1);
	std::getline(inStream_2, header_2);
}

//READ FROM SIMULATION LINE
void ProfileLogging::readSimulationLine(int sensorNumber, ProfilePoints &points)
{
	Sleep(5);
	if (sensorNumber == 1)
	{
		//READING IN SENSOR #1
		//READ A PROFILE LINE SET
		std::string line_1;
		bool eof = std::getline(inStream_1, line_1).eof();
		if (eof)
		{
			return;
		}
		//FORMAT IS
		char* aLine_1 = new char[line_1.length() + 1];
		char* toks_1;
		char* miniTok_1;
		strcpy(aLine_1, line_1.c_str());

		//TOKENIZE THE LINE
		toks_1 = std::strtok(aLine_1, ",");

		char* picCnt_1(toks_1);
		points.picCounter = std::stoi(picCnt_1, 0);
		toks_1 = std::strtok(NULL, ",");
		std::string torPoint_1(toks_1);
		toks_1 = std::strtok(NULL, ",");
		std::string baseWebPoint_1(toks_1);
		char* coords_1[2048];

		for (int i = 0; i < 2048; i++)
		{
			toks_1 = std::strtok(NULL, ",");
			coords_1[i] = toks_1;
		}

		for (int i = 0; i < 2048; i++)
		{
			miniTok_1 = std::strtok(coords_1[i], ";");
			points.xVals[i] = std::stod(miniTok_1, 0);
			miniTok_1 = std::strtok(NULL, "\n");
			points.yVals[i] = std::stod(miniTok_1, 0);
		}
		delete[] aLine_1;


	}
	else
	{
		if (sensorNumber == 2)
		{
			//READING IN SENSOR #2
			std::string line_2;
			std::getline(inStream_2, line_2);
			char* aLine_2 = new char[line_2.length() + 1];
			char* toks_2;
			char* miniTok_2;
			strcpy(aLine_2, line_2.c_str());
			//TOKENIZE THE LINE
			toks_2 = std::strtok(aLine_2, ",");

			char* picCnt_2(toks_2);
			points.picCounter = std::stoi(picCnt_2, 0);
			toks_2 = std::strtok(NULL, ",");
			std::string torPoint_2(toks_2);
			toks_2 = std::strtok(NULL, ",");
			std::string baseWebPoint_2(toks_2);
			char* coords_2[2048];

			for (int i = 0; i < 2048; i++)
			{
				toks_2 = std::strtok(NULL, ",");
				coords_2[i] = toks_2;
			}

			for (int i = 0; i < 2048; i++)
			{
				miniTok_2 = std::strtok(coords_2[i], ";");
				points.xVals[i] = std::stod(miniTok_2, 0);
				miniTok_2 = std::strtok(NULL, "\n");
				points.yVals[i] = std::stod(miniTok_2, 0);
			}
			delete[] aLine_2;
		}
	}
}

//CHECK THE FILE SIZE
void ProfileLogging::checkRollFile()
{
	if (fs1Size > 500000 || fs2Size > 500000) //IN KB? 500,000KB = 500MB
	{
		//ROLL TO A NEW FILE
		//GET CURRENT TIME
		std::chrono::time_point<std::chrono::system_clock> timePoint = std::chrono::system_clock::now();
		time_t now = time(0);
		struct tm * timeinfo;
		timeinfo = localtime(&now);
		char dt[80];// = ctime(&now);
		strftime(dt, 80, "%G%m%d%H%M%S", timeinfo);
		CString cstringDt1 = LOG_DIRECTORY + CString(dt) + "_1.csv";
		CString cstringDt2 = LOG_DIRECTORY + CString(dt) + "_2.csv";

		//CLOSE THE EXISTING FILES
		fileStream1.close();
		fileStream2.close();

		//OPEN NEW FILES
		openProfileFile(cstringDt1, cstringDt2);
	}
	
}



