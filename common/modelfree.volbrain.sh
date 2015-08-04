#!/bin/bash -ex

################################################################################
# Begin: Main inputs                                                           #
################################################################################
# Remove the # mark before each assignment if you want to hand write them.     #
################################################################################
# RADIUS=1.5
# VOLBRAIN_DIR=
# FLAIR=
# OUTPUT_DIR=
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

FSLPRE=fsl5.0-

RADIUS=${RADIUS:-1.5}

mkdir -p ${OUTPUT_DIR}

################################################################################
# VOLBRAIN_DIR=${VOLBRAIN_DIR}                                                 #
# OUTPUT_DIR=${OUTPUT_DIR}/                                                    #
# CASCADE_DATA=${CASCADE_DATA}                                                 #
# WHITE_MATTER_LABEL=3                                                         #
# OUTPUT=${OUTPUT_DIR}/model.free.pvalue.nii.gz                                #
# RADIUS=1                                                                     #
# REFERENCE=FLAIR                                                              #
################################################################################
# FLAIR=${FLAIR}                                                               #
# T1=${OUTPUT_DIR}/volbrain.t1.nii.gz                                          #
################################################################################


################################################################################
# Begin: Import volBrain results                                               #
################################################################################
# VOLBRAIN_DIR=${VOLBRAIN_DIR}                                                 #
# CASCADE_DATA=${CASCADE_DATA}                                                 #
# CASCADE_BIN=${CASCADE_BIN}/                                                  #
# OUTPUT_DIR=${OUTPUT_DIR}/                                                    #
# BTS_MAP=brain.tissues.nii.gz                                                 #
# VOLBRAIN_T1_NII=volbrain.t1.nii.gz                                           #
################################################################################
"${CASCADE_BIN}/ImportImage" "${VOLBRAIN_DIR}/native_n_mmni"* "${OUTPUT_DIR}/volbrain.t1.nii.gz"
"${CASCADE_BIN}/ImportImage" "${VOLBRAIN_DIR}/native_crisp_mmni"* "${OUTPUT_DIR}/brain.tissues.nii.gz"
"${CASCADE_BIN}/ImportImage" "${VOLBRAIN_DIR}/native_hemi_n_mmni"* "${OUTPUT_DIR}/parts.nii.gz"
################################################################################
# End: Import volBrain results                                                 #
################################################################################


################################################################################
# Begin: Preprocessing pipeline                                                #
################################################################################
# REFERENCE=FLAIR                                                              #
# BRAIN_TISSUE_SEG=${OUTPUT_DIR}/brain.tissues.nii.gz                          #
# BTS_REF=T1                                                                   #
# OUTPUT_DIR=${OUTPUT_DIR}/                                                    #
################################################################################
# FLAIR=${FLAIR}                                                               #
# T1=${OUTPUT_DIR}/volbrain.t1.nii.gz                                          #
################################################################################


################################################################################
# Begin: Coregister input images                                               #
################################################################################
# REFERENCE=${FLAIR}                                                           #
# OUTPUT_DIR=${OUTPUT_DIR}/                                                    #
# BTS_REF=T1                                                                   #
# BRAIN_TISSUE_SEG=${OUTPUT_DIR}/brain.tissues.nii.gz                          #
################################################################################
"${CASCADE_BIN}/linRegister" "${FLAIR}" "${FLAIR}" "${OUTPUT_DIR}/flair.to.ref.tfm" "${OUTPUT_DIR}/flair.from.ref.tfm"
"${CASCADE_BIN}/resample" "${FLAIR}" "${FLAIR}" "${OUTPUT_DIR}/flair.coreg.nii.gz" "${OUTPUT_DIR}/flair.to.ref.tfm"
"${CASCADE_BIN}/linRegister" "${FLAIR}" "${OUTPUT_DIR}/volbrain.t1.nii.gz" "${OUTPUT_DIR}/t1.to.ref.tfm" "${OUTPUT_DIR}/t1.from.ref.tfm"
"${CASCADE_BIN}/resample" "${FLAIR}" "${OUTPUT_DIR}/volbrain.t1.nii.gz" "${OUTPUT_DIR}/t1.coreg.nii.gz" "${OUTPUT_DIR}/t1.to.ref.tfm"
"${CASCADE_BIN}/resample" "${FLAIR}" "${OUTPUT_DIR}/brain.tissues.nii.gz" "${OUTPUT_DIR}/brain.tissues.coreg.nii.gz" "${OUTPUT_DIR}/t1.to.ref.tfm" "nn"
"${CASCADE_BIN}/resample" "${FLAIR}" "${OUTPUT_DIR}/parts.nii.gz" "${OUTPUT_DIR}/parts.coreg.nii.gz" "${OUTPUT_DIR}/t1.to.ref.tfm" "nn"
################################################################################
# End: Coregister input images                                                 #
################################################################################


################################################################################
# Begin: Create WM mask                                                        #
################################################################################
"${FSLPRE}fslmaths" "${OUTPUT_DIR}/brain.tissues.coreg.nii.gz" "-thr" "3" "-bin" "${OUTPUT_DIR}/wm_mask.nii.gz"
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
# OUTPUT=${OUTPUT_DIR}/model.free.pvalue.nii.gz                                #
# RADIUS=1                                                                     #
# OUTPUT_DIR=${OUTPUT_DIR}/                                                    #
################################################################################
# FLAIR=${OUTPUT_DIR}/flair.normalized.nii.gz                                  #
# T1=${OUTPUT_DIR}/t1.normalized.nii.gz                                        #
################################################################################


