NAME 		= pathogenomics_service
DIR_SRC = $(DIR_SERVICES_ROOT_SRC)/pathogenomics/src
DIR_INCLUDE = $(DIR_SERVICES_ROOT_SRC)/pathogenomics/include
CC		= g++
CXX		= g++
MAKEDEPEND 	= gcc -MM -MF

BUILD		= debug

ifeq ($(DIR_ROOT),)
export DIR_ROOT = $(realpath $(../..))
endif

ifneq ($(PROPERTIES_SET),1)
include $(DIR_BUILD_ROOT)/project.properties
endif

export DIR_INSTALL=$(DIR_GRASSROOTS_INSTALL)/services

VPATH	= \
	$(DIR_SRC) \
	

INCLUDES = \
	-I$(DIR_INCLUDE) \
	-I$(DIR_GRASSROOTS_MONGODB_INC) \
	-I$(DIR_GRASSROOTS_UTIL_INC) \
	-I$(DIR_GRASSROOTS_UTIL_INC)/containers \
	-I$(DIR_GRASSROOTS_UTIL_INC)/io \
	-I$(DIR_GRASSROOTS_HANDLER_INC) \
	-I$(DIR_GRASSROOTS_SERVER_INC) \
	-I$(DIR_GRASSROOTS_SERVICES_INC) \
	-I$(DIR_GRASSROOTS_NETWORK_INC) \
	-I$(DIR_GRASSROOTS_PARAMS_INC) \
	-I$(DIR_SHARED_IRODS_INC)/lib/api/include \
	-I$(DIR_SHARED_IRODS_INC)/lib/core/include \
	-I$(DIR_SHARED_IRODS_INC)/lib/md5/include \
	-I$(DIR_SHARED_IRODS_INC)/lib/sha1/include \
	-I$(DIR_SHARED_IRODS_INC)/server/core/include \
	-I$(DIR_SHARED_IRODS_INC)/server/drivers/include \
	-I$(DIR_SHARED_IRODS_INC)/server/icat/include \
	-I$(DIR_SHARED_IRODS_INC)/server/re/include \
	-I$(DIR_JANSSON_INC) \
	-I$(DIR_UUID_INC) \
	-I$(DIR_MONGODB_INC) \
	-I$(DIR_BSON_INC) \
	
SRCS 	= \
	pathogenomics_service.c \
	country_codes.c \
	files_metadata.c \
	sample_metadata.c	\
	phenotype_metadata.c \
	genotype_metadata.c \
	pathogenomics_utils.c

CPPFLAGS += -DPATHOGENOMICS_SERVICE_EXPORTS 

LDFLAGS += -L$(DIR_JANSSON_LIB) -ljansson \
	-L$(DIR_GRASSROOTS_UTIL_LIB) -l$(GRASSROOTS_UTIL_LIB_NAME) \
	-L$(DIR_GRASSROOTS_SERVICES_LIB) -l$(GRASSROOTS_SERVICES_LIB_NAME) \
	-L$(DIR_GRASSROOTS_NETWORK_LIB) -l$(GRASSROOTS_NETWORK_LIB_NAME) \
	-L$(DIR_GRASSROOTS_PARAMS_LIB) -l$(GRASSROOTS_PARAMS_LIB_NAME) \
	-L$(DIR_GRASSROOTS_MONGODB_LIB) -l$(GRASSROOTS_MONGODB_LIB_NAME) 


include $(DIR_BUILD_ROOT)/generic_makefiles/shared_library.makefile

