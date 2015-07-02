##############################################################################
#                                                                            #
# Filename     : cascade.config.sh                                           #
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
FS_ASEG_NII=aseg.nii.gz

VOLBRAIN_T1_NII=volbrain.t1.nii.gz

for IMG in T1 FLAIR T2 PD
do
IMG_U=$( tr '[:lower:]' '[:upper:]' <<<$IMG )
IMG_L=$( tr '[:upper:]' '[:lower:]' <<<$IMG )
##############################################################################
# Coregistration                                                             #
##############################################################################
eval ${IMG_U}_REGISTERED=${IMG_L}.coreg.nii.gz
eval ${IMG_U}_TO_REF=${IMG_L}.to.ref.tfm
eval ${IMG_U}_FROM_REF=${IMG_L}.from.ref.tfm

##############################################################################
# Normalizing input images                                                   #
##############################################################################
eval ${IMG_U}_NORM=${IMG_L}.normalized.nii.gz
##############################################################################
# Remove evidently normal brain tissues                                      #
##############################################################################
eval ${IMG_U}_EVID=${IMG_L}.evident.nii.gz
##############################################################################
# CASCADE Model free segmentation                                            #
##############################################################################
eval ${IMG_U}_PVAL=${IMG_L}.modelfree.pval.nii.gz
##############################################################################
# CASCADE Pre-Train model                                                    #
##############################################################################
eval ${IMG_U}_STD=${IMG_L}.standard.nii.gz
##############################################################################
# CASCADE definition segmentation                                            #
##############################################################################
eval ${IMG_U}_MODEL=${IMG_L}.model.nii.gz
eval ${IMG_U}_MODEL_NATIVE=${IMG_L}.model.native.nii.gz	

##############################################################################
# CASCADE Extra                                                              #
##############################################################################
eval ${IMG_U}_TO_MNI=${IMG_L}.to.mni.tfm
eval ${IMG_U}_TO_MNI_L=${IMG_L}.to.mni.lin.tfm
eval ${IMG_U}_TO_MNI_NL=${IMG_L}.to.mni.nl.nii.gz

eval MNI_TO_${IMG_U}=mni.to.${IMG_L}.tfm
eval MNI_TO_${IMG_U}_L=mni.to.${IMG_L}.lin.tfm
eval MNI_TO_${IMG_U}_NL=mni.to.${IMG_L}.nl.nii.gz

eval BTS_IN_${IMG_U}=brain.tissues.${IMG_L}.nii.gz																																																																																																																																																																																																																																																																																																																																																																																																																																																																								
done

##############################################################################
# Coregistration                                                             #
##############################################################################

IMG_TO_MNI=img.to.mni.tfm
IMG_TO_MNI_L=img.to.mni.lin.tfm
IMG_TO_MNI_NL=img.to.mni.nl.nii.gz

MNI_TO_IMG=mni.to.img.tfm
MNI_TO_IMG_L=mni.to.img.lin.tfm
MNI_TO_IMG_NL=mni.to.img.nl.nii.gz
BTS_REGISTERED=brain.tissues.coreg.nii.gz

##############################################################################
# Remove evidently normal brain tissues                                      #
##############################################################################

T1_PER=-0.90
FLAIR_PER=0.5
T2_PER=0.3
PD_PER=0.1

##############################################################################
# CASCADE Model free segmentation                                            #
##############################################################################

RADIUS=1

T1_DIR=neg
FLAIR_DIR=pos
T2_DIR=pos
PD_DIR=pos

##############################################################################
# CASCADE Pre-Train model                                                    #
##############################################################################

BTS_STD=brain.tissue.standard.nii.gz
TRAIN_MASK_STD=train.mask.standard.nii.gz


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
[ -z "$SILENT_CHECK" ] && [ $CREATE_PIPELINE ] && hrule
for should_be_set in $@
do
	eval value=\$${should_be_set}
	if [ -z "$value" ]
	then
		sanity_check=1
		if [ -z "$SILENT_CHECK" ]
		then
			ERROR "$should_be_set is not set"
		fi
	else
		if [ -z "$SILENT_CHECK" ]
		then		
			if [ "$CREATE_PIPELINE" ]
			then
				cline $should_be_set=$value
			else 
				INFO $should_be_set=$value
			fi
		fi
	fi
	unset -v value
done
[ -z "$SILENT_CHECK" ] && [ $CREATE_PIPELINE ] && hrule
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
	SHIFTED_ARGS=$SHIFTED_ARGS "$1"
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
			TO_EVAL="$( tr '[:lower:]' '[:upper:]'<<<$ARG_KEY )='$ARG_VALUE'"
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
	ERROR CASCADE_BIN not set. 
	exit 1		
fi

[ "$PRIOR_CSF_MNI" ] || PRIOR_CSF_MNI=${CASCADE_DATA}/std/avg152T1_csf.nii.gz
[ "$PRIOR_GM_MNI" ] || PRIOR_GM_MNI=${CASCADE_DATA}/std/avg152T1_gray.nii.gz
[ "$PRIOR_WM_MNI" ] || PRIOR_WM_MNI=${CASCADE_DATA}/std/avg152T1_white.nii.gz

[ "$MNI_ALL" ] || MNI_ALL=${CASCADE_DATA}/std/MNI152_T1_2mm.nii.gz
[ "$MNI_BRAIN" ] || MNI_BRAIN=${CASCADE_DATA}/std/MNI152_T1_2mm_brain.nii.gz
[ "$MNI_BRAIN_MASK" ] || MNI_BRAIN_MASK=${CASCADE_DATA}/std/MNI152_T1_2mm_brain_mask.nii.gz

[ $STANDARD_IMAGE ] || STANDARD_IMAGE=$MNI_ALL

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
	CMD=$(printf '"%s" ' "$@")
	echo $CMD
	if [ -z "$CREATE_PIPELINE" ];
	then
		"$@";
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
