#include "stdafx.h"
#include "ImageProcessing.h"
//#pragma optimize("", off)

unsigned			__stdcall mtProcessNumericData(void* data);
void				rotateXYPoints(double *xVals, double *yVals, double calibrationAngle, int capturedPoints, double *xRotated, double *yRotated);
void				calculateSlope(double *xPoints, double *yPoints, double *slopeVals, double *angleVals);
void				smoothPoints(double *xPoints, double *yPoints, double *smoothedXPoints, double *smoothedYPoints);
int					findTORv2(double *xPoints, double *yPoints, TemplatePoints *templates, double *firstAngles, double *secondSlopes, TemplatePoints &lastBestPoint, EdgePoint &tmpTOR, bool &angleMatch, bool &gradMatch);
void				calcTorYValue(double *xPoints, double *yPoints, EdgePoint &torPoint, double &avgTorY, double &avgFiveEightY);
void				calcFiveEightXValue(double *xPoints, double *yPoints, EdgePoint &torPoint, double avgFiveEightY, double &avgFiveEightX);
double				wjeAbs(double aNum);
double				maxVal(double *nums, int length);
double				minVal(double *nums, int length);
TemplatePoints		weightTorPointsV2(TemplatePoints *templateMatches, TemplatePoints &lastBestPoint);
double				avgVal(double *nums, int length);
double				calcDistanceBetweenPoints(double x1, double y1, double x2, double y2);
double				calcAngleBetweenPoints(double x1, double y1, double x2, double y2);
void				importRailTemplates();


//GLOBAL VARIABLES
double cv2Sensor1Scale = 2.0;
double cv2Sensor2Scale = 2.0;
//TEMPLATE ARGUEMNTS -- PROCESSOR
ThreadArgsProcessor templateArgs_1;
ThreadArgsProcessor templateArgs_2;
cv::Mat railTemplates[RAIL_TEMPLATE_COUNT];

ImageProcessing::ImageProcessing()
{
	double image_size_x_1 = CV2_IMAGE_SIZE_X_1;
	double image_size_y_1 = CV2_IMAGE_SIZE_Y_1;
	double image_max_x_1 = CV2_IMAGE_MAX_X_1;
	double image_max_y_1 = CV2_IMAGE_MAX_Y_1;
	double image_min_x_1 = CV2_IMAGE_MIN_X_1;
	double image_min_y_1 = CV2_IMAGE_MIN_Y_1;
	double s1XScale = (image_size_x_1 - 1) / (image_max_x_1 - image_min_x_1);
	double s1YScale = (image_size_y_1 - 1) / (image_max_y_1 - image_min_y_1);
	if (s1XScale < s1YScale)
	{
		cv2Sensor1Scale = s1XScale;
	}
	else
	{
		cv2Sensor1Scale = s1YScale;
	}
	//SENSOR 2
	double image_size_x_2 = CV2_IMAGE_SIZE_X_2;
	double image_size_y_2 = CV2_IMAGE_SIZE_Y_2;
	double image_max_x_2 = CV2_IMAGE_MAX_X_2;
	double image_max_y_2 = CV2_IMAGE_MAX_Y_2;
	double image_min_x_2 = CV2_IMAGE_MIN_X_2;
	double image_min_y_2 = CV2_IMAGE_MIN_Y_2;
	double s2XScale = (image_size_x_2 - 1) / (image_max_x_2 - image_min_x_2);
	double s2YScale = (image_size_y_2 - 1) / (image_max_y_2 - image_min_y_2);
	if (s2XScale < s2YScale)
	{
		cv2Sensor2Scale = s2XScale;
	}
	else
	{
		cv2Sensor2Scale = s2YScale;
	}

	//IMPORT RAIL HEAD TEMPLATES
	importRailTemplates();
}


ImageProcessing::~ImageProcessing()
{
}

