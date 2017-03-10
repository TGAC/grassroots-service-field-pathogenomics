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
#include <string.h>

#include "jansson.h"

#define ALLOCATE_PATHOGENOMICS_TAGS
#include "pathogenomics_service.h"
#include "memory_allocations.h"
#include "parameter.h"
#include "service_job.h"
#include "mongodb_tool.h"
#include "string_utils.h"
#include "json_tools.h"
#include "grassroots_config.h"
#include "country_codes.h"
#include "sample_metadata.h"
#include "phenotype_metadata.h"
#include "genotype_metadata.h"
#include "files_metadata.h"
#include "string_linked_list.h"
#include "math_utils.h"
#include "search_options.h"
#include "time_util.h"
#include "io_utils.h"
#include "audit.h"


#ifdef _DEBUG
#define PATHOGENOMICS_SERVICE_DEBUG	(STM_LEVEL_FINER)
#else
#define PATHOGENOMICS_SERVICE_DEBUG	(STM_LEVEL_NONE)
#endif


//#define TAG_UPDATE MAKE_TAG('P', 'G', 'U', 'P')
//#define TAG_QUERY MAKE_TAG('P', 'G', 'Q', 'U')
//#define TAG_REMOVE MAKE_TAG('P', 'G', 'R', 'M')
//#define TAG_DUMP MAKE_TAG('P', 'G', 'D', 'P')
//#define TAG_PREVIEW MAKE_TAG('P', 'G', 'P', 'V')
//#define TAG_COLLECTION MAKE_TAG('P', 'G', 'C', 'O')
//#define TAG_DATABASE MAKE_TAG('P', 'G', 'D', 'B')
//#define TAG_DELIMITER MAKE_TAG('P', 'G', 'D', 'L')
//#define TAG_FILE MAKE_TAG('P', 'G', 'F', 'I')


static NamedParameterType PGS_UPDATE = { "Update", PT_JSON };
static NamedParameterType PGS_QUERY = { "Search", PT_JSON };
static NamedParameterType PGS_REMOVE = { "Delete", PT_JSON };
static NamedParameterType PGS_DUMP = { "Dump data", PT_BOOLEAN };
static NamedParameterType PGS_PREVIEW = { "Preview", PT_BOOLEAN };
static NamedParameterType PGS_COLLECTION = { "Collection", PT_STRING };
static NamedParameterType PGS_DATABASE = { "Database", PT_STRING };
static NamedParameterType PGS_DELIMITER = { "Data delimiter", PT_CHAR };
static NamedParameterType PGS_FILE = { "Upload", PT_TABLE};


static const char *s_data_names_pp [PD_NUM_TYPES] =
{
	PG_SAMPLE_S,
	PG_PHENOTYPE_S,
	PG_GENOTYPE_S,
	PG_FILES_S
};



static const char s_default_column_delimiter =  '|';


/*
 * STATIC PROTOTYPES
 */

static Service *GetPathogenomicsService (void);

static const char *GetPathogenomicsServiceName (Service *service_p);

static const char *GetPathogenomicsServiceDesciption (Service *service_p);

static const char *GetPathogenomicsServiceInformationUri (Service *service_p);

static ParameterSet *GetPathogenomicsServiceParameters (Service *service_p, Resource *resource_p, UserDetails *user_p);

static void ReleasePathogenomicsServiceParameters (Service *service_p, ParameterSet *params_p);

static ServiceJobSet *RunPathogenomicsService (Service *service_p, ParameterSet *param_set_p, UserDetails *user_p, ProvidersStateTable *providers_p);

static  ParameterSet *IsResourceForPathogenomicsService (Service *service_p, Resource *resource_p, Handler *handler_p);


static PathogenomicsServiceData *AllocatePathogenomicsServiceData (void);

static json_t *GetPathogenomicsServiceResults (Service *service_p, const uuid_t job_id);

static bool ConfigurePathogenomicsService (PathogenomicsServiceData *data_p);


static void FreePathogenomicsServiceData (PathogenomicsServiceData *data_p);

static bool ClosePathogenomicsService (Service *service_p);


static uint32 InsertData (MongoTool *tool_p, ServiceJob *job_p, json_t *values_p, const PathogenomicsData collection_type, PathogenomicsServiceData *service_data_p);


static OperationStatus SearchData (MongoTool *tool_p, ServiceJob *job_p, json_t *data_p, const PathogenomicsData collection_type, PathogenomicsServiceData *service_data_p, const bool preview_flag);


static uint32 DeleteData (MongoTool *tool_p, ServiceJob *job_p, json_t *data_p, const PathogenomicsData collection_type, PathogenomicsServiceData *service_data_p);

static bool AddUploadParams (ServiceData *data_p, ParameterSet *param_set_p);

static bool GetCollectionName (ParameterSet *param_set_p, PathogenomicsServiceData *data_p, const char **collection_name_ss, PathogenomicsData *collection_type_p);

static bool AddLiveDateFiltering (json_t *record_p, const char * const date_s);


static json_t *ConvertToResource (const size_t i, json_t *src_record_p);

static json_t *CopyValidRecord (const size_t i, json_t *src_record_p);

static json_t *FilterResultsByDate (json_t *src_results_p, const bool preview_flag, json_t *(convert_record_fn) (const size_t i, json_t *src_record_p));

static bool AddErrorMessage (ServiceJob *job_p, const json_t *value_p, const char *error_s, const int index);


/*
 * API FUNCTIONS
 */


ServicesArray *GetServices (UserDetails *user_p)
{
	ServicesArray *services_p = AllocateServicesArray (1);

	if (services_p)
		{
			Service *service_p = GetPathogenomicsService ();

			if (service_p)
				{
					* (services_p -> sa_services_pp) = service_p;
					return services_p;
				}

			FreeServicesArray (services_p);
		}

	return NULL;
}



