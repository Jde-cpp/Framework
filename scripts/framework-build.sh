#!/bin/bash
branch=${1:-master}
clean=${2:-0};
shouldFetch=${3:-1};
tests=${4:-1}
scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
[[ "$branch" = master ]] && branch2=main || branch2="$branch"
if [[ -z $sourceBuild ]]; then source $scriptDir/source-build.sh; fi;
echo framework-build.sh branch=$branch clean=$clean shouldFetch=$shouldFetch tests=$tests;
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
fi;

fetchDefault $branch2 Public; if [ $? -ne 0 ]; then echo fetchDefault Public failed;  exit 1; fi;
cd $scriptDir/../../Public;
if [[ -z $JDE_BASH ]]; then JDE_BASH=$scriptDir/../..; fi;
stageDir=$JDE_BASH/Public/stage

function winBoostConfig {
	local file=$1;
	local config=$2;
	if [ ! -L $stageDir/$config/$file.lib ]; then
		echo NOT exists - $stageDir/$config/$file.lib
		cd $BOOST_BASH;
		echo BOOST_BASH2=$BOOST_BASH;
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
function winBoost {
	lib=$1;
	winBoostConfig boost_$lib-vc143-mt-x64-1_86 release;
	winBoostConfig boost_$lib-vc143-mt-gd-x64-1_86 debug;
}

if windows; then
	pushd `pwd` > /dev/null;
	moveToDir stage;
	moveToDir debug; cd ..;
	moveToDir release; popd  > /dev/null;


	if [ ! -d $REPO_BASH/vcpkg/installed/x64-windows/include/gtest ]; then
		vcpkg.exe install gtest --triplet x64-windows;
	fi;

	if [ $shouldFetch -eq 1 ]; then
		if [ ! -d $REPO_BASH/vcpkg/installed/x64-windows/include/nlohmann ]; then vcpkg.exe install nlohmann-json --triplet x64-windows; fi;
		if [ ! -f $REPO_BASH/vcpkg/installed/x64-windows/lib/zlib.lib ]; then
			vcpkg.exe install zlib --triplet x64-windows;
			pushd `pwd` > /dev/null;
			cd $stageDir/debug; mklink zlibd1.dll $REPO_BASH/vcpkg/installed/x64-windows/debug/bin;
			cd $stageDir/release; mklink zlib1.dll $REPO_BASH/vcpkg/installed/x64-windows/bin;
		fi;
		if [ ! -d $REPO_BASH/vcpkg/installed/x64-windows/include/fmt ]; then
			vcpkg.exe install fmt --triplet x64-windows;
		fi;
		if [ ! -d $REPO_BASH/vcpkg/installed/x64-windows/include/spdlog ]; then
			vcpkg.exe install spdlog --triplet x64-windows;
		fi;
		if [ ! -d $REPO_BASH/vcpkg/installed/x64-windows/include/google ]; then
			vcpkg.exe install protobuf --triplet x64-windows;
		fi;
		if [ -z $Boost_INCLUDE_DIR ]; then echo \$Boost_INCLUDE_DIR not set; exit 1; fi;
		toBashDir $Boost_INCLUDE_DIR BOOST_BASH;
		echo BOOST_BASH=$BOOST_BASH;
		if [ ! -d $BOOST_BASH ]; then
			echo ERROR - $BOOST_BASH not found;
			exit 1;
		else
			winBoost date_time;
			winBoost iostreams;  #b2 variant=debug link=shared threading=multi runtime-link=shared address-model=64  -sZLIB_BINARY=zlib -sZLIB_LIBPATH=C:\Users\duffyj\source\repos\vcpkg\installed\x64-windows\lib -sZLIB_INCLUDE=C:\Users\duffyj\source\repos\vcpkg\installed\x64-windows\include -sNO_ZLIB=0 -sNO_COMPRESSION=0 -sBOOST_IOSTREAMS_USE_DEPRECATED  --with-iostreams
			winBoost context;
			winBoost coroutine;
			winBoost thread;
		fi;
	fi;
fi;
echo -------------Protobuf-------------
buildProto=$shouldFetch
cd $REPO_BASH;

cd $baseDir/$jdeRoot;
if windows; then
	if [ ! -d Windows ]; then git clone https://github.com/Jde-cpp/Windows.git -q; fi;
else
	if [ ! -d Linux ]; then git clone https://github.com/Jde-cpp/Linux.git -q; fi;
fi;
echo -------------Framework-------------
fetchDefault $branch Framework 0;

buildCMake Jde;

#cd ../tests;
#buildTest Framework Tests.Framework.exe