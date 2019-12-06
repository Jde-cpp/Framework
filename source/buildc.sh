#!/bin/bash
type=${1:-asan}
clean=${2:-0}
#verbose=${3:-0}
cd "${0%/*}/.obj/$type"

if [ $clean -eq 1 ]; then
	rm CMakeCache.txt;
	cmake -DCMAKE_BUILD_TYPE=$type  ../..;
	make clean;
fi
make -j7
cd - > /dev/null
exit $?
