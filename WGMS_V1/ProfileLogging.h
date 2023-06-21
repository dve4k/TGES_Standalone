#pragma once
#include <mutex>
#include <ctime>
#include <vector>
#include <iostream>
#include <fstream>
#include "atlstr.h"
#include "Settings.h"
#include "ImageProcessing.h"

class ProfileLogging
{
	public:
				ProfileLogging();
				ProfileLogging(bool simulation, bool logging, CString fileName1, CString fileName2);
		void	readSimulationLine(int sensorNumber, ProfilePoints &points);
		void	writeProfilePoints(ProfilePoints points);
		void	checkRollFile();

private:
		void	openSimulationFile(CString fileName1, CString fileName2);
		void	openProfileFile(CString fileName1, CString fileName2);
};
