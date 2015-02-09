#! /bin/bash

##############################################################################
#                                                                            #
# Filename     : cascade.core.preparetrain.sh                                #
#                                                                            #
# Version      : 1.1                                                         #
# Date         : 2015-02-03                                                  #
#                                                                            #
# Description  : CASCADE setup the files for training                        #
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

CHECK_IF_SET TRAIN_MASK TRANS_TO_STD STANDARD_IMAGE TRAIN_MASK_STD
if [ $? -ne 0 ]
then
	ERROR Not all required input have been set.
	exit 1
fi
	
##############################################################################
# Prepare images in the standard space for training                          #
##############################################################################

for sequence in $ALL_SEQUENCES
do
	eval image=\$${sequence}
	eval standard=${OUTPUT_DIR}\$${sequence}_STD
	REGISTER "${image}" "${STANDARD_IMAGE}" "${standard}" "${TRANS_TO_STD}"
	unset -v image standard
done
standard=${OUTPUT_DIR}${TRAIN_MASK_STD}
REGISTER "${TRAIN_MASK}" "${STANDARD_IMAGE}" "${standard}" "${TRANS_TO_STD}" nn
