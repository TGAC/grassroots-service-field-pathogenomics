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
 * country_codes.c
 *
 *  Created on: 3 Aug 2015
 *      Author: tyrrells
 */

#include <string.h>
#include <stdlib.h>

#include "country_codes.h"
#include "typedefs.h"
#include "string_utils.h"
#include "pathogenomics_service.h"
#include "json_tools.h"

static const uint32 S_NUM_COUNTRIES = 249;

static CountryCode s_countries_by_name_p [S_NUM_COUNTRIES] =
/* "Country name","Code" */
{
	{ "Afghanistan","AF" },
	{ "Åland Islands","AX" },
	{ "Albania","AL" },
	{ "Algeria","DZ" },
	{ "American Samoa","AS" },
	{ "Andorra","AD" },
	{ "Angola","AO" },
	{ "Anguilla","AI" },
	{ "Antarctica","AQ" },
	{ "Antigua and Barbuda","AG" },
	{ "Argentina","AR" },
	{ "Armenia","AM" },
	{ "Aruba","AW" },
	{ "Australia","AU" },
	{ "Austria","AT" },
	{ "Azerbaijan","AZ" },
	{ "Bahamas","BS" },
	{ "Bahrain","BH" },
	{ "Bangladesh","BD" },
	{ "Barbados","BB" },
	{ "Belarus","BY" },
	{ "Belgium","BE" },
	{ "Belize","BZ" },
	{ "Benin","BJ" },
	{ "Bermuda","BM" },
	{ "Bhutan","BT" },
	{ "Bolivia, Plurinational State of","BO" },
	{ "Bonaire, Sint Eustatius and Saba","BQ" },
	{ "Bosnia and Herzegovina","BA" },
	{ "Botswana","BW" },
	{ "Bouvet Island","BV" },
	{ "Brazil","BR" },
	{ "British Indian Ocean Territory","IO" },
	{ "Brunei Darussalam","BN" },
	{ "Bulgaria","BG" },
	{ "Burkina Faso","BF" },
	{ "Burundi","BI" },
	{ "Cabo Verde","CV" },
	{ "Cambodia","KH" },
	{ "Cameroon","CM" },
	{ "Canada","CA" },
	{ "Cayman Islands","KY" },
	{ "Central African Republic","CF" },
	{ "Chad","TD" },
	{ "Chile","CL" },
	{ "China","CN" },
	{ "Christmas Island","CX" },
	{ "Cocos (Keeling) Islands","CC" },
	{ "Colombia","CO" },
	{ "Comoros","KM" },
	{ "Congo","CG" },
	{ "Congo, the Democratic Republic of the","CD" },
	{ "Cook Islands","CK" },
	{ "Costa Rica","CR" },
	{ "Côte d'Ivoire","CI" },
	{ "Croatia","HR" },
	{ "Cuba","CU" },
	{ "Curaçao","CW" },
	{ "Cyprus","CY" },
	{ "Czech Republic","CZ" },
	{ "Denmark","DK" },
	{ "Djibouti","DJ" },
	{ "Dominica","DM" },
	{ "Dominican Republic","DO" },
	{ "Ecuador","EC" },
	{ "Egypt","EG" },
	{ "El Salvador","SV" },
	{ "Equatorial Guinea","GQ" },
	{ "Eritrea","ER" },
	{ "Estonia","EE" },
	{ "Ethiopia","ET" },
	{ "Falkland Islands (Malvinas)","FK" },
	{ "Faroe Islands","FO" },
	{ "Fiji","FJ" },
	{ "Finland","FI" },
	{ "France","FR" },
	{ "French Guiana","GF" },
	{ "French Polynesia","PF" },
	{ "French Southern Territories","TF" },
	{ "Gabon","GA" },
	{ "Gambia","GM" },
	{ "Georgia","GE" },
	{ "Germany","DE" },
	{ "Ghana","GH" },
	{ "Gibraltar","GI" },
	{ "Greece","GR" },
	{ "Greenland","GL" },
	{ "Grenada","GD" },
	{ "Guadeloupe","GP" },
	{ "Guam","GU" },
	{ "Guatemala","GT" },
	{ "Guernsey","GG" },
	{ "Guinea","GN" },
	{ "Guinea-Bissau","GW" },
	{ "Guyana","GY" },
	{ "Haiti","HT" },
	{ "Heard Island and McDonald Islands","HM" },
	{ "Holy See","VA" },
	{ "Honduras","HN" },
	{ "Hong Kong","HK" },
	{ "Hungary","HU" },
	{ "Iceland","IS" },
	{ "India","IN" },
	{ "Indonesia","ID" },
	{ "Iran, Islamic Republic of","IR" },
	{ "Iraq","IQ" },
	{ "Ireland","IE" },
	{ "Isle of Man","IM" },
	{ "Israel","IL" },
	{ "Italy","IT" },
	{ "Jamaica","JM" },
	{ "Japan","JP" },
	{ "Jersey","JE" },
	{ "Jordan","JO" },
	{ "Kazakhstan","KZ" },
	{ "Kenya","KE" },
	{ "Kiribati","KI" },
	{ "Korea, Democratic People's Republic of","KP" },
	{ "Korea, Republic of","KR" },
	{ "Kuwait","KW" },
	{ "Kyrgyzstan","KG" },
	{ "Lao People's Democratic Republic","LA" },
	{ "Latvia","LV" },
	{ "Lebanon","LB" },
	{ "Lesotho","LS" },
	{ "Liberia","LR" },
	{ "Libya","LY" },
	{ "Liechtenstein","LI" },
	{ "Lithuania","LT" },
	{ "Luxembourg","LU" },
	{ "Macao","MO" },
	{ "Macedonia, the former Yugoslav Republic of","MK" },
	{ "Madagascar","MG" },
	{ "Malawi","MW" },
	{ "Malaysia","MY" },
	{ "Maldives","MV" },
	{ "Mali","ML" },
	{ "Malta","MT" },
	{ "Marshall Islands","MH" },
	{ "Martinique","MQ" },
	{ "Mauritania","MR" },
	{ "Mauritius","MU" },
	{ "Mayotte","YT" },
	{ "Mexico","MX" },
	{ "Micronesia, Federated States of","FM" },
	{ "Moldova, Republic of","MD" },
	{ "Monaco","MC" },
	{ "Mongolia","MN" },
	{ "Montenegro","ME" },
	{ "Montserrat","MS" },
	{ "Morocco","MA" },
	{ "Mozambique","MZ" },
	{ "Myanmar","MM" },
	{ "Namibia","NA" },
	{ "Nauru","NR" },
	{ "Nepal","NP" },
	{ "Netherlands","NL" },
	{ "New Caledonia","NC" },
	{ "New Zealand","NZ" },
	{ "Nicaragua","NI" },
	{ "Niger","NE" },
	{ "Nigeria","NG" },
	{ "Niue","NU" },
	{ "Norfolk Island","NF" },
	{ "Northern Mariana Islands","MP" },
	{ "Norway","NO" },
	{ "Oman","OM" },
	{ "Pakistan","PK" },
	{ "Palau","PW" },
	{ "Palestine, State of","PS" },
	{ "Panama","PA" },
	{ "Papua New Guinea","PG" },
	{ "Paraguay","PY" },
	{ "Peru","PE" },
	{ "Philippines","PH" },
	{ "Pitcairn","PN" },
	{ "Poland","PL" },
	{ "Portugal","PT" },
	{ "Puerto Rico","PR" },
	{ "Qatar","QA" },
	{ "Réunion","RE" },
	{ "Romania","RO" },
	{ "Russian Federation","RU" },
	{ "Rwanda","RW" },
	{ "Saint Barthélemy","BL" },
	{ "Saint Helena, Ascension and Tristan da Cunha","SH" },
	{ "Saint Kitts and Nevis","KN" },
	{ "Saint Lucia","LC" },
	{ "Saint Martin (French part)","MF" },
	{ "Saint Pierre and Miquelon","PM" },
	{ "Saint Vincent and the Grenadines","VC" },
	{ "Samoa","WS" },
	{ "San Marino","SM" },
	{ "Sao Tome and Principe","ST" },
	{ "Saudi Arabia","SA" },
	{ "Senegal","SN" },
	{ "Serbia","RS" },
	{ "Seychelles","SC" },
	{ "Sierra Leone","SL" },
	{ "Singapore","SG" },
	{ "Sint Maarten (Dutch part)","SX" },
	{ "Slovakia","SK" },
	{ "Slovenia","SI" },
	{ "Solomon Islands","SB" },
	{ "Somalia","SO" },
	{ "South Africa","ZA" },
	{ "South Georgia and the South Sandwich Islands","GS" },
	{ "South Sudan","SS" },
	{ "Spain","ES" },
	{ "Sri Lanka","LK" },
	{ "Sudan","SD" },
	{ "Suriname","SR" },
	{ "Svalbard and Jan Mayen","SJ" },
	{ "Swaziland","SZ" },
	{ "Sweden","SE" },
	{ "Switzerland","CH" },
	{ "Syrian Arab Republic","SY" },
	{ "Taiwan, Province of China","TW" },
	{ "Tajikistan","TJ" },
	{ "Tanzania, United Republic of","TZ" },
	{ "Thailand","TH" },
	{ "Timor-Leste","TL" },
	{ "Togo","TG" },
	{ "Tokelau","TK" },
	{ "Tonga","TO" },
	{ "Trinidad and Tobago","TT" },
	{ "Tunisia","TN" },
	{ "Turkey","TR" },
	{ "Turkmenistan","TM" },
	{ "Turks and Caicos Islands","TC" },
	{ "Tuvalu","TV" },
	{ "Uganda","UG" },
	{ "Ukraine","UA" },
	{ "United Arab Emirates","AE" },
	{ "United Kingdom of Great Britain and Northern Ireland","GB" },
	{ "United States Minor Outlying Islands","UM" },
	{ "United States of America","US" },
	{ "Uruguay","UY" },
	{ "Uzbekistan","UZ" },
	{ "Vanuatu","VU" },
	{ "Venezuela, Bolivarian Republic of","VE" },
	{ "Viet Nam","VN" },
	{ "Virgin Islands, British","VG" },
	{ "Virgin Islands, U.S.","VI" },
	{ "Wallis and Futuna","WF" },
	{ "Western Sahara","EH" },
	{ "Yemen","YE" },
	{ "Zambia","ZM" },
	{ "Zimbabwe","ZW" }
};


