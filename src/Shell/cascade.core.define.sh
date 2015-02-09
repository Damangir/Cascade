#! /bin/bash

##############################################################################
#                                                                            #
# Filename     : cascade.core.define.sh                                      #
#                                                                            #
# Version      : 1.1                                                         #
# Date         : 2015-02-03                                                  #
#                                                                            #
# Description  : CASCADE definition based segmentation                       #
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

CHECK_IF_SET POSSIBLE_MASK MODEL_DIR TRANS_FROM_STD OUTPUT RADIUS
if [ $? -ne 0 ]
then
	ERROR Not all required input have been set.
	exit 1
fi
	
##############################################################################
# Implementation of the CASCADE definition                                   #
##############################################################################

for sequence in $ALL_SEQUENCES
do
	eval image=\$${sequence}
	eval model_std=${MODEL_DIR}\$${sequence}_MODEL
	eval model_native=${OUTPUT_DIR}\$${sequence}_MODEL_NATIVE
	
	REGISTER_VECTOR "${model_std}" "${image}" "${model_native}" "${TRANS_FROM_STD}"
done

for sequence in $ALL_SEQUENCES
do
	eval image=\$${sequence}
	eval model_native=${OUTPUT_DIR}\$${sequence}_MODEL_NATIVE
	eval pval=${OUTPUT_DIR}\$${sequence}_PVAL
	eval direction=\$${sequence}_DIR
	
	EXEC ${CASCADE_BIN}TwoSampleKolmogorovSmirnovTest \
			${model_native} ${POSSIBLE_MASK} \
			${image} ${pval} ${RADIUS} ${direction}
	
	if [ -z "${populate}" ]
	then
		populate=$pval
	else
		populate="$populate -max $pval"
	fi
	unset -v image pval direction
done

EXEC fslmaths $populate $OUTPUT
 