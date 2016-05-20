#!/bin/bash -ex

################################################################################
# Begin: Main inputs                                                           #
################################################################################
# Remove the # mark before each assignment if you want to hand write them.     #
################################################################################
# RADIUS=1.5
# FREESURFER_DIR=
# FLAIR=
# OUTPUT_DIR=
################################################################################
# End: Main inputs                                                             #
################################################################################


################################################################################
# Begin: Main parametric inputs                                                #
################################################################################
# You can create inputs based on other parameters, e.g. SUBJ_ID for simplicity.#
# NOTE: These values will be overwritten by explicit input defenition.         #
################################################################################
PRJDIR=${PRJDIR:-~}
OUTPUT_DEF=${PRJDIR}/Cascade/${SUBJ_ID}
FS_DEF=${PRJDIR}/Freesurfer/${SUBJ_ID}
FLAIR_DEF=${PRJDIR}/Original/${SUBJ_ID}/flair.nii.gz
################################################################################
# End: Main inputs                                                             #
################################################################################


################################################################################
# Begin: CASCADE INSTALLATION                                                  #
################################################################################
CASCADE_BIN=${CASCADE_BIN:-"/opt/cascade/1.0.1/bin"}
CASCADE_DATA=${CASCADE_DATA:-"/opt/cascade/1.0.1/share/data"}
################################################################################
# End: CASCADE INSTALLATION                                                    #
# Please do not touch beyond this line                                         #
################################################################################

RADIUS=${RADIUS:-1.5}
FREESURFER_DIR=${FREESURFER_DIR:-$FS_DEF}
FLAIR=${FLAIR:-$FLAIR_DEF}
OUTPUT_DIR=${OUTPUT_DIR:-$OUTPUT_DEF}

FS_ASEG=${FREESURFER_DIR}/mri/aseg.mgz
FS_RAWAVG=${FREESURFER_DIR}/mri/rawavg.mgz

mkdir -p ${OUTPUT_DIR}

################################################################################
# Begin: Import Freesurfer results                                             #
################################################################################

if command -v mri_convert &>/dev/null
then
mri_convert "${FS_RAWAVG}" "${OUTPUT_DIR}/rawavg.nii.gz"
mri_convert "${FS_ASEG}" "${OUTPUT_DIR}/aseg.nii.gz"
else
"${CASCADE_BIN}/ImportImage" "${FS_RAWAVG}" "${OUTPUT_DIR}/rawavg.nii.gz"
"${CASCADE_BIN}/ImportImage" "${FS_ASEG}" "${OUTPUT_DIR}/aseg.nii.gz"
fi

"${CASCADE_BIN}/resample" "${OUTPUT_DIR}/rawavg.nii.gz" "${OUTPUT_DIR}/aseg.nii.gz" "${OUTPUT_DIR}/aseg.nii.gz" "${CASCADE_DATA}/transform/unity.tfm" "nn"
"${CASCADE_BIN}/relabel" "${OUTPUT_DIR}/aseg.nii.gz" "${CASCADE_DATA}/map/FS_label.map.txt" "${OUTPUT_DIR}/brain.tissues.nii.gz"
################################################################################
# End: Import Freesurfer results                                               #
################################################################################


################################################################################
# Begin: Preprocessing pipeline                                                #
################################################################################


################################################################################
# Begin: Coregister input images                                               #
################################################################################

"${CASCADE_BIN}/linRegister" "${FLAIR}" "${FLAIR}" "${OUTPUT_DIR}/flair.to.ref.tfm" "${OUTPUT_DIR}/flair.from.ref.tfm"

"${CASCADE_BIN}/resample" "${FLAIR}" "${FLAIR}" "${OUTPUT_DIR}/flair.coreg.nii.gz" "${OUTPUT_DIR}/flair.to.ref.tfm"

"${CASCADE_BIN}/linRegister" "${FLAIR}" "${OUTPUT_DIR}/rawavg.nii.gz" "${OUTPUT_DIR}/t1.to.ref.tfm" "${OUTPUT_DIR}/t1.from.ref.tfm"

"${CASCADE_BIN}/resample" "${FLAIR}" "${OUTPUT_DIR}/rawavg.nii.gz" "${OUTPUT_DIR}/t1.coreg.nii.gz" "${OUTPUT_DIR}/t1.to.ref.tfm"

"${CASCADE_BIN}/resample" "${FLAIR}" "${OUTPUT_DIR}/brain.tissues.nii.gz" "${OUTPUT_DIR}/brain.tissues.coreg.nii.gz" "${OUTPUT_DIR}/t1.to.ref.tfm" "nn"
################################################################################
# End: Coregister input images                                                 #
################################################################################


################################################################################
# Begin: Create WM mask                                                        #
################################################################################
"fslmaths" "${OUTPUT_DIR}/brain.tissues.coreg.nii.gz" "-thr" "3" "-bin" "${OUTPUT_DIR}/wm_mask.nii.gz"
################################################################################
# End: Create WM mask                                                          #
################################################################################


