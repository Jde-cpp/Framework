#!/bin/bash
#curl https://raw.githubusercontent.com/Jde-cpp/Framework/master/framework-build.sh -o framework-build.sh
##OR
# file=framework-build.sh
# dest=`pwd`;
# dest=${dest////\\};
# dest=${dest/\\c/c:};
# cmd <<< "mklink $dest\\$file $dest\\jde\\Framework\\$file" > /dev/null;

#chmod 777 framework-build.sh

clean=${1:-0};
shouldFetch=${2:-1};
scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
if [[ -z $sourceBuild ]]; then source $scriptDir/source-build.sh; fi;
echo framework-build.sh clean=$clean shouldFetch=$shouldFetch
cd $REPO_DIR;

if windows; then
	path_to_exe=$(which vcpkg.exe 2> /dev/null);
	if [ ! -x "$path_to_exe" ] ; then
		if [ ! -d vcpkg ]; then
			echo installing vcpkg;
			git clone https://github.com/microsoft/vcpkg;
			./vcpkg/bootstrap-vcpkg.bat;
		fi;
		findExecutable vcpkg.exe `pwd`/vcpkg
	fi;
else
	pushd `pwd` > /dev/null;
	cd $BOOST_ROOT;
	#./bootstrap.sh --with-toolset=clang
	#./b2 clean
	#./b2 variant=debug   toolset=clang cxxflags="-stdlib=libc++" linkflags="-stdlib=libc++" link=shared threading=multi runtime-link=shared address-model=64 --with-system
	#./b2 variant=release toolset=clang cxxflags="-stdlib=libc++" linkflags="-stdlib=libc++" link=shared threading=multi runtime-link=shared address-model=64 --with-system
	#./b2 variant=debug   toolset=clang cxxflags="-stdlib=libc++" linkflags="-stdlib=libc++" link=shared threading=multi runtime-link=shared address-model=64 --with-thread
	#./b2 variant=release toolset=clang cxxflags="-stdlib=libc++" linkflags="-stdlib=libc++" link=shared threading=multi runtime-link=shared address-model=64 --with-thread
	#./b2 variant=debug   toolset=clang cxxflags="-stdlib=libc++ -D_GLIBCXX_DEBUG" linkflags="-stdlib=libc++" link=shared threading=multi runtime-link=shared address-model=64 --with-iostreams
	#./b2 variant=release toolset=clang cxxflags="-stdlib=libc++" linkflags="-stdlib=libc++" link=shared threading=multi runtime-link=shared address-model=64 --with-iostreams
	popd  > /dev/null;
fi;
#if [ ! -d $jdeRoot ]; then mkdir $jdeRoot; fi;
fetchDefault Public;
cd $scriptDir/../Public;
stageDir=$REPO_BASH/jde/Public/stage

function winBoostConfig
{
	local file=$1;
	local config=$2;
	if [ ! -f $stageDir/$config/$file ]; then
		echo NOT exists - $stageDir/$config/$file
		cd $BOOST_BASH;
		if [ ! -f b2.exe ]; then echo boost bootstrap; cmd <<< bootstrap.bat; echo boost bootstrap finished; fi;
		local comand="b2 variant=$config link=shared threading=multi runtime-link=shared address-model=64 --with-$lib"
		echo $comand;
		cmd <<< "$command";#  > /dev/null;
		if [ ! -f `pwd`/stage/lib/$file ];
		then
			echo ERROR NOT FOUND:  `pwd`/stage/lib/$file
			echo `pwd`;
			echo $comand;
			exit 1;
		fi;
		echo `pwd`/stage/lib/$file found!
		linkFileAbs `pwd`/stage/lib/$file $stageDir/$config/$file;
#	else
#		echo exists - $stageDir/release/$file;
	fi;
}
function winBoost
{
	lib=$1;
	echo winBoost - $lib
	winBoostConfig boost_$lib-vc142-mt-x64-1_76.lib release;
	winBoostConfig boost_$lib-vc142-mt-gd-x64-1_76.lib debug;
}
if windows; then
	pushd `pwd` > /dev/null;
	moveToDir stage;
	moveToDir debug; cd ..;
	moveToDir release; popd  > /dev/null;
	if [ $shouldFetch -eq 1 ]; then
		if [ ! -d $REPO_BASH/vcpkg/installed/x64-windows/include/nlohmann ]; then vcpkg.exe install nlohmann-json --triplet x64-windows; fi;
		if [ ! -d $REPO_BASH/vcpkg/installed/x64-windows/lib/zlib.lib ]; then vcpkg.exe install zlib --triplet x64-windows; fi;
		#vcpkg.exe install fmt --triplet x64-windows
		#vcpkg.exe install spdlog --triplet x64-windows
		toBashDir $BOOST_DIR BOOST_BASH;
		if [ ! -d $BOOST_BASH ]; then
			echo ERROR - $BOOST_BASH not found;
			exit 1;
			vcpkg.exe install boost --triplet x64-windows; if [ $? -ne 0 ]; then echo "vcpkg install boost failed"; exit 1; fi;
		else
			winBoost date_time;
			winBoost iostreams;  #b2 variant=debug link=shared threading=multi runtime-link=shared address-model=64  -sZLIB_BINARY=zlib -sZLIB_LIBPATH=C:\Users\duffyj\source\repos\vcpkg\installed\x64-windows\lib -sZLIB_INCLUDE=C:\Users\duffyj\source\repos\vcpkg\installed\x64-windows\include -sNO_ZLIB=0 -sNO_COMPRESSION=0 -sBOOST_IOSTREAMS_USE_DEPRECATED  --with-iostreams
			winBoost context;
			winBoost coroutine;
		fi;
	fi;