void ReleaseService (Service *service_p)
{
	ClosePathogenomicsService (service_p);
	FreeMemory (service_p);
}


/*
 * STATIC FUNCTIONS
 */



static json_t *GetPathogenomicsServiceResults (Service *service_p, const uuid_t job_id)
{
	PathogenomicsServiceData *data_p = (PathogenomicsServiceData *) (service_p -> se_data_p);
	ServiceJob *job_p = GetServiceJobFromServiceJobSetById (service_p -> se_jobs_p, job_id);
	json_t *res_p = NULL;

	if (job_p)
		{
			if (job_p -> sj_status == OS_SUCCEEDED)
				{
					json_error_t error;
					//const char *buffer_data_p = GetCurlToolData (data_p -> wsd_curl_data_p);
					//res_p = json_loads (buffer_data_p, 0, &error);
				}
		}		/* if (job_p) */

	return res_p;
}



static Service *GetPathogenomicsService (void)
{
	Service *service_p = (Service *) AllocMemory (sizeof (Service));

	if (service_p)
		{
			PathogenomicsServiceData *data_p = AllocatePathogenomicsServiceData ();

			if (data_p)
				{
					InitialiseService (service_p,
														 GetPathogenomicsServiceName,
														 GetPathogenomicsServiceDesciption,
														 GetPathogenomicsServiceInformationUri,
														 RunPathogenomicsService,
														 IsResourceForPathogenomicsService,
														 GetPathogenomicsServiceParameters,
														 ReleasePathogenomicsServiceParameters,
														 ClosePathogenomicsService,
														 NULL,
														 false,
														 true,
														 (ServiceData *) data_p);

					if (ConfigurePathogenomicsService (data_p))
						{
							return service_p;
						}

					FreePathogenomicsServiceData (data_p);
				}

			FreeMemory (service_p);
		}		/* if (service_p) */

	return NULL;
}


static bool ConfigurePathogenomicsService (PathogenomicsServiceData *data_p)
{
	bool success_flag = false;
	const json_t *service_config_p = data_p -> psd_base_data.sd_config_p;


	data_p -> psd_database_s = GetJSONString (service_config_p, "database");

	if (data_p -> psd_database_s)
		{
			const char *value_s = GetJSONString (service_config_p, "default_geocoder");

			data_p -> psd_geocoder_uri_s = NULL;

			if (value_s)
				{
					json_t *geocoders_p = json_object_get (service_config_p, "geocoders");

					if (geocoders_p)
						{
							data_p -> psd_geocoder_uri_s = NULL;

							if (json_is_array (geocoders_p))
								{
									const size_t size = json_array_size (geocoders_p);
									size_t i = 0;

									while (i < size)
										{
											json_t *geocoder_p = json_array_get (geocoders_p, i);
											const char *name_s = GetJSONString (geocoder_p, "name");

											if (name_s && (strcmp (name_s, value_s) == 0))
												{
													data_p -> psd_geocoder_uri_s = GetJSONString (geocoder_p, "uri");
													i = size;
												}
											else
												{
													++ i;
												}
										}
								}
							else
								{
									const char *name_s = GetJSONString (geocoders_p, "name");

									if (name_s && (strcmp (name_s, value_s) == 0))
										{
											data_p -> psd_geocoder_uri_s = GetJSONString (geocoders_p, "uri");
										}
								}

							if (data_p -> psd_geocoder_uri_s)
								{
									if (strcmp (value_s, "google") == 0)
										{
											data_p -> psd_geocoder_fn = GetLocationDataByGoogle;
										}
									else if (strcmp (value_s, "opencage") == 0)
										{
											data_p -> psd_geocoder_fn = GetLocationDataByOpenCage;
										}
									else
										{
											data_p -> psd_geocoder_fn = NULL;
										}
								}
						}
				}		/* if (value_s) */

			if (data_p -> psd_geocoder_fn)
				{

					if ((* (data_p -> psd_collection_ss + PD_SAMPLE) = GetJSONString (service_config_p, "samples_collection")) != NULL)
						{
							if ((* (data_p -> psd_collection_ss + PD_PHENOTYPE) = GetJSONString (service_config_p, "phenotypes_collection")) != NULL)
								{
									if ((* (data_p -> psd_collection_ss + PD_GENOTYPE) = GetJSONString (service_config_p, "genotypes_collection")) != NULL)
										{
											if ((* (data_p -> psd_collection_ss + PD_FILES) = GetJSONString (service_config_p, "files_collection")) != NULL)
												{
													data_p -> psd_files_download_root_uri_s = GetJSONString (service_config_p, "files_host");

													success_flag = true;
												}
										}
								}
						}
				}

		} /* if (data_p -> psd_database_s) */

	return success_flag;
}


static PathogenomicsServiceData *AllocatePathogenomicsServiceData (void)
{
	MongoTool *tool_p = AllocateMongoTool ();

	if (tool_p)
		{
			PathogenomicsServiceData *data_p = (PathogenomicsServiceData *) AllocMemory (sizeof (PathogenomicsServiceData));

			if (data_p)
				{
					data_p -> psd_tool_p = tool_p;
					data_p -> psd_geocoder_fn = NULL;
					data_p -> psd_geocoder_uri_s = NULL;
					data_p -> psd_database_s = NULL;

					memset (data_p -> psd_collection_ss, 0, PD_NUM_TYPES * sizeof (const char *));

					data_p -> psd_files_download_root_uri_s = NULL;

					return data_p;
				}

			FreeMongoTool (tool_p);
		}

	return NULL;
}


