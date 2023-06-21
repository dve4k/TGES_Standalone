#pragma once
#include "EthernetScannerSDK.h"
#include "EthernetScannerSDKDefine.h"
#include "atlstr.h"
#include "windows.h"
#include <opencv/cv.h>
#include <opencv2/opencv.hpp>
#include <opencv2/opencv_modules.hpp>
#include <opencv/cv.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <vector>
#include <iostream>
#include <ctime>
#include <cstdlib>
#include <cudaimgproc.hpp>
#include <thread>
#include <mutex>
#include "ImageProcessing.h"
#include "Settings.h"

struct EdgePoint
{
	double xValue;
	double yValue;
	int origPointIndex;
	double slope;
	double angle;
	int houghIndex;
	double templateCornerX;
	double templateCornerY;
	double templateX1;
	double templateX2;
	double templateY1;
	double templateY2;
	double houghCenterX;
	double houghCenterY;
	double houghRadius;
};
struct TemplatePoints
{
	double xProfilePoint;
	double yProfilePoint;
	double xValue;
	double yValue;
	double leftX;
	double rightX;
	double topX;
	double topY;
	double bottomX;
	double bottomY;
	double rotation;
	double maxScore;
	double secondGrad;
	double angle;
	bool angleMatch;
	bool gradMatch;
	double metricTotal;
};
struct ProfilePoints
{
	double xVals[2048];
	double yVals[2048];
	int intensity[2048];
	int peak[2048];
	unsigned int encoder;
	unsigned char roi;
	int picCounter;
	double slope[2048];
	double angle[2048];
	double secondDervSlope[2048];
	double secondDervAngle[2048];
	int sensorNumber;
	TemplatePoints lastBestTemplatePoint;
	TemplatePoints templatePoint[RAIL_TEMPLATE_COUNT];
	bool foundHalfGage;
	double y5_8;
	double x5_8;
	double torAvgY;
	EdgePoint tor;
	double halfGage;
	int torPointsFound;
};
struct ThreadArgs
{
public:
	//HoughLine houghVertLines[100];
	//HoughLine houghHorizLines[100];
	//HoughCircle houghCircles[100];
	//HoughCircle houghFullCircles[100];
	cv::cuda::GpuMat gpuMat;
	cv::Ptr<cv::cuda::CornersDetector> cornerDetector;
	cv::Ptr<cv::cuda::HoughSegmentDetector> segmentDetector;
	cv::Ptr<cv::cuda::HoughCirclesDetector> circleDetector;
	cv::Ptr<cv::cuda::Filter> filter1;
	cv::Ptr<cv::cuda::Filter> filter2;
	cv::cuda::Stream cudaStream;
	int minXValue;
	int minYValue;
	int maxXValue;
	int maxYValue;
	double scale;
};
struct ThreadArgsProcessor
{
public:
	double scale;
	int minXValue;
	int minYValue;
	int maxXValue;
	int maxYValue;
	cv::Mat matrix;
	TemplatePoints templatePoints[RAIL_TEMPLATE_COUNT];
};
class ImageProcessing
{
public:
	//FUNCTION PROTOTYPES
	ImageProcessing();
	~ImageProcessing();

	//############## MULTI-THREAD REGULAR FUNCTIONS################//
	unsigned			__stdcall analyzeProfile(void* args);
	unsigned			__stdcall templateMatch(void* data);
};