static const char *s_country_codes_p [] =
{
	"AD",
	"AE",
	"AF",
	"AG",
	"AI",
	"AL",
	"AM",
	"AO",
	"AQ",
	"AR",
	"AS",
	"AT",
	"AU",
	"AW",
	"AX",
	"AZ",
	"BA",
	"BB",
	"BD",
	"BE",
	"BF",
	"BG",
	"BH",
	"BI",
	"BJ",
	"BL",
	"BM",
	"BN",
	"BO",
	"BQ",
	"BR",
	"BS",
	"BT",
	"BV",
	"BW",
	"BY",
	"BZ",
	"CA",
	"CC",
	"CD",
	"CF",
	"CG",
	"CH",
	"CI",
	"CK",
	"CL",
	"CM",
	"CN",
	"CO",
	"CR",
	"CU",
	"CV",
	"CW",
	"CX",
	"CY",
	"CZ",
	"DE",
	"DJ",
	"DK",
	"DM",
	"DO",
	"DZ",
	"EC",
	"EE",
	"EG",
	"EH",
	"ER",
	"ES",
	"ET",
	"FI",
	"FJ",
	"FK",
	"FM",
	"FO",
	"FR",
	"GA",
	"GB",
	"GD",
	"GE",
	"GF",
	"GG",
	"GH",
	"GI",
	"GL",
	"GM",
	"GN",
	"GP",
	"GQ",
	"GR",
	"GS",
	"GT",
	"GU",
	"GW",
	"GY",
	"HK",
	"HM",
	"HN",
	"HR",
	"HT",
	"HU",
	"ID",
	"IE",
	"IL",
	"IM",
	"IN",
	"IO",
	"IQ",
	"IR",
	"IS",
	"IT",
	"JE",
	"JM",
	"JO",
	"JP",
	"KE",
	"KG",
	"KH",
	"KI",
	"KM",
	"KN",
	"KP",
	"KR",
	"KW",
	"KY",
	"KZ",
	"LA",
	"LB",
	"LC",
	"LI",
	"LK",
	"LR",
	"LS",
	"LT",
	"LU",
	"LV",
	"LY",
	"MA",
	"MC",
	"MD",
	"ME",
	"MF",
	"MG",
	"MH",
	"MK",
	"ML",
	"MM",
	"MN",
	"MO",
	"MP",
	"MQ",
	"MR",
	"MS",
	"MT",
	"MU",
	"MV",
	"MW",
	"MX",
	"MY",
	"MZ",
	"NA",
	"NC",
	"NE",
	"NF",
	"NG",
	"NI",
	"NL",
	"NO",
	"NP",
	"NR",
	"NU",
	"NZ",
	"OM",
	"PA",
	"PE",
	"PF",
	"PG",
	"PH",
	"PK",
	"PL",
	"PM",
	"PN",
	"PR",
	"PS",
	"PT",
	"PW",
	"PY",
	"QA",
	"RE",
	"RO",
	"RS",
	"RU",
	"RW",
	"SA",
	"SB",
	"SC",
	"SD",
	"SE",
	"SG",
	"SH",
	"SI",
	"SJ",
	"SK",
	"SL",
	"SM",
	"SN",
	"SO",
	"SR",
	"SS",
	"ST",
	"SV",
	"SX",
	"SY",
	"SZ",
	"TC",
	"TD",
	"TF",
	"TG",
	"TH",
	"TJ",
	"TK",
	"TL",
	"TM",
	"TN",
	"TO",
	"TR",
	"TT",
	"TV",
	"TW",
	"TZ",
	"UA",
	"UG",
	"UM",
	"US",
	"UY",
	"UZ",
	"VA",
	"VC",
	"VE",
	"VG",
	"VI",
	"VN",
	"VU",
	"WF",
	"WS",
	"YE",
	"YT",
	"ZA",
	"ZM",
	"ZW"
};


