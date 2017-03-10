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
/*
 * phenotype_metadata.c
 *
 *  Created on: 16 Sep 2015
 *      Author: tyrrells
 */

#include "phenotype_metadata.h"
#include "pathogenomics_utils.h"
#include "json_tools.h"
#include "string_utils.h"


#ifdef _DEBUG
	#define PHENOTYPE_METADATA_DEBUG	(STM_LEVEL_FINE)
#else
	#define PHENOTYPE_METADATA_DEBUG	(STM_LEVEL_NONE)
#endif


static const char * const PM_ISOLATE_S = "Isolate";
static const char * const PM_HOST_S = "Host Variety";


const char *InsertPhenotypeData (MongoTool *tool_p, json_t *values_p, PathogenomicsServiceData * UNUSED_PARAM (data_p))
{
	const char *error_s = NULL;
	const char * const key_s = "Isolate";
	const char *isolate_s = GetJSONString (values_p, key_s);

	if (isolate_s)
		{
			/* Create a json doc with  "phenotype"=values_p and PG_UKCPVS_ID_S=isolate_s */
			json_t *doc_p = json_object ();

			if (doc_p)
				{
					if (json_object_set_new (doc_p, PG_UKCPVS_ID_S, json_string (isolate_s)) == 0)
						{
							if (json_object_del (values_p, key_s) != 0)
								{
									PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to remove %s from phenotype", key_s);
								}

							if (json_object_set (doc_p, PG_PHENOTYPE_S, values_p) == 0)
								{
									char *date_s = ConcatenateStrings (PG_PHENOTYPE_S, PG_LIVE_DATE_SUFFIX_S);

									if (date_s)
										{
											if (AddPublishDateToJSON (doc_p, date_s, true))
												{
													error_s = InsertOrUpdateMongoData (tool_p, doc_p, NULL, NULL, PG_UKCPVS_ID_S, NULL, NULL);
												}
											else
												{
													error_s = "Failed to add current date to phenotyope data";
												}

											FreeCopiedString (date_s);
										}
									else
										{
											error_s = "Failed to make phenotype date key";
										}
								}
							else
								{
									error_s = "Failed to add values to new phenotype data";
								}

						}		/* if (json_object_set_new (doc_p, PG_UKCPVS_ID_S, json_string (isolate_s)) == 0) */
					else
						{
							error_s = "Failed to add id to new phenotype data";
						}

					WipeJSON (doc_p);
				}		/* if (doc_p) */
			else
				{
					error_s = "Failed to create phenotype data to add";
				}

		}		/* if (isolate_s) */
	else
		{
			error_s = "Failed to get Isolate value";
		}

	return error_s;
}


bool CheckPhenotypeData (const LinkedList *headers_p, ServiceJob *job_p, PathogenomicsServiceData *data_p)
{
	const char *headers_ss [] = {
		PG_ID_S,
		PM_ISOLATE_S,
		PM_HOST_S,
		NULL
	};

	return CheckForFields (headers_p, headers_ss, job_p);
}

