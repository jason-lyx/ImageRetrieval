/*
 * cql_handler.h
 *
 *  Created on: May 14, 2015
 *      Author: main
 */

#ifndef CQL_HANDLER_H_
#define CQL_HANDLER_H_

#include <opencv2/opencv.hpp>

using namespace cv;

// initialize database;
void dBase_init();

// close database
void dBase_close();

// return -1 if not found matching feature
int dBase_find_features(const Mat &features);

// return the index of the new feature inserted
int dBase_insert_features(const Mat &features);


#endif /* CQL_HANDLER_H_ */