static int CompareCountriesByName (const void *v0_p, const void  *v1_p);

static int CompareCountryCodeStrings (const void *v0_p, const void  *v1_p);


/**********************************************************************/


bool GetLocationData (MongoTool * UNUSED_PARAM (tool_p), json_t *row_p, PathogenomicsServiceData *data_p, const char *id_s)
{
	bool success_flag = data_p -> psd_geocoder_fn (data_p, row_p, id_s);

	return success_flag;
}


//const char * InsertLocationData (MongoTool *tool_p, const json_t *row_p, PathogenomicsServiceData *data_p, const char *id_s)
//{
//	const char *error_s = NULL;
//	json_t *location_data_p = GetLocationData (tool_p, row_p, data_p, id_s);
//
//	if (location_data_p)
//		{
//			json_error_t error;
//			json_t *row_json_p = json_pack_ex (&error, 0, "{s:s,s:o}", PG_ID_S, id_s, PG_LOCATION_S, location_data_p);
//
//			if (row_json_p)
//				{
//					error_s = InsertOrUpdateMongoData (tool_p, row_json_p, data_p -> psd_database_s, data_p -> psd_locations_collection_s, PG_ID_S, NULL, NULL);
//
//					WipeJSON (row_json_p);
//				}		/* if (row_json_p) */
//			else
//				{
//					WipeJSON (location_data_p);
//				}
//
//		}		/* if (location_data_p) */
//
//
//	return error_s;
//}


