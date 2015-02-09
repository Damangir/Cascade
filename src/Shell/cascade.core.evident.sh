#! /bin/bash

##############################################################################
#                                                                            #
# Filename     : cascade.core.evident.sh                                     #
#                                                                            #
# Version      : 1.1                                                         #
# Date         : 2015-02-03                                                  #
#                                                                            #
# Description  : CASCADE remove evidently non-WMC tissues                    #
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

CHECK_IF_SET BRAIN_TISSUE_SEG WHITE_MATTER_LABEL OUTPUT

if [ $? -ne 0 ]
then
	ERROR Not all required input have been set.
	exit 1
fi

##############################################################################
# Remove evidently normal brain tissues                                      #
##############################################################################

for sequence in $ALL_SEQUENCES
do
	eval image=\$${sequence}
	eval evident=${OUTPUT_DIR}\$${sequence}_EVID
	eval percentile=\$${sequence}_PER
	
	EXEC ${CASCADE_BIN}EvidentNormal \
		${image} ${BRAIN_TISSUE_SEG} \
		${evident} ${percentile} ${WHITE_MATTER_LABEL}
	if [ -z "${populate}" ]
	then
		populate="$evident -bin"
	else
		populate="$populate -add $evident -bin"
	fi
	unset -v image evident percentile
done

EXEC fslmaths $populate -mul -1 -add 1 $OUTPUT 
EXEC fslmaths $BRAIN_TISSUE_SEG -thr $WHITE_MATTER_LABEL -bin -mul $OUTPUT $OUTPUT  