// ROTATES THE IMAGE BY THE CALIBRATION CORRECTION ANGLE
void rotateXYPoints(double *xVals, double *yVals, double calibrationAngle, int capturedPoints, double *xRotated, double *yRotated)
{
	//ROTATE IMAGE
	for (int i = 0; i < 2048; i++)
	{
		double calTheta = (double)calibrationAngle * 3.14159265 / 180;//NEED RADIANS
		double resTheta = calTheta - 3.14159264 / 2;
		double cosTheta = cos(resTheta);
		double sinTheta = sin(resTheta);
		double tmpX = xVals[i];
		double tmpY = yVals[i];
		xRotated[i] = tmpX * cosTheta + tmpY * sinTheta;
		yRotated[i] = -1.0 * (tmpY * cosTheta - tmpX * sinTheta);
	}

}

//CALCULATE SLOPE OVER 100 POINTS
void calculateSlope(double *xPoints, double *yPoints, double *slopeVals, double *angleVals)
{
	int range = 3;
	int offsetSide = 10;
	for (int j = offsetSide; j < 2048 - offsetSide; j++)
	{
		double x1 = xPoints[j - offsetSide];
		double x2 = xPoints[j + offsetSide];
		double y1 = yPoints[j - offsetSide];
		double y2 = yPoints[j + offsetSide];
		slopeVals[j] = (y2 - y1) / (x2 - x1);
		angleVals[j] = atan(slopeVals[j]) * 180 / CV_PI;
	}
}

//MULTI THREAD PROCESSOR::TEMPLATE MATCHING
unsigned __stdcall ImageProcessing::templateMatch(void* data)
{

	int matchMethod = CV_TM_CCORR;
	ThreadArgsProcessor *args = (ThreadArgsProcessor*)data;
	cv::Mat profileMat = args->matrix;
	double scale = args->scale;

	for (int i = 0; i < /*RAIL_TEMPLATE_COUNT*/1; i++)
	{
		int result_cols = profileMat.cols - railTemplates[i].cols + 1;
		int result_rows = profileMat.rows - railTemplates[i].rows + 1;
		cv::Mat result;
		result.create(result_rows, result_cols, CV_32FC1);
		cv::matchTemplate(profileMat, railTemplates[i], result, matchMethod);
		cv::normalize(result, result, 0, 1, cv::NORM_MINMAX, -1, cv::Mat());
		double minVal;
		double maxVal;
		cv::Point minLoc;
		cv::Point maxLoc;
		cv::Point matchLoc;
		cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, cv::Mat());
		if (matchMethod == CV_TM_SQDIFF || matchMethod == CV_TM_SQDIFF_NORMED)
		{
			matchLoc = minLoc;
		}
		else
		{
			matchLoc = maxLoc;
		}
		//args->templatePoints[i].xProfilePoint = matchLoc.x / scale + args->minXValue;
		//args->templatePoints[i].yProfilePoint = matchLoc.y / scale + args->minYValue;
		args->templatePoints[i].rightX = (matchLoc.x + railTemplates[i].cols) / scale + args->minXValue;
		args->templatePoints[i].bottomY = (matchLoc.y + railTemplates[i].rows) / scale + args->minYValue;
		args->templatePoints[i].topY = matchLoc.y / scale + args->minYValue;
		args->templatePoints[i].leftX = matchLoc.x / scale + args->minXValue;
	}

	return 1;
}

//IMPORT THE RAIL TEMPLATES USED FOR ANALYSIS
void importRailTemplates()
{
	for (int i = 0; i < RAIL_TEMPLATE_COUNT; i++)
	{
		char fName[80];
		sprintf(fName, "C:/TGC/TEMPLATES/%d.jpg", i);
		railTemplates[i] = cv::imread(fName, cv::IMREAD_GRAYSCALE);
	}
}

