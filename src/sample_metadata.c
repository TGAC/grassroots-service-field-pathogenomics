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
 * sample_metadata.c
 *
 *  Created on: 1 Jul 2015
 *      Author: tyrrells
 */
#include "sample_metadata.h"
#include "mongodb_tool.h"
#include "string_utils.h"
#include "country_codes.h"
#include "curl_tools.h"
#include "json_util.h"
#include "json_tools.h"
#include "math_utils.h"
#include "pathogenomics_utils.h"



#ifdef _DEBUG
#define SAMPLE_METADATA_DEBUG	(STM_LEVEL_FINE)
#else
#define SAMPLE_METADATA_DEBUG	(STM_LEVEL_NONE)
#endif


static bool FillInPathogenomicsFromGoogleData (const json_t *google_result_p, json_t *doc_p);

static bool ReplacePathogen (json_t *data_p);

static bool ParseCollector (json_t *values_p);

static bool ParseCompany (json_t *values_p);

static bool ParseAddressForSchemaOrg (json_t *values_p, const char * const town_s, const char * const county_s, const char * const country_s, const char * const postcode_s);

static bool AddValidJSONField (json_t *json_p, const char *key_s, const char *value_s);

static bool ConvertToSchemaOrgRepresentation (json_t *values_p, const char * const input_key_s, const char * const type_s, const char * const output_subkey_s);


static const char *PrepareSampleData (MongoTool *tool_p, json_t *values_p, PathogenomicsServiceData *data_p, const char *pathogenomics_id_s);

static const char *MergeData (MongoTool *tool_p, json_t *values_p, const char * const pathogenomics_id_s, const char * const ukcpvs_id_s, json_t **selector_pp);


/****************************************/
/********** PUBLIC FUNCTIONS ************/
/****************************************/


bool CheckSampleData (const LinkedList *headers_p, ServiceJob *job_p, PathogenomicsServiceData *data_p)
{
	const char *headers_ss [] = {
		PG_ID_S,
		PG_UKCPVS_ID_S,
		PG_DATE_S,
		PG_COLLECTOR_S,
		PG_COMPANY_S,
		PG_COUNTRY_S,
		PG_COUNTY_S,
		PG_TOWN_S,
		PG_POSTCODE_S,
		PG_GPS_S,
		PG_RUST_S,
		PG_VARIETY_S,
		"Host",
		NULL
	};

	return CheckForFields (headers_p, headers_ss, job_p);
}


const char *InsertSampleData (MongoTool *tool_p, json_t *values_p, PathogenomicsServiceData *data_p)
{
	const char *error_s = NULL;
	const char *pathogenomics_id_s = GetJSONString (values_p, PG_ID_S);

	if (pathogenomics_id_s)
		{
			error_s = PrepareSampleData (tool_p, values_p, data_p, pathogenomics_id_s);

			if (!error_s)
				{
					/*
					 * The genotype data is keyed by PG_ID_S and the phenotype data is
					 * keyed by PG_UKCPVS_ID_S. So we need to check for both of these.
					 *
					 * If none exist in the db, we can insert as normal.
					 * If only one of them is set, then we can update as normal.
					 * If both exist as separate entries, we need to merge them together
					 * and update with our sample data.
					 * If both exist on the same entry, then we update as normal.
					 */
					bool success_flag = true;
					const char *ukcpvs_id_s = GetJSONString (values_p, PG_UKCPVS_ID_S);
					json_t *selector_p = NULL;

					if (ukcpvs_id_s)
						{
							error_s = MergeData (tool_p, values_p, pathogenomics_id_s, ukcpvs_id_s, &selector_p);
						}		/* if (ukcpvs_id_s) */

					if (!error_s)
						{
							/*
							 * Now we insert it into the database as a json_object along with its
							 * live date
							 */
							json_t *record_p = json_object ();

							if (record_p)
								{
									if (json_object_set (record_p, PG_SAMPLE_S, values_p) == 0)
										{
											/*
											 * jansson implementation doesn't appear to be able to have
											 * things like json_object_get/set where the key allows the
											 * use of a child key e.g.
											 *
											 * 	json_object_get (value_p, "sample.ID")
											 *
											 * where value_p has a "sample" child with an "ID entry
											 *
											 * So copy the PG_ID_S and PG_UKCPVS_ID_S to the top level
											 */

											if (json_object_set_new (record_p, PG_ID_S, json_string (pathogenomics_id_s)) == 0)
												{
													if (json_object_del (values_p, PG_ID_S) != 0)
														{
															error_s = "Failed to remove PG_ID_S from values";
														}

													if (ukcpvs_id_s)
														{
															success_flag = (json_object_set_new (record_p, PG_UKCPVS_ID_S, json_string (ukcpvs_id_s)) == 0);

															if (success_flag)
																{
																	if (json_object_del (values_p, PG_UKCPVS_ID_S) != 0)
																		{
																			error_s = "Failed to remove PG_UKCPVS_ID_S from values";
																		}
																}

														}		/* if (ukcpvs_id_s) */


													if (success_flag)
														{
															char *date_s = ConcatenateStrings (PG_SAMPLE_S, PG_LIVE_DATE_SUFFIX_S);

															if (date_s)
																{
																	if (AddPublishDateToJSON (record_p, date_s, true))
																		{
																			#if SAMPLE_METADATA_DEBUG >= STM_LEVEL_FINE
																			PrintJSONToLog (STM_LEVEL_FINE, __FILE__, __LINE__, record_p, "sample json:");
																			#endif

																			error_s = InsertOrUpdateMongoData (tool_p, record_p, NULL, NULL, PG_ID_S, NULL, NULL);

																			if ((!error_s) && selector_p)
																				{
																					if (!RemoveMongoDocuments (tool_p, selector_p, true))
																						{
																							error_s = "Failed to remove existing phenotype doc";
																						}
																				}
																		}
																	else
																		{
																			error_s = "Failed to add current date to sample data";
																		}

																	FreeCopiedString (date_s);
																}
															else
																{
																	error_s = "Failed to make sample date key";
																}

														}		/* if (success_flag) */

												}		/* if (json_object_set_new (sample_p, PG_ID_S, json_string (pathogenomics_id_s)) == 0) */
											else
												{

												}

										}		/* if (json_object_set (sample_p, PG_SAMPLE_S, values_p) == 0) */
									else
										{
											error_s = "Failed to add data to parent sample object";
										}

									json_decref (record_p);
								}		/* if (record_p) */
							else
								{
									error_s = "Failed to allocate parent sample object";
								}

						}		/* if (!error_s) */
					else
						{
							error_s = "Failed to merge previous data";
						}

					if (selector_p)
						{
							WipeJSON (selector_p);
						}

				}		/* if (!error_s) */

		}		/* if (pathogenomics_id_s) */
	else
		{
			error_s = "Could not get pathogenomics id";
		}

	return error_s;
}


