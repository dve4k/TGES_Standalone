#pragma once
#include "resource.h"
#include "windows.h"
#include "EthernetScannerSDK.h"
#include "EthernetScannerSDKDefine.h"
#include <mutex>
#include <ctime>
#include <chrono>
#include <vector>
#include <iostream>
#include <fstream>
#include "atlstr.h"
#include "Settings.h"
#include "ImageProcessing.h"
#include "ProfileLogging.h"
#include "NetworkInterface.h"
#include "stdio.h"
#include "stdlib.h"
#include "Applanix.h"
#include "ApplanixConnectionClient.h"
#include "CoreGeometry_A.h"
#include "GeoCalculations.h"
#include "GeometryLogging.h"
#include "Calculation_Simulation.h"
#include "GageSystem.h"
#include "InertialSystem.h"
#include "NI_UDP_Interface.h"
#include "BackOfficeUDP.h"
#include <process.h>

using namespace std;
using namespace chrono;