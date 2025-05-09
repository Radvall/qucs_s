cmake_minimum_required(VERSION 3.10)
PROJECT(qucs-suite CXX)

set(QUCS_NAME "qucs-s")

# Prohibit in-source builds
if("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
  message(
    FATAL_ERROR
      "QUCS-S: You cannot build in a source directory (or any directory with "
      "CMakeLists.txt file). Please make a build subdirectory. Feel free to "
      "remove CMakeCache.txt and CMakeFiles.")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file (STRINGS "${qucs-suite_SOURCE_DIR}/VERSION" QUCS_VERSION)

if(DEFINED CI_VERSION)
    set(PROJECT_VERSION "${CI_VERSION}")
else()
    set(PROJECT_VERSION "${QUCS_VERSION}")
endif()

message(STATUS "Configuring Qucs: VERSION ${PROJECT_VERSION}")

set(GIT "")
if(EXISTS ${CMAKE_SOURCE_DIR}/.git )
  find_package(Git)
  # Get the latest abbreviated commit hash of the working branch
  execute_process(
    COMMAND ${GIT_EXECUTABLE} log --pretty=format:%h -n 1
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
  )
  set(GIT ${GIT_COMMIT_HASH})
  message(STATUS "Found Git repository, last commit hash: ${GIT}")
endif()

message(STATUS "${PROJECT_NAME} ${CMAKE_INSTALL_PREFIX} ${qucs-suite_BINARY_DIR}" )

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets LinguistTools)
set(QT_VERSION ${Qt6Core_VERSION})

message(STATUS "Qt Version: " ${QT_VERSION})

add_definitions(${QT_DEFINITIONS})
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(QT_NO_DEBUG_OUTPUT)
endif()

enable_testing()

add_subdirectory( qucs )
#add_subdirectory( converter )
add_subdirectory( qucs-activefilter )
add_subdirectory( qucs-attenuator )
#add_subdirectory( qucs-doc )
add_subdirectory( qucs-filter )
add_subdirectory( library )
add_subdirectory( qucs-transcalc )
add_subdirectory( qucs-powercombining )
add_subdirectory( qucs-s-spar-viewer )
#add_subdirectory( examples )
if(EXISTS ${CMAKE_SOURCE_DIR}/qucsator_rf/CMakeLists.txt)
    add_subdirectory(qucsator_rf)
else()
    message("Qucsator RF submodule not found. Please update submodules")
endif()

IF (UPDATE_TRANSLATIONS)
  file(GLOB_RECURSE FILES_TO_TRANSLATE "*.cpp" "*.h" "*.ui")
ENDIF (UPDATE_TRANSLATIONS)
add_subdirectory( translations )

install(DIRECTORY "examples" DESTINATION "share/${QUCS_NAME}")

install(FILES contrib/io.github.ra3xdh.qucs_s.metainfo.xml DESTINATION ${CMAKE_INSTALL_PREFIX}/share/metainfo)

#
# Custom uninstall target
#
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake)