bool ConvertDate (json_t *row_p)
{
	bool success_flag = false;
	const char *date_s = GetJSONString (row_p, PG_DATE_S);
	char iso_date_s [11];
	char raw_date_s [9];

	/* Add the dashes between the year, month and day */
	* (iso_date_s + 4) = '-';
	* (iso_date_s + 7) = '-';
	* (iso_date_s + 10) = '\0';
	* (raw_date_s + 8) = '\0';

	if (date_s)
		{
			const size_t date_length = strlen (date_s);


			if ((* (date_s + 2) == '/') && (* (date_s + 5) == '/'))
				{
					bool year_flag = false;
					const char *part_s = date_s + 6;

					/* Is it DD/MM/YY */
					if (date_length == 8)
						{
							/* year */
							const char *year_prefix_s = "20";

							memcpy (iso_date_s, year_prefix_s, 2 * sizeof (char));
							memcpy (iso_date_s, year_prefix_s, 2 * sizeof (char));

							memcpy (iso_date_s + 2, part_s, 2 * sizeof (char));
							memcpy (raw_date_s + 2, part_s, 2 * sizeof (char));

							year_flag = true;
						}
					/* Is it DD/MM/YYYY */
					else if (date_length == 10)
						{
							/* year */
							memcpy (iso_date_s, part_s, 4 * sizeof (char));
							memcpy (raw_date_s, part_s, 4 * sizeof (char));

							year_flag = true;
						}

					if (year_flag)
						{
							/* month */
							part_s = date_s + 3;
							memcpy (iso_date_s + 5, part_s, 2 * sizeof (char));
							memcpy (raw_date_s + 4, part_s, 2 * sizeof (char));

							/* day */
							memcpy (iso_date_s + 8, date_s, 2 * sizeof (char));
							memcpy (raw_date_s + 6, date_s, 2 * sizeof (char));

							success_flag = true;
						}

				}		/* if ((* (date_s + 2) == '/') && (* (date_s + 5) == '/')) */
			else
				{
#define NUM_MONTHS (12)
					const char *months_ss [NUM_MONTHS] = { "jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec" };

					bool loop_flag = true;
					uint month_index = 0;
					uint8 i = 0;

					while (loop_flag)
						{
							const char *month_s = * (months_ss + i);

							if (Strnicmp (month_s, date_s, strlen (month_s)) == 0)
								{
									month_index = i;
									loop_flag = false;
								}
							else
								{
									++ i;

									if (i == NUM_MONTHS)
										{
											loop_flag = false;
										}
								}

						}		/* while (loop_flag) */

					if (month_index > 0)
						{
							const char *value_p = date_s + strlen (* (months_ss + month_index));

							loop_flag = (*value_p != '\0');

							while (loop_flag)
								{
									if (isdigit (*value_p))
										{
											loop_flag = false;
										}
									else
										{
											++ value_p;

											loop_flag = (*value_p != '\0');
										}
								}

							if (*value_p != '\0')
								{
									/* Do we have a number? */
									if (isdigit (* (value_p + 1)))
										{
											bool match_flag = false;

											/* Do we have a 2 digit year... */
											if (* (value_p + 2) == '\0')
												{
													*iso_date_s = '2';
													* (iso_date_s + 1) = '0';
													memcpy (iso_date_s + 2, value_p, 2 * sizeof (char));

													*raw_date_s = '2';
													* (raw_date_s + 1) = '0';
													memcpy (raw_date_s + 2, value_p, 2 * sizeof (char));

													match_flag = true;
												}
											/* ... or a 4 digit one? */
											else if ((isdigit (* (value_p + 2))) && (isdigit (* (value_p + 3))))
												{
													memcpy (iso_date_s, value_p, 4 * sizeof (char));
													memcpy (raw_date_s, value_p, 4 * sizeof (char));

													match_flag = true;
												}

											if (match_flag)
												{
													char month_s [3];

													sprintf (month_s, "%02d", month_index + 1);

													memcpy (iso_date_s + 5, month_s, 2 * sizeof (char));
													* (iso_date_s + 8) = '0';
													* (iso_date_s + 9) = '1';

													memcpy (raw_date_s + 4, month_s, 2 * sizeof (char));
													* (raw_date_s + 6) = '0';
													* (raw_date_s + 7) = '1';

													success_flag = true;
												}

										}
								}
						}		/* if (month_index > 0) */
				}

			if (success_flag)
				{
					if (SetDateForSchemaOrg (row_p, PG_DATE_S, iso_date_s))
						{
							if (json_object_set_new (row_p, PG_RAW_DATE_S, json_string (raw_date_s)) != 0)
								{
									PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to set raw date to " INT32_FMT, raw_date_s);
									success_flag = false;
								}
						}
					else
						{
							PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to SetDateForSchemaOrge for %s", iso_date_s);
							success_flag = false;
						}
				}
			else
				{
					char *dump_s = json_dumps (row_p, JSON_INDENT (2) | JSON_PRESERVE_ORDER);

					PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to get date from %s", dump_s);

					free (dump_s);
				}


		}		/* if (date_s) */
	else
		{
			char *dump_s = json_dumps (row_p, JSON_INDENT (2) | JSON_PRESERVE_ORDER);

			PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "No date for %s", date_s, dump_s);

			free (dump_s);
		}

	return success_flag;
}