################################################################################
# Begin: Create WM mask                                                        #
################################################################################
"${FSLPRE}fslmaths" "${OUTPUT_DIR}/brain.tissues.coreg.nii.gz" "-thr" "2" "-bin" "${OUTPUT_DIR}/wg_mask.nii.gz"
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
"${CASCADE_BIN}/EvidentNormal" "${OUTPUT_DIR}/flair.normalized.nii.gz" "${OUTPUT_DIR}/brain.tissues.coreg.nii.gz" "${OUTPUT_DIR}/flair.evident.nii.gz" "0.5" "3"
"${CASCADE_BIN}/EvidentNormal" "${OUTPUT_DIR}/t1.normalized.nii.gz" "${OUTPUT_DIR}/brain.tissues.coreg.nii.gz" "${OUTPUT_DIR}/t1.evident.nii.gz" "-0.90" "3"
"${FSLPRE}fslmaths" "${OUTPUT_DIR}/flair.evident.nii.gz" "-bin" "-add" "${OUTPUT_DIR}/t1.evident.nii.gz" "-bin" "-mul" "-1" "-add" "1" "${OUTPUT_DIR}/evident_mask.nii.gz"
"${FSLPRE}fslmaths" "${OUTPUT_DIR}/brain.tissues.coreg.nii.gz" "-thr" "3" "-bin" "-mul" "${OUTPUT_DIR}/evident_mask.nii.gz" "${OUTPUT_DIR}/evident_mask.nii.gz"
################################################################################
# End: Remove enidently non-WMC                                                #
################################################################################


################################################################################
# Begin: Model free segmentation                                               #
################################################################################
# INCLUSION=${OUTPUT_DIR}/wg_mask.nii.gz                                       #
# POSSIBLE_MASK=${OUTPUT_DIR}/evident_mask.nii.gz                              #
# OUTPUT=${OUTPUT_DIR}/model.free.pvalue.nii.gz                                #
# RADIUS=1                                                                     #
################################################################################

"${FSLPRE}fslmaths" "${OUTPUT_DIR}/wg_mask.nii.gz" -mul 0 "${OUTPUT_DIR}/t1.modelfree.pval.nii.gz"
cp "${OUTPUT_DIR}/t1.modelfree.pval.nii.gz" "${OUTPUT_DIR}/flair.modelfree.pval.nii.gz"

for lbl in 1 2 3 4 5
do
"${FSLPRE}fslmaths" "${OUTPUT_DIR}/parts.coreg.nii.gz" -thr $lbl -uthr $lbl -mul "${OUTPUT_DIR}/wg_mask.nii.gz" "${OUTPUT_DIR}/train_mask.nii.gz"
"${FSLPRE}fslmaths" "${OUTPUT_DIR}/parts.coreg.nii.gz" -thr $lbl -uthr $lbl -mul "${OUTPUT_DIR}/evident_mask.nii.gz" "${OUTPUT_DIR}/test_mask.nii.gz"
"${CASCADE_BIN}/StatisticTest" "--input" "${OUTPUT_DIR}/flair.normalized.nii.gz" --output "${OUTPUT_DIR}/flair.modelfree.p${lbl}.pval.nii.gz" --radius ${RADIUS} --test "${OUTPUT_DIR}/test_mask.nii.gz" --train "${OUTPUT_DIR}/train_mask.nii.gz" --type AP --direction pos

"${CASCADE_BIN}/StatisticTest" "--input" "${OUTPUT_DIR}/t1.normalized.nii.gz" --output "${OUTPUT_DIR}/t1.modelfree.p${lbl}.pval.nii.gz" --radius ${RADIUS} --test "${OUTPUT_DIR}/test_mask.nii.gz" --train "${OUTPUT_DIR}/train_mask.nii.gz" --type AP --direction neg

"${FSLPRE}fslmaths" "${OUTPUT_DIR}/t1.modelfree.p${lbl}.pval.nii.gz" -add "${OUTPUT_DIR}/t1.modelfree.pval.nii.gz" "${OUTPUT_DIR}/t1.modelfree.pval.nii.gz"
"${FSLPRE}fslmaths" "${OUTPUT_DIR}/flair.modelfree.p${lbl}.pval.nii.gz" -add "${OUTPUT_DIR}/flair.modelfree.pval.nii.gz" "${OUTPUT_DIR}/flair.modelfree.pval.nii.gz"

rm -f "${OUTPUT_DIR}/test_mask.nii.gz" "${OUTPUT_DIR}/train_mask.nii.gz"

done

"${FSLPRE}fslmaths" "${OUTPUT_DIR}/flair.modelfree.pval.nii.gz" "-max" "${OUTPUT_DIR}/t1.modelfree.pval.nii.gz" "${OUTPUT_DIR}/wml.pvalue.nii.gz"

################################################################################
# End: Model free segmentation                                                 #
################################################################################
################################################################################
# End: Model free segmentation pipeline                                        #
################################################################################

