# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 2.8

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list

# Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/sameer/pwsa_project/libsbpl

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/sameer/pwsa_project/libsbpl/build

# Include any dependencies generated for this target.
include CMakeFiles/test_astar.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/test_astar.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/test_astar.dir/flags.make

CMakeFiles/test_astar.dir/src/test/run_astar.o: CMakeFiles/test_astar.dir/flags.make
CMakeFiles/test_astar.dir/src/test/run_astar.o: ../src/test/run_astar.cpp
	$(CMAKE_COMMAND) -E cmake_progress_report /home/sameer/pwsa_project/libsbpl/build/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object CMakeFiles/test_astar.dir/src/test/run_astar.o"
	/usr/bin/g++-4.9   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/test_astar.dir/src/test/run_astar.o -c /home/sameer/pwsa_project/libsbpl/src/test/run_astar.cpp

CMakeFiles/test_astar.dir/src/test/run_astar.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/test_astar.dir/src/test/run_astar.i"
	/usr/bin/g++-4.9  $(CXX_DEFINES) $(CXX_FLAGS) -E /home/sameer/pwsa_project/libsbpl/src/test/run_astar.cpp > CMakeFiles/test_astar.dir/src/test/run_astar.i

CMakeFiles/test_astar.dir/src/test/run_astar.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/test_astar.dir/src/test/run_astar.s"
	/usr/bin/g++-4.9  $(CXX_DEFINES) $(CXX_FLAGS) -S /home/sameer/pwsa_project/libsbpl/src/test/run_astar.cpp -o CMakeFiles/test_astar.dir/src/test/run_astar.s

CMakeFiles/test_astar.dir/src/test/run_astar.o.requires:
.PHONY : CMakeFiles/test_astar.dir/src/test/run_astar.o.requires

CMakeFiles/test_astar.dir/src/test/run_astar.o.provides: CMakeFiles/test_astar.dir/src/test/run_astar.o.requires
	$(MAKE) -f CMakeFiles/test_astar.dir/build.make CMakeFiles/test_astar.dir/src/test/run_astar.o.provides.build
.PHONY : CMakeFiles/test_astar.dir/src/test/run_astar.o.provides

CMakeFiles/test_astar.dir/src/test/run_astar.o.provides.build: CMakeFiles/test_astar.dir/src/test/run_astar.o

# Object files for target test_astar
test_astar_OBJECTS = \
"CMakeFiles/test_astar.dir/src/test/run_astar.o"

# External object files for target test_astar
test_astar_EXTERNAL_OBJECTS =

test_astar: CMakeFiles/test_astar.dir/src/test/run_astar.o
test_astar: libsbpl.so
test_astar: /usr/lib/libboost_filesystem-mt.a
test_astar: /usr/lib/libboost_system-mt.a
test_astar: /usr/lib/libboost_thread-mt.a
test_astar: /usr/lib/libboost_regex-mt.a
test_astar: /usr/local/bin/libpasl.a
test_astar: CMakeFiles/test_astar.dir/build.make
test_astar: CMakeFiles/test_astar.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking CXX executable test_astar"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/test_astar.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/test_astar.dir/build: test_astar
.PHONY : CMakeFiles/test_astar.dir/build

CMakeFiles/test_astar.dir/requires: CMakeFiles/test_astar.dir/src/test/run_astar.o.requires
.PHONY : CMakeFiles/test_astar.dir/requires

CMakeFiles/test_astar.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/test_astar.dir/cmake_clean.cmake
.PHONY : CMakeFiles/test_astar.dir/clean

CMakeFiles/test_astar.dir/depend:
	cd /home/sameer/pwsa_project/libsbpl/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/sameer/pwsa_project/libsbpl /home/sameer/pwsa_project/libsbpl /home/sameer/pwsa_project/libsbpl/build /home/sameer/pwsa_project/libsbpl/build /home/sameer/pwsa_project/libsbpl/build/CMakeFiles/test_astar.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/test_astar.dir/depend

