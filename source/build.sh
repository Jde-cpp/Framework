#!/bin/bash
debug=${1:-1}
clean=${2:-0}
cd "${0%/*}"

../../Linux/source/build.sh $debug $clean || exit 1;
if [ $clean -eq 1 ]; then
	make clean DEBUG=$debug
	if [ $debug -eq 1 ]; then
		ccache g++-8 -c -g -pthread -fPIC -std=c++17 -Wall -Wno-unknown-pragmas -Wno-ignored-attributes -DJde_EXPORTS -O0 -fsanitize=address -fno-omit-frame-pointer ./pc.h -o .obj/debug2/stdafx.h.gch -I/home/duffyj/code/libraries/spdlog/include -I$BOOST_ROOT -I/home/duffyj/code/libraries/json/include -I/home/duffyj/code/libraries/eigen
	else
		ccache g++-8 -c -g -pthread -fPIC -std=c++17 -Wall -Wno-unknown-pragmas -Wno-ignored-attributes -DJde_EXPORTS -march=native -DNDEBUG -O3 ./pc.h -o .obj/release2/stdafx.h.gch -I/home/duffyj/code/libraries/spdlog/include -I$BOOST_ROOT -I/home/duffyj/code/libraries/json/include -I/home/duffyj/code/libraries/eigen
	fi;
	if [ $? -eq 1 ]; then
		exit 1
	fi;
else
	echo "Build framework"
fi
make -j7 DEBUG=$debug
cd -
exit $?