bool GetGeoLocationObjectForSchemaOrg (json_t *parent_p, const char *key_s, const double latitude, const double longitude)
{
	bool success_flag = false;
	json_t *child_p = json_object ();

	if (child_p)
		{
			if ((json_object_set_new (child_p, "@type", json_string ("GeoCoordinates")) == 0) &&
					(json_object_set_new (child_p, PG_LATITUDE_S, json_real (latitude)) == 0) &&
					(json_object_set_new (child_p, PG_LONGITUDE_S, json_real (longitude)) == 0))
				{
					if (json_object_set_new (parent_p, key_s, child_p) == 0)
						{
							success_flag = true;

						}
				}

			if (!success_flag)
				{
					WipeJSON (child_p);
				}
		}		/* if (child_p) */

	return success_flag;
}


bool GetLocationDataByGoogle (PathogenomicsServiceData *data_p, json_t *row_p, const char * const id_s)
{
	bool got_location_flag = false;
	const char *town_s = GetJSONString (row_p, PG_TOWN_S);
	const char *county_s = GetJSONString (row_p, PG_COUNTY_S);
	const char *country_s = GetJSONString (row_p, PG_COUNTRY_S);
	const char *postcode_s = GetJSONString (row_p, PG_POSTCODE_S);
	const char *gps_s = GetJSONString (row_p, PG_GPS_S);
	const char *country_code_s = NULL;


	if (gps_s)
		{
			/*
			 * Start to try and scan the coords as decimals
			 */
			const char *value_s = gps_s;
			double latitude;
			double longitude;
			bool match_flag = false;

			if (GetValidRealNumber (&value_s, &latitude, ","))
				{
					while (!isdigit (*value_s))
						{
							++ value_s;
						}

					if (*value_s != '\0')
						{
							if (GetValidRealNumber (&value_s, &longitude, ","))
								{
									while ((isspace (*value_s)) && (*value_s != '\0'))
										{
											++ value_s;
										}

									match_flag = (*value_s == '\0');
								}
						}
				}

			if (match_flag)
				{
					got_location_flag = GetGeoLocationObjectForSchemaOrg (row_p, PG_LOCATION_S, latitude, longitude);

					if (json_object_del (row_p, PG_GPS_S) != 0)
						{
							PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to remove raw gps coords from %s", id_s);
						}
				}
		}
	else
		{
			ByteBuffer *buffer_p = AllocateByteBuffer (1024);

			if (buffer_p)
				{
					if (AppendStringToByteBuffer (buffer_p, data_p -> psd_geocoder_uri_s))
						{
							bool success_flag = true;
							bool added_query_flag = false;

							/* Replace UK entries with GB */
							if (country_s)
								{
									if (strcmp (country_s, "UK") == 0)
										{
											country_s = "GB";
										}
								}

							/* post code */
							if (postcode_s)
								{
									const char *value_s = postcode_s;

									/* remove and leading spaces */
									while (isspace (*value_s))
										{
											++ value_s;
										}

									success_flag = (*value_s != '\0') && AppendStringsToByteBuffer (buffer_p, "&components=postal_code:", value_s, NULL);

									/* country */
									if (success_flag)
										{
											if (country_s)
												{
													if (IsValidCountryCode (country_s))
														{
															country_code_s = country_s;
														}
													else
														{
															country_code_s = GetCountryCodeFromName (country_s);
														}

													if (country_code_s)
														{
															success_flag = AppendStringsToByteBuffer (buffer_p, "|country:", country_code_s, NULL);
														}


												}

										}		/* if (success_flag) */

								}		/* if (postcode_s) */
							else
								{
									/* town */
									if (town_s)
										{
											success_flag = AppendStringsToByteBuffer (buffer_p, "&address=", town_s, NULL);
											added_query_flag = true;
										}

									/* county */
									if (success_flag)
										{
											county_s = GetJSONString (row_p, PG_COUNTY_S);

											if (county_s)
												{
													if (added_query_flag)
														{
															success_flag = AppendStringsToByteBuffer (buffer_p, ",%20", county_s, NULL);
														}
													else
														{
															success_flag = AppendStringsToByteBuffer (buffer_p, "&address=", county_s, NULL);
															added_query_flag = true;
														}
												}		/* if (county_s) */

										}		/* if (success_flag) */


									/* country */
									if (success_flag)
										{
											if (country_s)
												{
													if (IsValidCountryCode (country_s))
														{
															country_code_s = country_s;
														}
													else
														{
															country_code_s = GetCountryCodeFromName (country_s);
														}

													if (country_code_s)
														{
															success_flag = AppendStringsToByteBuffer (buffer_p, "&components=country:", country_code_s, NULL);
														}
												}
										}

								}

							if (success_flag)
								{
									CurlTool *curl_tool_p = AllocateCurlTool (CM_MEMORY);

									if (curl_tool_p)
										{
											const char *uri_s = NULL;

											ReplaceCharsInByteBuffer (buffer_p, ' ', '+');
											uri_s = GetByteBufferData (buffer_p);
											PrintLog (STM_LEVEL_INFO, __FILE__, __LINE__, "uri for %s is \"%s\"\n", id_s, uri_s);

											if (SetUriForCurlTool (curl_tool_p, uri_s))
												{
													CURLcode c = RunCurlTool (curl_tool_p);

													if (c == CURLE_OK)
														{
															const char *response_s = GetCurlToolData (curl_tool_p);

															if (response_s)
																{
																	json_error_t error;
																	json_t *raw_res_p = NULL;

																	PrintLog (STM_LEVEL_INFO, __FILE__, __LINE__, "geo response for %s\n%s\n", uri_s, response_s);

																	raw_res_p = json_loads (response_s, 0, &error);

																	if (raw_res_p)
																		{
																			PrintJSONToLog (STM_LEVEL_FINE, __FILE__, __LINE__, raw_res_p, "raw: ");
																			got_location_flag = RefineLocationDataForGoogle (data_p, row_p, raw_res_p, town_s, county_s);

																			WipeJSON (raw_res_p);
																		}
																	else
																		{

																		}
																}
														}
													else
														{

														}
												}


											FreeCurlTool (curl_tool_p);
										}		/* if (curl_tool_p) */

								}

						}		/* if (AppendStringToByteBuffer (buffer_p, data_p -> msd_geocoding_uri_s)) */

					FreeByteBuffer (buffer_p);
				}		/* if (buffer_p) */

		}


	ParseAddressForSchemaOrg (row_p, town_s, county_s, country_code_s, postcode_s);

	json_object_del (row_p, PG_TOWN_S);
	json_object_del (row_p, PG_COUNTY_S);
	json_object_del (row_p, PG_COUNTRY_S);
	json_object_del (row_p, PG_POSTCODE_S);
	json_object_del (row_p, PG_GPS_S);


	return got_location_flag;
}