//FIND THE LOCATION OF TOP-OF-RAIL
int findTORv2(double *xPoints, double *yPoints, TemplatePoints *templates, double *firstAngles, double *secondSlopes,
	TemplatePoints &lastBestPoint, EdgePoint &tmpTOR, bool &angleMatch, bool &gradMatch)
{
	EdgePoint localTOR;
	EdgePoint multiTOR[RAIL_TEMPLATE_COUNT];
	int torCnt = 0;
	bool tmpAngleMatch = false;
	bool tmpGradMatch = false;
	for (int i = 0; i < RAIL_TEMPLATE_COUNT; i++)
	{
		bool templateHasGradient = false;
		bool templateHasAngle = false;

		int matchIndex[2048] = { -999 };
		int matchIndexCnt = 0;

		double matchSlope[2048] = { -999 };
		double matchSlopeDist[2048] = { -999 };
		int matchSlopeCnt = 0;

		double matchAngle[2048] = { -999 };
		double matchAngleDist[2048] = { -999 };
		int matchAngleCnt = 0;

		if (templates[i].leftX > 0)
		{
			double templateX1 = templates[i].leftX;
			double templateX2 = templates[i].rightX;
			double templateY1 = templates[i].topY;
			double templateY2 = templates[i].bottomY;
			double approxCorner_x = templateX1 + 10;
			double approxCorner_y = templateY1 + 10;
			templates[i].xProfilePoint = approxCorner_x;
			templates[i].yProfilePoint = approxCorner_y;
			for (int j = 0; j < 2048; j++)
			{
				if (xPoints[j] >= approxCorner_x - TOR_CIRCLE_MATCH_RADIUS
					&& xPoints[j] <= approxCorner_x + TOR_CIRCLE_MATCH_RADIUS
					&& yPoints[j] * -1.0 >= approxCorner_y - TOR_CIRCLE_MATCH_RADIUS
					&& yPoints[j] * -1.0 <= approxCorner_y + TOR_CIRCLE_MATCH_RADIUS)
				{
					//matchIndex.push_back(j);
					matchIndex[matchIndexCnt] = j;
					matchIndexCnt++;

					//IDENTIFY TOP-OF-RAIL POINT
					if (secondSlopes[j] <= -60.0 && secondSlopes[j] >= -100.0) //-60 to -100
					{
						templateHasGradient = true;
						tmpGradMatch = true;
						////matchSlope.push_back(secondSlopes[j]);
						matchSlope[matchSlopeCnt] = secondSlopes[j];
						matchSlopeDist[matchSlopeCnt] = calcDistanceBetweenPoints(xPoints[j], yPoints[j], approxCorner_x, approxCorner_y);
						matchSlopeCnt++;

					}

					if (firstAngles[j] >= 20.0 && firstAngles[j] <= 80.0) //20.0 to 80.0
					{
						////matchAngle.push_back(firstAngles[j]);
						matchAngle[matchAngleCnt] = firstAngles[j];
						matchAngleDist[matchAngleCnt] = calcDistanceBetweenPoints(xPoints[j], yPoints[j], approxCorner_x, approxCorner_y);
						matchAngleCnt++;
						templateHasAngle = true;
						tmpAngleMatch = true;
					}
					//MAY HAVE TO REMOVE!!!!!!!!WJE
					templateHasGradient = true;
					tmpGradMatch = true;
					templateHasAngle = true;
					tmpAngleMatch = true;

				}
			}

			//THIS WAS IN IF STATEMENT BELOW w/ all the && circle has gradient....
			int numMatches = matchIndexCnt;
			int midMatches = matchIndex[(int)floor(((double)numMatches) / 2.0)];

			//FOR METRICS PIECE -- 6/1/2018 TESTING

			templates[i].xProfilePoint = xPoints[midMatches];
			templates[i].yProfilePoint = yPoints[midMatches];
			templates[i].angle = avgVal(matchAngle, matchAngleCnt);
			templates[i].secondGrad = avgVal(matchSlope, matchSlopeCnt);

			if (templateHasAngle && templateHasGradient)
			{
				//FIND A CORRESPONDING TOR POINT FOR THIS CIRCLE -- LOOK AT TOP-LEFT CORNER
				//MIND MEDIAN VALUE OF MATCH INDEX

				multiTOR[torCnt].origPointIndex = midMatches;
				multiTOR[torCnt].slope = maxVal(matchSlope, matchSlopeCnt);
				multiTOR[torCnt].angle = maxVal(matchAngle, matchAngleCnt);
				multiTOR[torCnt].xValue = xPoints[midMatches];
				multiTOR[torCnt].yValue = yPoints[midMatches];
				multiTOR[torCnt].templateCornerX = approxCorner_x;
				multiTOR[torCnt].templateCornerY = approxCorner_y;

				localTOR = multiTOR[torCnt];

				torCnt = torCnt + 1;
			}
		}
	}
	if (torCnt > 1)
	{
		//FIND THE TOR WITH HIGHEST Y VALUE
		double maxY = -999.0;
		int maxYIndex = -99;
		for (int i = 0; i < torCnt; i++)
		{
			if (maxY < multiTOR[i].yValue)
			{
				maxY = multiTOR[i].yValue;
				maxYIndex = i;
			}
		}
		localTOR = multiTOR[maxYIndex];
		localTOR.templateCornerX = multiTOR[maxYIndex].templateCornerX;
	}
	////NEED TO CLEAN UP CODE BELOW -- NOT ASSIGNING ALL NEEDED VALUES?? XValue, YValue, EXT -- 6/1/2018
	TemplatePoints bestTemplate = weightTorPointsV2(templates, lastBestPoint);
	localTOR.houghCenterX = bestTemplate.xValue;
	localTOR.houghCenterY = bestTemplate.yValue;
	localTOR.xValue = bestTemplate.xProfilePoint;
	localTOR.yValue = bestTemplate.yProfilePoint;
	angleMatch = bestTemplate.angleMatch;
	gradMatch = bestTemplate.gradMatch;
	tmpTOR = localTOR;
	return torCnt;
}

