#!/bin/bash -ex

################################################################################
# Begin: Main inputs                                                           #
################################################################################
# Remove the # mark before assignment if you want to hand write them.          #
################################################################################
MINT=${MINT:-0.8}   # Minimum p-value for a detection
MAXT=${MAXT:-0.9}   # Minimum p-value at the peak for each detection
MINS=${MINS:-27}    # Minimum detection size (mm^3)
WMLP=${WMLP:-75}    # If a gray matter is greater WMLP percent of WML is will be considered as WML
DBLCHECK=${DBLCHECK:-wg_mask.nii.gz}
COVERAGE_RAD=${COVERAGE_RAD:-1}

# SUBJECT_DIR
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
SUBJECT_DEF=${PRJDIR}/${SUBJ_ID}
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

OUTPUT_DIR=${SUBJECT_DIR:-$SUBJECT_DEF}
SUBJ_ID=${SUBJ_ID:-$(basename $( cd $OUTPUT_DIR && pwd))}

FLAIR="${OUTPUT_DIR}/flair.normalized.nii.gz"
T1="${OUTPUT_DIR}/t1.normalized.nii.gz"
BTS="${OUTPUT_DIR}/brain.tissues.coreg.nii.gz"
EVIDENT="${OUTPUT_DIR}/${DBLCHECK}"

PVALUE="${OUTPUT_DIR}/flair.modelfree.pval.nii.gz"
BIN_IMG="${OUTPUT_DIR}/wml_mask_${MINT}_${MAXT}_${MINS}mm3_${WMLP}p.nii.gz"

PVALUE1="${OUTPUT_DIR}/flair.modelfree.pval.1.nii.gz"
TOUCH_MASK="${OUTPUT_DIR}/touch_mask.nii.gz"
BIN_IMG_TRIM="${OUTPUT_DIR}/wml_mask_${MINT}_${MAXT}_${MINS}mm3_${WMLP}p_${COVERAGE}cp_${COVERAGE_RAD}mm.nii.gz"


if [ "${USER_THRESHOLD}" ]
then
"${FSLPRE}fslmaths" "${FLAIR}" -thr "${USER_THRESHOLD}" -bin -mul "${PVALUE}" "${PVALUE1}"
else
cp "${PVALUE}" "${PVALUE1}"
fi

"${CASCADE_BIN}/ReportMap" --min-size ${MINS} --threshold ${MINT} --max-threshold ${MAXT} --binary-seg "${BIN_IMG}" "${PVALUE1}" &>/dev/null

THR=$("${FSLPRE}fslstats" "${FLAIR}" -k "${BIN_IMG}" -P ${WMLP})

if (( $(bc -l <<< "$THR > 0 ") ))
then
"${FSLPRE}fslmaths" "${FLAIR}" -thr ${THR} -mul "${EVIDENT}" -bin -max "${PVALUE1}" "${PVALUE1}"
VOLS=$("${CASCADE_BIN}/ReportMap" --min-size ${MINS} --threshold ${MINT} --max-threshold ${MAXT} --binary-seg "${BIN_IMG}" "${PVALUE1}")
else
"${FSLPRE}fslmaths" ${BIN_IMG} -mul 0 ${BIN_IMG}
VOLS=0,0
fi


if [ "${COVERAGE}" ]
then
"${FSLPRE}fslmaths" ${BTS} -thr 2 -uthr 2 -bin ${TOUCH_MASK}
"${FSLPRE}fslmaths" ${BTS} -binv -add ${TOUCH_MASK} ${TOUCH_MASK}
VOLS=$("${CASCADE_BIN}/CleanMap" --input-seg ${BIN_IMG} --mask  ${TOUCH_MASK} --radius ${COVERAGE_RAD} --overlap-thresh  ${COVERAGE} --output-seg  ${BIN_IMG_TRIM} --verbose )
fi

echo ${SUBJ_ID},${VOLS}
