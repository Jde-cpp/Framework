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
fetch=${2:-1};
#windows() { [[ -n "$WINDIR" ]]; }

# if [ ! -f source-build.sh ]; then
# 	echo no source;
# 	if [ -f jde/Framework/source-build.sh ]; then
# 		echo source found;
# 		if windows; then
# 			file=source-build.sh;
# 			dest=`pwd`;
# 			dest=${dest////\\};
# 			dest=${dest/\\c/c:};
# 			cmd <<< "mklink $dest\\$file $dest\\jde\\Framework\\$file" > /dev/null;
# 		else
# 			ln -s jde/Framework/source-build.sh .;
# 		fi;
# 	else
# 		curl https://raw.githubusercontent.com/Jde-cpp/Framework/master/source-build.sh -o source-build.sh
# 	fi;
# fi
source ./common.sh
source ./source-build.sh


path_to_exe=$(which vcpkg.exe 2> /dev/null);
if [ ! -x "$path_to_exe" ] ; then
	if [ ! -d vcpkg ]; then
		git clone https://github.com/microsoft/vcpkg;
		./vcpkg/bootstrap-vcpkg.bat;
	fi;
	findExecutable vcpkg.exe `pwd`/vcpkg
fi;
function protocBuild
{
	type="sln";
	if [ ! -d $type ]; then
		mkdir $type; cd $type;
		#subDir=$([ "$type" = "Debug" ] && echo "/$type" || echo "");
		#cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=$type -Dprotobuf_WITH_ZLIB=ON -DZLIB_INCLUDE_DIR=$REPO_DIR/vcpkg/installed/x64-windows$subDir/include -DZLIB_LIB=$REPO_DIR/vcpkg/installed/x64-windows$subDir/lib ../..
		#cmake -G "Visual Studio 16 2019" -DCMAKE_BUILD_TYPE=$type -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_WITH_ZLIB=ON -DZLIB_INCLUDE_DIR=$REPO_DIR/vcpkg/installed/x64-windows/include -DZLIB_LIB=$REPO_DIR/vcpkg/installed/x64-windows$subDir/lib ../..
		cmake -G "Visual Studio 16 2019" -DCMAKE_BUILD_TYPE=Release -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_WITH_ZLIB=ON -DZLIB_INCLUDE_DIR=$REPO_DIR/vcpkg/installed/x64-windows/include -DZLIB_LIB=$REPO_DIR/vcpkg/installed/x64-windows/lib -Dprotobuf_BUILD_SHARED_LIBS=ON ../..
	else 
		cd $type; 
	fi;
	#nmake;
	msbuild.exe libprotobuf.vcxproj -p:Configuration=Debug -p:Platform=x64 -maxCpuCount -nologo -v:q
	msbuild.exe libprotobuf.vcxproj -p:Configuration=Release -p:Platform=x64 -maxCpuCount -nologo -v:q
	msbuild.exe protoc.vcxproj -p:Configuration=Release -p:Platform=x64 -maxCpuCount -nologo -v:q
}
 
if [ $fetch -eq 1 ]; then
	vcpkg.exe install nlohmann-json --triplet x64-windows
	vcpkg.exe install fmt --triplet x64-windows
	vcpkg.exe install spdlog --triplet x64-windows
	vcpkg.exe install boost --triplet x64-windows
	#vcpkg.exe install protobuf[zlib]:x64-windows ./configure --disable-shared
fi;
buildProto=$fetch
if [ ! -d protobuf ]; then  git clone https://github.com/Jde-cpp/protobuf.git; buildProto=1; fi;
if [ $buildProto -eq 1 ]; then
	echo building protoc clean=$clean
	cd protobuf;
	if [ $fetch ]; then git pull; fi;
	cd cmake;
	if [ $clean ]; then rm -r -f build; fi;
	moveToDir build;
	protocBuild;
	#protocBuild Release;
fi;
#protobufInclude=`pwd`/vcpkg/installed/x64-windows/include
#findExecutable protoc.exe `pwd`/vcpkg/installed/x64-windows/tools/protobuf 1
toBashDir $REPO_DIR REPO_BASH;
protobufInclude=$REPO_DIR/protobuf/src;
findExecutable protoc.exe $REPO_BASH/protobuf/cmake/build/sln/Release;

if [ ! -d $jdeRoot ]; then mkdir $jdeRoot; fi;
cd $jdeRoot
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
build Framework 0 'frameworkProtoc'
