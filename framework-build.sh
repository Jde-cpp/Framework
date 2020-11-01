clean=${1:-0};
windows=${2:-0};
fetch=${3:-1};
set disable-completion on;

baseDir=`pwd`;
jdeRoot=jde;
if [[ -z "$REPO_DIR" ]]; then
	#echo export REPO_DIR=`pwd`;
	export REPO_DIR=`pwd`;
fi;
cd $REPO_DIR
echo REPO_DIR=$REPO_DIR
#echo `pwd`
#exit 1;
function findExecutable
{
	exe=$1;
	echo exe=$1;
	defaultPath=$2;
	echo defaultPath=$2;
	echo path_to_exe='$(which $exe 2> /dev/null)';
   path_to_exe=$(which "$exe" 2> /dev/null);
   if [ ! -x "$path_to_exe" ] ; then
		echo 'PATH=$PATH:$defaultPath;'
      PATH=$PATH:$defaultPath;
		echo $PATH
		path_to_exe=$(which "$exe" 2> /dev/null);
		echo $path_to_exe;
		if [ ! -x "$path_to_exe" ] ; then
			echo can not find $defaultPath/$exe;
			exit 1;
		fi;
	fi;
}

if [ $windows -eq 1 ]; then
	findExecutable MSBuild.exe '/c/Program\ Files\ \(X86\)/Microsoft\ Visual\ Studio/2019/BuildTools/MSBuild/Current/Bin'
fi;

# if [ $windows -eq 1 ]; then
#    path_to_executable=$(which MSBuild.exe);
#    if [ ! -x "$path_to_executable" ] ; then
#       PATH=$PATH:;
#    fi;
#    path_to_executable=$(which MSBuild.exe);
#    if [ ! -x "$path_to_executable" ] ; then
# 		echo 'can not find MSBuild.exe';
# 		exit 1;
# 	fi;
# fi;
path_to_exe=$(which vcpkg.exe 2> /dev/null);
if [ ! -x "$path_to_exe" ] ; then
	if [ ! -d vcpkg ]; then
		git clone https://github.com/microsoft/vcpkg;
		./vcpkg/bootstrap-vcpkg.bat;
	fi;
	findExecutable vcpkg.exe `pwd`/vcpkg
fi;
if [ $fetch -eq 1 ]; then
	vcpkg.exe install nlohmann-json
	vcpkg.exe install spdlog
	vcpkg.exe install boost --triplet x64-windows

	vcpkg.exe install protobuf[zlib]:x64-windows
fi;
findExecutable protoc.exe `pwd`/vcpkg/installed/x64-windows/tools/protobuf
protobufInclude=`pwd`/vcpkg/installed/x64-windows/include

function fetch
{
	if [ ! -d $1 ]; then
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
	#echo configClean=$configClean
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
	   #./buildc.sh RelWithDebInfo $projectClean 0;
	else
		buildConfig asan $projectClean
		buildConfig release $projectClean
		#buildConfig RelWithDebInfo $projectClean
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
	dir=$1;
	localSH=$2;
	proto=$3;
	echo build $dir $localSH
	cd $baseDir/$jdeRoot;
	fetch $dir
	if [[ ! -z "$proto" ]]; then
	   `$proto`;
   	fi;
	if [ $windows -eq 1 ]; then
		buildWindows $dir
	else
		buildLinux $localSH
	fi;
}

if [ ! -d $jdeRoot ]; then mkdir $jdeRoot; fi;
cd $jdeRoot
if [ $windows -eq 1 ]; then
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
	   #echo frameworkProtoc running;
	   #echo pwd
	   #echo protoc --cpp_out dllexport_decl=JDE_NATIVE_VISIBILITY:. -I$protobufInclude -I. messages.proto;
	   protoc --cpp_out dllexport_decl=JDE_NATIVE_VISIBILITY:. -I$protobufInclude -I. messages.proto;
		if [ $? -ne 0 ]; then exit 1; fi;
	fi;
	cd ../../..;
}
echo 'a'
build Framework 0 'frameworkProtoc'
