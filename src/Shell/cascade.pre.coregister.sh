#! /bin/bash

##############################################################################
#                                                                            #
# Filename     : cascade.pre.coregister.sh                                   #
#                                                                            #
# Version      : 1.1                                                         #
# Date         : 2015-02-03                                                  #
#                                                                            #
# Description  : CASCADE Coregistration                                      #
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

if [[ ${ALL_SEQUENCES} == *"${REFERENCE}"* ]]
then
	REFERENCE=$(ENSURE_FILE_IF_SET $REFERENCE)
	eval REFERENCE=\$${REFERENCE}
else
	REFERENCE=$(ENSURE_FILE_IF_SET REFERENCE)
fi



BTS_REF=$( tr '[:lower:]' '[:upper:]'<<<$BTS_REF )
[ ${BRAIN_TISSUE_SEG} ] && TO_BE_CHECKED="$TO_BE_CHECKED BTS_REF BRAIN_TISSUE_SEG"
	
CHECK_IF_SET REFERENCE OUTPUT_DIR $TO_BE_CHECKED
if [ $? -ne 0 ]
then
	ERROR Not all required input have been set.
	exit 1
fi

if [[ ${ALL_SEQUENCES} != *"${BTS_REF}"* ]]
then
	ERROR Reference for brain tissue segmentation is \"${BTS_REF}\" which is not\
	specified in the input sequences : $ALL_SEQUENCES 
	exit 1	
fi
							
##############################################################################
# Coregister images                                                          #
##############################################################################

for sequence in $ALL_SEQUENCES
do
	eval image=\$${sequence}
	eval registered=\$${sequence}_REGISTERED
	eval transferFile=\$${sequence}_TO_REF
	eval invTransferFile=\$${sequence}_FROM_REF
	
	registered=${OUTPUT_DIR}${registered}
	transferFile=${OUTPUT_DIR}${transferFile}
	invTransferFile=${OUTPUT_DIR}${invTransferFile}
	
	EXEC ${CASCADE_BIN}linRegister ${REFERENCE} ${image} ${transferFile} ${invTransferFile}
	EXEC ${CASCADE_BIN}resample ${REFERENCE} ${image} ${registered} ${transferFile}
	unset -v image registered
done

if [ ${BRAIN_TISSUE_SEG} ]
then
	eval registered=${BTS_REGISTERED}
	eval transferFile=\$${BTS_REF}_TO_REF
	registered=${OUTPUT_DIR}${registered}
	transferFile=${OUTPUT_DIR}${transferFile}
	EXEC ${CASCADE_BIN}resample ${REFERENCE} ${BRAIN_TISSUE_SEG} ${registered} ${transferFile}	nn
fi