static bool AddIndexes (PathogenomicsServiceData *data_p)
{
	bool success_flag = true;
	bson_t keys;
	bson_error_t error;
	mongoc_index_opt_t opt;

	mongoc_index_opt_init (&opt);

	bson_init (&keys);
	BSON_APPEND_UTF8 (&keys, PG_VARIETY_S, "text");
	mongoc_collection_create_index (data_p -> psd_tool_p -> mt_collection_p, &keys, &opt, &error);

	bson_reinit (&keys);
	BSON_APPEND_UTF8 (&keys, PG_DISEASE_S, "text");
	mongoc_collection_create_index (data_p -> psd_tool_p -> mt_collection_p, &keys, &opt, &error);

	return success_flag;
}


static void FreePathogenomicsServiceData (PathogenomicsServiceData *data_p)
{
	FreeMongoTool (data_p -> psd_tool_p);

	FreeMemory (data_p);
}


static const char *GetPathogenomicsServiceName (Service * UNUSED_PARAM (service_p))
{
	return "Pathogenomics Geoservice";
}


static const char *GetPathogenomicsServiceDesciption (Service * UNUSED_PARAM (service_p))
{
	return "A service to analyse the spread of Wheat-related diseases both geographically and temporally";
}


static const char *GetPathogenomicsServiceInformationUri (Service * UNUSED_PARAM (service_p))
{
	return NULL;
}


static ParameterSet *GetPathogenomicsServiceParameters (Service *service_p, Resource * UNUSED_PARAM (resource_p), UserDetails * UNUSED_PARAM (user_p))
{
	ParameterSet *params_p  = AllocateParameterSet ("Pathogenomics service parameters", "The parameters used for the Pathogenomics service");

	if (params_p)
		{
			ServiceData *service_data_p = service_p -> se_data_p;
			Parameter *param_p = NULL;
			SharedType def;

			def.st_json_p = NULL;

			if ((param_p = EasyCreateAndAddParameterToParameterSet (service_data_p, params_p, NULL, PGS_UPDATE.npt_type, PGS_UPDATE.npt_name_s, "Update", "Add data to the system", def, PL_ADVANCED)) != NULL)
				{
					if ((param_p = EasyCreateAndAddParameterToParameterSet (service_data_p, params_p, NULL, PGS_QUERY.npt_type, PGS_QUERY.npt_name_s, "Search", "Find data to the system", def, PL_ALL)) != NULL)
						{
							if ((param_p = EasyCreateAndAddParameterToParameterSet (service_data_p, params_p, NULL, PGS_REMOVE.npt_type, PGS_REMOVE.npt_name_s, "Delete", "Delete data to the system", def, PL_ADVANCED)) != NULL)
								{
									def.st_boolean_value = false;

									if ((param_p = EasyCreateAndAddParameterToParameterSet (service_data_p, params_p, NULL, PGS_DUMP.npt_type, PGS_DUMP.npt_name_s, "Dump", "Get all of the data in the system", def, PL_INTERMEDIATE | PL_ADVANCED)) != NULL)
										{
											if ((param_p = EasyCreateAndAddParameterToParameterSet (service_data_p, params_p, NULL, PGS_PREVIEW.npt_type, PGS_PREVIEW.npt_name_s, "Preview", "Ignore the live dates", def, PL_ADVANCED)) != NULL)
												{
													def.st_string_value_s = (char *) *s_data_names_pp;

													if ((param_p = EasyCreateAndAddParameterToParameterSet (service_data_p, params_p, NULL, PGS_COLLECTION.npt_type, PGS_COLLECTION.npt_name_s, "Collection", "The collection to act upon", def, PL_ALL)) != NULL)
														{
															bool success_flag = true;
															uint32 i;

															for (i = 0; i < PD_NUM_TYPES; ++ i)
																{
																	def.st_string_value_s = (char *) s_data_names_pp [i];

																	if (!CreateAndAddParameterOptionToParameter (param_p, def, NULL))
																		{
																			i = PD_NUM_TYPES;
																			success_flag = false;
																		}
																}

															if (success_flag)
																{
																	if (AddUploadParams (service_p -> se_data_p, params_p))
																		{
																			return params_p;
																		}

																}
														}
												}
										}
								}
						}
				}

			FreeParameterSet (params_p);
		}

	return NULL;
}



static bool AddUploadParams (ServiceData *data_p, ParameterSet *param_set_p)
{
	bool success_flag = false;
	Parameter *param_p = NULL;
	SharedType def;
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Spreadsheet Import Parameters", NULL, data_p, param_set_p);

	def.st_char_value = s_default_column_delimiter;

	if ((param_p = CreateAndAddParameterToParameterSet (data_p, param_set_p, group_p, PGS_DELIMITER.npt_type, false, PGS_DELIMITER.npt_name_s, "Delimiter", "The character delimiting columns", NULL, def, NULL, NULL, PL_ALL, NULL)) != NULL)
		{
			def.st_string_value_s = NULL;

			if ((param_p = CreateAndAddParameterToParameterSet (data_p, param_set_p, group_p, PGS_FILE.npt_type, false, PGS_FILE.npt_name_s, "Data to upload", "The data to upload", NULL, def, NULL, NULL, PL_ALL, NULL)) != NULL)
				{
					success_flag = true;
				}
		}

	return success_flag;
}


static void ReleasePathogenomicsServiceParameters (Service * UNUSED_PARAM (service_p), ParameterSet *params_p)
{
	FreeParameterSet (params_p);
}




static bool ClosePathogenomicsService (Service *service_p)
{
	bool success_flag = true;

	FreePathogenomicsServiceData ((PathogenomicsServiceData *) (service_p -> se_data_p));;

	return success_flag;
}


static json_type GetPathogenomicsJSONFieldType (const char *name_s, const void *data_p)
{
	json_type t = JSON_STRING;
	PathogenomicsServiceData *service_data_p = (PathogenomicsServiceData *) (data_p);
	const json_t *config_p = service_data_p -> psd_base_data.sd_config_p;

	if (config_p)
		{
			const json_t *field_defs_p = json_object_get (config_p, "field_defs");

			if (field_defs_p)
				{
					const char *field_def_s = GetJSONString (field_defs_p, name_s);

					if (field_def_s)
						{
							if (strcmp (field_def_s, "int") == 0)
								{
									t = JSON_INTEGER;
								}
							else if (strcmp (field_def_s, "real") == 0)
								{
									t = JSON_REAL;
								}
							else if (strcmp (field_def_s, "bool") == 0)
								{
									t = JSON_TRUE;
								}
						}
				}
		}

	return t;
}


