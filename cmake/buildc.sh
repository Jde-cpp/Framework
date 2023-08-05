#!/bin/bash
path=${1}
type=${2:-asan}
clean=${3:-0}
dir=${4:-../..}
cwd=`pwd`
#export CXX=clang++
output=$path/.obj/$type
cd $path
if [ ! -d .obj ]; then mkdir .obj; fi;
cd .obj;
if [ ! -d $type ]; then mkdir $type; fi;
cd $type
if (( $clean == 1 )) || [ ! -f CMakeCache.txt ]; then
	echo cleaning...
	if [ -f CMakeCache.txt ]; then rm CMakeCache.txt; fi;
	cmake -DCMAKE_BUILD_TYPE=$type $dir > /dev/null;
	make clean;
fi
make -j;
result=$?;
cd $cwd;
exit $result;