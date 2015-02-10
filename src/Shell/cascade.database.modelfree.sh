#! /bin/bash

##############################################################################
#                                                                            #
# Filename     : cascade.database.modelfree.sh                               #
#                                                                            #
# Version      : 1.1                                                         #
# Date         : 2015-02-03                                                  #
#                                                                            #
# Description  : CASCADE Model free segmentation using original images and   #
#                Freesurfer output  for the whole database                   #
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

DATABASE=$(ENSURE_FILE_IF_SET ${DATABASE})
OUTPUT=${OUTPUT:-wml.pvalue.nii.gz}

CHECK_IF_SET DATABASE OUTPUT_DIR CASCADE_DATA OUTPUT
if [ $? -ne 0 ]
then
	ERROR Not all required input have been set.
	exit 1
fi

##############################################################################
# Start the procedure                                                        #
##############################################################################

OUTPUT_DIR=${OUTPUT_DIR%/}/

while IFS= read -r aline
do
	IFS=' ' read -r JOB_ID ARGUMENTS0 <<< "$aline"
	[ -z "$JOB_ID" ] && continue
	[ "${JOB_ID:0:1}" == "#" ]  && continue
	ARGUMENTS="CREATE_PIPELINE=YES"
	JOB_OUT_DIR=${OUTPUT_DIR}${JOB_ID}
	OUTPUT_NAME=wml.pval.nii.gz
	for var in $ARGUMENTS0
	do
		if [[ $var == *=* ]];
		then
			IFS='=' read -r ARG_KEY ARG_VALUE <<< "$var"
			if [ "${ARG_KEY}" = "OUTPUT_DIR" ]
			then
				JOB_OUT_DIR=${ARG_VALUE%/}
				continue
			fi
			if [ "${ARG_KEY}" = "OUTPUT" ]
			then
				OUTPUT_NAME=$ARG_VALUE
				continue
			fi
			ARGUMENTS="$ARGUMENTS ${ARG_KEY}=${ARG_VALUE}"		
		fi
	done
	
	if [ "$(dirname $OUTPUT_NAME)" == "." ]
	then
		OUTPUT_NAME=${JOB_OUT_DIR}/$(basename ${OUTPUT_NAME})
	fi
	
	ARGUMENTS="$ARGUMENTS OUTPUT=${OUTPUT_NAME} OUTPUT_DIR=${JOB_OUT_DIR}"
	echo $ARGUMENTS
	GOOD_SCRIPT=1
	
	printf "Generating ${JOB_ID} "
	for pipeline in  cascade.pipeline.{modelfree,modelfree.bts,modelfree.freesurfer}.sh
	do
		SCRIPT_TO_RUN=${SCRIPT_DIR}/${pipeline}
		JOB_SCRIPT_CONTENT=$($SCRIPT_TO_RUN $ARGUMENTS 2>/dev/null)
		GOOD_SCRIPT=$?
		[ $GOOD_SCRIPT -eq 0 ] && break 
	done
	
	JOB_SCRIPT=${OUTPUT_DIR}${JOB_ID}.sh
	if [ $GOOD_SCRIPT -eq 0 ]
	then
	(
		SCRIPT_HEADING
		printf "export ITK_GLOBAL_DEFAULT_NUMBER_OF_THREADS=1\n"
		if [ "${JOB_OUT_DIR}" ]
		then
			printf "mkdir -p "%s"\n" $JOB_OUT_DIR
		fi
		printf "\n%s\n" "$JOB_SCRIPT_CONTENT"
	) > ${JOB_SCRIPT}
		printf "done!\n"
	else
		ERROR Can not find proper pipeline for ${JOB_ID}
		rm $JOB_SCRIPT
		printf "fail!\n"
	fi
	
	 
done < ${DATABASE}