static bool GetCollectionName (ParameterSet *param_set_p, PathogenomicsServiceData *data_p, const char **collection_name_ss, PathogenomicsData *collection_type_p)
{
	SharedType value;

	if (GetParameterValueFromParameterSet (param_set_p, PGS_COLLECTION.npt_name_s, &value, true))
		{
			const char *collection_s = value.st_string_value_s;

			if (collection_s)
				{
					uint32 i;

					for (i = 0; i < PD_NUM_TYPES; ++ i)
						{
							if (strcmp (collection_s, * (s_data_names_pp + i)) == 0)
								{
									*collection_name_ss = * ((data_p -> psd_collection_ss) + i);
									*collection_type_p = (PathogenomicsData) i;

									return true;
								}
						}		/* for (i = 0; i < PD_NUM_TYPES; ++ i) */
				}
		}

	return false;
}


static ServiceJobSet *RunPathogenomicsService (Service *service_p, ParameterSet *param_set_p, UserDetails * UNUSED_PARAM (user_p), ProvidersStateTable * UNUSED_PARAM (providers_p))
{
	PathogenomicsServiceData *data_p = (PathogenomicsServiceData *) (service_p -> se_data_p);

	service_p -> se_jobs_p = AllocateSimpleServiceJobSet (service_p, NULL, "Pathogenomics");

	if (service_p -> se_jobs_p)
		{
			ServiceJob *job_p = GetServiceJobFromServiceJobSet (service_p -> se_jobs_p, 0);


			LogParameterSet (param_set_p, job_p);

			job_p -> sj_status = OS_FAILED_TO_START;

			if (param_set_p)
				{
					/* get the collection to work on */
					const char *collection_name_s = NULL;
					bool preview_flag = false;
					PathogenomicsData collection_type = PD_NUM_TYPES;
					Parameter *param_p = GetParameterFromParameterSetByName (param_set_p, PGS_PREVIEW.npt_name_s);

					if (param_p && (param_p -> pa_type == PT_BOOLEAN))
						{
							preview_flag = param_p -> pa_current_value.st_boolean_value;
						}

					if (GetCollectionName (param_set_p, data_p, &collection_name_s, &collection_type))
						{
							MongoTool *tool_p = data_p -> psd_tool_p;

							if (tool_p)
								{
									param_p = GetParameterFromParameterSetByName (param_set_p, PGS_DUMP.npt_name_s);

									SetMongoToolCollection (tool_p, data_p -> psd_database_s, collection_name_s);

									job_p -> sj_status = OS_STARTED;
									LogServiceJob (job_p);

									/* Do we want to get a dump of the entire collection? */
									if (param_p && (param_p -> pa_type == PT_BOOLEAN) && (param_p -> pa_current_value.st_boolean_value == true))
										{
											json_t *raw_results_p = GetAllMongoResultsAsJSON (tool_p, NULL);

											if (raw_results_p)
												{
													if (!preview_flag)
														{
															raw_results_p = FilterResultsByDate (raw_results_p, preview_flag, CopyValidRecord);
														}

													PrintJSONToLog (STM_LEVEL_FINER, __FILE__, __LINE__, raw_results_p, "dump: ");

													if (raw_results_p)
														{
															size_t i;
															json_t *src_record_p;
															char uuid_s [UUID_STRING_BUFFER_SIZE];

															ConvertUUIDToString (job_p -> sj_id, uuid_s);

															json_array_foreach (raw_results_p, i, src_record_p)
															{
																char *title_s = ConvertNumberToString ((double) i, -1);
																json_t *dest_record_p = GetResourceAsJSONByParts (PROTOCOL_INLINE_S, NULL, title_s, src_record_p);

																if (dest_record_p)
																	{
																		if (!AddResultToServiceJob (job_p, dest_record_p))
																			{
																				PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, dest_record_p, "Failed to add job to %s", uuid_s);

																				if (!AddErrorToServiceJob (job_p, title_s, "Failed to add document to result"))
																					{
																						PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to set job error data");
																					}

																				json_decref (dest_record_p);
																			}

																	}		/* if (dest_record_p) */
																else
																	{
																		PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetResourceAsJSONByParts failed");

																		if (!AddErrorToServiceJob (job_p, title_s, "Failed to get document to add to result"))
																			{
																				PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to set job error data");
																			}
																	}

																if (title_s)
																	{
																		FreeCopiedString (title_s);
																	}

															}		/* json_array_foreach (raw_results_p, i, src_record_p) */


															i = GetNumberOfServiceJobResults (job_p);

															if (i > 0)
																{
																	job_p -> sj_status = (i == json_array_size (raw_results_p)) ? OS_SUCCEEDED : OS_PARTIALLY_SUCCEEDED;
																}
															else
																{
																	job_p -> sj_status = OS_FAILED;
																}

															json_decref (raw_results_p);
														}		/* if (raw_results_p) */
													else
														{
															/* The call succeeded but all results were filtered out */
															job_p -> sj_status = OS_SUCCEEDED;
														}


												}		/* if (dump_p) */
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetAllMongoResultsAsJSON for \"%s\".\"%s\" failed", data_p -> psd_database_s, collection_name_s);

													if (!AddErrorToServiceJob (job_p, "Search failure", "Failed to get results from query"))
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add job error value");
														}
												}

										}		/* if (param_p && (param_p -> pa_type == PT_BOOLEAN) && (param_p -> pa_current_value.st_boolean_value == true)) */
									else
										{
											json_t *json_param_p = NULL;
											char delimiter = s_default_column_delimiter;
											uint32 num_successes = 0;
											bool free_json_param_flag = false;

											/* Get the current delimiter */
											param_p = GetParameterFromParameterSetByName (param_set_p, PGS_DELIMITER.npt_name_s);
											if (param_p)
												{
													delimiter = param_p -> pa_current_value.st_char_value;
												}

											/*
											 * Data can be updated either from the table or as a json insert statement
											 *
											 * Has a tabular dataset been uploaded...
											 */
											param_p = GetParameterFromParameterSetByName (param_set_p, PGS_FILE.npt_name_s);
											if (param_p && (!IsStringEmpty (param_p -> pa_current_value.st_string_value_s)))
												{
													char *data_s = param_p -> pa_current_value.st_string_value_s;
													LinkedList *headers_p = GetTabularHeaders (&data_s, delimiter, '\n', GetPathogenomicsJSONFieldType, data_p);

													if (headers_p)
														{
															bool success_flag = false;

															/* Check that all of the required columns are present */
															switch (collection_type)
																{
																	case PD_SAMPLE:
																		success_flag = CheckSampleData (headers_p, job_p, data_p);
																		break;

																	case PD_PHENOTYPE:
																		success_flag = CheckPhenotypeData (headers_p, job_p, data_p);
																		break;

																	case PD_GENOTYPE:
																		success_flag = CheckGenotypeData (headers_p, job_p, data_p);
																		break;

																	case PD_FILES:
																		break;

																	default:
																		break;
																}

															if (success_flag)
																{
																	json_param_p = ConvertTabularDataToJSON (param_p -> pa_current_value.st_string_value_s, delimiter, '\n', headers_p);
																}

															FreeLinkedList (headers_p);
														}		/* if (headers_p) */


													if (json_param_p)
														{
															#if PATHOGENOMICS_SERVICE_DEBUG >= STM_LEVEL_FINE
															PrintJSONToLog (STM_LEVEL_FINE, __FILE__, __LINE__, json_param_p, "table");
															#endif

															free_json_param_flag = true;
														}
												}
											/* ... or do we have an insert statement? */
											else if (((param_p = GetParameterFromParameterSetByName (param_set_p, PGS_UPDATE.npt_name_s)) != NULL) && (!IsJSONEmpty (param_p -> pa_current_value.st_json_p)))
												{
													json_param_p = param_p -> pa_current_value.st_json_p;
												}

											/* Are we doing an update? */
											if (json_param_p)
												{
													uint32 size = 1;

													if (json_is_array (json_param_p))
														{
															size = json_array_size (json_param_p);
														}

													num_successes = InsertData (tool_p, job_p, json_param_p, collection_type, data_p);


													if (num_successes == 0)
														{
															job_p -> sj_status = OS_FAILED;
														}
													else if (num_successes == size)
														{
															job_p -> sj_status = OS_SUCCEEDED;
														}
													else
														{
															job_p -> sj_status = OS_PARTIALLY_SUCCEEDED;
														}

													if (free_json_param_flag)
														{
															WipeJSON (json_param_p);
														}
												}
											else if (((param_p = GetParameterFromParameterSetByName (param_set_p, PGS_QUERY.npt_name_s)) != NULL) && (!IsJSONEmpty (param_p -> pa_current_value.st_json_p)))
												{
													json_t *results_p = NULL;
													OperationStatus search_status;

													json_param_p = param_p -> pa_current_value.st_json_p;

													search_status = SearchData (tool_p, job_p, json_param_p, collection_type, data_p, preview_flag);

													if (search_status == OS_SUCCEEDED || search_status == OS_PARTIALLY_SUCCEEDED)
														{
															#if PATHOGENOMICS_SERVICE_DEBUG >= STM_LEVEL_FINER
															PrintJSONToLog (STM_LEVEL_FINER, __FILE__, __LINE__, job_p -> sj_result_p, "initial results");
															#endif

															num_successes = GetNumberOfServiceJobResults (job_p);
														}
												}
											else if (((param_p = GetParameterFromParameterSetByName (param_set_p, PGS_REMOVE.npt_name_s)) != NULL) && (!IsJSONEmpty (param_p -> pa_current_value.st_json_p)))
												{
													uint32 size = 1;

													if (json_is_array (json_param_p))
														{
															size = json_array_size (json_param_p);
														}

													json_param_p = param_p -> pa_current_value.st_json_p;
													num_successes = DeleteData (tool_p, job_p, json_param_p, collection_type, data_p);

													if (num_successes == 0)
														{
															job_p -> sj_status = OS_FAILED;
														}
													else if (num_successes == size)
														{
															job_p -> sj_status = OS_SUCCEEDED;
														}
													else
														{
															job_p -> sj_status = OS_PARTIALLY_SUCCEEDED;
														}
												}

											if (json_param_p)
												{
													json_error_t error;
													json_t *metadata_p = NULL;

													metadata_p = json_pack_ex (&error, 0, "{s:i}", "successful entries", num_successes);


#if PATHOGENOMICS_SERVICE_DEBUG >= STM_LEVEL_FINE
													{
														PrintJSONToLog (STM_LEVEL_FINE, __FILE__, __LINE__, job_p -> sj_errors_p, "errors: ");
														PrintJSONToLog (STM_LEVEL_FINE, __FILE__, __LINE__, metadata_p, "metadata 1: ");
													}
#endif

													if (metadata_p)
														{
															job_p -> sj_metadata_p = metadata_p;
														}

#if PATHOGENOMICS_SERVICE_DEBUG >= STM_LEVEL_FINE
													PrintJSONToLog (STM_LEVEL_FINE, __FILE__, __LINE__, job_p -> sj_errors_p, "job errors: ");
													PrintJSONToLog (STM_LEVEL_FINE, __FILE__, __LINE__, job_p -> sj_metadata_p, "job metadata: ");
													PrintJSONToLog (STM_LEVEL_FINE, __FILE__, __LINE__, job_p -> sj_result_p, "job results: ");
													PrintLog (STM_LEVEL_FINE, __FILE__, __LINE__, "job status %d", job_p -> sj_status);
#endif


													/*
													if (json_param_p != value.st_json_p)
														{
															WipeJSON (json_param_p);
														}
													 */
												}		/* if (json_param_p) */

										}		/* if (param_p && (param_p -> pa_type == PT_BOOLEAN) && (param_p -> pa_current_value.st_boolean_value == true)) else */

								}		/* if (tool_p) */
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "no collection specified");
						}

				}		/* if (param_set_p) */