const char *GetCountryCodeFromName (const char * const country_name_s)
{
	const char *code_s = NULL;
	CountryCode key;
	CountryCode *country_p = NULL;

	key.cc_code_s = NULL;
	key.cc_name_s = country_name_s;

	country_p = (CountryCode *) bsearch (&key, s_countries_by_name_p, S_NUM_COUNTRIES, sizeof (CountryCode), CompareCountriesByName);

	if (country_p)
		{
			code_s = country_p -> cc_code_s;
		}

	return code_s;
}


bool IsValidCountryCode (const char * const code_s)
{
	return ((CountryCode *) bsearch (code_s, s_country_codes_p, S_NUM_COUNTRIES, sizeof (const char *), CompareCountryCodeStrings) ? true : false);
}


static int CompareCountriesByName (const void *v0_p, const void  *v1_p)
{
	const CountryCode * const country0_p = (const CountryCode * const) v0_p;
	const CountryCode * const country1_p = (const CountryCode * const) v1_p;

	return Stricmp (country0_p -> cc_name_s, country1_p -> cc_name_s);
}


static int CompareCountryCodeStrings (const void *v0_p, const void  *v1_p)
{
	const char * const country0_s = (const char * const) v0_p;
	const char * const country1_s = * (const char ** const) v1_p;

	return Stricmp (country0_s, country1_s);
}



