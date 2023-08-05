#!/bin/bash
sourceBuildDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
if [[ -z $commonBuild ]]; then source $sourceBuildDir/common.sh; fi;
baseDir=$sourceBuildDir/../..;
jdeRoot=jde;
if [ -z $JDE_DIR ]; then JDE_DIR=$baseDir/$jdeRoot; fi;
if [ -z $JDE_BASH ]; then toBashDir $JDE_DIR JDE_BASH; fi;

t=$(readlink -f "${BASH_SOURCE[0]}"); sourceBuild=$(basename "$t"); unset t;

if [[ -z "$REPO_DIR" ]]; then export REPO_DIR=$baseDir; fi;
pushd `pwd` > /dev/null;
cd $REPO_DIR

if windows; then
	findExecutable MSBuild.exe '/c/Program\ Files/Microsoft\ Visual\ Studio/2022/BuildTools/MSBuild/Current/Bin' 0
	findExecutable MSBuild.exe '/c/Program\ Files/Microsoft\ Visual\ Studio/2022/Enterprise/MSBuild/Current/Bin'
	findExecutable cmake.exe '/c/Program\ Files/CMake/bin'
	findExecutable cl.exe '/c/Program\ Files/Microsoft\ Visual\ Studio/2022/BuildTools/VC/Tools/MSVC/14.34.31933/bin/Hostx64/x64' 0  #TODO go to any directory
	findExecutable cl.exe '/c/Program\ Files/Microsoft\ Visual\ Studio/2022/Enterprise/VC/Tools/MSVC/14.34.31933/bin/Hostx64/x64' 1
	findExecutable vswhere.exe '/c/Program\ Files\ \(X86\)/Microsoft\ Visual\ Studio/installer' 0
fi;

function buildConfig
{
	config=$1;
	configClean=${2:-0};
	if [ ! -f .obj/$config/Makefile ]; then
		echo no `pwd`/.obj/$config/Makefile configClean=1;
		configClean=1;
	fi;
	cmd="$sourceBuildDir/cmake/buildc.sh `pwd` $config $configClean";
	echo $cmd
	$cmd; if [ $? -ne 0 ]; then echo "FAILED"; echo `pwd`; echo $cmd; exit 1; fi;
}

function buildLinux
{
	local=${1:-0};
	projectClean=$clean;
	if [ ! -d .obj ]; then
		mkdir .obj;
		projectClean=1;
	fi;
	if [ $local -eq 1 ]; then
	   ./buildc.sh asan $projectClean 0;
	   ./buildc.sh RelWithDebInfo $projectClean 0;
	else
		buildConfig asan $projectClean
		buildConfig RelWithDebInfo $projectClean
	fi;
}
function buildWindows2
{
	echo buildWindows2 $1 $2 $3;
	echo $3 - starting;
	local configuration=$3;
	local cmd="$1=$configuration";
	local file=$2;
	local out=.bin/$configuration/$file;
	local targetDir=$baseDir/$jdeRoot/Public/stage/$configuration;
	local target=$targetDir/$file;
	if [ ! -f $target ]; then
		echo $target - not found;
		$cmd; if [ $? -ne 0 ]; then echo `pwd`; echo $cmd; exit 1; fi;
		if [ -f $target ]; then echo $cmd outputing stage dir.; rm $taget; fi;
		sourceDir=`pwd`;
		subDir=$(if [ -d .bin ]; then echo "/.bin"; else echo ""; fi);
		cd $targetDir;

		cp "$sourceDir$subDir/$configuration/$file" .;
		if [[ $file == *.dll ]]; then
			cp "$sourceDir$subDir/$configuration/${file:0:-3}lib" .;
		fi;
		cd "$sourceDir";
	else
		echo $target - found;
	fi;
}
function buildWindows
{
	dir=$1;
	echo buildWindows - $dir clean=$clean;
	if [[ ! -f "$dir.vcxproj.user" && -f "$dir.vcxproj._user" ]]; then
		echo 'linked $dir.vcxproj.user to $dir.vcxproj._user'
		linkFile $dir.vcxproj._user $dir.vcxproj.user;	if [ $? -ne 0 ]; then echo `pwd`; echo FAILED:  linkFile $dir.vcxproj._user $dir.vcxproj.user; exit 1; fi;
	fi;
	if [ ${clean:-1} -eq 1 ]; then
		echo rm -r -f $dir/.bin;
		rm -r -f .bin;
	fi;
	file=$2;
	echo file=$file dir=$dir;
	if [[ -z $file ]]; then
		echo file empty;
		if [[ $dir = "Framework" ]]; then
			file="Jde.dll";
		else
			file="Jde.$dir.dll";
		fi;
		echo file=$file;
	fi;
	projFile=$([ "${PWD##*/}" = "tests" ] && echo "Tests.$dir" || echo "$dir");
	baseCmd="msbuild.exe $projFile.vcxproj -p:Platform=x64 -maxCpuCount -nologo -v:q /clp:ErrorsOnly -p:Configuration"
	echo buildWindows $baseCmd $file release
	buildWindows2 "$baseCmd" $file release;
	buildWindows2 "$baseCmd" $file debug;
	echo build $projFile complete.
}

function createProto
{
	dir=$1;
	file=$2;
	export=$3;
	cleanProtoc=$clean;
	pushd `pwd` > /dev/null;
	cd $dir;
	if [ ! -f file.pb.cc ]; then
		cleanProtoc=1;
	fi;
	if [ $cleanProtoc -eq 1 ]; then
		protoc --cpp_out . $file.proto;
		if [ $? -ne 0 ]; then exit 1; fi;
	fi;
	popd > /dev/null;
}

function fetchDefault
{
	cd $baseDir/$jdeRoot;
	fetch $1;
	return $?;
}
function build
{
	if windows; then
		buildWindows $1 $3
	else
		buildLinux $2
	fi;
}
function buildTest
{
	if windows; then
		MSBuild.exe -t:restore -p:RestorePackagesConfig=true
		if [[ ! -f "Tests.$dir.vcxproj.user" && -f "Tests.$dir.vcxproj._user" ]]; then
			linkFile Tests.$dir.vcxproj._user Tests.$dir.vcxproj.user;	if [ $? -ne 0 ]; then echo `pwd`; echo FAILED:  linkFile Tests.$dir.vcxproj._user Tests.$dir.vcxproj.user; exit 1; fi;
		fi;

		buildWindows $1 $2
	else
		buildLinux 1
	fi;
}
function fetchBuild
{
	fetchDefault $1;
	#echo fetchDefault $1 finished;
	build $1 $2 $3;
}
function findProtoc
{
	if windows; then
		findExecutable protoc.exe $JDE_BASH/Public/stage/release;
	fi;
}
popd > /dev/null;