//SMOOTH POINTS
void smoothPoints(double *xPoints, double *yPoints, double *smoothedXPoints, double *smoothedYPoints)
{
	for (int i = HALF_SMOOTHING_WINDOW; i < 2048 - HALF_SMOOTHING_WINDOW; i++)
	{
		double countX = 0;
		double countY = 0;
		double sumX = 0;
		double sumY = 0;
		for (int j = i - HALF_SMOOTHING_WINDOW; j < i + HALF_SMOOTHING_WINDOW; j++)
		{
			sumX += xPoints[j];
			sumY += yPoints[j];
			countX = countX + 1.0;
			countY = countY + 1.0;
		}
		double deltaX = wjeAbs(xPoints[i - HALF_SMOOTHING_WINDOW] - xPoints[i + HALF_SMOOTHING_WINDOW]);
		double deltaY = wjeAbs(yPoints[i - HALF_SMOOTHING_WINDOW] - yPoints[i + HALF_SMOOTHING_WINDOW]);
		if (deltaX < SMOOTHING_MAX_JUMP && deltaY < SMOOTHING_MAX_JUMP)
		{
			smoothedXPoints[i] = sumX / countX;
			smoothedYPoints[i] = sumY / countY;
		}
		else
		{
			smoothedXPoints[i] = xPoints[i];
			smoothedYPoints[i] = yPoints[i];
		}
	}
}

//CALCULATE LINEAR DISTANCE BETWEEN TWO X-Y COORDINATES
double calcDistanceBetweenPoints(double x1, double y1, double x2, double y2)
{
	double deltaX = x2 - x1;
	double deltaY = y2 - y1;
	if (deltaX < 0)
	{
		deltaX = deltaX * -1.0;
	}
	if (deltaY < 0)
	{
		deltaY = deltaY * -1.0;
	}
	double answer = sqrt(deltaX * deltaX + deltaY * deltaY);
	return answer;
}

//CALCULATE ANGLE (IN DEGREES) BETWEEN TWO X-Y COORDINATES
double calcAngleBetweenPoints(double x1, double y1, double x2, double y2)
{
	double deltaX = x2 - x1;
	double deltaY = y2 - y1;
	//tan(theta) = deltaY / deltaX
	double angle = atan(deltaY / deltaX) * (180 / CV_PI);
	return angle;
}

