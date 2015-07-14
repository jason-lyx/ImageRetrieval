/*
 * extractFeatureMain.cpp
 *
 *  Created on: May 6, 2015
 *      Author: main
 *
 *  Comments: This is a program to extract, match, retrieve, and insert image features. It uses OpenCV
 *            for feature extraction and matching. It uses Cassandra for database.
 */

#include <iostream>
#include <string>
#include <stdexcept>
#include <cassandra.h>
#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include "cql_handler.h"
#include "test_methods.h"

using namespace std;
using namespace cv;

int main(int argc, char** argv) {
	// Read image and generate a series of test images by consecutive rotation
	Mat image;
	image = imread(argv[1], IMREAD_GRAYSCALE);

	if (argc != 2 || !image.data) {
		throw runtime_error("No image data");
	}
	vector<Mat> testImages;
	genAffineSeries(image, testImages);
	const char *windowName = "Display Image";
	for (Mat testImage : testImages) {
		namedWindow(windowName, WINDOW_AUTOSIZE);
		imshow(windowName, testImage);
		waitKey(0);
	}
	destroyAllWindows();

	// Find features
	vector<KeyPoint> keyPoints;
	Mat desc;
	Ptr<AKAZE> featureFinder = AKAZE::create();
	featureFinder->detectAndCompute(image, noArray(), keyPoints, desc, false);

	// Look up in the database, find the image with the most matching features (ratio)
	// If it is deemed a new image, insert the new features into the database. Otherwise,
	// print the matched image id.
	dBase_init();
	int index = dBase_find_features(desc);
	if (index > 0) {
		cout << "find image, id = " << index << endl;
	}
	else {
		index = dBase_insert_features(desc);
		cout << "new image, id = " << index << endl;
	}

	// Test the rotated images to check how consistent the match is
	cout << "test affine series:\n";
	for (auto testImage : testImages) {
		featureFinder->detectAndCompute(testImage, noArray(), keyPoints, desc, false);
		index = dBase_find_features(desc);
		if (index > 0) {
			cout << "pass, id = " << index << endl;
		}
		else {
			cout << "failed\n";
		}
	}

	dBase_close();

	return 0;
}

/* Test region

string type2str(int type);
void testCassandra();

int main_test(int argc, char** argv) {
	testCassandra();

	Mat image;
	image = imread(argv[1], IMREAD_GRAYSCALE);

	if (argc < 2 || !image.data) {
		cout << "No image data \n";
		return -1;
	}

	vector<KeyPoint> keyPoints;
	Mat desc;
	Ptr<AKAZE> featureFinder = AKAZE::create();
	featureFinder->detectAndCompute(image, noArray(), keyPoints, desc, false);

	Mat refDesc;
	if (argc == 3) {
		FileStorage fs(argv[2], FileStorage::READ);
		fs["Features"] >> refDesc;

		BFMatcher matcher(NORM_HAMMING);

		vector<vector<DMatch>> matches;
		matcher.knnMatch(desc, refDesc, matches, 2);
		const float distRatio = 0.8f;
		int nMatch = 0;
		for (vector<DMatch> vm : matches) {
			if (vm[0].distance < distRatio * vm[1].distance) {
				nMatch++;
			}
		}
		cout << nMatch << "/" << refDesc.rows << endl;
	}
	else {
		string fn(argv[1]);
		size_t dotLoc = fn.rfind('.');
		if (dotLoc == string::npos) {
			dotLoc = fn.length();
		}
		fn = fn.substr(0, dotLoc) + ".yml";
		FileStorage fs(fn, FileStorage::WRITE);
		fs << "Features" << desc;
	}

	Mat outImage;
	drawKeypoints(image, keyPoints, outImage);

	namedWindow("Display Image", WINDOW_AUTOSIZE);
	imshow("Display Image", outImage);

	waitKey(0);

	return 0;
}

string type2str(int type) {
  string r;

  uchar depth = type & CV_MAT_DEPTH_MASK;
  uchar chans = 1 + (type >> CV_CN_SHIFT);

  switch ( depth ) {
    case CV_8U:  r = "8U"; break;
    case CV_8S:  r = "8S"; break;
    case CV_16U: r = "16U"; break;
    case CV_16S: r = "16S"; break;
    case CV_32S: r = "32S"; break;
    case CV_32F: r = "32F"; break;
    case CV_64F: r = "64F"; break;
    default:     r = "User"; break;
  }

  r += "C";
  r += (chans+'0');

  return r;
}

*/
