##############################################################################
#                                                                            #
# Filename     : cascade.config.sh                                      #
#                                                                            #
# Version      : 1.0                                                         #
# Date         : 2015-02-03                                                  #
#                                                                            #
# Description  : CASCADE default filenames and configuration                 #
#                http://ki.se/en/nvs/cascade                                 #
#                                                                            #
# Copyright 2013-2015 Soheil Damangir                                        #
#                                                                            #
# Author: Soheil Damangir                                                    #
##############################################################################

WHITE_MATTER_LABEL=3

BTS_MAP=brain.tissues.nii.gz
FS_T1_NII=rawavg.nii.gz

##############################################################################
# Coregistration                                                             #
##############################################################################

T1_REGISTERED=t1.coreg.nii.gz
FLAIR_REGISTERED=flair.coreg.nii.gz
T2_REGISTERED=t2.coreg.nii.gz
PD_REGISTERED=pd.coreg.nii.gz

BTS_REGISTERED=brain.tissues.coreg.nii.gz

T1_TO_REF=t1.to.ref.tfm
FLAIR_TO_REF=flair.to.ref.tfm
T2_TO_REF=t2.to.ref.tfm
PD_TO_REF=pd.to.ref.tfm

T1_FROM_REF=t1.from.ref.tfm
FLAIR_FROM_REF=flair.from.ref.tfm
T2_FROM_REF=t2.from.ref.tfm
PD_FROM_REF=pd.from.ref.tfm

##############################################################################
# Normalizing input images                                                   #
##############################################################################

T1_NORM=t1.normalized.nii.gz
FLAIR_NORM=flair.normalized.nii.gz
T2_NORM=t2.normalized.nii.gz
PD_NORM=pd.normalized.nii.gz

##############################################################################
# Remove evidently normal brain tissues                                      #
##############################################################################

T1_EVID=t1.evident.nii.gz
FLAIR_EVID=flair.evident.nii.gz
T2_EVID=t2.evident.nii.gz
PD_EVID=pd.evident.nii.gz

T1_PER=-0.90
FLAIR_PER=0.5
T2_PER=0.3
PD_PER=0.1

##############################################################################
# CASCADE Model free segmentation                                            #
##############################################################################

RADIUS=1

T1_PVAL=t1.modelfree.pval.nii.gz
FLAIR_PVAL=flair.modelfree.pval.nii.gz
T2_PVAL=t2.modelfree.pval.nii.gz
PD_PVAL=pd.modelfree.pval.nii.gz

T1_DIR=neg
FLAIR_DIR=pos
T2_DIR=pos
PD_DIR=pos

##############################################################################
# CASCADE Pre-Train model                                                    #
##############################################################################

T1_STD=t1.standard.nii.gz
FLAIR_STD=flair.standard.nii.gz
T2_STD=t2.standard.nii.gz
PD_STD=pd.standard.nii.gz

BTS_STD=brain.tissue.standard.nii.gz
TRAIN_MASK_STD=train.mask.standard.nii.gz

STANDARD_IMAGE=${CASCADE_DATA%/}/MNI.nii.gz
##############################################################################
# CASCADE definition segmentation                                            #
##############################################################################

T1_MODEL=t1.model.nii.gz
FLAIR_MODEL=flair.model.nii.gz
T2_MODEL=t2.model.nii.gz
PD_MODEL=pd.model.nii.gz

T1_MODEL_NATIVE=t1.model.native.nii.gz
FLAIR_MODEL_NATIVE=flair.model.native.nii.gz
T2_MODEL_NATIVE=t2.model.native.nii.gz
PD_MODEL_NATIVE=pd.model.native.nii.gz

##############################################################################
# Utilities                                                                  #
##############################################################################

ERROR()   { cat <<< "ERROR:    $@" 1>&2; }
WARNING() { cat <<< "WARNING:  $@" 1>&2; }
INFO()    { cat <<< "INFO:     $@" 1>&2; }
MESSAGE() { cat <<< "          $@" 1>&2; }

