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
 * file_metadata.h
 *
 *  Created on: 3 Mar 2016
 *      Author: tyrrells
 */

#ifndef SERVICES_PATHOGENOMICS_INCLUDE_FILES_METADATA_H_
#define SERVICES_PATHOGENOMICS_INCLUDE_FILES_METADATA_H_


#include "pathogenomics_service_library.h"
#include "mongodb_tool.h"
#include "pathogenomics_service_data.h"

#include "pathogenomics_service.h"
#include "jansson.h"


#ifdef __cplusplus
extern "C"
{
#endif


PATHOGENOMICS_SERVICE_LOCAL	const char *InsertFilesData (MongoTool *tool_p, json_t *values_p, PathogenomicsServiceData *data_p);


#ifdef __cplusplus
}
#endif


#endif /* SERVICES_PATHOGENOMICS_INCLUDE_FILES_METADATA_H_ */
