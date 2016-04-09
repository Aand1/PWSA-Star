cmake_minimum_required(VERSION 2.4.6)
PROJECT( sbpl )
set(CXX g++-4.9)
find_library(PASL_LIBRARY NAMES pasl PATHS /usr/local/bin)
set(CMAKE_SHARED_LINKER_FLAGS -fPIC)
# version (e.g. for packaging)
set(SBPL_MAJOR_VERSION 1)
set(SBPL_MINOR_VERSION 3)
set(SBPL_PATCH_VERSION 0)
set(SBPL_VERSION ${SBPL_MAJOR_VERSION}.${SBPL_MINOR_VERSION}.${SBPL_PATCH_VERSION})

set(CMAKE_BUILD_TYPE Release)
include_directories(src/include)

include_directories(${PROJECT_SOURCE_DIR}/code)
set(CMAKE_CXX_FLAGS "-fPIC -std=gnu++1y -pthread -DNDEBUG -fno-optimize-sibling-calls -DDISABLE_INTERRUPTS -DSTATS_IDLE -D_GNU_SOURCE -Wfatal-errors -m64 -DTARGET_X86_64 -DTARGET_LINUX -lm -DHAVE_GCC_TLS")
add_library(sbpl SHARED
            src/planners/PWSA/pwsaplanner.cpp
            src/planners/pARAStar/pARA.cpp
	    src/planners/ARAStar/araplanner.cpp
	    src/discrete_space_information/nav2d/environment_nav2D.cpp
            src/discrete_space_information/navxythetalat/environment_navxythetalat.cpp
            src/discrete_space_information/navxythetalat/environment_navxythetamlevlat.cpp
            src/discrete_space_information/nav2d_uu/environment_nav2Duu.cpp
            src/discrete_space_information/template/environment_XXX.cpp
            src/discrete_space_information/robarm/environment_robarm.cpp
            src/utils/heap.cpp
            src/utils/mdp.cpp
            src/utils/utils.cpp
            src/utils/2Dgridsearch.cpp
)
target_link_libraries(sbpl ${PASL_LIBRARY})
set(SBPL_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/include")
set(SBPL_LIB_DIR "${CMAKE_INSTALL_PREFIX}/lib")

# support for pkg-config
set(PKG_NAME "sbpl")
set(PKG_DESC "The Search Based Planning Library")
set(PKG_SBPL_LIBS "-lsbpl")
set(pkg_conf_file_in "${CMAKE_CURRENT_SOURCE_DIR}/sbpl.pc.in")
set(pkg_conf_file "${PROJECT_BINARY_DIR}/InstallFiles/sbpl.pc")
configure_file("${pkg_conf_file_in}" "${pkg_conf_file}" @ONLY)
install(FILES ${pkg_conf_file} DESTINATION lib/pkgconfig/ COMPONENT pkgconfig)

# support for cmake-config:
configure_file(sbpl-config.cmake.in
  "${PROJECT_BINARY_DIR}/InstallFiles/sbpl-config.cmake" @ONLY)
configure_file(sbpl-config-version.cmake.in
  "${PROJECT_BINARY_DIR}/InstallFiles/sbpl-config-version.cmake" @ONLY)
install(FILES
  "${PROJECT_BINARY_DIR}/InstallFiles/sbpl-config.cmake"
  "${PROJECT_BINARY_DIR}/InstallFiles/sbpl-config-version.cmake"
DESTINATION share/sbpl/)

# support for build tree export
option(SBPL_BUILD_TREE_EXPORT "Enable build tree exports" OFF)
if (${SBPL_BUILD_TREE_EXPORT})
    export(TARGETS sbpl FILE sbpl-targets.cmake)
    set(CONF_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/src")
    configure_file(
        sbpl-config-tree-export.cmake.in 
        "${PROJECT_BINARY_DIR}/sbpl-config.cmake" 
        @ONLY)
    export(PACKAGE sbpl)
endif()

install(DIRECTORY src/include/sbpl DESTINATION include
	FILES_MATCHING PATTERN "*.h")
install(TARGETS sbpl
	LIBRARY DESTINATION lib)


link_directories(/usr/local/lib)

#add_executable(test_sbpl src/test/main.cpp)
#target_link_libraries(test_sbpl sbpl)


#add_executable(test_para src/test/run.cpp)
#target_link_libraries(test_para sbpl pasl)
#SET(Boost_USE_STATIC_LIBS ON)
#find_package(Boost COMPONENTS thread regex system filesystem REQUIRED)
#find_library(LIBRT_LIBRARIES rt)
#target_link_libraries(test_para
#  ${Boost_FILESYSTEM_LIBRARY}
#  ${Boost_SYSTEM_LIBRARY}
#  ${Boost_THREAD_LIBRARY}
#  ${Boost_REGEX_LIBRARY}
#  ${LIBRT_LIBRARIES_RT}
#)

#add_executable(test_adjacency_list src/test/test_adjacency_list.cpp)
#target_link_libraries(test_adjacency_list sbpl)

add_executable(test_pwsa src/test/run_pwsa.cpp)
target_link_libraries(test_pwsa sbpl)
SET(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS thread regex system filesystem REQUIRED)
find_library(LIBRT_LIBRARIES rt)
target_link_libraries(test_pwsa
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_THREAD_LIBRARY}
  ${Boost_REGEX_LIBRARY}
  ${LIBRT_LIBRARIES_RT}
  ${PASL_LIBRARY}
)

add_executable(test_astar src/test/run_astar.cpp)
target_link_libraries(test_astar sbpl)
SET(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS thread regex system filesystem REQUIRED)
find_library(LIBRT_LIBRARIES rt)
target_link_libraries(test_astar
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_THREAD_LIBRARY}
  ${Boost_REGEX_LIBRARY}
  ${LIBRT_LIBRARIES_RT}
  ${PASL_LIBRARY}
)