#if PATHOGENOMICS_SERVICE_DEBUG >= STM_LEVEL_FINE
			PrintJSONToLog (STM_LEVEL_FINE, __FILE__, __LINE__, job_p -> sj_metadata_p, "metadata 3: ");
#endif

			LogServiceJob (job_p);
		}		/* if (service_p -> se_jobs_p) */

	return service_p -> se_jobs_p;
}


static bool AddLiveDateFiltering (json_t *record_p, const char * const date_s)
{
	bool success_flag = false;
	uint32 i;

	for (i = 0; i < PD_NUM_TYPES; ++ i)
		{
			const char * const group_name_s = * (s_data_names_pp + i);
			char *key_s = ConcatenateStrings (group_name_s, PG_LIVE_DATE_SUFFIX_S);

			if (key_s)
				{
					json_t *date_p = json_object_get (record_p, key_s);

					if (date_p)
						{
							const char *live_date_s = GetJSONString (date_p, "date");

							if (live_date_s)
								{
									/*
									 * Although both values are dates as strings, we don't need to convert
									 * them as they are both in YYYY-MM-DD format. So strcmp is enough to
									 * see if the "go live" date has been met.
									 */
									if (strcmp (live_date_s, date_s) > 0)
										{
											if (json_object_del (record_p, group_name_s) == 0)
												{
													success_flag = true;
												}
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to remove %s from record", group_name_s);
												}
										}
									else
										{
											success_flag = true;
										}
								}

							/* Remove the "_live_date" object as any */
							if (json_object_del (record_p, key_s) != 0)
								{
									PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to remove %s from record", key_s);
								}
						}		/* if (date_p) */

					FreeCopiedString (key_s);
				}		/* if (key_s) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create key from  \"%s\" and \"%s\"", group_name_s, PG_LIVE_DATE_SUFFIX_S);
				}

		}		/* for (i = 0; i < PD_NUM_TYPES; ++ i) */

	return success_flag;
}


