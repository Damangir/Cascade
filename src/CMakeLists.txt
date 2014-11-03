cmake_minimum_required(VERSION 2.8)

PROJECT(CascadeAcademic)

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

# Inhomogeneity correction
add_executable(inhomogeneity inhomogeneity.cxx)
target_link_libraries(inhomogeneity ${ITK_LIBRARIES})

# Brain extraction
add_executable(brainExtraction brainExtraction.cxx)
target_link_libraries(brainExtraction ${ITK_LIBRARIES})

# Brain tissue segmentation tools
add_executable(TissueTypeSegmentation TissueTypeSegmentation.cxx)
target_link_libraries(TissueTypeSegmentation ${ITK_LIBRARIES})

add_executable(extractCSF extractCSF.cxx)
target_link_libraries(extractCSF ${ITK_LIBRARIES})

add_executable(separateWG separateWG.cxx)
target_link_libraries(separateWG ${ITK_LIBRARIES})

add_executable(refineBTS refineBTS.cxx)
target_link_libraries(refineBTS ${ITK_LIBRARIES})

# Reporting
add_executable(atlas atlas.cxx)
target_link_libraries(atlas ${ITK_LIBRARIES})

add_executable(ReportMap ReportMap.cxx)
target_link_libraries(ReportMap ${ITK_LIBRARIES})

# Utilities
add_executable(confusion confusion.cxx)
target_link_libraries(confusion ${ITK_LIBRARIES})

add_executable(relabel relabel.cxx)
target_link_libraries(relabel ${ITK_LIBRARIES})

add_executable(ComposeToVector ComposeToVector.cxx)
target_link_libraries(ComposeToVector ${ITK_LIBRARIES})

add_executable(linRegister linRegister.cxx)
target_link_libraries(linRegister ${ITK_LIBRARIES})

add_executable(resample resample.cxx)
target_link_libraries(resample ${ITK_LIBRARIES})

add_executable(resampleVector resampleVector.cxx)
target_link_libraries(resampleVector ${ITK_LIBRARIES})

# Future
add_executable(Shark shark.cxx)
target_link_libraries(Shark ${ITK_LIBRARIES})

