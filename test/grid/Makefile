####################################################################
# Configuration

# Paths to auxiliary Makefile definitions

TOOLS_BUILD_FOLDER=../../src/pasl/tools/build

####################################################################
# Mandatory options

USE_PTHREADS=1
USE_MATH=1

####################################################################
# Default options

USE_ALLOCATOR=tcmalloc
USE_HWLOC=0
USE_NUMA=0

USE_FATAL_ERRORS=1
VERBOSE_MAKE=0
VERBOSE_DEPENDENCIES=0

PROGRAMS=\
	grid_main.cpp

####################################################################
# Makefile options

# Create a file called "settings.sh" in this folder if you want to
# configure particular options. See section below for options.

-include settings.sh

# Options are configured by the auxiliary file below

include $(TOOLS_BUILD_FOLDER)/Makefile_options


####################################################################
# Modes

MODES=dbg opt log #sta opt seq cilk

# Compilation options for each mode

COMPILE_OPTIONS_COMMON= -fPIC -DDISABLE_INTERRUPTS -DSTATS_IDLE $(OPTIONS_ALL) -mcx16
COMPILE_OPTIONS_FOR_dbg=$(OPTIONS_DEBUG) -DSTATS -DDEBUG # -DUSE_UCONTEXT  -DLOGGING
COMPILE_OPTIONS_FOR_log=$(OPTIONS_O2) -DSTATS -DLOGGING
COMPILE_OPTIONS_FOR_sta=$(OPTIONS_O2) -DSTATS
COMPILE_OPTIONS_FOR_seq=$(OPTIONS_O2) -DSTATS -DSEQUENTIAL_ELISION
COMPILE_OPTIONS_FOR_opt=$(OPTIONS_O2)
COMPILE_OPTIONS_FOR_cilk=$(OPTIONS_cilk) $(OPTIONS_O2)

# Folders where to find all the header files and main sources

INCLUDES=. src ../../src/ ../../src/shared/ $(SEQUTIL_PATH) $(PARUTIL_PATH) $(SCHED_PATH) $(PBBS_PATH) $(MALLOC_COUNT_PATH)

# Folders where to find all the source files

FOLDERS=$(INCLUDES)


####################################################################
# Targets

all: opt

opt: $(PROGRAMS:.cpp=.opt)

fib.all: $(call all_modes_for,fib)

old_pase_main.opt:
	g++ src/old_pase_main.cpp -I../../src/shared/ -lpthread -o pase_main.opt

old_pastar_main.opt:
	g++ src/old_pastar_main.cpp -I../../src/shared/ -lpthread -o pastar_main.opt

####################################################################
# Clean

clean_generated:
	rm -Rf generated/*

clean: clean_build clean_modes clean_generated


####################################################################
# Main rules for the makefile

include $(TOOLS_BUILD_FOLDER)/Makefile_modes
