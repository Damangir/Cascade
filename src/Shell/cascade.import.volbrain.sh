#! /bin/bash

##############################################################################
#                                                                            #
# Filename     : cascade.import.volbrain.sh                                  #
#                                                                            #
# Version      : 1.1                                                         #
# Date         : 2015-04-13                                                  #
#                                                                            #
# Description  : CASCADE import volbrain segmentations                       #
#                http://ki.se/en/nvs/cascade                                 #
#                                                                            #
# Copyright 2013-2015 Soheil Damangir                                        #
#                                                                            #
# Author: Soheil Damangir                                                    #
##############################################################################

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
CONFIG_FILE=cascade.config.sh

##############################################################################
# Variable definition                                                        #
##############################################################################

source ${SCRIPT_DIR}/${CONFIG_FILE}

VOLBRAIN_DIR=${VOLBRAIN_DIR%/}

CHECK_IF_SET VOLBRAIN_DIR CASCADE_DATA CASCADE_BIN OUTPUT_DIR BTS_MAP VOLBRAIN_T1_NII
if [ $? -ne 0 ]
then
	ERROR Not all required input have been set.
	exit 1
fi

VOLBRAIN_T1=$(echo ${VOLBRAIN_DIR}/native_n_mmni*)
VOLBRAIN_BTS=$(echo ${VOLBRAIN_DIR}/native_crisp_mmni*)
VOLBRAIN_HEMISPHERE=$(echo ${VOLBRAIN_DIR}/native_hemi_n_mmni*)
VOLBRAIN_BRAIN=$(echo ${VOLBRAIN_DIR}/native_mask_n_mmni*)
VOLBRAIN_LABEL=$(echo ${VOLBRAIN_DIR}/native_lab_n_mmni*)


##############################################################################
# Import volbrain data                                                       #
##############################################################################

EXEC ${CASCADE_BIN}ImportImage "${VOLBRAIN_T1}" "${OUTPUT_DIR}${VOLBRAIN_T1_NII}"
EXEC ${CASCADE_BIN}ImportImage "${VOLBRAIN_BTS}" "${OUTPUT_DIR}${BTS_MAP}"