fi;
buildProto=$shouldFetch
#buildProto=0
cd $REPO_BASH;
if [ ! -d protobuf ]; then  git clone https://github.com/Jde-cpp/protobuf.git; buildProto=1; fi;
if [ ! -d spdlog ]; then
	git clone https://github.com/gabime/spdlog.git;
elif [ $shouldFetch -eq 1 ]; then
	cd spdlog; echo pulling spdlog; git pull > /dev/null; cd ..;
fi;

if windows; then
	if [ ! -d fmt ]; then
		git clone https://github.com/fmtlib/fmt.git; cd fmt;
	elif [ $shouldFetch -eq 1 ]; then
		cd fmt; echo pulling fmt; git pull;
	fi;
	if [ ! -d build ]; then
		moveToDir build;
		cmake -DCMAKE_CXX_STANDARD=20 ..;
	else
		cd build;
	fi;
	baseCmd="msbuild.exe fmt.vcxproj -p:Platform=x64 -maxCpuCount -nologo -v:q /clp:ErrorsOnly -p:Configuration"
	if [ ! -f $stageDir/release/fmt.lib ]; then echo build fmt.lib; buildWindows2 "$baseCmd" fmt.lib release; fi;
	if [ ! -f $stageDir/debug/fmtd.lib ]; then echo build fmtd.lib; buildWindows2 "$baseCmd" fmtd.lib debug; fi;
	cd ../..;
fi;

