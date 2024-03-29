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
 * pathogenomics_service_data.h
 *
 *  Created on: 28 Sep 2015
 *      Author: tyrrells
 */

#ifndef PATHOGENOMICS_SERVICE_DATA_H_
#define PATHOGENOMICS_SERVICE_DATA_H_

#include "jansson.h"

#include "service.h"
#include "mongodb_tool.h"
#include "pathogenomics_service_library.h"


typedef enum
{
	PD_SAMPLE,
	PD_PHENOTYPE,
	PD_GENOTYPE,
	PD_FILES,
	PD_NUM_TYPES
} PathogenomicsData;

typedef struct PathogenomicsServiceData PathogenomicsServiceData;

/**
 * The configuration data used by the Pathogenomics Service.
 *
 * @extends ServiceData
 */
struct /*PATHOGENOMICS_SERVICE_LOCAL*/ PathogenomicsServiceData
{
	/** The base ServiceData. */
	ServiceData psd_base_data;

	/**
	 * @private
	 *
	 * The MongoTool to connect to the database where our data is stored.
	 */
	MongoTool *psd_tool_p;


	/**
	 * @private
	 *
	 * The name of the database to use.
	 */
	const char *psd_database_s;

	/**
	 * @private
	 *
	 * The collection name of use for each of the different types of data.
	 */
	const char *psd_collection_ss [PD_NUM_TYPES];


	/**
	 * @private
	 *
	 * The URI of the root folder where any downloadable files is stored.
	 */
	const char *psd_files_download_root_uri_s;

	/**
	 * @private
	 *
	 * The default stage time, in days, before the filed sample data gets published
	 */
	json_int_t psd_default_stage_time;
};


#endif /* PATHOGENOMICS_SERVICE_DATA_H_ */