################################################################################
# Begin: Normalize input images                                                #
################################################################################
# NORMALIZE_MASK=${OUTPUT_DIR}/wm_mask.nii.gz                                  #
################################################################################
"${CASCADE_BIN}/inhomogeneity" "${OUTPUT_DIR}/flair.coreg.nii.gz" "${OUTPUT_DIR}/wm_mask.nii.gz" "${OUTPUT_DIR}/flair.normalized.nii.gz"
"${CASCADE_BIN}/inhomogeneity" "${OUTPUT_DIR}/t1.coreg.nii.gz" "${OUTPUT_DIR}/wm_mask.nii.gz" "${OUTPUT_DIR}/t1.normalized.nii.gz"
################################################################################
# End: Normalize input images                                                  #
################################################################################
################################################################################
# End: Preprocessing pipeline                                                  #
################################################################################


################################################################################
# Begin: Model free segmentation pipeline                                      #
################################################################################
# BRAIN_TISSUE_SEG=${OUTPUT_DIR}/brain.tissues.coreg.nii.gz                    #
# WHITE_MATTER_LABEL=3                                                         #
# OUTPUT=wml.pvalue.nii.gz                                                     #
# RADIUS=1                                                                     #
# OUTPUT_DIR=${OUTPUT_DIR}/                                                    #
################################################################################
# FLAIR=${OUTPUT_DIR}/flair.normalized.nii.gz                                  #
# T1=${OUTPUT_DIR}/t1.normalized.nii.gz                                        #
################################################################################


################################################################################
# Begin: Create WM mask                                                        #
################################################################################
"fslmaths" "${OUTPUT_DIR}/brain.tissues.coreg.nii.gz" "-thr" "3" "-bin" "${OUTPUT_DIR}/wm_mask.nii.gz"
"fslmaths" "${OUTPUT_DIR}/brain.tissues.coreg.nii.gz" "-thr" "2" "-bin" "${OUTPUT_DIR}/wg_mask.nii.gz"
"fslmaths" "${OUTPUT_DIR}/brain.tissues.coreg.nii.gz" "-thr" "1" "-bin" "${OUTPUT_DIR}/brain_mask.nii.gz"
################################################################################
# End: Create WM mask                                                          #
################################################################################

################################################################################
# Begin: Remove enidently non-WMC                                              #
################################################################################
# BRAIN_TISSUE_SEG=${OUTPUT_DIR}/brain.tissues.coreg.nii.gz                    #
# WHITE_MATTER_LABEL=3                                                         #
# OUTPUT=${OUTPUT_DIR}/evident_mask.nii.gz                                     #
################################################################################
if [ -z "${NO_EVIDENT}" ]
then
"${CASCADE_BIN}/EvidentNormal" "${OUTPUT_DIR}/flair.normalized.nii.gz" "${OUTPUT_DIR}/brain.tissues.coreg.nii.gz" "${OUTPUT_DIR}/flair.evident.nii.gz" "0.5" "3"
"${CASCADE_BIN}/EvidentNormal" "${OUTPUT_DIR}/t1.normalized.nii.gz" "${OUTPUT_DIR}/brain.tissues.coreg.nii.gz" "${OUTPUT_DIR}/t1.evident.nii.gz" "-0.90" "3"
"fslmaths" "${OUTPUT_DIR}/flair.evident.nii.gz" "-bin" "-add" "${OUTPUT_DIR}/t1.evident.nii.gz" "-bin" "-mul" "-1" "-add" "1" "${OUTPUT_DIR}/evident_mask.nii.gz"
"fslmaths" "${OUTPUT_DIR}/brain.tissues.coreg.nii.gz" "-thr" "3" "-bin" "-mul" "${OUTPUT_DIR}/evident_mask.nii.gz" "${OUTPUT_DIR}/evident_mask.nii.gz"
else
"fslmaths" "${OUTPUT_DIR}/brain.tissues.coreg.nii.gz" "-thr" "3" "-bin" "${OUTPUT_DIR}/evident_mask.nii.gz"
fi
################################################################################
# End: Remove enidently non-WMC                                                #
################################################################################


################################################################################
# Begin: Model free segmentation                                               #
################################################################################
# INCLUSION=${OUTPUT_DIR}/wm_mask.nii.gz                                       #
# POSSIBLE_MASK=${OUTPUT_DIR}/evident_mask.nii.gz                              #
# OUTPUT=wml.pvalue.nii.gz                                                     #
# RADIUS=1                                                                     #
################################################################################
"${CASCADE_BIN}/StatisticTest" "--input" "${OUTPUT_DIR}/flair.normalized.nii.gz" --output "${OUTPUT_DIR}/flair.modelfree.pval.nii.gz" --radius ${RADIUS} --test "${OUTPUT_DIR}/evident_mask.nii.gz" --train "${OUTPUT_DIR}/wg_mask.nii.gz" --type AP --direction pos

"${CASCADE_BIN}/StatisticTest" "--input" "${OUTPUT_DIR}/t1.normalized.nii.gz" --output "${OUTPUT_DIR}/t1.modelfree.pval.nii.gz" --radius ${RADIUS} --test "${OUTPUT_DIR}/evident_mask.nii.gz" --train "${OUTPUT_DIR}/wg_mask.nii.gz" --type AP --direction neg

"fslmaths" "${OUTPUT_DIR}/flair.modelfree.pval.nii.gz" "-max" "${OUTPUT_DIR}/t1.modelfree.pval.nii.gz" "${OUTPUT_DIR}/wml.pvalue.nii.gz"
################################################################################
# End: Model free segmentation                                                 #
################################################################################
################################################################################
# End: Model free segmentation pipeline                                        #
################################################################################