bool GetLocationDataByOpenCage (PathogenomicsServiceData *data_p, json_t *row_p, const char * const id_s)
{
	bool got_location_flag = false;
	json_t *res_p = NULL;

	const char *gps_s = GetJSONString (row_p, PG_GPS_S);

	if (gps_s)
		{

		}
	else
		{
			ByteBuffer *buffer_p = AllocateByteBuffer (1024);

			if (buffer_p)
				{
					if (AppendStringToByteBuffer (buffer_p, data_p -> psd_geocoder_uri_s))
						{
							const char *town_s = GetJSONString (row_p, PG_TOWN_S);
							const char *county_s = NULL;
							const char *country_code_s = NULL;
							bool success_flag = true;
							bool added_query_flag = false;

							/* town */
							if (town_s)
								{
									success_flag = AppendStringsToByteBuffer (buffer_p, "&query=", town_s, NULL);
									added_query_flag = true;
								}

							/* county */
							if (success_flag)
								{
									county_s = GetJSONString (row_p, PG_COUNTY_S);

									if (county_s)
										{
											if (added_query_flag)
												{
													success_flag = AppendStringsToByteBuffer (buffer_p, ",%20", county_s, NULL);
												}
											else
												{
													success_flag = AppendStringsToByteBuffer (buffer_p, "&query=", county_s, NULL);
													added_query_flag = true;
												}
										}		/* if (county_s) */

								}		/* if (success_flag) */


							/* country */
							if (success_flag)
								{
									const char *value_s = GetJSONString (row_p, PG_COUNTRY_S);

									if (value_s)
										{
											if (IsValidCountryCode (value_s))
												{
													country_code_s = value_s;
												}
											else
												{
													country_code_s = GetCountryCodeFromName (value_s);
												}

											if (country_code_s)
												{
													success_flag = AppendStringsToByteBuffer (buffer_p, "&countrycode=", country_code_s, NULL);
												}

										}

								}		/* if (success_flag) */


							if (success_flag)
								{
									CurlTool *curl_tool_p = AllocateCurlTool (CM_MEMORY);

									if (curl_tool_p)
										{
											const char *uri_s = GetByteBufferData (buffer_p);

											if (SetUriForCurlTool (curl_tool_p, uri_s))
												{
													CURLcode c = RunCurlTool (curl_tool_p);

													if (c == CURLE_OK)
														{
															const char *response_s = GetCurlToolData (curl_tool_p);

															if (response_s)
																{
																	json_error_t error;
																	json_t *raw_res_p = NULL;

																	PrintLog (STM_LEVEL_INFO, __FILE__, __LINE__, "geo response for %s\n%s\n", uri_s, response_s);

																	raw_res_p = json_loads (response_s, 0, &error);

																	if (raw_res_p)
																		{
																			char *dump_s = json_dumps (res_p, JSON_INDENT (2) | JSON_PRESERVE_ORDER);

																			PrintLog (STM_LEVEL_INFO, __FILE__, __LINE__, "json:\n%s\n", dump_s);
																			free (dump_s);

																			/*
																			res_p = data_p -> msd_refine_location_fn (data_p, raw_res_p, town_s, county_s, country_code_s);

																			WipeJSON (raw_res_p);
																			 */

																			/** TODO */
																			res_p = raw_res_p;
																		}
																	else
																		{

																		}
																}
														}
													else
														{

														}
												}


											FreeCurlTool (curl_tool_p);
										}		/* if (curl_tool_p) */

								}

						}		/* if (AppendStringToByteBuffer (buffer_p, data_p -> msd_geocoding_uri_s)) */

					FreeByteBuffer (buffer_p);
				}		/* if (buffer_p) */

		}

	return got_location_flag;
}