static char *GetCurrentDateAsString (void)
{
	char *date_s = NULL;
	struct tm current_time;

	if (GetCurrentTime (&current_time))
		{
			date_s = GetTimeAsString (&current_time);

			if (!date_s)
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to convert time to a string");
				}
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get current time");
		}

	return date_s;
}


static json_t *ConvertToResource (const size_t i, json_t *src_record_p)
{
	json_t *resource_p = NULL;
	char *title_s = ConvertNumberToString ((double) i, -1);

	if (title_s)
		{
			resource_p = GetResourceAsJSONByParts (PROTOCOL_INLINE_S, NULL, title_s, src_record_p);

			FreeCopiedString (title_s);
		}		/* if (raw_result_p) */

	return resource_p;
}


static json_t *CopyValidRecord (const size_t UNUSED_PARAM (i), json_t *src_record_p)
{
	return json_deep_copy (src_record_p);
}


static json_t *FilterResultsByDate (json_t *src_results_p, const bool preview_flag, json_t *(convert_record_fn) (const size_t i, json_t *src_record_p))
{
	json_t *results_p = json_array ();

	if (results_p)
		{
			char *date_s = GetCurrentDateAsString ();

			if (date_s)
				{
					const size_t size = json_array_size (src_results_p);
					size_t i = 0;

					for (i = 0; i < size; ++ i)
						{
							json_t *src_result_p = json_array_get (src_results_p, i);
							json_t *dest_result_p = NULL;

							/* We don't need to return the internal mongo id so remove it */
							json_object_del (src_result_p, MONGO_ID_S);

							if (!preview_flag)
								{
									if (AddLiveDateFiltering (src_result_p, date_s))
										{
											/*
											 * If the result is non-trivial i.e. has at least one of the sample,
											 * genotype or phenotype, then keep it
											 */
											if ((json_object_get (src_result_p, PG_SAMPLE_S) != NULL) ||
													(json_object_get (src_result_p, PG_PHENOTYPE_S) != NULL) ||
													(json_object_get (src_result_p, PG_GENOTYPE_S) != NULL))
												{
													dest_result_p = convert_record_fn (i, src_result_p);

													if (!dest_result_p)
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to convert src json \"%s\" " SIZET_FMT " to results array", i);
														}

												}
											else
												{
#if PATHOGENOMICS_SERVICE_DEBUG >= STM_LEVEL_FINE
													const char *id_s = GetJSONString (src_result_p, PG_ID_S);
													PrintLog (STM_LEVEL_FINE, __FILE__, __LINE__, "Discarding %s after filtering", id_s ? id_s : "");
#endif
												}
										}
									else
										{
											const char *id_s = GetJSONString (src_result_p, PG_ID_S);
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add date filtering for %s", id_s ? id_s : "");
										}		/* if (!AddLiveDateFiltering (values_p, collection_type)) */

								}		/* if (!private_view_flag) */
							else
								{
									dest_result_p = convert_record_fn (i, src_result_p);

									if (!dest_result_p)
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to convert src json \"%s\" " SIZET_FMT " to results array", i);
										}
								}


							if (dest_result_p)
								{
									if (json_array_append_new (results_p, dest_result_p) != 0)
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add json resource for " SIZET_FMT " to results array", i);
											json_decref (dest_result_p);
										}
								}

						}		/* for (i = 0; i < size; ++ i) */

					FreeCopiedString (date_s);
				}		/* if (date_s) */

		}		/* if (results_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create results object");
		}

	return results_p;
}


static OperationStatus SearchData (MongoTool *tool_p, ServiceJob *job_p, json_t *data_p, const PathogenomicsData collection_type, PathogenomicsServiceData * UNUSED_PARAM (service_data_p), const bool preview_flag)
{
	OperationStatus status = OS_FAILED;
	json_t *values_p = json_object_get (data_p, MONGO_OPERATION_DATA_S);

	if (values_p)
		{
			const char **fields_ss = NULL;
			json_t *fields_p = json_object_get (data_p, MONGO_OPERATION_FIELDS_S);

			if (fields_p)
				{
					if (json_is_array (fields_p))
						{
							size_t size = json_array_size (fields_p);

							fields_ss = (const char **) AllocMemoryArray (size + 1, sizeof (const char *));

							if (fields_ss)
								{
									const char **field_ss = fields_ss;
									size_t i;
									json_t *field_p;

									json_array_foreach (fields_p, i, field_p)
										{
											if (json_is_string (field_p))
												{
													*field_ss = json_string_value (field_p);
													++ field_ss;
												}
											else
												{
													char *dump_s = json_dumps (field_p, JSON_INDENT (2));

													PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to get field from %s", dump_s);
													free (dump_s);
												}
										}

								}		/* if (fields_ss) */

						}		/* if (json_is_array (fields_p)) */

				}		/* if (fields_p) */

			if (FindMatchingMongoDocumentsByJSON (tool_p, values_p, fields_ss))
				{
					json_t *raw_results_p = GetAllExistingMongoResultsAsJSON (tool_p);

					if (raw_results_p)
						{
							if (json_is_array (raw_results_p))
								{
									const size_t size = json_array_size (raw_results_p);
									size_t i = 0;
									json_t *raw_result_p = NULL;
									char *date_s = NULL;
									bool success_flag = true;

									/*
									 * If we are on the public view, we need to filter
									 * out entries that don't meet the live dates.
									 */
									if (!preview_flag)
										{
											struct tm current_time;

											if (GetCurrentTime (&current_time))
												{
													date_s = GetTimeAsString (&current_time);

													if (!date_s)
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to convert time to a string");
															success_flag = false;
														}
												}
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get current time");
													success_flag = false;
												}

										}		/* if (!private_view_flag) */

									if (success_flag)
										{
											for (i = 0; i < size; ++ i)
												{
													raw_result_p = json_array_get (raw_results_p, i);
													char *title_s = ConvertNumberToString ((double) i + 1, -1);

													if (!title_s)
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to convert " SIZET_FMT " to string for title", i + 1);
														}

													/* We don't need to return the internal mongo id so remove it */
													json_object_del (raw_result_p, MONGO_ID_S);

													if (!preview_flag)
														{
															if (AddLiveDateFiltering (raw_result_p, date_s))
																{
																	/*
																	 * If the result is non-trivial i.e. has at least one of the sample,
																	 * genotype or phenotype, then keep it
																	 */
																	if ((json_object_get (raw_result_p, PG_SAMPLE_S) == NULL) &&
																			(json_object_get (raw_result_p, PG_PHENOTYPE_S) == NULL) &&
																			(json_object_get (raw_result_p, PG_GENOTYPE_S) == NULL))
																		{
#if PATHOGENOMICS_SERVICE_DEBUG >= STM_LEVEL_FINE
																			const char *id_s = GetJSONString (raw_result_p, PG_ID_S);
																			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Discarding %s after filtering", id_s ? id_s : "");
#endif

																			raw_result_p = NULL;
																		}
																}
															else
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add date filtering for %d", collection_type);
																	raw_result_p = NULL;
																}		/* if (!AddLiveDateFiltering (values_p, collection_type)) */
														}		/* if (!private_view_flag) */

													if (raw_result_p)
														{
															json_t *resource_p = GetResourceAsJSONByParts (PROTOCOL_INLINE_S, NULL, title_s, raw_result_p);

															if (resource_p)
																{
																	if (!AddResultToServiceJob (job_p, resource_p))
																		{
																			AddErrorToServiceJob (job_p, title_s, "Failed to add result");

																			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add json resource for " SIZET_FMT " to results array", i);
																			json_decref (resource_p);
																		}
																}		/* if (resource_p) */
															else
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create json resource for " SIZET_FMT, i);
																}

														}		/* if (raw_result_p) */

													if (title_s)
														{
															FreeCopiedString (title_s);
														}
												}		/* for (i = 0; i < size; ++ i) */


											i = GetNumberOfServiceJobResults (job_p);
											status = (i == json_array_size (raw_results_p)) ? OS_SUCCEEDED : OS_PARTIALLY_SUCCEEDED;
										}		/* if (success_flag) */

									if (date_s)
										{
											FreeCopiedString (date_s);
										}

								}		/* if (json_is_array (raw_results_p)) */
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Search results is not an array");
								}

							json_decref (raw_results_p);
						}		/* if (raw_results_p) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Couldn't get raw results from search");
						}
				}		/* if (FindMatchingMongoDocumentsByJSON (tool_p, values_p, fields_ss)) */
			else
				{
#if PATHOGENOMICS_SERVICE_DEBUG >= STM_LEVEL_FINE
					PrintJSONToLog (STM_LEVEL_SEVERE, __FILE__, __LINE__, values_p, "No results found for ");
#endif
				}

			if (fields_ss)
				{
					FreeMemory (fields_ss);
				}
		}		/* if (values_p) */


