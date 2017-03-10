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
 * pathogenomics_utils.c
 *
 *  Created on: 2 Dec 2015
 *      Author: tyrrells
 */

#include <string.h>

#include "pathogenomics_utils.h"
#include "time_util.h"
#include "streams.h"
#include "json_tools.h"
#include "string_utils.h"
#include "json_util.h"


bool AddPublishDateToJSON (json_t *json_p, const char * const key_s, const bool add_flag)
{
	bool success_flag = false;
	struct tm current_time;

	if (GetCurrentTime (&current_time))
		{
			char *date_s = NULL;

			if (add_flag)
				{
					/* Set the "go live" date to be 1 month from now */
					if (current_time.tm_mon == 11)
						{
							++ current_time.tm_year;
							current_time.tm_mon = 0;
						}
					else
						{
							++ current_time.tm_mon;
						}
				}

			date_s = GetTimeAsString (&current_time);

			if (date_s)
				{
					if (SetDateForSchemaOrg (json_p, key_s, date_s))
						{
							success_flag = true;
						}

					FreeCopiedString (date_s);
				}
			else
				{
					PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to create publish date for %s");
				}
		}
	else
		{
			PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to get current time");
		}

	return success_flag;
}


bool SetDateForSchemaOrg (json_t *values_p, const char * const key_s, const char * const iso_date_s)
{
	bool success_flag = false;
	json_t *child_p = json_object ();

	if (child_p)
		{
			if ((json_object_set_new (child_p, "@type", json_string ("Date")) == 0) &&
					(json_object_set_new (child_p, "date", json_string (iso_date_s)) == 0))
				{
					if (json_object_set_new (values_p, key_s, child_p) == 0)
						{
							success_flag = true;
						}
					else
						{
							PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to set schema org date context for %s = %s", key_s, iso_date_s);
						}

				}
			else
				{
					PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to add schema org date context for %s = %s", key_s, iso_date_s);
				}

			if (!success_flag)
				{
					WipeJSON (child_p);
				}
		}
	else
		{
			PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to create json child for schema org date context for %s = %s", key_s, iso_date_s);
		}

	return success_flag;
}


bool CheckForFields (const LinkedList *column_headers_p, const char **headers_ss, ServiceJob *job_p)
{
	bool success_flag = true;


	while (*headers_ss)
		{
			FieldNode *node_p = (FieldNode *) (column_headers_p -> ll_head_p);
			bool found_flag = false;

			while ((node_p) && (!found_flag))
				{
					if (strcmp (node_p -> fn_base_node.sln_string_s, *headers_ss) == 0)
						{
							found_flag = true;
						}
					else
						{
							node_p = (FieldNode *) (node_p -> fn_base_node.sln_node.ln_next_p);
						}

				}		/* while ((node_p) && (!found_flag)) */


			if (!found_flag)
				{
					char *key_s = ConcatenateVarargsStrings ("Missing column \"", *headers_ss, "\"", NULL);

					if (key_s)
						{
							if (!AddErrorToServiceJob (job_p, key_s, "Column not found"))
								{
									PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to add error for %s to job response", key_s);
								}

							FreeCopiedString (key_s);
						}		/* if (key_s) */
					else
						{
							PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to add error for %s to job response", key_s);
						}

					if (success_flag)
						{
							success_flag = false;
						}

				}		/* if (!found_flag) */

			++ headers_ss;
		}		/* while (*headers_ss) */


	return success_flag;
}