ENSURE_FILE_IF_SET(){
eval the_file=\$${1}
	
if [ "${the_file}" ]; then
	if [ -e "${the_file}" ]; then
		echo "${1}"
	else
		if [ "$CREATE_PIPELINE" ] || [ "$IGNORE_FILE_CHECK" ]
		then
			echo "${1}"
		else
			ERROR ${1}: ${the_file} not found.
			exit 1
		fi
	fi
fi  
}

CHECK_IF_SET(){
sanity_check=0
[ $CREATE_PIPELINE ] &&  hrule
for should_be_set in $@
do
	eval value=\$${should_be_set}
	if [ -z $value ]
	then
		sanity_check=1
		ERROR "$should_be_set is not set"
	else
		if [ "$CREATE_PIPELINE" ]
		then
			cline $should_be_set=$value
		else 
			INFO $should_be_set=$value
		fi
	fi
	unset -v value
done
[ $CREATE_PIPELINE ] && hrule
return $sanity_check
}



##############################################################################
# Parse input variable                                                       #
##############################################################################
SHIFTED_ARGS=
if [ -f "$1" ]
then
	INFO "Importing settings from $1"
	source $1
	SHIFTED_ARGS="$SHIFTED_ARGS $1"
	shift
fi

for var in "$@"
do
	if [[ $var == *=* ]];
	then
		IFS='=' read -r ARG_KEY ARG_VALUE <<< "$var"
		
		if [ "${ARG_VALUE:0:1}" == "$" ]
		then
			TO_EVAL="$( tr '[:lower:]' '[:upper:]'<<<$ARG_KEY )=\\$ARG_VALUE"
		else
			TO_EVAL="$( tr '[:lower:]' '[:upper:]'<<<$ARG_KEY )=$ARG_VALUE"
		fi
		eval "$TO_EVAL"
	else 
		WARNING "Argument $var can not be parsed."
	fi
done

[ $OUTPUT_DIR ] || OUTPUT_DIR=.
[ "${OUTPUT_DIR:0:1}" != "$" ] && OUTPUT_DIR=${OUTPUT_DIR%/}/

if  [ "${CASCADE_DATA}" ] && [ -d "${CASCADE_DATA}" ] 
then
	CASCADE_DATA=${CASCADE_DATA%/}
else
	ERROR CASCADE_DATA not set. 
	exit 1		
fi

if  [ "${CASCADE_BIN}" ] && [ -d "${CASCADE_BIN}" ] 
then
	CASCADE_BIN=${CASCADE_BIN%/}/
else
	ERROR CASCADE_DATA not set. 
	exit 1		
fi


[ $STANDARD_IMAGE ] || STANDARD_IMAGE=${CASCADE_DATA%/}/MNI.nii.gz

##############################################################################
# Utility functions                                                          #
##############################################################################

hrule(){
printf "#%.0s" {1..80};
printf "\n";
}
cline()
{
text="# ""$@"
printf "$text"
num_spaces=$((79 - ${#text}))
printf "%-${num_spaces}s#\n"
}
comment()
{
hrule
cline $@
hrule
}

EXEC()
{
	echo $@
	if [ -z "$CREATE_PIPELINE" ];
	then
		$@;
		return $?
	else
		return 0
	fi
}

SCRIPT_HEADING()
{
	echo "#!/bin/bash -x"
	echo
	
	hrule
	cline "The CASCADE Pileline"
	cline "http://ki.se/en/nvs/cascade"
	cline "Copyright 2013-2015 Soheil Damangir"
	hrule
	
	echo
}
##############################################################################
# Dummy template for registration                                            #
##############################################################################
if ! type -t REGISTER >/dev/null
then
	REGISTER(){
	ERROR Register function not implemented. Template:\
		REGISTER moving fixed moved transform [nn for nearest neighbor] 
}
fi

if ! type -t REGISTER_VECTOR >/dev/null
then
	REGISTER_VECTOR(){
	ERROR Vector register function not implemented. Template:\
		REGISTER_VECTOR moving fixed moved transform [nn for nearest neighbor] 
}
fi
