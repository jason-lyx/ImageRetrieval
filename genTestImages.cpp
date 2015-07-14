/*
 * genTestImages.cpp
 *
 *  Created on: May 15, 2015
 *      Author: main
 */

#include <opencv2/opencv.hpp>
#include <vector>

using namespace std;
using namespace cv;

// transform the source image to a series of images by rotating each 30 degree consecutively
void genAffineSeries(const Mat &src, vector<Mat> &output) {
	const float alpha = -30.f;
	int cnt = 360.f / fabs(alpha);

	float angle = alpha;
	for (int i = 0; i < cnt; i++, angle += alpha) {
		Mat affine = getRotationMatrix2D(Point2f(src.cols / 2, src.rows / 2), angle, 1);
		Mat dst;
		warpAffine(src, dst, affine, Size(src.cols * 1.41, src.rows * 1.41));
		output.push_back(dst);
	}
}
