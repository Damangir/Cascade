#! /bin/bash

##############################################################################
#                                                                            #
# Filename     : cascade.pre.refinebts.sh                                    #
#                                                                            #
# Version      : 1.1                                                         #
# Date         : 2015-02-13                                                  #
#                                                                            #
# Description  : CASCADE refining brain tissue segmentation using clues from #
#                        FLAIR and T2 images.                                #
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
ALL_SEQUENCES=
for sequence in FLAIR T2
do
	safe_seq=$(ENSURE_FILE_IF_SET ${sequence}) || exit 1
	ALL_SEQUENCES="${ALL_SEQUENCES} ${safe_seq}"
done

ALL_SEQUENCES=$(echo $ALL_SEQUENCES)
if [ -z "$ALL_SEQUENCES" ]
then
	WARNING "At least one of FLAIR or T2 should be set"
fi

	
CHECK_IF_SET BTS_MAP BTS_PRE OUTPUT_DIR
if [ $? -ne 0 ]
then
	ERROR Not all required input have been set.
	exit 1
fi

							
##############################################################################
# Refine brain tissue segmentation                                           #
##############################################################################


if [[ ${ALL_SEQUENCES} == *"FLAIR"* ]]
then
	EXEC ${CASCADE_BIN}refineBTS $FLAIR $BTS_PRE ${OUTPUT_DIR}${BTS_MAP} 0.85 0.2 
	EXEC ${CASCADE_BIN}CorrectGrayMatterFalsePositive $FLAIR ${OUTPUT_DIR}${BTS_MAP} ${OUTPUT_DIR}${BTS_MAP} 0.9 0.4
	exit 0
fi	

if [[ ${ALL_SEQUENCES} == *"T2"* ]]
then
	EXEC ${CASCADE_BIN}refineBTS $T2 $BTS_PRE ${OUTPUT_DIR}${BTS_MAP} 0.85 0.2 
	EXEC ${CASCADE_BIN}CorrectGrayMatterFalsePositive $T2 ${OUTPUT_DIR}${BTS_MAP} ${OUTPUT_DIR}${BTS_MAP} 0.9 0.4
	exit 0
fi

cp $BTS_PRE ${OUTPUT_DIR}${BTS_MAP}