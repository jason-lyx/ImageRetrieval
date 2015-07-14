/*
 * cql_handler.cpp
 *
 *  Created on: May 13, 2015
 *      Author: main
 */

#include <cassandra.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <stdexcept>
#include "cql_handler.h"

using namespace std;
using namespace cv;

// single threaded design;
static int _last_index;

static CassFuture* _connect_future = NULL;
static CassCluster* _cluster = NULL;
static CassSession* _session = NULL;

void dBase_init() {
	_cluster = cass_cluster_new();
	_session = cass_session_new();

	cass_cluster_set_contact_points(_cluster, "127.0.0.1");
	_connect_future = cass_session_connect(_session, _cluster);
	CassError rc = cass_future_error_code(_connect_future);
	if (rc != CASS_OK) {
		throw runtime_error(cass_error_desc(rc));
	}
}

void dBase_close() {
	CassFuture*  close_future = cass_session_close(_session);
	cass_future_wait(close_future);
	cass_future_free(close_future);

	cass_future_free(_connect_future);
	cass_cluster_free(_cluster);
	cass_session_free(_session);
}

// Match image features with that in the database, select all the features
// that is some Hamming distance away from the second closest match. Find the
// database record that has the highest match ratio.
//
// To do: construct Hough Transform for the matching features and check Pose consistency
// for more accurate match.
int dBase_find_features(const Mat &features) {
	CassStatement* statement = cass_statement_new(
			                        "select * from mykeyspace.image_features", 0);

	CassFuture* result_future = cass_session_execute(_session, statement);

	int index = -1;
	float maxratio = .0f;
	CassError rc = cass_future_error_code(result_future);
	if (rc == CASS_OK) {
		const CassResult* result = cass_future_get_result(result_future);
		CassIterator* rows = cass_iterator_from_result(result);

		while(cass_iterator_next(rows)) {
			const CassRow* row = cass_iterator_get_row(rows);
			const CassValue* value = cass_row_get_column_by_name(row, "id");
			int id;
			cass_value_get_int32(value, &id);

			if (id > _last_index) {
				_last_index = id;
			}

			value = cass_row_get_column_by_name(row, "yaml");
			const char* yml_str;
			size_t yml_length;
			cass_value_get_string(value, &yml_str, &yml_length);

			cout << id << ": yaml length = " << yml_length << ", ";

			Mat refFeatures;
			FileStorage fs(yml_str, FileStorage::READ + FileStorage::MEMORY);
			fs["Features"] >> refFeatures;

			BFMatcher matcher(NORM_HAMMING);

			vector<vector<DMatch>> matches;
			matcher.knnMatch(features, refFeatures, matches, 2);
			const float distRatio = 0.8f;
			int nMatch = 0;
			for (vector<DMatch> vm : matches) {
				if (vm[0].distance < distRatio * vm[1].distance) {
					nMatch++;
				}
			}
			cout << "match metric = " << nMatch << "/" << refFeatures.rows << endl;
			float ratio = (float) nMatch / refFeatures.rows;
			if (ratio > maxratio) {
				index = id;
				maxratio = ratio;
			}
		}
	}
	else {
		throw runtime_error(cass_error_desc(rc));
	}

	cass_statement_free(statement);
	cass_future_free(result_future);

	const float acceptratio = 0.5f;
	if (maxratio >= acceptratio) {
		return index;
	}
	else {
		return -1;
	}
}

int dBase_insert_features(const Mat &features) {
	_last_index++;

	FileStorage fs(".yml", FileStorage::WRITE + FileStorage::MEMORY);
	fs << "Features" << features;
	string yml_str = fs.releaseAndGetString();

	CassStatement* statement = cass_statement_new(
			"INSERT INTO mykeyspace.image_features (timestamp, id, yaml) VALUES (now(), ?, ?);", 2);

	cass_statement_bind_int32(statement, 0, _last_index);
	cass_statement_bind_string(statement, 1, yml_str.c_str());

	CassFuture* query_future = cass_session_execute(_session, statement);

	// Statement objects can be freed immediately after being executed
	cass_statement_free(statement);

	// This will block until the query has finished
	CassError rc = cass_future_error_code(query_future);
	if (rc != CASS_OK) {
		throw runtime_error(cass_error_desc(rc));
	}

	cass_future_free(query_future);

	return _last_index;
}

/* Test region
void testCassandra() {
	CassFuture* connect_future = NULL;
	CassCluster* cluster = cass_cluster_new();
	CassSession* session = cass_session_new();

	cass_cluster_set_contact_points(cluster, "127.0.0.1");

	connect_future = cass_session_connect(session, cluster);

	if (cass_future_error_code(connect_future) == CASS_OK) {
		CassFuture* close_future = NULL;

		CassStatement* statement =
				cass_statement_new("select *"
				                   "from mykeyspace.users", 0);

		CassFuture* result_future = cass_session_execute(session, statement);

		if (cass_future_error_code(result_future) == CASS_OK) {
			const CassResult* result = cass_future_get_result(result_future);
			CassIterator* rows = cass_iterator_from_result(result);

			while(cass_iterator_next(rows)) {
				const CassRow* row = cass_iterator_get_row(rows);
				const CassValue* value = cass_row_get_column_by_name(row, "user_id");
				int user_id;
				cass_value_get_int32(value, &user_id);

				value = cass_row_get_column_by_name(row, "fname");
				const char* fname;
				size_t fname_length;
				cass_value_get_string(value, &fname, &fname_length);

				value = cass_row_get_column_by_name(row, "lname");
				const char* lname;
				size_t lname_length;
				cass_value_get_string(value, &lname, &lname_length);

				cout << user_id << ": " << lname << ", " << fname << endl;
			}
		}
		else {
			const char* message;
			size_t message_length;
			cass_future_error_message(result_future, &message, &message_length);
			cerr << "Unable to run query: " << message << endl;
		}

		cass_statement_free(statement);
		cass_future_free(result_future);

		close_future = cass_session_close(session);
		cass_future_wait(close_future);
		cass_future_free(close_future);
	}
	else {
		const char* message;
		size_t message_length;
		cass_future_error_message(connect_future, &message, &message_length);
		cerr << "Unable to connect: " << message << endl;
	}

	cass_future_free(connect_future);
	cass_cluster_free(cluster);
	cass_session_free(session);
}
*/
