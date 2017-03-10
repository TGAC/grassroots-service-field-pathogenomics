/*
** Copyright 2014-2016 The Earlham Institute
** 
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
** 
**     http://www.apache.org/licenses/LICENSE-2.0
** 
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

/**
 * @file
 * @brief
 */
/*
 * sample_metadata.h
 *
 *  Created on: 1 Jul 2015
 *      Author: tyrrells
 */

#ifndef SAMPLE_METADATA_H_
#define SAMPLE_METADATA_H_

#include "bson.h"
#include "pathogenomics_service_library.h"
#include "pathogenomics_service_data.h"
#include "mongodb_tool.h"

#include "pathogenomics_service.h"

#ifdef __cplusplus
extern "C"
{
#endif



PATHOGENOMICS_SERVICE_LOCAL bool GetLocationDataByGoogle (PathogenomicsServiceData *data_p, json_t *row_p, const char * const id_s);


PATHOGENOMICS_SERVICE_LOCAL bool GetLocationDataByOpenCage (PathogenomicsServiceData *data_p, json_t *row_p, const char * const id_s);


/**
 * Convert a date from the given json.
 *
 * If successful, the converted date will
 *
 * @row_p
 */
PATHOGENOMICS_SERVICE_LOCAL bool ConvertDate (json_t *row_p);


PATHOGENOMICS_SERVICE_LOCAL bool RefineLocationDataForGoogle (PathogenomicsServiceData *service_data_p, json_t *row_p, const json_t *raw_data_p, const char * const town_s, const char * const county_s);


PATHOGENOMICS_SERVICE_LOCAL bool RefineLocationDataForOpenCage (PathogenomicsServiceData *service_data_p, json_t *row_p, const json_t *raw_data_p, const char * const town_s, const char * const county_s);


PATHOGENOMICS_SERVICE_LOCAL const char *InsertSampleData (MongoTool *tool_p, json_t *values_p, PathogenomicsServiceData *data_p);


PATHOGENOMICS_SERVICE_LOCAL bool CheckSampleData (const LinkedList *headers_p, ServiceJob *job_p, PathogenomicsServiceData *data_p);

#ifdef __cplusplus
}
#endif


#endif /* SAMPLE_METADATA_H_ */
