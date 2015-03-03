#! /bin/bash

##############################################################################
#                                                                            #
# Filename     : cascade.extra.stdregister.sh                                #
#                                                                            #
# Version      : 1.1                                                         #
# Date         : 2015-02-13                                                  #
#                                                                            #
# Description  : CASCADE unofficial extra script: Registration to standard   #
#                        MNI space                                           #
#                http://ki.se/en/nvs/cascade                                 #
#                                                                            #
# Copyright 2013-2015 Soheil Damangir                                        #
#                                                                            #
# Author: Soheil Damangir                                                    #
##############################################################################

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
SCRIPT_NAME=$( basename ${BASH_SOURCE[0]} )

CONFIG_FILE=cascade.config.sh

##############################################################################
# Variable definition                                                        #
##############################################################################

source ${SCRIPT_DIR}/${CONFIG_FILE}



CHECK_IF_SET OUTPUT_DIR MNI_ALL MNI_BRAIN MNI_BRAIN_MASK
if [ $? -ne 0 ]
then
	ERROR Not all required input have been set.
	exit 1
fi

ALL_SEQUENCES=
for sequence in T1 FLAIR T2 PD
do
	safe_seq=$(ENSURE_FILE_IF_SET ${sequence}) || exit 1
	ALL_SEQUENCES="${ALL_SEQUENCES} ${safe_seq}"
done
for sequence in $ALL_SEQUENCES
do
	eval image=\$${sequence}
	if [ "$CREATE_PIPELINE" ]
	then
		cline $sequence=$image
	else
		INFO $sequence=$image
	fi
	break
done
[ "$CREATE_PIPELINE" ] && hrule
						
##############################################################################
# Standard registration implementation of the CASCADE                        #
##############################################################################

TEMP_DIR=${OUTPUT_DIR}${SCRIPT_NAME%.sh}.temp.${PPID}
EXEC mkdir ${TEMP_DIR}

eval "transferFile=\$${sequence}_TO_MNI_L"
eval "invTransferFile=\$MNI_TO_${sequence}_L"

brain=${TEMP_DIR}/brain.nii.gz

registered=${OUTPUT_DIR}in_std.nii.gz

EXEC ${CASCADE_BIN}linRegister "${MNI_ALL}" "${image}" "${TEMP_DIR}/${transferFile}" "${TEMP_DIR}/${invTransferFile}"
EXEC ${CASCADE_BIN}resample "${image}" "${MNI_BRAIN_MASK}" "${brain}" "${TEMP_DIR}/${invTransferFile}"
EXEC ${CASCADE_BIN}brainExtraction "${image}" "${brain}" "${brain}"
EXEC fslmaths "${image}" -mas "${brain}" "${brain}"

EXEC ${CASCADE_BIN}linRegister "${MNI_BRAIN}" "${brain}" "${OUTPUT_DIR}/${transferFile}" "${OUTPUT_DIR}/${invTransferFile}"

EXEC rm -rf "${TEMP_DIR}"