#if PATHOGENOMICS_SERVICE_DEBUG >= STM_LEVEL_FINE
	PrintJSONToLog (STM_LEVEL_SEVERE, __FILE__, __LINE__, job_p -> sj_result_p, "results_p: ");
#endif

	SetServiceJobStatus (job_p, status);

	return status;
}


static char *CheckDataIsValid (const json_t *row_p, PathogenomicsServiceData *data_p)
{
	char *errors_s = NULL;
	ByteBuffer *buffer_p = AllocateByteBuffer (1024);

	if (buffer_p)
		{
			/*
			 * Each row the pathogenmics must contain an ID field
			 * not the mongo _id, and the ability to get a geojson
			 * stub and a date.
			 */

			if (!json_object_get (row_p, PG_ID_S))
				{
					if (!AppendStringsToByteBuffer (buffer_p, "The row does not have an ",  PG_ID_S,  " field", NULL))
						{
						}
				}

			// http://api.opencagedata.com/geocode/v1/geojson?pretty=1&key=1a9d04b5e924fd52d1f306d924d023a5&query=Osisek,+Croatia'



			if (GetByteBufferSize (buffer_p) > 0)
				{
					errors_s = DetachByteBufferData (buffer_p);
				}
		}		/* if (buffer_p) */

	return errors_s;
}


