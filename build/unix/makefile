NAME 		:= pathogenomics_service
DIR_BUILD :=  $(realpath $(dir $(lastword $(MAKEFILE_LIST)))/$(PLATFORM))
DIR_SRC := $(realpath $(DIR_BUILD)/../../../src)
DIR_INCLUDE := $(realpath $(DIR_BUILD)/../../../include)

ifeq ($(DIR_BUILD_CONFIG),)
export DIR_BUILD_CONFIG = $(realpath $(DIR_BUILD)/../../../../../build-config/unix/)
#export DIR_BUILD_CONFIG = /Users/tyrrells/Projects/grassroots/build-config/unix/
endif

include $(DIR_BUILD_CONFIG)/project.properties

VPATH := $(DIR_SRC)

BUILD		:= debug

export DIR_INSTALL := $(DIR_GRASSROOTS_INSTALL)/services

VPATH	= \
	$(DIR_SRC) \
	

INCLUDES = \
	-I$(DIR_INCLUDE) \
	-I$(DIR_GRASSROOTS_MONGODB_INC) \
	-I$(DIR_GRASSROOTS_USERS_INC) \
	-I$(DIR_GRASSROOTS_UUID_INC) \
	-I$(DIR_GRASSROOTS_UTIL_INC) \
	-I$(DIR_GRASSROOTS_UTIL_INC)/containers \
	-I$(DIR_GRASSROOTS_UTIL_INC)/io \
	-I$(DIR_GRASSROOTS_HANDLER_INC) \
	-I$(DIR_GRASSROOTS_SERVER_INC) \
	-I$(DIR_GRASSROOTS_SERVICES_INC) \
	-I$(DIR_GRASSROOTS_SERVICES_INC)/parameters \
	-I$(DIR_GRASSROOTS_NETWORK_INC) \
	-I$(DIR_GRASSROOTS_PLUGIN_INC) \
	-I$(DIR_GRASSROOTS_TASK_INC) \
	-I$(DIR_GRASSROOTS_GEOCODER_INC) \
	-I$(DIR_JANSSON_INC) \
	-I$(DIR_UUID_INC) \
	-I$(DIR_MONGODB_INC) \
	-I$(DIR_BSON_INC) \
	
SRCS 	= \
	pathogenomics_service.c \
	files_metadata.c \
	sample_metadata.c	\
	phenotype_metadata.c \
	genotype_metadata.c \
	pathogenomics_utils.c

CPPFLAGS += -DPATHOGENOMICS_SERVICE_EXPORTS 

LDFLAGS += -L$(DIR_JANSSON_LIB) -ljansson \
	-L$(DIR_GRASSROOTS_UTIL_LIB) -l$(GRASSROOTS_UTIL_LIB_NAME) \
	-L$(DIR_GRASSROOTS_UUID_LIB) -l$(GRASSROOTS_UUID_LIB_NAME) \
	-L$(DIR_GRASSROOTS_SERVICES_LIB) -l$(GRASSROOTS_SERVICES_LIB_NAME) \
	-L$(DIR_GRASSROOTS_SERVER_LIB) -l$(GRASSROOTS_SERVER_LIB_NAME) \
	-L$(DIR_GRASSROOTS_NETWORK_LIB) -l$(GRASSROOTS_NETWORK_LIB_NAME) \
	-L$(DIR_GRASSROOTS_PARAMS_LIB) -l$(GRASSROOTS_PARAMS_LIB_NAME) \
	-L$(DIR_GRASSROOTS_MONGODB_LIB) -l$(GRASSROOTS_MONGODB_LIB_NAME)  \
	-L$(DIR_GRASSROOTS_GEOCODER_LIB) -l$(GRASSROOTS_GEOCODER_LIB_NAME)


all:: 
	make -C $(DIR_GRASSROOTS_GEOCODER_BUILD) all

include $(DIR_BUILD_CONFIG)/generic_makefiles/shared_library.makefile

