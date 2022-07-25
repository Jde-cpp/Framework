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
tests=${3:-1}
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
	popd  > /dev/null;
fi;
fetchDefault Public;
cd $scriptDir/../Public;
if [[ -z $JDE_BASH ]]; then JDE_BASH=$scriptDir/..; fi;
stageDir=$JDE_BASH/Public/stage

function winBoostConfig
{
	local file=$1;
	local config=$2;
	if [ ! -L $stageDir/$config/$file.lib ]; then
		echo NOT exists - $stageDir/$config/$file.lib
		cd $BOOST_BASH;
		if [ ! -f b2.exe ]; then echo boost bootstrap; cmd <<< bootstrap.bat; echo boost bootstrap finished; fi;
		local command="b2 variant=$config link=shared threading=multi runtime-link=shared address-model=64 --with-$lib"
		cmd <<< "$command";#  > /dev/null;
		echo $file - $config - build complete
		linkFileAbs `pwd`/stage/lib/$file.lib $stageDir/$config/$file.lib;
		if [ ! -f `pwd`/stage/lib/$file.lib ]; then
			echo ERROR NOT FOUND:  `pwd`/stage/lib/$file
			echo `pwd`;
			echo $command;
			exit 1;
		fi;
		linkFileAbs `pwd`/stage/lib/$file.dll $stageDir/$config/$file.dll;
	fi;
}
function winBoost
{
	lib=$1;
	echo winBoost - $lib
	winBoostConfig boost_$lib-vc143-mt-x64-1_79 release;
	winBoostConfig boost_$lib-vc143-mt-gd-x64-1_79 debug;
}
if windows; then
	pushd `pwd` > /dev/null;
	moveToDir stage;
	moveToDir debug; cd ..;
	moveToDir release; popd  > /dev/null;
	if [ $shouldFetch -eq 1 ]; then
		if [ ! -d $REPO_BASH/vcpkg/installed/x64-windows/include/nlohmann ]; then vcpkg.exe install nlohmann-json --triplet x64-windows; fi;
		if [ ! -d $REPO_BASH/vcpkg/installed/x64-windows/lib/zlib.lib ]; 
		then 
			vcpkg.exe install zlib --triplet x64-windows; 
			pushd `pwd` > /dev/null;
			cd $stageDir/debug; mklink zlibd1.dll $REPO_BASH/vcpkg/installed/x64-windows/debug/bin;
			cd $stageDir/release; mklink zlib1.dll $REPO_BASH/vcpkg/installed/x64-windows/bin;
		fi;
		if [ -z $BOOST_DIR ]; then echo \$BOOST_DIR not set; exit 1; fi;
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
			winBoost thread;
		fi;
	fi;
fi;
buildProto=$shouldFetch
cd $REPO_BASH;
if [ ! -d protobuf ]; then  git clone https://github.com/Jde-cpp/protobuf.git; buildProto=1; fi;
if [ ! -d spdlog ]; then
	git clone https://github.com/Jde-cpp/spdlog;
elif [ $shouldFetch -eq 1 ]; then
	cd spdlog; echo pulling spdlog; git pull > /dev/null; cd ..;
fi;

if windows; then
	CMAKE_VS_PLATFORM_NAME_DEFAULT="Visual Studio 17 2022"
	if [ ! -d fmt ]; then
		git clone https://github.com/fmtlib/fmt.git; cd fmt;
	elif [ $shouldFetch -eq 1 ]; then
		cd fmt; echo pulling fmt; git pull;
	fi;
	if [ ! -d build ]; then
		moveToDir buil;cmake -DCMAKE_CXX_STANDARD=20 ..;
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
	moveToDir $type;#cmake\build\sln
	if test ! -f libprotobuf.vcxproj; then
		cmake -G "Visual Studio 17 2022" -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_WITH_ZLIB=ON -DZLIB_INCLUDE_DIR=$REPO_DIR/vcpkg/installed/x64-windows/include -DZLIB_LIB=$REPO_DIR/vcpkg/installed/x64-windows/lib -Dprotobuf_MSVC_STATIC_RUNTIME=OFF -Dprotobuf_BUILD_SHARED_LIBS=$sharedLibs ../..
		if [ $? -ne 0 ]; then echo `pwd`/$cmd; exit 1; fi;
	fi;
	baseCmd="msbuild.exe libprotobuf.vcxproj -p:Platform=x64 -maxCpuCount -nologo -v:q /clp:ErrorsOnly -p:Configuration"
	buildWindows2 "$baseCmd" libprotobuf.dll release;
	buildWindows2 "$baseCmd" libprotobufd.dll debug;
	buildWindows2 "msbuild.exe protoc.vcxproj -p:Platform=x64 -maxCpuCount -nologo -v:q /clp:ErrorsOnly -p:Configuration" protoc.exe release;
	echo `pwd`;
	cp Release/libprotoc.dll $JDE_BASH/Public/stage/Release/libprotoc.dll;
}
function protocBuildLinux
{
	echo protocBuildLinux;
	# cd $REPO_DIR/protobuf;
	# git submodule update --init --recursive;
	# ./autogen.sh;
	# export CXX='clang++';
	# export CXXFLAGS='-std=c++20 -stdlib=libc++';
	# export LDFLAGS='-stdlib=libc++';
	# export CC=clang;
	# ./configure;
	# if (( $clean == 1 )); then make clean; fi;
	# make;
	# make check;
	# sudo make install;
	# sudo ldconfig;
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

protobufInclude=$REPO_DIR/protobuf/src;
if windows; then findExecutable protoc.exe $JDE_BASH/Public/stage/release; fi;

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
	echo frameworkProtoc $cleanProtoc
	if [ $cleanProtoc -eq 1 ]; then
	   	protoc --cpp_out dllexport_decl=JDE_EXPORT:. -I$protobufInclude -I. messages.proto;
		if [ $? -ne 0 ]; then exit 1; fi;
		sed -i -e 's/JDE_EXPORT/Γ/g' messages.pb.h;
		sed -i '1s/^/\xef\xbb\xbf/' messages.pb.h;
		if windows; then
			echo `pwd`;
			sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT InstanceDefaultTypeInternal _Instance_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY InstanceDefaultTypeInternal _Instance_default_instance_;/' messages.pb.cc;
			sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT RequestStringDefaultTypeInternal _RequestString_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY RequestStringDefaultTypeInternal _RequestString_default_instance_;/' messages.pb.cc;
			sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT CustomMessageDefaultTypeInternal _CustomMessage_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY CustomMessageDefaultTypeInternal _CustomMessage_default_instance_;/' messages.pb.cc;
			sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT GenericFromServerDefaultTypeInternal _GenericFromServer_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY GenericFromServerDefaultTypeInternal _GenericFromServer_default_instance_;/' messages.pb.cc;
		fi;
	fi;
	cd ../../..;
}
fetchDefault Framework;
frameworkProtoc;
build Framework 0;

#cd ../tests;
#buildTest Framework Tests.Framework.exe