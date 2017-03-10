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
 * pathogenomics_utils.h
 *
 *  Created on: 2 Dec 2015
 *      Author: tyrrells
 */

#ifndef PATHOGENOMICS_UTILS_H_
#define PATHOGENOMICS_UTILS_H_

#include "pathogenomics_service_library.h"
#include "jansson.h"
#include "linked_list.h"
#include "service_job.h"


#ifdef __cplusplus
extern "C"
{
#endif


PATHOGENOMICS_SERVICE_LOCAL bool AddPublishDateToJSON (json_t *json_p, const char * const key_s, const bool add_flag);


PATHOGENOMICS_SERVICE_LOCAL bool SetDateForSchemaOrg (json_t *values_p, const char * const key_s, const char * const iso_date_s);


PATHOGENOMICS_SERVICE_LOCAL bool CheckForFields (const LinkedList *column_headers_p, const char **headers_ss, ServiceJob *job_p);


#ifdef __cplusplus
}
#endif

#endif /* PATHOGENOMICS_UTILS_H_ */
