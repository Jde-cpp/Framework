baseDir=`pwd`;
jdeRoot=jde;
if [[ -z "$REPO_DIR" ]]; then
	export REPO_DIR=`pwd`;
fi;
cd $REPO_DIR
#echo REPO_DIR=$REPO_DIR

function findExecutable
{
	exe=$1;
	echo exe=$1;
	defaultPath=$2;
	exitFailure=${3:-1};
	echo defaultPath=$2;
	echo path_to_exe='$(which $exe 2> /dev/null)';
	path_to_exe=$(which "$exe" 2> /dev/null);
	if [ ! -x "$path_to_exe" ]; then
		echo 'PATH=$PATH:$defaultPath;'
     	PATH=$PATH:$defaultPath;
		path_to_exe=$(which "$exe" 2> /dev/null);
		echo $path_to_exe;
		echo 'if [ ! -x "'$path_to_exe'" ]; then echo no; else echo yes; fi;'
		if [ ! -x "$path_to_exe" ]; then
			echo can not find $defaultPath/$exe;
			if [ $exitFailure -eq 1 ]; then
				exit 1;
			fi;
		fi;
	fi;
}

if windows; then
	findExecutable MSBuild.exe '/c/Program\ Files\ \(X86\)/Microsoft\ Visual\ Studio/2019/BuildTools/MSBuild/Current/Bin' 0
	findExecutable MSBuild.exe '/c/Program\ Files\ \(X86\)/Microsoft\ Visual\ Studio/2019/Enterprise/MSBuild/Current/Bin' 1
fi;


function fetch
{
	if [ ! -d $1 ]; then
		echo calling git clone $1
		git clone https://github.com/Jde-cpp/$1.git -q; cd $1;
	else
		cd $1; git pull -q;
	fi;
	cd source/;
}

function buildConfig
{
	config=$1;
	configClean=${2:-0};
	if [ ! -f .obj/$config/Makefile ]; then
		echo no `pwd`/.obj/$config/Makefile configClean=1;
		configClean=1;
	fi;
	echo ../../Framework/cmake/buildc.sh `pwd` $config $configClean;
	../../Framework/cmake/buildc.sh `pwd` $config $configClean;
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
	   ./buildc.sh release $projectClean 0;
	else
		buildConfig asan $projectClean
		buildConfig release $projectClean
	fi;
}
function buildWindows
{
	dir=${1};
	if [ ! -f "$dir.vcxproj.user" ]; then
		echo cp $dir.vcxproj._user $dir.vcxproj.user
		cp $dir.vcxproj._user $dir.vcxproj.user
	fi;
	if [ $clean -eq 1 ]; then
		rm -r -f .bin;
	fi;
	if [ $dir != "Framework" ]; then
		mkdir .bin/debug -p
		mkdir .bin/release -p
		cp ../../Framework/source/.bin/debug/Jde.lib .bin/debug
		cp ../../Framework/source/.bin/release/Jde.lib .bin/release
	fi;
	msbuild.exe $dir.vcxproj -p:Configuration=Release -p:Platform=x64 -maxCpuCount -nologo -v:q
	if [ $? -ne 0 ]; then
		echo `pwd`;
		echo msbuild.exe $dir.vcxproj -p:Configuration=Release -p:Platform=x64 -maxCpuCount -nologo -v:q
		exit 1;
	fi;
	msbuild.exe $dir.vcxproj -p:Configuration=Debug -p:Platform=x64 -maxCpuCount -nologo -v:q
	if [ $? -ne 0 ]; then
		echo `pwd`;
		echo msbuild.exe $dir.vcxproj -p:Configuration=Debug -p:Platform=x64 -maxCpuCount -nologo -v:q
		exit 1;
	fi;
}
function build
{
	echo 'start build'
	dir=$1;
	localSH=$2;
	proto=$3;
	echo build $dir $localSH
	cd $baseDir/$jdeRoot;
	echo `pwd`
	fetch $dir
	if [[ ! -z "$proto" ]]; then
	   `$proto`;
   	fi;
	if windows; then
		buildWindows $dir
	else
		buildLinux $localSH
	fi;
}
