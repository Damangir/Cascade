#! /bin/bash

##############################################################################
#                                                                            #
# Filename     : cascade.core.evident.sh                                     #
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
export IGNORE_FILE_CHECK=YES

ALL_SEQUENCES=
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

CHECK_IF_SET REFERENCE BRAIN_TISSUE_SEG BTS_REF OUTPUT_DIR
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

export CREATE_PIPELINE

echo;echo
hrule
cline "Begin: Coregister input images"
[ -z "$CREATE_PIPELINE" ] && hrule
${SCRIPT_DIR}/cascade.pre.coregister.sh $SHIFTED_ARGS $INPUT_SEQUENCES OUTPUT_DIR="${OUTPUT_DIR}" REFERENCE=${REFERENCE} BRAIN_TISSUE_SEG=${BRAIN_TISSUE_SEG} BTS_REF=${BTS_REF}
comment "End:   Coregister input images"

echo;echo
comment "Begin: Create WM mask"
WM_MASK=${OUTPUT_DIR}wm_mask.nii.gz
EXEC fslmaths ${OUTPUT_DIR}${BTS_REGISTERED} -thr $WHITE_MATTER_LABEL -bin $WM_MASK 
comment "End:   Create WM mask"

echo;echo
hrule
cline "Begin: Normalize input images"
[ -z "$CREATE_PIPELINE" ] && hrule
${SCRIPT_DIR}/cascade.pre.normalize.sh $SHIFTED_ARGS $REGISTERED_SEQUENCES NORMALIZE_MASK=$WM_MASK
comment "End:   Normalize input images"