bool AddErrorMessage (json_t *errors_p, const json_t *values_p, const size_t row, const char * const error_s)
{
	bool success_flag = false;
	const char *pathogenomics_id_s = GetJSONString (values_p, PG_ID_S);

	if (pathogenomics_id_s)
		{
			json_error_t error;
			json_t *error_p = json_pack_ex (&error, 0, "{s:s,s:i,s:s}", "ID", pathogenomics_id_s, "row", row, "error", error_s);

			if (error_p)
				{
					if (json_array_append_new (errors_p, error_p) == 0)
						{
							success_flag = true;
						}
				}
		}

#if PATHOGENOMICS_SERVICE_DEBUG >= STM_LEVEL_FINE
	PrintJSONToLog (STM_LEVEL_FINE, __FILE__, __LINE__, errors_p, "errors data: ");
#endif

	return success_flag;
}



static uint32 InsertData (MongoTool *tool_p, ServiceJob *job_p, json_t *values_p, const PathogenomicsData collection_type, PathogenomicsServiceData *data_p)
{
	uint32 num_imports = 0;
	const char *(*insert_fn) (MongoTool *tool_p, json_t *values_p, PathogenomicsServiceData *data_p) = NULL;

	#if PATHOGENOMICS_SERVICE_DEBUG >= STM_LEVEL_FINE
	PrintJSONToLog (STM_LEVEL_FINE, __FILE__, __LINE__, values_p, "values_p: ");
	#endif

	switch (collection_type)
		{
			case PD_SAMPLE:
				insert_fn = InsertSampleData;
				break;

			case PD_PHENOTYPE:
				insert_fn = InsertPhenotypeData;
				break;

			case PD_GENOTYPE:
				insert_fn = InsertGenotypeData;
				break;

			case PD_FILES:
				insert_fn = InsertFilesData;
				break;

			default:
				break;
		}


	if (insert_fn)
		{
			if (json_is_array (values_p))
				{
					json_t *value_p;
					size_t i;

					json_array_foreach (values_p, i, value_p)
					{
						const char *error_s = insert_fn (tool_p, value_p, data_p);

						if (error_s)
							{
								AddErrorMessage (job_p, value_p, error_s, i);
							}
						else
							{
								++ num_imports;
							}
					}
				}
			else
				{
					const char *error_s = insert_fn (tool_p, values_p, data_p);

					if (error_s)
						{
							AddErrorMessage (job_p, values_p, error_s, 0);

							PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "%s", error_s);
						}
					else
						{
							++ num_imports;
						}
				}
		}


	return num_imports;
}


static bool AddErrorMessage (ServiceJob *job_p, const json_t *value_p, const char *error_s, const int index)
{
	char *dump_s = json_dumps (value_p, JSON_INDENT (2) | JSON_PRESERVE_ORDER);
	const char *id_s = GetJSONString (value_p, PG_ID_S);
	bool added_error_flag = false;

	if (!id_s)
		{
			id_s = GetJSONString (value_p, PG_UKCPVS_ID_S);
		}

	if (id_s)
		{
			added_error_flag = AddErrorToServiceJob (job_p, id_s, error_s);
		}
	else
		{
			char *index_s = GetIntAsString (index);

			if (index_s)
				{
					char *row_s = ConcatenateStrings ("row ", index_s);

					if (row_s)
						{
							added_error_flag = AddErrorToServiceJob (job_p, row_s, error_s);

							FreeCopiedString (row_s);
						}

					FreeCopiedString (index_s);
				}

		}

	if (!added_error_flag)
		{
			PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "failed to add %s to client feedback messsage", error_s);
		}


	if (dump_s)
		{
			PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to import \"%s\": error=%s", dump_s, error_s);
			free (dump_s);
		}
	else
		{
			PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to import error=%s", dump_s, error_s);
		}

	return added_error_flag;
}


static uint32 DeleteData (MongoTool *tool_p, ServiceJob * UNUSED_PARAM (job_p), json_t *data_p, const PathogenomicsData UNUSED_PARAM (collection_type), PathogenomicsServiceData * UNUSED_PARAM (service_data_p))
{
	bool success_flag = false;
	json_t *selector_p = json_object_get (data_p, MONGO_OPERATION_DATA_S);

	if (selector_p)
		{
			success_flag = RemoveMongoDocuments (tool_p, selector_p, false);
		}		/* if (values_p) */

	return success_flag ? 1 : 0;
}


static ParameterSet *IsResourceForPathogenomicsService (Service * UNUSED_PARAM (service_p), Resource * UNUSED_PARAM (resource_p), Handler * UNUSED_PARAM (handler_p))
{
	return NULL;
}