/*
 {
    "results" : [
      {
         "address_components" : [
            {
               "long_name" : "Werbig",
               "short_name" : "Werbig",
               "types" : [ "sublocality_level_1", "sublocality", "political" ]
            },
            {
               "long_name" : "Brandenburg",
               "short_name" : "BB",
               "types" : [ "administrative_area_level_1", "political" ]
            },
            {
               "long_name" : "Germany",
               "short_name" : "DE",
               "types" : [ "country", "political" ]
            }
         ],
         "formatted_address" : "Werbig, Germany",
         "geometry" : {
            "bounds" : {
               "northeast" : {
                  "lat" : 52.5949616,
                  "lng" : 14.4702795
               },
               "southwest" : {
                  "lat" : 52.54852649999999,
                  "lng" : 14.3639304
               }
            },
            "location" : {
               "lat" : 52.5718829,
               "lng" : 14.3937375
            },
            "location_type" : "APPROXIMATE",
            "viewport" : {
               "northeast" : {
                  "lat" : 52.5949616,
                  "lng" : 14.4702795
               },
               "southwest" : {
                  "lat" : 52.54852649999999,
                  "lng" : 14.3639304
               }
            }
         },
         "place_id" : "ChIJQzFlvbR5B0cRQL6B1kggIQo",
         "types" : [ "sublocality_level_1", "sublocality", "political" ]
      },
      {
         "address_components" : [
            {
               "long_name" : "Werbig",
               "short_name" : "Werbig",
               "types" : [ "sublocality_level_1", "sublocality", "political" ]
            },
            {
               "long_name" : "Bad Belzig",
               "short_name" : "Bad Belzig",
               "types" : [ "locality", "political" ]
            },
            {
               "long_name" : "Brandenburg",
               "short_name" : "BB",
               "types" : [ "administrative_area_level_1", "political" ]
            },
            {
               "long_name" : "Germany",
               "short_name" : "DE",
               "types" : [ "country", "political" ]
            }
         ],
         "formatted_address" : "Werbig, Bad Belzig, Germany",
         "geometry" : {
            "bounds" : {
               "northeast" : {
                  "lat" : 52.2306637,
                  "lng" : 12.5418018
               },
               "southwest" : {
                  "lat" : 52.1621099,
                  "lng" : 12.4341461
               }
            },
            "location" : {
               "lat" : 52.2024014,
               "lng" : 12.4757793
            },
            "location_type" : "APPROXIMATE",
            "viewport" : {
               "northeast" : {
                  "lat" : 52.2306637,
                  "lng" : 12.5418018
               },
               "southwest" : {
                  "lat" : 52.1621099,
                  "lng" : 12.4341461
               }
            }
         },
         "place_id" : "ChIJGdomWMeiqEcRVKxTUIFd3gs",
         "types" : [ "sublocality_level_1", "sublocality", "political" ]
      },
      {
         "address_components" : [
            {
               "long_name" : "Werbig",
               "short_name" : "Werbig",
               "types" : [ "sublocality_level_1", "sublocality", "political" ]
            },
            {
               "long_name" : "Niederer Fläming",
               "short_name" : "Niederer Fläming",
               "types" : [ "locality", "political" ]
            },
            {
               "long_name" : "Brandenburg",
               "short_name" : "BB",
               "types" : [ "administrative_area_level_1", "political" ]
            },
            {
               "long_name" : "Germany",
               "short_name" : "DE",
               "types" : [ "country", "political" ]
            },
            {
               "long_name" : "14913",
               "short_name" : "14913",
               "types" : [ "postal_code" ]
            }
         ],
         "formatted_address" : "Werbig, 14913 Niederer Fläming, Germany",
         "geometry" : {
            "bounds" : {
               "northeast" : {
                  "lat" : 51.9568939,
                  "lng" : 13.2256467
               },
               "southwest" : {
                  "lat" : 51.9140655,
                  "lng" : 13.1596687
               }
            },
            "location" : {
               "lat" : 51.9320151,
               "lng" : 13.1920298
            },
            "location_type" : "APPROXIMATE",
            "viewport" : {
               "northeast" : {
                  "lat" : 51.9568939,
                  "lng" : 13.2256467
               },
               "southwest" : {
                  "lat" : 51.9140655,
                  "lng" : 13.1596687
               }
            }
         },
         "place_id" : "ChIJsceHQvDYp0cR8BCC1kggIQo",
         "types" : [ "sublocality_level_1", "sublocality", "political" ]
      }
   ],
   "status" : "OK"
}
 */

bool RefineLocationDataForGoogle (PathogenomicsServiceData *service_data_p, json_t *row_p, const json_t *raw_data_p, const char * const town_s, const char * const county_s)
{
	bool success_flag = false;
	const json_t *results_p = json_object_get (raw_data_p, "results");

	if (results_p)
		{
			if (json_is_array (results_p))
				{
					const size_t size = json_array_size (results_p);
					size_t i = 0;

					while ((i < size) && (!success_flag))
						{
							const json_t *result_p = json_array_get (results_p, i);

							if (FillInPathogenomicsFromGoogleData (result_p, row_p))
								{
									success_flag = true;
								}
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, result_p, "FillInPathogenomicsFromGoogleData failed");
								}

							++ i;
						}		/* while (i < size) */

				}		/* if (json_is_array (results_p)) */
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, results_p, "results is not an array");
				}

		}		/* if (results_p) */
	else
		{
			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, raw_data_p, "Failed to get results array");
		}

	return success_flag;
}



/****************************************/
/********** STATIC FUNCTIONS ************/
/****************************************/


static bool AddValidJSONField (json_t *json_p, const char *key_s, const char *value_s)
{
	bool success_flag = true;

	if (key_s && value_s)
		{
			success_flag = (json_object_set_new (json_p, key_s, json_string (value_s)) == 0);
		}

	return success_flag;
}


static bool ParseAddressForSchemaOrg (json_t *values_p, const char * const town_s, const char * const county_s, const char * const country_s, const char * const postcode_s)
{
	bool success_flag = false;

	if (town_s || county_s || country_s || postcode_s)
		{
			json_t *postal_address_p = json_object ();

			success_flag = false;

			if (postal_address_p)
				{
					if (json_object_set_new (postal_address_p, "@type", json_string ("PostalAddress")) == 0)
						{
							if (AddValidJSONField (postal_address_p, "addressLocality", town_s))
								{
									success_flag = true;
								}

							if (AddValidJSONField (postal_address_p, "addressRegion", county_s))
								{
									success_flag = true;
								}

							if (AddValidJSONField (postal_address_p, "addressCountry", country_s))
								{
									success_flag = true;
								}

							if (AddValidJSONField (postal_address_p, "postalCode", postcode_s))
								{
									success_flag = true;
								}

							if (success_flag)
								{
									success_flag = (json_object_set_new (values_p, PG_ADDRESS_S, postal_address_p) == 0);
								}
						}

					if (!success_flag)
						{
							WipeJSON (postal_address_p);
						}
				}

		}		/* if (town_s || county_s || country_s || postcode_s) */


	return success_flag;
}




static bool ConvertToSchemaOrgRepresentation (json_t *values_p, const char * const input_key_s, const char * const type_s, const char * const output_subkey_s)
{
	bool success_flag = true;
	char *values_s = json_dumps (values_p, JSON_INDENT (2) | JSON_PRESERVE_ORDER);
	const char *input_value_s = GetJSONString (values_p, input_key_s);

	if (input_value_s)
		{
			json_t *child_p = json_object ();

			success_flag = false;

			if (child_p)
				{
					if ((json_object_set_new (child_p, "@type", json_string (type_s)) == 0) &&
							(json_object_set_new (child_p, output_subkey_s, json_string (input_value_s)) == 0))
						{
							if (json_object_set_new (values_p, input_key_s, child_p) == 0)
								{
									success_flag = true;
								}
							else
								{
									PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to write child object %s for ", input_key_s, values_s ? values_s : "input data");
								}
						}
					else
						{
							PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to create set @type to %s and %s to %s for child json object", type_s, output_subkey_s, input_key_s);
						}

					if (!success_flag)
						{
							WipeJSON (child_p);
						}
				}
			else
				{
					PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to create child json object for converting %s in %s", input_key_s, values_s ? values_s : "input data");
				}
		}
	else
		{
			PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to find %s in %s", input_key_s, values_s ? values_s : "input data");
		}

	if (values_s)
		{
			free (values_s);
		}

	return success_flag;
}



