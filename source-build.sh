baseDir=`pwd`;
jdeRoot=jde;
source common.sh

if [[ -z "$REPO_DIR" ]]; then
	export REPO_DIR=`pwd`;
fi;
cd $REPO_DIR
#echo REPO_DIR=$REPO_DIR

function findExecutable
{
	exe=$1;
	#echo exe=$1;
	defaultPath=$2;
	exitFailure=${3:-1};
	#echo defaultPath=$2;
	#echo path_to_exe='$(which $exe 2> /dev/null)';
	path_to_exe=$(which "$exe" 2> /dev/null);
	if [ ! -x "$path_to_exe" ]; then
		if  [[ -x "${defaultPath//\\}/$exe" ]]; then
			echo found $exe adding to path;
			#echo 'PATH=$defaultPath:$PATH'
     		PATH=${defaultPath//\\}:$PATH;
			#path_to_exe=$(which "$exe" 2> /dev/null);
			#echo path_to_exe=$path_to_exe;pop
			#echo 'if [ ! -x "'$path_to_exe'" ]; then echo no; else echo yes; fi;'
		else 
			#echo can not find $exe;
			if [ $exitFailure -eq 1 ]; then
				echo can not find "${defaultPath//\\}/$exe";
				exit 1;
			fi;
		fi;
	fi;
}

if windows; then
	findExecutable MSBuild.exe '/c/Program\ Files\ \(X86\)/Microsoft\ Visual\ Studio/2019/BuildTools/MSBuild/Current/Bin' 0
	findExecutable MSBuild.exe '/c/Program\ Files\ \(X86\)/Microsoft\ Visual\ Studio/2019/Enterprise/MSBuild/Current/Bin' 1
	#findExecutable protoc.exe `pwd`/vcpkg/installed/x64-windows/tools/protobuf 1
	findExecutable cmake.exe
	findExecutable cl.exe '/c/Program\ Files\ \(X86\)/Microsoft\ Visual\ Studio/2019/BuildTools/VC/Tools/MSVC/14.28.29910/bin/Hostx64/x64' 0
	findExecutable cl.exe '/c/Program\ Files\ \(X86\)/Microsoft\ Visual\ Studio/2019/Enterprise/VC/Tools/MSVC/14.28.29910/bin/Hostx64/x64' 1
fi;

function fetch
{
	if [ ! -d $1 ]; then
		echo calling git clone $1
		git clone https://github.com/Jde-cpp/$1.git -q; cd $1;
	else
		cd $1; git pull -q;
	fi;
	if [ -f source ];then cd source; fi;
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
function buildWindows2
{
	#echo `pwd`;
	#echo $1
	$1
	if [ $? -ne 0 ]; then
		echo `pwd`;
		echo $1
		exit 1;
	fi;
}
function buildWindows
{
	dir=${1};
	if [[ ! -f "$dir.vcxproj.user" && -f "$dir.vcxproj._user" ]]; then
		echo cp $dir.vcxproj._user $dir.vcxproj.user
		cp $dir.vcxproj._user $dir.vcxproj.user
	fi;
	if [ $clean -eq 1 ]; then
		rm -r -f .bin;
	fi;
	if [[ $dir != "Framework" && -f ../../Framework/source/.bin/debug/Jde.lib ]]; then
		mkdir .bin/debug -p
		mkdir .bin/release -p
		cp ../../Framework/source/.bin/debug/Jde.lib .bin/debug
		cp ../../Framework/source/.bin/release/Jde.lib .bin/release
	fi;
	cmd="msbuild.exe $dir.vcxproj -p:Platform=x64 -maxCpuCount -nologo -v:q  /clp:ErrorsOnly -p:Configuration"
	buildWindows2 "$cmd=Release";
	buildWindows2 "$cmd=Debug";
	# msbuild.exe $dir.vcxproj -p:Configuration=Debug -p:Platform=x64 -maxCpuCount -nologo -v:q
	# if [ $? -ne 0 ]; then
	# 	echo `pwd`;
	# 	echo msbuild.exe $dir.vcxproj -p:Configuration=Debug -p:Platform=x64 -maxCpuCount -nologo -v:q
	# 	exit 1;
	# fi;
	echo build $dir complete.
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
	fetch $1
}
function build
{
	if windows; then 
		buildWindows $1 
	else 
		buildLinux $2 
	fi;
}
function fetchBuild
{
	proto=$3;
	fetchDefault $1 $2;
	if [[ ! -z "$proto" ]]; then
		echo calling \"$proto\";
	   (`$proto`);
	   echo finished \"$proto\";
   	fi;
	build
}
