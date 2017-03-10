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
 * genotype_metadata.c
 *
 *  Created on: 19 Oct 2015
 *      Author: tyrrells
 */
#include "genotype_metadata.h"
#include "pathogenomics_utils.h"
#include "json_tools.h"
#include "string_utils.h"


#ifdef _DEBUG
	#define GENOTYPE_METADATA_DEBUG	(STM_LEVEL_FINE)
#else
	#define GENOTYPE_METADATA_DEBUG	(STM_LEVEL_NONE)
#endif


static const char * const GM_LIB_NAME_S = "Library name";
static const char * const GM_GENETIC_GROUP_S = "Genetic group";
static const char * const GM_SAMPLE_NAME_S = "Sample name";


const char *InsertGenotypeData (MongoTool *tool_p, json_t *values_p, PathogenomicsServiceData * UNUSED_PARAM (data_p))
{
	const char *error_s = NULL;
	const char * const key_s = PG_ID_S;
	const char *id_s = GetJSONString (values_p, key_s);

	if (id_s)
		{
			/* Create a json doc with "genotype"=values_p and PG_ID_S=id_s */
			json_t *doc_p = json_object ();

			if (doc_p)
				{
					if (json_object_set_new (doc_p, PG_ID_S, json_string (id_s)) == 0)
						{
							if (json_object_del (values_p, key_s) != 0)
								{
									PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to remove %s from genotype", key_s);
								}

							if (json_object_set (doc_p, PG_GENOTYPE_S, values_p) == 0)
								{
									char *date_s = ConcatenateStrings (PG_GENOTYPE_S, PG_LIVE_DATE_SUFFIX_S);

									if (date_s)
										{
											/* Is the data needed to be hidden for company signoff? */
											bool hidden_flag = false;
											const char *company_s = GetJSONString (values_p, PG_COMPANY_S);
											if (company_s && (strlen (company_s) > 0))
												{
													hidden_flag = true;
												}

											if (AddPublishDateToJSON (doc_p, date_s, hidden_flag))
												{
													error_s = InsertOrUpdateMongoData (tool_p, doc_p, NULL, NULL, PG_ID_S, NULL, NULL);
												}
											else
												{
													error_s = "Failed to add current date to genotype data";
												}

											FreeCopiedString (date_s);
										}
									else
										{
											error_s = "Failed to make genotype date key";
										}
								}
							else
								{
									error_s = "Failed to add values to new genotype data";
								}

						}		/* if (json_object_set_new (doc_p, PG_ID_S, json_string (id_s)) == 0) */
					else
						{
							error_s = "Failed to add id to new genotype data";
						}

					WipeJSON (doc_p);
				}		/* if (doc_p) */
			else
				{
					error_s = "Failed to create genotype data to add";
				}

		}		/* if (id_s) */
	else
		{
			error_s = "Failed to get ID value";
		}

	return error_s;
}


bool CheckGenotypeData (const LinkedList *headers_p, ServiceJob *job_p, PathogenomicsServiceData *data_p)
{
	const char *headers_ss [] = {
		PG_ID_S,
		GM_LIB_NAME_S,
		GM_GENETIC_GROUP_S,
		GM_SAMPLE_NAME_S,
		NULL
	};

	return CheckForFields (headers_p, headers_ss, job_p);
}

