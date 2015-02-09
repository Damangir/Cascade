#! /bin/bash

##############################################################################
#                                                                            #
# Filename     : cascade.pipeline.modelfree.freesurfer.sh                         #
#                                                                            #
# Version      : 1.1                                                         #
# Date         : 2015-02-03                                                  #
#                                                                            #
# Description  : CASCADE Model free segmentation using original images and   #
#                Freesurfer output                                           #
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
export IGNORE_FILE_CHECK=YES

T1=${T1:-${OUTPUT_DIR}${FS_T1_NII}}

for sequence in FLAIR T1 T2 PD
do
	safe_seq=$(ENSURE_FILE_IF_SET ${sequence}) || exit 1
	ALL_SEQUENCES="${ALL_SEQUENCES} ${safe_seq}"
done

ALL_SEQUENCES=$(echo $ALL_SEQUENCES)
if [ -z "$ALL_SEQUENCES" ]
then
	ERROR "At least one sequence should be set"
	exit 1	
fi

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


CHECK_IF_SET FREESURFER_DIR FS_ASEG FREESURFER_BTS_IMPORT OUTPUT_DIR CASCADE_DATA WHITE_MATTER_LABEL OUTPUT RADIUS REFERENCE

if [ $? -ne 0 ]
then
	ERROR Not all required input have been set.
	exit 1
fi

for sequence in $ALL_SEQUENCES
do
	eval image=\$${sequence}
	eval normal=${OUTPUT_DIR}\$${sequence}_NORM
	eval registered=${OUTPUT_DIR}\$${sequence}_REGISTERED
			
	INPUT_SEQUENCES="$INPUT_SEQUENCES $sequence=$image"
	NORMAL_SEQUENCES="$NORMAL_SEQUENCES $sequence=$normal"
	REGISTERED_SEQUENCES="$REGISTERED_SEQUENCES $sequence=$registered"
	if [ "$CREATE_PIPELINE" ]
	then
		cline $sequence=$image
	else
		INFO $sequence=$image
	fi
done
[ "$CREATE_PIPELINE" ] && hrule

##############################################################################
# Start the procedure                                                        #
##############################################################################

export OUTPUT_DIR
export CASCADE_DATA
export CASCADE_BIN
export CREATE_PIPELINE


BTS_MAP=${OUTPUT_DIR}${BTS_MAP}
BTS_REG=${OUTPUT_DIR}${BTS_REGISTERED}

echo;echo
hrule
cline "Begin: Import Freesurfer results"
[ -z "$CREATE_PIPELINE" ] && hrule
${SCRIPT_DIR}/cascade.import.freesurfer.sh $SHIFTED_ARGS FREESURFER_DIR=${FREESURFER_DIR} FS_ASEG=${FS_ASEG} FREESURFER_BTS_IMPORT=${FREESURFER_BTS_IMPORT}
comment "End:   Import Freesurfer results"

echo;echo
hrule
cline "Begin: Preprocessing pipeline"
[ -z "$CREATE_PIPELINE" ] && hrule
${SCRIPT_DIR}/cascade.pipeline.pre.sh $SHIFTED_ARGS ${INPUT_SEQUENCES} BRAIN_TISSUE_SEG=${BTS_MAP} WHITE_MATTER_LABEL=${WHITE_MATTER_LABEL} BTS_REF=T1 REFERENCE=${REFERENCE}
comment "End:   Preprocessing pipeline"


echo;echo
hrule
cline "Begin: Model free segmentation pipeline"
[ -z "$CREATE_PIPELINE" ] && hrule
${SCRIPT_DIR}/cascade.pipeline.modelfree.sh $SHIFTED_ARGS ${NORMAL_SEQUENCES} BRAIN_TISSUE_SEG=${BTS_REG} WHITE_MATTER_LABEL=${WHITE_MATTER_LABEL} OUTPUT=${OUTPUT} RADIUS=${RADIUS} 
comment "End:   Model free segmentation pipeline"