static bool ParseCollector (json_t *values_p)
{
	return ConvertToSchemaOrgRepresentation (values_p, PG_COLLECTOR_S, "Person", "name");
}



static bool ParseCompany (json_t *values_p)
{
	return ConvertToSchemaOrgRepresentation (values_p, PG_COMPANY_S, "Organization", "name");
}



static bool AddSchemaOrgContext (json_t *values_p)
{
	bool success_flag = true;
	const char * const context_key_s = "@context";
	const char * const context_value_s = "http://schema.org";
	const char *context_s = GetJSONString (values_p, context_key_s);

	if ((!context_s) || (strcmp (context_s, context_value_s) != 0))
		{
			success_flag = (json_object_set_new (values_p,  context_key_s, json_string (context_value_s)) == 0);
		}

	return success_flag;
}


static const char *PrepareSampleData (MongoTool *tool_p, json_t *values_p, PathogenomicsServiceData *data_p, const char *pathogenomics_id_s)
{
	const char *error_s = NULL;

	if (AddSchemaOrgContext (values_p))
		{
			if (ConvertDate (values_p))
				{
					if (GetLocationData (tool_p, values_p, data_p, pathogenomics_id_s))
						{
							/* convert YR/SR/LR to yellow, stem or leaf rust */
							if (ReplacePathogen (values_p))
								{
									if (ParseCollector (values_p))
										{
											if (!ParseCompany (values_p))
												{
													error_s = "Failed to parse company value";
												}

										}		/* if (ParseCollector (values_p)) */
									else
										{
											error_s = "Failed to parse collector value";
										}

								}		/* if (ReplacePathogen (values_p) */
							else
								{
									error_s = "Failed to convert pathogen name";
								}

						}		/* if (GetLocationData (tool_p, values_p, data_p, pathogenomics_id_s)) */
					else
						{
							error_s = "Failed to add location data into system";
						}

				}		/* if (ConvertDate (values_p)) */
			else
				{
					error_s = "Could not get date";
				}

		}		/* if (AddSchemaOrgContext (values_p)) */
	else
		{
			error_s = "Could not set json-ld context to schema.org";
		}

	return error_s;
}


static const char *MergeData (MongoTool *tool_p, json_t *values_p, const char * const pathogenomics_id_s, const char * const ukcpvs_id_s, json_t **selector_pp)
{
	const char *error_s = NULL;
	json_t *ukcpvs_docs_p = NULL;
	int32 num_ukcpvs_matches = GetAllMongoResultsForKeyValuePair (tool_p, &ukcpvs_docs_p, PG_UKCPVS_ID_S, ukcpvs_id_s, NULL);

	if (num_ukcpvs_matches == 1)
		{
			json_t *id_docs_p = NULL;
			int32 num_id_matches = GetAllMongoResultsForKeyValuePair (tool_p, &id_docs_p, PG_ID_S, pathogenomics_id_s, NULL);

			if (num_id_matches == 1)
				{
					/* We need to merge the existing documents together and then update the values with the sample data */
					json_t *ukcpvs_doc_p = json_array_get (ukcpvs_docs_p, 0);

					if (ukcpvs_doc_p)
						{
							json_t *id_doc_p = json_array_get (id_docs_p, 0);

							if (id_doc_p)
								{
									json_error_t err;

									/* remove the mongodb _id attribute */
									json_object_del (ukcpvs_doc_p, MONGO_ID_S);

									/* after the update call, ukcpvs_id_s will go out of scope so store its value */
									*selector_pp = json_pack_ex (&err, 0, "{s:s}", PG_UKCPVS_ID_S, ukcpvs_id_s);

									if (*selector_pp)
										{
											/* Update our sample data with the values from ukcpvs_id_p */
											if (!json_object_update (values_p, ukcpvs_doc_p) == 0)
												{
													error_s = "Failed to merge ukcpvs_id-based data with our sample data";
												}
										}
									else
										{
											error_s = "Failed to store ukcpvs_id-based data for merging";
										}

								}		/* if (id_doc_p) */

						}		/* if (ukcpvs_doc_p) */

				}		/* if (num_id_matches == 1) */

		}		/* if (num_ukcpvs_matches == 1) */

	return error_s;
}




static bool FillInPathogenomicsFromGoogleData (const json_t *google_result_p, json_t *doc_p)
{
	bool success_flag = false;
	json_t *location_values_p = json_object ();

	if (location_values_p)
		{
			const json_t *geometry_p = json_object_get (google_result_p, "geometry");

			if (geometry_p)
				{
					const json_t *location_p = json_object_get (geometry_p, "location");
					const char * const LATITUDE_KEY_S = "lat";
					const char * const LONGITUDE_KEY_S = "lng";

#if SAMPLE_METADATA_DEBUG >= STM_LEVEL_FINE
					PrintJSONToLog (STM_LEVEL_FINE, __FILE__, __LINE__, geometry_p, "geometry_p: ");
#endif

					if (location_p)
						{
							const json_t *viewport_p = json_object_get (geometry_p, "viewport");
							double latitude;
							double longitude;

							if (GetJSONReal (location_p, LATITUDE_KEY_S, &latitude))
								{
									if (GetJSONReal (location_p, LONGITUDE_KEY_S, &longitude))
										{
											success_flag = GetGeoLocationObjectForSchemaOrg (location_values_p, PG_LOCATION_S, latitude, longitude);
										}
								}

							if (viewport_p)
								{
									const json_t *corner_p = json_object_get (viewport_p, "northeast");

									if (corner_p)
										{
											if (GetJSONReal (corner_p, LATITUDE_KEY_S, &latitude))
												{
													if (GetJSONReal (corner_p, LONGITUDE_KEY_S, &longitude))
														{
															if (!GetGeoLocationObjectForSchemaOrg (location_values_p, PG_NORTH_EAST_LOCATION_S, latitude, longitude))
																{
																	PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to set north east location from %lf %lf ", latitude, longitude);
																}
														}
												}
										}

									corner_p = json_object_get (viewport_p, "southwest");

									if (corner_p)
										{
											if (GetJSONReal (corner_p, LATITUDE_KEY_S, &latitude))
												{
													if (GetJSONReal (corner_p, LONGITUDE_KEY_S, &longitude))
														{
															if (!GetGeoLocationObjectForSchemaOrg (location_values_p, PG_SOUTH_WEST_LOCATION_S, latitude, longitude))
																{
																	PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to set south west location from %lf %lf ", latitude, longitude);
																}
														}
												}
										}
								}		/* if (viewport_p) */

						}		/* if (location_p) */

				}		/* if (geometry_p) */

			if (success_flag)
				{
					success_flag = (json_object_set_new (doc_p, PG_LOCATION_S, location_values_p) == 0);
				}

			if (!success_flag)
				{
					WipeJSON (location_values_p);
				}

		}		/* if (location_values_p) */


	return success_flag;
}



