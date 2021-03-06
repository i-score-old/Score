cmake_minimum_required(VERSION 2.6)
project(Automation)

find_package(LibXml2)
if(NOT LIBXML2_FOUND)
  message(STATUS "Package libxml2 required, but not found! Won't build extension Automation.")
  return()
endif()

set(PROJECT_SRCS
${CMAKE_CURRENT_SOURCE_DIR}/../TimePluginLib.cpp
${CMAKE_CURRENT_SOURCE_DIR}/source/Automation.cpp
)
file(GLOB_RECURSE PROJECT_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/includes/*.h)

include_directories("${PROJECT_SOURCE_DIR}/")
include_directories("${PROJECT_SOURCE_DIR}/..")
include_directories("${PROJECT_SOURCE_DIR}/includes")
include_directories("${PROJECT_SOURCE_DIR}/../../library/includes")
include_directories("${PROJECT_SOURCE_DIR}/../../library/tests")
include_directories("${JAMOMA_MODULAR_INCLUDE_DIR}")
include_directories("${JAMOMA_MODULAR_PROTOCOLLIB_INCLUDE_DIR}")
include_directories("${JAMOMA_MODULAR_SCHEDULERLIB_INCLUDE_DIR}")
include_directories("${JAMOMA_MODULAR_PEEROBJECT_INCLUDE_DIR}")
include_directories("${JAMOMA_FOUNDATION_INCLUDE_DIR}")
include_directories("${JAMOMA_DSP_INCLUDE_DIR}")
include_directories("${JAMOMA_FOUNDATION_INCLUDE_DIR}/../libiconv/include")

include_directories(${LIBXML2_INCLUDE_DIR})
if(ANDROID)
include_directories("${CMAKE_FIND_ROOT_PATH}/include")
endif()

add_library(${PROJECT_NAME} SHARED ${PROJECT_SRCS} ${PROJECT_HDRS})

target_link_libraries(${PROJECT_NAME} ${JAMOMA_FOUNDATION_LIBRARY})
target_link_libraries(${PROJECT_NAME} ${JAMOMA_DSP_LIBRARY})
target_link_libraries(${PROJECT_NAME} ${JAMOMA_MODULAR_LIBRARY})
target_link_libraries(${PROJECT_NAME} JamomaScore)
target_link_libraries(${PROJECT_NAME} ${LIBXML2_LIBRARIES})

### Output ###
setOutput()

### Tests ###
addTestTarget()
