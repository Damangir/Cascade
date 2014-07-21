cmake_minimum_required(VERSION 2.8)

PROJECT(CascadeUtil)

SET(CMAKE_BUILD_TYPE DEBUG)

FIND_PACKAGE(ITK REQUIRED)
IF(ITK_FOUND)
	INCLUDE(${ITK_USE_FILE})
ELSE(ITK_FOUND)
	MESSAGE(FATAL_ERROR"ITK not found. Please set ITK_DIR.")
ENDIF(ITK_FOUND)

add_executable(linRegister linRegister.cxx)
target_link_libraries(linRegister ${ITK_LIBRARIES})

add_executable(resample resample.cxx)
target_link_libraries(resample ${ITK_LIBRARIES})

add_executable(resampleVector resampleVector.cxx)
target_link_libraries(resampleVector ${ITK_LIBRARIES})

add_executable(inhomogeneity inhomogeneity.cxx)
target_link_libraries(inhomogeneity ${ITK_LIBRARIES})

add_executable(brainExtraction brainExtraction.cxx)
target_link_libraries(brainExtraction ${ITK_LIBRARIES})

add_executable(extractCSF extractCSF.cxx)
target_link_libraries(extractCSF ${ITK_LIBRARIES})

add_executable(separateWG separateWG.cxx)
target_link_libraries(separateWG ${ITK_LIBRARIES})

add_executable(refineBTS refineBTS.cxx)
target_link_libraries(refineBTS ${ITK_LIBRARIES})

add_executable(modelFree modelFree.cxx)
target_link_libraries(modelFree ${ITK_LIBRARIES})

add_executable(localFeature localFeature.cxx)
target_link_libraries(localFeature ${ITK_LIBRARIES})

add_executable(combine combine.cxx)
target_link_libraries(combine ${ITK_LIBRARIES})

add_executable(ks ks.cxx)
target_link_libraries(ks ${ITK_LIBRARIES})