//CALCULATE AVERAGE Y-VALUE FOR TOP-OF-RAIL
void calcTorYValue(double *xPoints, double *yPoints, EdgePoint &torPoint, double &avgTorY, double &avgFiveEightY)
{
	////DETERMINE MEAN Y-VALUE; +/-39mm FROM TOR POINT
	std::vector<double> torYVals;
	double sum = 0;
	double cnt = 0;

	//EXPAND SEARCH AREA -- COUNT = 0
	if (cnt == 0)
	{
		for (int i = 0; i < 2048; i++)
		{
			if (xPoints[i] >= torPoint.xValue + 5 && xPoints[i] <= torPoint.xValue + 70.0
				&& yPoints[i] >= torPoint.yValue - 30 && yPoints[i] <= torPoint.yValue + 20)
			{
				cnt = cnt + 1;
				sum = sum + yPoints[i];
				torYVals.push_back(yPoints[i]);
			}
		}
	}

	std::sort(torYVals.begin(), torYVals.end());
	int middleIndex = (int)floor((int)torYVals.size() / 2.0);

	double avgTopRail_Y = sum / cnt;//MEAN -- IN CASE THE MEDIAN DOESN'T WORK
	if (middleIndex > 0)
	{
		double median = torYVals.at(middleIndex);
		avgTorY = median; // OVERWRITE AVERAGE WITH THE MEDIAN
	}
	double avgVal = avgTopRail_Y - 15.8;
	if (_isnan(avgVal))
	{
		int viperlead = 1;
	}
	avgFiveEightY = avgVal;
}

//CALCULATE AVERAGE X-VALUE FOR GAGE FACE 5/8"
void calcFiveEightXValue(double *xPoints, double *yPoints, EdgePoint &torPoint, double avgFiveEightY, double &avgFiveEightX)
{
	int cnt = 0;
	double sum = 0;
	for (int i = 0; i < 2048; i++)
	{
		if (xPoints[i] <= torPoint.xValue + TOR_CIRCLE_MATCH_RADIUS
			&& xPoints[i] >= torPoint.xValue - TOR_CIRCLE_MATCH_RADIUS * 2
			&& yPoints[i] >= avgFiveEightY - 1.58 && yPoints[i] <= avgFiveEightY + 1.58)//+/- 1/16"=1.58
		{
			cnt = cnt + 1;
			sum = sum + xPoints[i];
		}
	}

	if (cnt == 0)
	{
		//GO DOWN THE FACE OF RAIL B/C OF LIP MOST LIKELY -- - 
		for (int i = 0; i < 2048; i++)
		{
			if (xPoints[i] <= torPoint.xValue + TOR_CIRCLE_MATCH_RADIUS //MAYBE INCREASE THIS TOO?????
				&& xPoints[i] >= torPoint.xValue - TOR_CIRCLE_MATCH_RADIUS * 2
				&& yPoints[i] >= avgFiveEightY - 1.58*5.0 && yPoints[i] <= avgFiveEightY + 1.58*3)//+/- 1/16"=1.58   //DOES THIS NEED TO BE A SMALLER OR LARGER NUMBER TO MOVE DOWN RAIL???
			{
				cnt = cnt + 1;
				sum = sum + xPoints[i];
			}
		}
	}
	double avgValue = sum / cnt;

	//if (_isnan(avgValue))
	//{
	//	//PROBLEM
	//	int problem = 1;
	//}
	avgFiveEightX = avgValue;
}