function protocBuildWin
{
	type=sln;#lib;
	sharedLibs=$(if [ $type = "sln" ]; then echo "ON"; else echo "OFF"; fi)
	if [ ! -d "`pwd`/$type" ]; then mkdir $type; fi;
	cd $type;
	if test ! -f libprotobuf.vcxproj; then
		#subDir=$([ "$type" = "Debug" ] && echo "/$type" || echo "");
		#cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=$type -Dprotobuf_WITH_ZLIB=ON -DZLIB_INCLUDE_DIR=$REPO_DIR/vcpkg/installed/x64-windows$subDir/include -DZLIB_LIB=$REPO_DIR/vcpkg/installed/x64-windows$subDir/lib ../..
		#cmake -G "Visual Studio 16 2019" -DCMAKE_BUILD_TYPE=$type -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_WITH_ZLIB=ON -DZLIB_INCLUDE_DIR=$REPO_DIR/vcpkg/installed/x64-windows/include -DZLIB_LIB=$REPO_DIR/vcpkg/installed/x64-windows$subDir/lib ../..
		#-DCMAKE_BUILD_TYPE=Release
		cmake -G "Visual Studio 16 2019" -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_WITH_ZLIB=ON -DZLIB_INCLUDE_DIR=$REPO_DIR/vcpkg/installed/x64-windows/include -DZLIB_LIB=$REPO_DIR/vcpkg/installed/x64-windows/lib -Dprotobuf_MSVC_STATIC_RUNTIME=OFF -Dprotobuf_BUILD_SHARED_LIBS=$sharedLibs ../..
		if [ $? -ne 0 ]; then
		 	#t="/c/Program\ Files\ \(X86\)/Microsoft\ Visual\ Studio/2019/Enterprise/Common7/Tools"
			#if [[ -d ${t//\\} ]]; then

				#toWinDir $t winDir;
				#CMAKE_GENERATOR_INSTANCE=$winDir;
			#fi;
			echo `pwd`/$cmd;
			exit 1;
		fi;
	fi;
	#nmake;
	#if [ ! -f Debug/libprotobufd.lib ]; then
		baseCmd="msbuild.exe libprotobuf.vcxproj -p:Platform=x64 -maxCpuCount -nologo -v:q /clp:ErrorsOnly -p:Configuration"
		buildWindows2 "$baseCmd" libprotobuf.dll release;
		#cp Release/libprotobuf.lib $REPO_BASH/jde/Public/stage/Release/libprotobufd.lib;
		buildWindows2 "$baseCmd" libprotobufd.dll debug;
		#cp Debug/libprotobufd.lib $REPO_BASH/jde/Public/stage/Debug/libprotobufd.lib;
		buildWindows2 "msbuild.exe protoc.vcxproj -p:Platform=x64 -maxCpuCount -nologo -v:q /clp:ErrorsOnly -p:Configuration" protoc.exe release;
		cp Release/libprotoc.dll $REPO_BASH/jde/Public/stage/Release/libprotoc.dll;
		#msbuild.exe libprotobuf.vcxproj -p:Configuration=Debug -p:Platform=x64 -maxCpuCount -nologo -v:q /clp:ErrorsOnly;
		#cp Debug/libprotobufd.dll $REPO_BASH/jde/Public/stage/Debug/libprotobufd.dll;
	#fi;
	# if [ ! -f Release/libprotobuf.dll ]; then
	# 	msbuild.exe libprotobuf.vcxproj -p:Configuration=Release -p:Platform=x64 -maxCpuCount -nologo -v:q /clp:ErrorsOnly;
	# 	cp Release/libprotobuf.dll $REPO_BASH/jde/Public/stage/Release/libprotobuf.dll;
	# fi;
	# if [ ! -f Release/protoc.exe ]; then
	# 	msbuild.exe protoc.vcxproj -p:Configuration=Release -p:Platform=x64 -maxCpuCount -nologo -v:q /clp:ErrorsOnly;
	# 	cp Release/protoc.exe $REPO_BASH/jde/Public/stage/Release/protoc.exe;
	# fi;
}
function protocBuildLinux
{
	cd $REPO_DIR/protobuf;
	git submodule update --init --recursive;
	./autogen.sh;
	export CXX='clang++';
	export CXXFLAGS='-std=c++20 -stdlib=libc++';
	export LDFLAGS='-stdlib=libc++';
	export CC=clang;
	./configure;
	if (( $clean == 1 )); then make clean; fi;
	make;
	make check;
	sudo make install;
	sudo ldconfig;
}
if [ $buildProto -eq 1 ]; then
	echo building protoc clean=$clean; echo `pwd`;
	cd protobuf;
	if [ $shouldFetch -eq 1 ]; then git pull; fi;
	cd cmake;
	if [ $clean -eq 1 ]; then echo rm protoc/build; rm -r -f build; fi;
	moveToDir build;
	if windows; then protocBuildWin; else protocBuildLinux; fi;
fi;
#protobufInclude=`pwd`/vcpkg/installed/x64-windows/include
#findExecutable protoc.exe `pwd`/vcpkg/installed/x64-windows/tools/protobuf 1
#echo $REPO_DIR;
#echo $REPO_BASH;
#exit 1;
protobufInclude=$REPO_DIR/protobuf/src;
if windows; then findExecutable protoc.exe $REPO_BASH/jde/Public/stage/release; fi;
#echo findExecutable protoc.exe; exit 1;

cd $baseDir/$jdeRoot;
if windows; then
	if [ ! -d Windows ]; then git clone https://github.com/Jde-cpp/Windows.git -q; fi;
else
	if [ ! -d Linux ]; then git clone https://github.com/Jde-cpp/Linux.git -q; fi;
fi;

function frameworkProtoc
{
	cleanProtoc=$clean
	cd log/server/proto;
	if [ ! -f messages.pb.cc ]; then
		cleanProtoc=1;
	fi;
	if [ $cleanProtoc -eq 1 ]; then
	   protoc --cpp_out dllexport_decl=JDE_NATIVE_VISIBILITY:. -I$protobufInclude -I. messages.proto;
		if [ $? -ne 0 ]; then exit 1; fi;
	fi;
	cd ../../..;
}
fetchDefault Framework;
frameworkProtoc;
build Framework 0;
