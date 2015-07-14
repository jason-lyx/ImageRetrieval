/*
 * test_methods.h
 *
 *  Created on: May 15, 2015
 *      Author: main
 */

#ifndef TEST_METHODS_H_
#define TEST_METHODS_H_

#include <opencv2/opencv.hpp>
#include <vector>

using namespace cv;
using namespace std;

void genAffineSeries(const Mat &src, vector<Mat> &output);


#endif /* TEST_METHODS_H_ */
