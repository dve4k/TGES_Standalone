#include "stdafx.h"
#include "Calculation_Simulation.h"
#include "GeometryLogging.h"
#include <mutex>
#include <condition_variable>

//PROTOTYPES
void calcSimBuffer_Put(CoreGeomCalculationPacket p);

//GLOBAL VARIABLES FOR QUEUE
std::mutex calcSimBuffer_mutex;
std::condition_variable calcSimBuffer_condVar;
std::queue<CoreGeomCalculationPacket> calcSimBuffer_queue;

//SIMULATION FILE
std::ifstream calcSimStreamIn;

//SIMULATION HEADER/TIMESTAMP
std::string calcSimHeader_1;

Calculation_Simulation::Calculation_Simulation()
{

}

Calculation_Simulation::~Calculation_Simulation()
{
}

void Calculation_Simulation::startSimulation()
{
	CString fileName = "C:\\TGC\\CalcSim\\Sim1.data";
	calcSimStreamIn = std::ifstream(fileName);
	std::getline(calcSimStreamIn, calcSimHeader_1);
	std::string line_1;

	while (true)
	{
		bool eof = std::getline(calcSimStreamIn, line_1).eof();
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
		toks_1 = std::strtok(aLine_1, ";");
		std::string lineType(toks_1);
		toks_1 = std::strtok(NULL, ";");
		std::string mp_s(toks_1);
		toks_1 = std::strtok(NULL, ";");
		std::string seq_s(toks_1);
		toks_1 = std::strtok(NULL, ";");
		std::string ft_s(toks_1);
		toks_1 = std::strtok(NULL, ";");
		std::string align31_s(toks_1);
		toks_1 = std::strtok(NULL, ";");
		std::string align62_s(toks_1);
		toks_1 = std::strtok(NULL, ";");
		std::string curv_s(toks_1);
		toks_1 = std::strtok(NULL, ";");
		std::string crosslevel_s(toks_1);
		toks_1 = std::strtok(NULL, ";");
		std::string profL11_s(toks_1);
		toks_1 = std::strtok(NULL, ";");
		std::string profR11_s(toks_1);
		toks_1 = std::strtok(NULL, ";");
		std::string profL62_s(toks_1);
		toks_1 = std::strtok(NULL, ";");
		std::string profR62_s(toks_1);
		toks_1 = std::strtok(NULL, ";");
		std::string profL31_s(toks_1);
		toks_1 = std::strtok(NULL, ";");
		std::string profR31_s(toks_1);
		toks_1 = std::strtok(NULL, ";");
		std::string instGrade_s(toks_1);
		toks_1 = std::strtok(NULL, ";");
		std::string twist11_s(toks_1);
		toks_1 = std::strtok(NULL, ";");
		std::string twistMult_s(toks_1);
		toks_1 = std::strtok(NULL, ";");
		std::string avgGrade_s(toks_1);
		toks_1 = std::strtok(NULL, ";");
		std::string vertBounce_s(toks_1);
		toks_1 = std::strtok(NULL, ";");
		std::string combLeft_s(toks_1);
		toks_1 = std::strtok(NULL, ";");
		std::string combRight_s(toks_1);
		toks_1 = std::strtok(NULL, ";");
		std::string runoff_s(toks_1);
		toks_1 = std::strtok(NULL, ";");
		std::string gage_s(toks_1);
		toks_1 = std::strtok(NULL, ";");
		std::string varCL_s(toks_1);
		toks_1 = std::strtok(NULL, ";");
		std::string varCurv_s(toks_1);
		toks_1 = std::strtok(NULL, ";");
		std::string warp62_s(toks_1);


		delete[] aLine_1;

		CoreGeomCalculationPacket packet = CoreGeomCalculationPacket();
		packet.L_HORIZ_MCO_31_NOC = std::stod(align31_s);
		packet.R_HORIZ_MCO_31_NOC = std::stod(align31_s);
		packet.L_HORIZ_MCO_62_NOC = std::stod(align62_s);
		packet.R_HORIZ_MCO_62_NOC = std::stod(align62_s);
		//packet.filteredCurvature = std::stod(curv_s);
		packet.rawCurvature = std::stod(curv_s);
		packet.crosslevel_NOC = std::stod(crosslevel_s);
		packet.L_VERT_MCO_11_NOC = std::stod(profL11_s);
		packet.R_VERT_MCO_11_NOC = std::stod(profR11_s);
		packet.L_VERT_MCO_31_NOC = std::stod(profL31_s);
		packet.R_VERT_MCO_31_NOC = std::stod(profR31_s);
		packet.L_VERT_MCO_62_NOC = std::stod(profL62_s);
		packet.R_VERT_MCO_62_NOC = std::stod(profR62_s);
		packet.grade_raw = std::stod(instGrade_s);
		packet.fullGage_raw = std::stod(gage_s);
		packet.location.MP = std::stoi(mp_s);
		packet.location.FTCnt = std::stoi(ft_s);
		packet.location.mpSequence = 'U';
		packet.location.RID[0] = '1';
		packet.location.trackNumber = 1;
		packet.location.carOrientation = 'F';
		packet.altitude = 0;
		packet.latitude = 0;
		packet.longitude = 0;

		//calc.putRawCalcPacket_512Buffer(packet);
		calcSimBuffer_Put(packet);
		Sleep(10);

	}
}
void calcSimBuffer_Put(CoreGeomCalculationPacket p)
{
	calcSimBuffer_mutex.lock();
	calcSimBuffer_queue.push(p);
	calcSimBuffer_mutex.unlock();
	calcSimBuffer_condVar.notify_one();
}

bool Calculation_Simulation::calcSimBuffer_Get(CoreGeomCalculationPacket &packet)
{
	std::unique_lock<std::mutex> l(calcSimBuffer_mutex);
	//calcSimBuffer_condVar.wait(l, [] { return calcSimBuffer_queue.size() > 0; });
	calcSimBuffer_condVar.wait_for(l, std::chrono::milliseconds(100), [] {return calcSimBuffer_queue.size() > 0; });
	int bufferSize = calcSimBuffer_queue.size();

	if (bufferSize > 0)
	{
		packet = calcSimBuffer_queue.front();
		calcSimBuffer_queue.pop();
		l.unlock();

		return true;
	}
	l.unlock();
	return false;
}