/*
 {
    "licenses" : [
       {
          "name" : "CC-BY-SA",
          "url" : "http://creativecommons.org/licenses/by-sa/3.0/"
       },
       {
          "name" : "ODbL",
          "url" : "http://opendatacommons.org/licenses/odbl/summary/"
       }
    ],
    "rate" : {
       "limit" : 2500,
       "remaining" : 2489,
       "reset" : 1439337600
    },
    "results" : [
       {
          "annotations" : {
             "DMS" : {
                "lat" : "52\u00b0 37' 42.98160'' N",
                "lng" : "1\u00b0 17' 32.17200'' E"
             },
             "MGRS" : "31UCU8441732326",
             "Maidenhead" : "JO02pp50bu",
             "Mercator" : {
                "x" : 143854.838,
                "y" : 6880612.848
             },
             "OSGB" : {
                "easting" : 622783.039,
                "gridref" : "TG 227 085",
                "northing" : 308555.893
             },
             "OSM" : {
                "url" : "http://www.openstreetmap.org/?mlat=52.62861&mlon=1.29227#map=17/52.62861/1.29227"
             },
             "callingcode" : 44,
             "geohash" : "u12gmktpb54r76b7ngce",
             "sun" : {
                "rise" : {
                   "astronomical" : 1439257860,
                   "civil" : 1439265060,
                   "nautical" : 1439261940
                },
                "set" : {
                   "astronomical" : 1439330700,
                   "civil" : 1439323680,
                   "nautical" : 1439326800
                }
             },
             "timezone" : {
                "name" : "Europe/London",
                "now_in_dst" : 1,
                "offset_sec" : 3600,
                "offset_string" : 100,
                "short_name" : "BST"
             },
             "what3words" : {
                "words" : "dragon.crunch.coins"
             }
          },
          "bounds" : {
             "northeast" : {
                "lat" : 52.6849096,
                "lng" : 1.5407848
             },
             "southwest" : {
                "lat" : 52.555435,
                "lng" : 1.2038691
             }
          },
          "components" : {
             "city" : "Norwich",
             "country" : "United Kingdom",
             "country_code" : "gb",
             "county" : "Norfolk",
             "state" : "England",
             "state_district" : "East of England"
          },
          "confidence" : 5,
          "formatted" : "Norwich, Norfolk, United Kingdom",
          "geometry" : {
             "lat" : 52.628606,
             "lng" : 1.29227
          }
       },
       {
          "annotations" : {
             "DMS" : {
                "lat" : "52\u00b0 37' 36.40656'' N",
                "lng" : "1\u00b0 18' 24.44434'' E"
             },
             "MGRS" : "31UCU8539432100",
             "Maidenhead" : "JO02pp60tk",
             "Mercator" : {
                "x" : 145471.208,
                "y" : 6880278.724
             },
             "OSGB" : {
                "easting" : 623774.629,
                "gridref" : "TG 237 083",
                "northing" : 308397.845
             },
             "OSM" : {
                "url" : "http://www.openstreetmap.org/?mlat=52.62678&mlon=1.30679#map=17/52.62678/1.30679"
             },
             "callingcode" : 44,
             "geohash" : "u12gmsrt4qv26np1jsqv",
             "sun" : {
                "rise" : {
                   "astronomical" : 1439257860,
                   "civil" : 1439265060,
                   "nautical" : 1439261940
                },
                "set" : {
                   "astronomical" : 1439330700,
                   "civil" : 1439323680,
                   "nautical" : 1439326740
                }
             },
             "timezone" : {
                "name" : "Europe/London",
                "now_in_dst" : 1,
                "offset_sec" : 3600,
                "offset_string" : 100,
                "short_name" : "BST"
             },
             "what3words" : {
                "words" : "ample.sketch.cares"
             }
          },
          "bounds" : {
             "northeast" : {
                "lat" : 52.6271858,
                "lng" : 1.3078487
             },
             "southwest" : {
                "lat" : 52.6263854,
                "lng" : 1.306142
             }
          },
          "components" : {
             "city" : "Norwich",
             "country" : "United Kingdom",
             "country_code" : "gb",
             "county" : "Norfolk",
             "postcode" : "NR1 1EF",
             "road" : "Station Approach",
             "state" : "England",
             "state_district" : "East of England",
             "station" : "Norwich",
             "suburb" : "Thorpe Hamlet"
          },
          "confidence" : 10,
          "formatted" : "Norwich, Station Approach, Norwich NR1 1EF, United Kingdom",
          "geometry" : {
             "lat" : 52.6267796,
             "lng" : 1.3067900949115
          }
       }
    ],
    "status" : {
       "code" : 200,
       "message" : "OK"
    },
    "stay_informed" : {
       "blog" : "http://blog.opencagedata.com",
       "twitter" : "https://twitter.com/opencagedata"
    },
    "thanks" : "For using an OpenCage Data API",
    "timestamp" : {
       "created_http" : "Tue, 11 Aug 2015 16:18:05 GMT",
       "created_unix" : 1439309885
    },
    "total_results" : 2
 }
 */

