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


static int GetAndRemoveJSONKeyValuePair (json_t *doc_to_add_to_p, const char *add_key_s, json_t *doc_to_delete_from_p,  const char *delete_key_s);


static int GetAndRemoveJSONKeyValuePair (json_t *doc_to_add_to_p, const char *add_key_s, json_t *doc_to_delete_from_p,  const char *delete_key_s)
{
	int res = -1;
	const char *value_s = GetJSONString (doc_to_delete_from_p, delete_key_s);

	if (value_s)
		{
			if (json_object_set_new (doc_to_add_to_p, add_key_s, json_string (value_s)) == 0)
				{
					if (json_object_del (doc_to_delete_from_p, delete_key_s) == 0)
						{
							res = 1;
						}
					else
						{
							PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to remove %s from old phenotype data", delete_key_s);
						}
				}
			else
				{
					PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to add %s to new phenotype data", add_key_s);
				}
		}
	else
		{
			res = 0;
		}

	return res;
}


const char *InsertPhenotypeData (MongoTool *tool_p, json_t *values_p, const uint32 stage_time, PathogenomicsServiceData * UNUSED_PARAM (data_p))
{
	const char *error_s = NULL;

	/* Create a json doc with  "phenotype"=values_p and PG_UKCPVS_ID_S=isolate_s */
	json_t *doc_p = json_object ();

	if (doc_p)
		{
			int ukcpvs_res = GetAndRemoveJSONKeyValuePair (doc_p, PG_UKCPVS_ID_S, values_p, "Isolate");

			if (ukcpvs_res != -1)
				{
					int id_res = GetAndRemoveJSONKeyValuePair (doc_p, PG_ID_S, values_p, PG_ID_S);

					if (id_res != -1)
						{
							const char *primary_key_s = NULL;

							if (id_res == 1)
								{
									primary_key_s = PG_ID_S;
								}
							else if (ukcpvs_res == 1)
								{
									primary_key_s = PG_UKCPVS_ID_S;
								}

							if (primary_key_s)
								{
									if (json_object_set (doc_p, PG_PHENOTYPE_S, values_p) == 0)
										{
											char *date_s = ConcatenateStrings (PG_PHENOTYPE_S, PG_LIVE_DATE_SUFFIX_S);

											if (date_s)
												{
													if (AddPublishDateToJSON (doc_p, date_s, stage_time, true))
														{
															error_s = EasyInsertOrUpdateMongoData (tool_p, doc_p, primary_key_s);
														}
													else
														{
															error_s = "Failed to add current date to phenotyope data";
														}

													FreeCopiedString (date_s);
												}		/* if (date_s) */
											else
												{
													error_s = "Failed to make phenotype date key";
												}

										}		/* if (json_object_set (doc_p, PG_PHENOTYPE_S, values_p) == 0) */
									else
										{
											error_s = "Failed to add values to new phenotype data";
										}

								}		/* if (primary_key_s) */
							else
								{
									error_s = "Failed to get primary key from phenotype values";
								}

						}		/* if (id_res != -1) */
					else
						{
							error_s = "Failed to move ID key to top-level phenotype data";
						}

				}		/* if (ukcpvs_res != -1) */
			else
				{
					error_s = "Failed to move UKCPVS ID key to top-level phenotype data";
				}

			json_decref (doc_p);
		}		/* if (doc_p) */
	else
		{
			error_s = "Failed to create phenotype data to add";
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

