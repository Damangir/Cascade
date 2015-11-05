#! /bin/bash

##############################################################################
#                                                                            #
# Filename     : cascade.extra.bts.sh                                        #
#                                                                            #
# Version      : 1.1                                                         #
# Date         : 2015-02-13                                                  #
#                                                                            #
# Description  : CASCADE unofficial extra script: Brain tissue segmentation  #
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

ALL_SEQUENCES=
for sequence in T1 FLAIR T2 PD
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
# Brain tissue segmentation implementation of the CASCADE                    #
##############################################################################

export OUTPUT_DIR
export CASCADE_DATA
export CASCADE_BIN
export CREATE_PIPELINE

TEMP_DIR=${OUTPUT_DIR}${SCRIPT_NAME%.sh}.temp.${PPID}
EXEC mkdir ${TEMP_DIR}


eval "transferFile=\$${sequence}_TO_MNI_L"
eval "invTransferFile=\$MNI_TO_${sequence}_L"

SILENT_CHECK=1
CHECK_IF_SET PRIOR_CSF PRIOR_GM PRIOR_WM BRAIN_MASK
if [ $? -ne 0 ]
then
	echo;echo
	hrule
	cline "Begin: Claculating registration from $sequence to MNI"
	# Either priors or brain mask is not set, we need the registration to MNI	
	${SCRIPT_DIR}/cascade.extra.stdregister.sh $SHIFTED_ARGS ${sequence}="${image}" OUTPUT_DIR="${OUTPUT_DIR}"
	comment  "End: Claculating registration from $sequence to MNI"
fi

CHECK_IF_SET BRAIN_MASK
if [ $? -ne 0 ]
then
	echo;echo
	comment "Begin: Brain extraction on $sequence"
	BRAIN_MASK=${OUTPUT_DIR}/brain.mask.nii.gz
	EXEC ${CASCADE_BIN}resample "${T1}" "${MNI_BRAIN_MASK}" "${BRAIN_MASK}" "${OUTPUT_DIR}/${invTransferFile}"
	EXEC ${CASCADE_BIN}brainExtraction "${T1}" "${BRAIN_MASK}" "${BRAIN_MASK}"
	comment "End:   Brain extraction on $sequence"
	
	if [ "$ONLY_BRAIN_MASK" ]
	then
		EXEC rm -rf ${TEMP_DIR}
		exit 0
	fi
fi

CHECK_IF_SET PRIOR_CSF PRIOR_GM PRIOR_WM
if [ $? -ne 0 ]
then
	echo;echo
	comment "Begin: Resampling brain tissue priors"
	PRIOR_CSF=${OUTPUT_DIR}${sequence}.$(basename "${PRIOR_CSF_MNI}")
	PRIOR_GM=${OUTPUT_DIR}${sequence}.$(basename "${PRIOR_GM_MNI}")
	PRIOR_WM=${OUTPUT_DIR}${sequence}.$(basename "${PRIOR_WM_MNI}")
	
	EXEC ${CASCADE_BIN}resample "${image}" "${PRIOR_CSF_MNI}" "${PRIOR_CSF}" "${OUTPUT_DIR}/${invTransferFile}"
	EXEC ${CASCADE_BIN}resample "${image}" "${PRIOR_WM_MNI}" "${PRIOR_WM}" "${OUTPUT_DIR}/${invTransferFile}"
	EXEC ${CASCADE_BIN}resample "${image}" "${PRIOR_GM_MNI}" "${PRIOR_GM}" "${OUTPUT_DIR}/${invTransferFile}"
	comment "End:   Resampling brain tissue priors"				
fi

echo;echo
comment "Begin: Tissue type segmentation"
BTS_UNCOR=${TEMP_DIR}/bts_uncorrected.nii.gz
EXEC ${CASCADE_BIN}TissueTypeSegmentation "$image" "$BRAIN_MASK" "${OUTPUT_DIR}$(eval echo \$BTS_IN_${sequence})" "$PRIOR_CSF" "$PRIOR_GM" "$PRIOR_WM"
comment   "End: Tissue type segmentation"


EXEC rm -rf ${TEMP_DIR}
