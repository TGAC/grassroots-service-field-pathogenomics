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
 * files_metadata.c
 *
 *  Created on: 3 Mar 2016
 *      Author: tyrrells
 */

#include "files_metadata.h"
#include "string_utils.h"
#include "pathogenomics_utils.h"



const char *InsertFilesData (MongoTool *tool_p, json_t *values_p, PathogenomicsServiceData *data_p)
{
	const char *error_s = NULL;
	const char * const key_s = PG_ID_S;
	const char *id_s = GetJSONString (values_p, key_s);

	if (id_s)
		{
			/* Create a json doc with "files"=values_p and PG_ID_S=id_s */
			json_t *doc_p = json_object ();

			if (doc_p)
				{
					if (json_object_set_new (doc_p, PG_ID_S, json_string (id_s)) == 0)
						{
							if (json_object_del (values_p, key_s) != 0)
								{
									PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to remove %s from files", key_s);
								}

							if (json_object_set (doc_p, PG_FILES_S, values_p) == 0)
								{
									error_s = InsertOrUpdateMongoData (tool_p, doc_p, NULL, NULL, PG_ID_S, NULL, NULL);

									/*
									char *date_s = ConcatenateStrings (PG_FILES_S, PG_LIVE_DATE_SUFFIX_S);

									if (date_s)
										{
											if (AddPublishDateToJSON (doc_p, date_s))
												{
													error_s = InsertOrUpdateMongoData (tool_p, doc_p, NULL, NULL, PG_ID_S, NULL, NULL);
												}
											else
												{
													error_s = "Failed to add current date to files data";
												}

											FreeCopiedString (date_s);
										}
									else
										{
											error_s = "Failed to make files date key";
										}
									*/
								}
							else
								{
									error_s = "Failed to add values to new files data";
								}

						}		/* if (json_object_set_new (doc_p, PG_ID_S, json_string (id_s)) == 0) */
					else
						{
							error_s = "Failed to add id to new v data";
						}

					json_decref (doc_p);
				}		/* if (doc_p) */
			else
				{
					error_s = "Failed to create files data to add";
				}

		}		/* if (id_s) */
	else
		{
			error_s = "Failed to get ID value";
		}

	return error_s;
}