//CALCULATE SLOPE/ANGLE/GRADIENT EXT
unsigned __stdcall mtProcessNumericData(void* data)
{
	ProfilePoints *rotatedPoints = (ProfilePoints*)data;

	//CALCULATE 1st Derivative -- SLOPES/ANGLES BETWEEN POINTS
	calculateSlope(rotatedPoints->xVals, rotatedPoints->yVals, rotatedPoints->slope, rotatedPoints->angle);
	//SMOOTH THE CALCULATED ANGLES
	double dummyCntVals1[2048];
	double dummyCntVals2[2048];
	double dummyAngleVals[2048];
	for (int i = 0; i < 2048; i++)
	{
		dummyCntVals1[i] = i;
	}
	smoothPoints(dummyCntVals1, rotatedPoints->angle, dummyCntVals2, dummyAngleVals);
	//COPY ANGLES INTO ROTATED POINTS STRUCT
	memcpy(rotatedPoints->angle, dummyAngleVals, sizeof(dummyAngleVals));
	//INCREASE AMPLITUDE OF ANGLE BY 100
	double ampdAngles[2048];
	double dummyXs[2048];
	for (int i = 0; i < 2048; i++)
	{
		dummyXs[i] = (double)i;
		ampdAngles[i] = rotatedPoints->angle[i] * 100.0;
	}

	//CALCULATE 2nd Derivative --  DERIVATIVE -- CHANGE IN ANGLES
	calculateSlope(dummyXs, ampdAngles, rotatedPoints->secondDervSlope, rotatedPoints->secondDervAngle);

	//SMOOTH THE CALCULATED GRADIENT
	double dummyCntVals3[2048];
	double dummy2ndDervs[2048];
	smoothPoints(dummyCntVals1, rotatedPoints->secondDervSlope, dummyCntVals3, dummy2ndDervs);
	memcpy(rotatedPoints->secondDervSlope, dummy2ndDervs, sizeof(dummy2ndDervs));

	return 1;
}

//ABSOLUTE VALUE FUNCTION -- WILLIAMS FAST VERSION
double wjeAbs(double aNum)
{
	double result;

	if (aNum < 0)
		result = aNum * -1.0;
	else
		result = aNum;

	return result;
}

//MAX VALUE FUNCTION -- WILLIAMS
double maxVal(double *nums, int length)
{
	double max = nums[0];
	for (int i = 1; i < length; i++)
	{
		if (nums[i] > max)
			max = nums[i];
	}
	return max;
}

//MIN VALUE FUNCTION -- WILLIAMS
double minVal(double *nums, int length)
{
	double min = nums[0];
	for (int i = 1; i < length; i++)
	{
		if (nums[i] < min)
			min = nums[i];
	}
	return min;
}

//AVG VALUE FUNCTION -- WILLIAMS
double avgVal(double *nums, int length)
{
	double sum = 0;
	double cnt = 0;
	for (int i = 1; i < length; i++)
	{
		sum += nums[i];
		cnt = cnt + 1;
	}
	double avg = sum / cnt;
	return avg;
}

//USE WEIGHTING SYSTEM TO DETERMINE TOR POINT
TemplatePoints weightTorPointsV2(TemplatePoints *templateMatches, TemplatePoints &lastBestPoint)
{
	//WEIGHTING FACTORS
	double angleWeight = 25.0;
	double gradWeight = 25.0;
	double horzWeight = 25.0;
	double vertWeight = 15.0;
	double circWeight = 10.0;
	double yWeight = 10.0;
	double fblWeight = 5.0;
	double deductionFbWeight = 5.0;

	int bestIndex = 0;
	double bestMetric = 0;

	for (int i = 0; i < RAIL_TEMPLATE_COUNT; i++)
	{
		if (templateMatches[i].xValue > 0)
		{
			double metric = 0;
			if (templateMatches[i].angleMatch)
			{
				metric += 0.50 * angleWeight * wjeAbs(templateMatches[i].angle - IDEAL_CORNER_ANGLE) / 30;
				metric += .50 * angleWeight;
			}
			if (templateMatches[i].gradMatch)
			{
				metric += 0.50 * gradWeight * wjeAbs(templateMatches[i].secondGrad - IDEAL_CORNER_GRADIENT) / 20;
				metric += 0.50 * angleWeight;
			}

			templateMatches[i].metricTotal = metric;
			if (metric > bestMetric)
			{
				bestMetric = metric;
				bestIndex = i;
			}
		}
	}

	//CHANGE THE FEEDBACK LOOP POINT
	lastBestPoint = templateMatches[bestIndex];

	return templateMatches[bestIndex];
}

