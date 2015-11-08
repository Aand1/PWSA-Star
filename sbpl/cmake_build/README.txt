To compile sbpl against pasl you have to do the following:

1) link pasl into libpasl.so

ex: g++-4.9 -shared -std=gnu++1y -DNDEBUG -O2 -fno-optimize-sibling-calls -DDISABLE_INTERRUPTS -DSTATS_IDLE -D_GNU_SOURCE -Wfatal-errors -m64 -DTARGET_X86_64 -DTARGET_LINUX -lm -pthread -DHAVE_GCC_TLS -I . -I ../tools/build//../../sequtil -I ../tools/build//../../parutil -I ../tools/build//../../sched -I ../tools/build//../../tools/pbbs -I ../tools/build//../../tools/malloc_count -I _build/opt $* _build/opt/cmdline.o _build/opt/threaddag.o _build/opt/callback.o _build/opt/atomic.o _build/opt/machine.o _build/opt/worker.o _build/opt/logging.o _build/opt/stats.o _build/opt/microtime.o _build/opt/ticks.o _build/opt/scheduler.o _build/opt/messagestrategy.o _build/opt/instrategy.o _build/opt/estimator.o _build/opt/outstrategy.o _build/opt/workstealing.o _build/opt/native.o -o libpasl.so

it's easiest to do this if you do something like follows:
- make fib.opt 
- run the above command to create the shared lib
If things aren't working, try changing the pasl makefile to compile all the .o files with -fPIC 
 
2) move libpasl.so into /usr/lib/libpasl.so

3) run: export CXX=g++-4.9 

4) run: cmake CMakeLists.txt

5) run: make VERBOSE=1
- this should break when it's trying to compile the executables. Copy the g++ command it issued, ex:
/usr/bin/g++-4.9   -std=gnu++1y -pthread -DNDEBUG -fno-optimize-sibling-calls -DDISABLE_INTERRUPTS -DSTATS_IDLE -D_GNU_SOURCE -Wfatal-errors -m64 -DTARGET_X86_64 -DTARGET_LINUX -lm -DHAVE_GCC_TLS -O3 -DNDEBUG    CMakeFiles/test_pwsa.dir/usr2/home/ldhulipa/PWSA-Star/sbpl/src/test/run_pwsa.o  -o bin/test_pwsa -rdynamic libsbpl.a -Wl,-Bstatic -lboost_filesystem -lboost_system -lboost_thread -lboost_regex -Wl,-Bdynamic 

and change this to:
/usr/bin/g++-4.9   -std=gnu++1y -pthread -DNDEBUG -fno-optimize-sibling-calls -DDISABLE_INTERRUPTS -DSTATS_IDLE -D_GNU_SOURCE -Wfatal-errors -m64 -DTARGET_X86_64 -DTARGET_LINUX -lm -DHAVE_GCC_TLS -O3 -DNDEBUG    CMakeFiles/test_pwsa.dir/usr2/home/ldhulipa/PWSA-Star/sbpl/src/test/run_pwsa.o  -o bin/test_pwsa -rdynamic libsbpl.a -Wl,-Bstatic -lboost_filesystem -lboost_system -lboost_thread -lboost_regex -Wl,-Bdynamic -lrt -lpasl

adding -lrt is necessary for compiling test_para, and -lrt -lpasl are necessary for compiling test_pwsa. 

6) you're done!
