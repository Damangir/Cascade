#! /bin/bash

##############################################################################
#                                                                            #
# Filename     : cascade.core.modelfree.sh                                   #
#                                                                            #
# Version      : 1.1                                                         #
# Date         : 2015-02-03                                                  #
#                                                                            #
# Description  : CASCADE model free segmentation                             #
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

FREESURFER_DIR=${FREESURFER_DIR%/}
if [ $FS_ASEG ]
then
	FREESURFER_DIR=$(dirname $(dirname $FS_ASEG))
else
	[ $FREESURFER_DIR ] && FS_ASEG=${FREESURFER_DIR}/mri/aseg.mgz
fi
[ -z $FS_T1 ] && [ $FREESURFER_DIR ] && FS_T1=${FREESURFER_DIR}/mri/rawavg.mgz
[ ${FREESURFER_BTS_IMPORT} ] || FREESURFER_BTS_IMPORT=${CASCADE_DATA}/map/FS_label.map.txt
[ ${UNITY_TRANSFORM} ] || UNITY_TRANSFORM=${CASCADE_DATA}/transform/unity.tfm


CHECK_IF_SET FREESURFER_DIR FS_ASEG FS_T1 FREESURFER_BTS_IMPORT CASCADE_DATA OUTPUT_DIR BTS_MAP FS_T1_NII
if [ $? -ne 0 ]
then
	ERROR Not all required input have been set.
	exit 1
fi
		
##############################################################################
# Import freesurfer data                                                     #
##############################################################################

if command -v mri_convert
then
	EXEC mri_convert ${FS_T1} ${OUTPUT_DIR}${FS_T1_NII}
	EXEC mri_convert ${FS_ASEG} ${OUTPUT_DIR}${BTS_MAP}
	EXEC ${CASCADE_BIN}resample ${OUTPUT_DIR}${FS_T1_NII} ${OUTPUT_DIR}${BTS_MAP} ${OUTPUT_DIR}${BTS_MAP} ${UNITY_TRANSFORM} nn
else
	EXEC ${CASCADE_BIN}ImportImage ${FS_T1} ${OUTPUT_DIR}${FS_T1_NII}
	EXEC ${CASCADE_BIN}ImportImage ${FS_ASEG} ${OUTPUT_DIR}${BTS_MAP}
	EXEC ${CASCADE_BIN}resample ${OUTPUT_DIR}${FS_T1_NII} ${OUTPUT_DIR}${BTS_MAP} ${OUTPUT_DIR}${BTS_MAP} ${UNITY_TRANSFORM} nn
fi
EXEC ${CASCADE_BIN}relabel ${OUTPUT_DIR}${BTS_MAP} ${FREESURFER_BTS_IMPORT} ${OUTPUT_DIR}${BTS_MAP}