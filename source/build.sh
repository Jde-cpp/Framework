#!/bin/bash
debug=${1:-1}
clean=${2:-0}
cd "${0%/*}"
if [ $clean -eq 1 ]; then
	echo "Build framework - Clean"
	make clean DEBUG=$debug
	if [ $debug -eq 1 ]; then
		ccache g++-8 -c -g -pthread -fPIC -std=c++17 -Wall -Wno-ignored-attributes -Wno-unknown-pragmas -DJDE_NATIVE_EXPORTS -O0 -I.obj/debug ./pc.h -o .obj/debug/stdafx.h.gch -I/home/duffyj/code/libraries/spdlog/include -I$BOOST_ROOT -I/home/duffyj/code/libraries/json/include -I/home/duffyj/code/libraries/eigen  -fsanitize=address -fno-omit-frame-pointer
	else
		ccache g++-8 -c -g -pthread -fPIC -std=c++17 -Wall -Wno-ignored-attributes -Wno-unknown-pragmas -DJDE_NATIVE_EXPORTS -march=native -DNDEBUG -O3 -I.obj/release ./pc.h -o .obj/release/stdafx.h.gch -I/home/duffyj/code/libraries/spdlog/include -I$BOOST_ROOT -I/home/duffyj/code/libraries/json/include -I/home/duffyj/code/libraries/eigen
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
