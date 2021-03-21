#!/bin/bash
type=${1:-asan}
clean=${2:-0}
all=${3:-1}
export CXX=clang++
if [ $all -eq 1 ]; then
	../cmake/buildc.sh ../source $type $clean || exit 1;
	../cmake/buildc.sh ../../MySql/source $type $clean || exit 1;
fi
if [ ! -d .obj ]; then mkdir .obj; fi;

../cmake/buildc.sh `pwd` $type $clean

exit $?
