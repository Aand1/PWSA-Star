cmake_minimum_required(VERSION 2.4.6)
set(CMAKE_BUILD_TYPE Release)
include_directories(${PROJECT_SOURCE_DIR}/code)

add_library(sbpl
  ../src/discrete_space_information/nav2d/environment_nav2D.cpp
  ../src/discrete_space_information/navxythetalat/environment_navxythetalat.cpp
  ../src/discrete_space_information/navxythetalat/environment_navxythetamlevlat.cpp
  ../src/discrete_space_information/nav2d_uu/environment_nav2Duu.cpp
  ../src/discrete_space_information/template/environment_XXX.cpp
  ../src/discrete_space_information/robarm/environment_robarm.cpp
  ../src/planners/ARAStar/araplanner.cpp
  ../src/planners/ADStar/adplanner.cpp
  ../src/planners/VI/viplanner.cpp
  ../src/planners/PPCP/ppcpplanner.cpp
  ../src/planners/pARAStar/pARA.cpp
  ../src/planners/PWSA/pwsa.cpp
  ../src/planners/RStar/rstarplanner.cpp
  ../src/planners/ANA/ANAplanner.cpp
  ../src/utils/heap.cpp
  ../src/utils/mdp.cpp
  ../src/utils/utils.cpp
  ../src/utils/2Dgridsearch.cpp
)

#add_library(pasl STATIC IMPORTED
#  ../pasl/_build/opt/atomic.o
#  ../pasl/_build/opt/callback.o
#  ../pasl/_build/opt/cmdline.o
#)

#g++-4.9 -std=gnu++1y -DNDEBUG -O2 -fno-optimize-sibling-calls -DDISABLE_INTERRUPTS -DSTATS_IDLE -D_GNU_SOURCE -Wfatal-errors -m64 -DTARGET_X86_64 -DTARGET_LINUX -lm -pthread -DHAVE_GCC_TLS -I . -I ../tools/build//../../sequtil -I ../tools/build//../../parutil -I ../tools/build//../../sched -I ../tools/build//../../tools/pbbs -I ../tools/build//../../tools/malloc_count -I _build/opt $* _build/opt/fib.o _build/opt/cmdline.o _build/opt/threaddag.o _build/opt/callback.o _build/opt/atomic.o _build/opt/machine.o _build/opt/worker.o _build/opt/logging.o _build/opt/stats.o _build/opt/microtime.o _build/opt/ticks.o _build/opt/scheduler.o _build/opt/messagestrategy.o _build/opt/instrategy.o _build/opt/estimator.o _build/opt/outstrategy.o _build/opt/workstealing.o _build/opt/native.o -o fib.opt

# Build executables in the bin directory
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
# rosbuild_add_executable(test_sbpl src/test/main.cpp)
SET(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS thread regex system filesystem REQUIRED)
add_executable(test_sbpl ../src/test/main.cpp)
target_link_libraries(test_sbpl sbpl ${Boost_THREAD_LIBRARY} ${Boost_SYSTEM_LIBRARY})

## rosbuild_add_executable(test_adjacency_list src/test/test_adjacency_list.cpp)
#add_executable(test_adjacency_list ../src/test/test_adjacency_list.cpp)
#target_link_libraries(test_adjacency_list sbpl)


# rosbuild_add_executable(test_sbpl src/test/run.cpp)
add_executable(test_para ../src/test/run.cpp)
target_link_libraries(test_para sbpl)
SET(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS thread regex system filesystem REQUIRED)
find_library(LIBRT_LIBRARIES rt)
target_link_libraries(test_para
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_THREAD_LIBRARY}
  ${Boost_REGEX_LIBRARY}
  ${LIBRT_LIBRARIES_RT} 
)

link_directories(/usr/lib)

#g++-4.9 -shared -std=gnu++1y -DNDEBUG -fno-optimize-sibling-calls -DDISABLE_INTERRUPTS -DSTATS_IDLE -D_GNU_SOURCE -Wfatal-errors -m64 -DTARGET_X86_64 -DTARGET_LINUX -lm -DHAVE_GCC_TLS -I . -I ../tools/build//../../sequtil -I ../tools/build//../../parutil -I ../tools/build//../../sched -I ../tools/build//../../tools/pbbs -I ../tools/build//../../tools/malloc_count -I _build/opt $* _build/opt/cmdline.o _build/opt/threaddag.o _build/opt/callback.o _build/opt/atomic.o _build/opt/machine.o _build/opt/worker.o _build/opt/logging.o _build/opt/stats.o _build/opt/microtime.o _build/opt/ticks.o _build/opt/scheduler.o _build/opt/messagestrategy.o _build/opt/instrategy.o _build/opt/estimator.o _build/opt/outstrategy.o _build/opt/workstealing.o _build/opt/native.o -o libpasl.so

add_executable(test_pwsa ../src/test/run_pwsa.cpp)
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
)
set(CMAKE_CXX_FLAGS "-std=gnu++1y -pthread -DNDEBUG -fno-optimize-sibling-calls -DDISABLE_INTERRUPTS -DSTATS_IDLE -D_GNU_SOURCE -Wfatal-errors -m64 -DTARGET_X86_64 -DTARGET_LINUX -lm -DHAVE_GCC_TLS")
