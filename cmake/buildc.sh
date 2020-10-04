#!/bin/bash
path=${1}
type=${2:-asan}
clean=${3:-0}
export CXX=clang++
output=$path/.obj/$type
if [ ! -d $output ]; then mkdir $output; fi;
cd $output

if [ $clean -eq 1 ]; then
	if [ -f CMakeCache.txt ]; then rm CMakeCache.txt; fi;
	cmake -DCMAKE_BUILD_TYPE=$type  ../.. > /dev/null;
	make clean;
fi
make -j7;
cd - > /dev/null
exit $?