//ANALYZE PROFILES
unsigned __stdcall ImageProcessing::analyzeProfile(void* args)
{
	//UNPACK ARGUMENTS
	ProfilePoints points = *(ProfilePoints*)args;
	HANDLE analyzeThreads[3];

	unsigned numericThreadID;
	ProfilePoints rotatedPoints;
	rotatedPoints.sensorNumber = points.sensorNumber;

	//CREATE ARRAY FOR CV2 MAT FILES
	double scale = 1;

	int imageMinX = 0;
	int imageMinY = 0;
	int imageMaxX = 500;
	int imageMaxY = 500;
	int cvImageSizeX = 0;
	int cvImageSizeY = 0;
	bool flip = false;
	double rotationAngle = 0;
	switch (points.sensorNumber)
	{
	case 1:
		imageMinX = CV2_IMAGE_MIN_X_1;
		imageMaxX = CV2_IMAGE_MAX_X_1;
		imageMinY = CV2_IMAGE_MIN_Y_1;
		imageMaxY = CV2_IMAGE_MAX_Y_1;
		scale = cv2Sensor1Scale;
		flip = FLIP_IMAGE_1;
		rotationAngle = CALIBRATION_ANGLE_1;
		break;

	case 2:
		imageMinX = CV2_IMAGE_MIN_X_2;
		imageMaxX = CV2_IMAGE_MAX_X_2;
		imageMinY = CV2_IMAGE_MIN_Y_2;
		imageMaxY = CV2_IMAGE_MAX_Y_2;
		scale = cv2Sensor2Scale;
		flip = FLIP_IMAGE_2;
		rotationAngle = CALIBRATION_ANGLE_2;
		break;
	}


	//FLIP THE POINTS IF REQUIRED
	if (flip)
	{
		double tmpXVals2[2048];
		double tmpYVals2[2048];
		for (int i = 0; i < 2048; i++)
		{
			tmpYVals2[2047 - i] = points.yVals[i] * -1.0;
			tmpXVals2[2047 - i] = points.xVals[i];
		}
		for (int i = 0; i < 2048; i++)
		{
			points.xVals[i] = tmpXVals2[i];
			points.yVals[i] = tmpYVals2[i];
		}
	}

	//ROTATE THE POINTS DEPENDING ON CALIBRATION ANGLE
	rotateXYPoints(points.xVals, points.yVals, rotationAngle, 2048, rotatedPoints.xVals, rotatedPoints.yVals);


	//CALCULATE SLOPE - ANGLE - GRADIENT - ECT
	//ProfilePoints rotatedpointArg = points;
	analyzeThreads[0] = (HANDLE)_beginthreadex(NULL, 0, &mtProcessNumericData, &rotatedPoints, NULL, &numericThreadID);

	uint8_t tmpPic1[CV2_IMAGE_SIZE_Y_1][CV2_IMAGE_SIZE_X_1] = { 0 };

	for (int i = 0; i < 2048; i++)
	{
		int tmpX = 0;
		if (rotatedPoints.xVals[i] > imageMinX && rotatedPoints.xVals[i] < imageMaxX && rotatedPoints.xVals[i] > imageMinX)
		{
			tmpX = (int)floor(scale * (rotatedPoints.xVals[i] - imageMinX));
			int tmpY = (int)floor(scale * (rotatedPoints.yVals[i] * -1.0 - imageMinY));
			if (tmpY > -1000 && rotatedPoints.yVals[i] * -1.0 > imageMinY && rotatedPoints.yVals[i] * -1.0 < imageMaxY)
			{
				if (tmpY >= CV2_IMAGE_SIZE_Y_1 || tmpX >= CV2_IMAGE_SIZE_X_1)
				{
					int charlieBreak = 1;
				}
				tmpPic1[tmpY][tmpX] = 255;
			}
		}
	}

	cv::Mat mat1 = cv::Mat(CV2_IMAGE_SIZE_Y_1, CV2_IMAGE_SIZE_X_1, CV_8UC1, &tmpPic1);

	TemplatePoints tmpTemplatePoints[RAIL_TEMPLATE_COUNT];

	//SMOOTH IMAGE
	if (rotatedPoints.sensorNumber == 1)
	{

		//DO TEMPLATE MATCH
		templateArgs_1.matrix = mat1;
		templateArgs_1.scale = cv2Sensor1Scale;
		templateArgs_1.minXValue = imageMinX;
		templateArgs_1.minYValue = imageMinY;
		templateArgs_1.maxXValue = imageMaxX;
		templateArgs_1.maxYValue = imageMaxY;

		int r1 = templateMatch(&templateArgs_1);

		memcpy(points.templatePoint, templateArgs_1.templatePoints, sizeof(templateArgs_1.templatePoints));
		memcpy(tmpTemplatePoints, templateArgs_1.templatePoints, sizeof(templateArgs_1.templatePoints));

		////WAIT FOR THREADS -- CLOSE HANDLES
		WaitForSingleObject(analyzeThreads[0], INFINITE);
		CloseHandle(analyzeThreads[0]);
	}
	else
	{
		//DO TEMPLATE MATCH
		templateArgs_2.matrix = mat1;
		templateArgs_2.scale = cv2Sensor2Scale;
		templateArgs_2.minXValue = imageMinX;
		templateArgs_2.minYValue = imageMinY;
		templateArgs_2.maxXValue = imageMaxX;
		templateArgs_2.maxYValue = imageMaxY;

		int r1 = templateMatch(&templateArgs_2);

		memcpy(points.templatePoint, templateArgs_2.templatePoints, sizeof(templateArgs_2.templatePoints));
		memcpy(tmpTemplatePoints, templateArgs_2.templatePoints, sizeof(templateArgs_2.templatePoints));

		////WAIT FOR THREADS -- CLOSE HANDLES
		WaitForSingleObject(analyzeThreads[0], INFINITE);
		CloseHandle(analyzeThreads[0]);
	}

	bool angleMatch = false;
	bool gradMatch = false;
	EdgePoint tmpTOR;
	TemplatePoints lastBestPoint;

	lastBestPoint = points.lastBestTemplatePoint;

	int torFoundCnt = findTORv2(rotatedPoints.xVals, rotatedPoints.yVals, tmpTemplatePoints, rotatedPoints.angle, rotatedPoints.secondDervSlope, lastBestPoint, tmpTOR, angleMatch, gradMatch);


	double fiveEight_Y = nan("n");
	double avgTopRail_Y = nan("n");
	double fiveEight_X = nan("n");

	if (torFoundCnt > 0)
	{
		////DETERMINE MEAN Y-VALUE; +/-39mm FROM TOR POINT
		calcTorYValue(rotatedPoints.xVals, rotatedPoints.yVals, tmpTOR, avgTopRail_Y, fiveEight_Y);
		////AVERAGE X-VALUES OVER +/- 1/16" AT 5/8" POINT:: 1/16" = 1.58MM
		calcFiveEightXValue(rotatedPoints.xVals, rotatedPoints.yVals, tmpTOR, fiveEight_Y, fiveEight_X);
	}

	if (torFoundCnt < 1 || _isnan(fiveEight_X) || _isnan(fiveEight_Y))
	{
		points.foundHalfGage = false;
	}
	else
	{
		points.foundHalfGage = true;
	}

	//X-Y PROFILE POINTS
	memcpy(points.yVals, rotatedPoints.yVals, sizeof(rotatedPoints.yVals));
	memcpy(points.xVals, rotatedPoints.xVals, sizeof(rotatedPoints.xVals));

	points.tor = tmpTOR;
	points.torAvgY = avgTopRail_Y;
	points.y5_8 = fiveEight_Y;
	points.x5_8 = fiveEight_X;
	points.halfGage = fiveEight_X * (0.0393701); //CONVERT MM TO INCH
	points.torPointsFound = torFoundCnt;

	*((ProfilePoints*)args) = points;

	return 1;
}