bool RefineLocationDataForOpenCage (PathogenomicsServiceData *service_data_p, json_t *row_p, const json_t *raw_data_p, const char * const town_s, const char * const county_s)
{
	json_t *results_array_p = json_object_get (raw_data_p, "results");

	if (results_array_p)
		{
			if (json_is_array (results_array_p))
				{
					size_t index;
					json_t *result_p;

					json_array_foreach (results_array_p, index, result_p)
					{
						json_t *address_p = json_object_get (result_p, "components");

						if (address_p)
							{
								#if SAMPLE_METADATA_DEBUG >=STM_LEVEL_FINE
								PrintJSONToLog (STM_LEVEL_FINE, __FILE__, __LINE__, address_p, "Address: ");
								#endif

								if (county_s)
									{
										const char *result_county_s = GetJSONString (address_p, "county");

										if (result_county_s)
											{
												if (Stricmp (county_s, result_county_s) == 0)
													{
														return true;
													}
											}
									}		/* if (county_s) */

							}		/* if (address_p) */

					}		/* json_array_foreach (raw_res_p, index, raw_result_p) */

				}		/* if (json_is_array (results_p)) */

		}		/* if (results_p) */


	return false;
}


static bool ReplacePathogen (json_t *data_p)
{
	bool success_flag = true;
	const char *value_s = NULL;
	const char *pathogen_s = GetJSONString (data_p, PG_RUST_S);

	if (pathogen_s)
		{
			if ((strcmp ("YR", pathogen_s) == 0) || (strcmp ("Yellow Rust", pathogen_s) == 0))
				{
					value_s = "Yellow Rust";
				}
			else if ((strcmp ("SR", pathogen_s) == 0) || (strcmp ("Stem Rust", pathogen_s) == 0))
				{
					value_s = "Stem Rust";
				}
			else if ((strcmp ("LR", pathogen_s) == 0) || (strcmp ("Leaf Rust", pathogen_s) == 0))
				{
					value_s = "Leaf Rust";
				}

			if (value_s)
				{
					success_flag = (json_object_set_new (data_p, PG_DISEASE_S, json_string (value_s)) == 0);

					if (success_flag)
						{
							json_object_del (data_p, PG_RUST_S);
						}
				}
		}
	else
		{
			char *data_s = json_dumps (data_p, JSON_INDENT (2) | JSON_PRESERVE_ORDER);
			PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Could not find %s in %s", PG_RUST_S, data_s ? data_s : "input data");

			if (data_s)
				{
					free (data_s);
				}
		}

	return success_flag;
}


static const char *AddToFieldsCollection (MongoTool *tool_p, json_t *values_p, const char * const collection_s, PathogenomicsServiceData *data_p)
{
	const char *error_s = NULL;

	/**
	 * Add the fields to the list of available fields for this
	 * collection
	 */
	char *fields_collection_s = ConcatenateStrings (collection_s, ".fields");

	if (fields_collection_s)
		{
			json_t *field_p = json_object ();
			bson_oid_t doc_id;

			if (field_p)
				{
					const char *fields_ss [2] = { NULL, NULL };
					const char *key_s;
					json_t *value_p;

					SetMongoToolCollection (tool_p, data_p -> psd_database_s, fields_collection_s);

					json_object_foreach (values_p, key_s, value_p)
					{
						bson_t *query_p = bson_new (); //BCON_NEW ("$query", "{", key_s, BCON_INT32 (1), "}");

						if (query_p)
							{
								int32 value = 0;
								json_t *update_p = NULL;
								json_error_t error;

								*fields_ss = key_s;

								if (FindMatchingMongoDocumentsByBSON (tool_p, query_p, fields_ss))
									{
										const bson_t *doc_p = NULL;

										/* should only be one of these */
										while (mongoc_cursor_next (tool_p -> mt_cursor_p, &doc_p))
											{
												bson_iter_t iter;
												json_t *json_value_p = NULL;

												#if SAMPLE_METADATA_DEBUG >= STM_LEVEL_FINE
												LogAllBSON (doc_p, STM_LEVEL_FINE, __FILE__, __LINE__, "matched doc: ");
												#endif

												if (bson_iter_init (&iter, doc_p))
													{
														if (bson_iter_find (&iter, "_id"))
															{
																const bson_oid_t *src_p = NULL;

																if (BSON_ITER_HOLDS_OID (&iter))
																	{
																		src_p = (const bson_oid_t  *) bson_iter_oid (&iter);
																	}

																bson_oid_copy (src_p, &doc_id);

																#if SAMPLE_METADATA_DEBUG >= STM_LEVEL_FINE
																LogBSONOid (src_p, STM_LEVEL_FINE, __FILE__, __LINE__, "src_p");
																LogBSONOid (&doc_id, STM_LEVEL_FINE, __FILE__, __LINE__, "doc id");
																#endif
															}
													}

												json_value_p = ConvertBSONToJSON (doc_p);

												if (json_value_p)
													{
														json_t *count_p = json_object_get (json_value_p, key_s);

														if (count_p)
															{
																if (json_is_integer (count_p))
																	{
																		value = json_integer_value (count_p);
																	}
															}

														WipeJSON (json_value_p);
													}
											}
									}

								++ value;

								/*
								 * Now need to set key_s = value into the collection
								 */
								update_p = json_pack_ex (&error, 0, "{s:i}", key_s, value);

								if (update_p)
									{
										if (value == 1)
											{
												bson_oid_t *fields_id_p = InsertJSONIntoMongoCollection (tool_p, update_p);

												if (fields_id_p)
													{
														FreeMemory (fields_id_p);
													}
												else
													{
														PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to update %s in %s", key_s, fields_collection_s);
														error_s = "Failed to insert data into fields collection";
													}
											}
										else
											{
												if (!UpdateMongoDocument (tool_p, (bson_oid_t *) &doc_id, update_p))
													{
														PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to update %s in %s", key_s, fields_collection_s);
														error_s = "Failed to update data in fields collection";
													}
											}
									}
								else
									{

										PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to create json for updating fields collection %s,  %s", fields_collection_s, error.text);
										error_s = "Failed to create JSON data to update fields collection";
									}

								bson_destroy (query_p);
							}
					}		/* json_object_foreach (values_p, key_s, value_p) */


				}		/* if (field_p) */


			FreeCopiedString (fields_collection_s);
		}		/* if (fields_collection_s) */

	return error_s;
}
