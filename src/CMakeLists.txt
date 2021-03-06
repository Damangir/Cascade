cmake_minimum_required(VERSION 2.8)

PROJECT(Cascade)

set (Cascade_VERSION_MAJOR 1)
set (Cascade_VERSION_MINOR 0)

SET(CMAKE_BUILD_TYPE DEBUG)

FIND_PACKAGE(ITK 4.3 REQUIRED)
IF(ITK_FOUND)
	INCLUDE(${ITK_USE_FILE})
ELSE(ITK_FOUND)
	MESSAGE(FATAL_ERROR"ITK 4.3+ not found. Please set ITK_DIR.")
ENDIF(ITK_FOUND)

# WML separation
add_executable(EvidentNormal EvidentNormal.cxx)
target_link_libraries(EvidentNormal ${ITK_LIBRARIES})

add_executable(TwoSampleKolmogorovSmirnovTest TwoSampleKolmogorovSmirnovTest.cxx)
target_link_libraries(TwoSampleKolmogorovSmirnovTest ${ITK_LIBRARIES})

add_executable(OneSampleKolmogorovSmirnovTest OneSampleKolmogorovSmirnovTest.cxx)
target_link_libraries(OneSampleKolmogorovSmirnovTest ${ITK_LIBRARIES})

add_executable(StatisticTest StatisticTest.cxx)
target_link_libraries(StatisticTest ${ITK_LIBRARIES})

install(TARGETS StatisticTest TwoSampleKolmogorovSmirnovTest OneSampleKolmogorovSmirnovTest EvidentNormal
        DESTINATION bin)


# Inhomogeneity correction
add_executable(inhomogeneity inhomogeneity.cxx)
target_link_libraries(inhomogeneity ${ITK_LIBRARIES})

install(TARGETS inhomogeneity
        DESTINATION bin)

# Brain extraction
add_executable(brainExtraction brainExtraction.cxx)
target_link_libraries(brainExtraction ${ITK_LIBRARIES})

install(TARGETS brainExtraction
        DESTINATION bin)

# Brain tissue segmentation tools
add_executable(TissueTypeSegmentation TissueTypeSegmentation.cxx)
target_link_libraries(TissueTypeSegmentation ${ITK_LIBRARIES})

add_executable(extractCSF extractCSF.cxx)
target_link_libraries(extractCSF ${ITK_LIBRARIES})

add_executable(separateWG separateWG.cxx)
target_link_libraries(separateWG ${ITK_LIBRARIES})

add_executable(refineBTS refineBTS.cxx)
target_link_libraries(refineBTS ${ITK_LIBRARIES})

add_executable(CorrectGrayMatterFalsePositive CorrectGrayMatterFalsePositive.cxx)
target_link_libraries(CorrectGrayMatterFalsePositive ${ITK_LIBRARIES})

install(TARGETS TissueTypeSegmentation extractCSF separateWG refineBTS CorrectGrayMatterFalsePositive
        DESTINATION bin)

# Reporting
add_executable(atlas atlas.cxx)
target_link_libraries(atlas ${ITK_LIBRARIES})

add_executable(ReportMap ReportMap.cxx)
target_link_libraries(ReportMap ${ITK_LIBRARIES})

add_executable(ReportFeature ReportFeature.cxx)
target_link_libraries(ReportFeature ${ITK_LIBRARIES})

add_executable(CleanMap CleanMap.cxx)
target_link_libraries(CleanMap ${ITK_LIBRARIES})

add_executable(Labeler Labeler.cxx)
target_link_libraries(Labeler ${ITK_LIBRARIES})

install(TARGETS Labeler CleanMap ReportFeature ReportMap atlas
        DESTINATION bin)

# Registration
add_executable(linRegister linRegister.cxx)
target_link_libraries(linRegister ${ITK_LIBRARIES})

add_executable(resample resample.cxx)
target_link_libraries(resample ${ITK_LIBRARIES})

add_executable(resampleVector resampleVector.cxx)
target_link_libraries(resampleVector ${ITK_LIBRARIES})

install(TARGETS linRegister resample resampleVector
        DESTINATION bin)

# Reading Images
add_executable(ImportImage ImportImage.cxx)
target_link_libraries(ImportImage ${ITK_LIBRARIES})

add_executable(DICOMReader DICOMReader.cxx)
target_link_libraries(DICOMReader ${ITK_LIBRARIES})

install(TARGETS DICOMReader ImportImage
        DESTINATION bin)

# Utilities
add_executable(LabelImageOpening LabelImageOpening.cxx)
target_link_libraries(LabelImageOpening ${ITK_LIBRARIES})

add_executable(confusion confusion.cxx)
target_link_libraries(confusion ${ITK_LIBRARIES})

add_executable(relabel relabel.cxx)
target_link_libraries(relabel ${ITK_LIBRARIES})

add_executable(ComposeToVector ComposeToVector.cxx)
target_link_libraries(ComposeToVector ${ITK_LIBRARIES})

add_executable(CollapseModel CollapseModel.cxx)
target_link_libraries(CollapseModel ${ITK_LIBRARIES})

install(TARGETS CollapseModel ComposeToVector relabel confusion LabelImageOpening
        DESTINATION bin)


# Install scripts        
install(DIRECTORY Shell
		DESTINATION bin
		PATTERN "*.sh"
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
                    GROUP_EXECUTE GROUP_READ
                    WORLD_EXECUTE WORLD_READ)

# install(DIRECTORY python
#		DESTINATION bin
#		PATTERN ".pyc" EXCLUDE
#		PATTERN "Cascade*.py"
#        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
#                    GROUP_EXECUTE GROUP_READ
#                    WORLD_EXECUTE WORLD_READ)
		
install(DIRECTORY ../data
		DESTINATION share )
