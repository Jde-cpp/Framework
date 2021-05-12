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
#source ./common.sh;
if [[ -z $sourceBuild ]]; then source source-build.sh; fi;
echo framework-build.sh clean=$clean shouldFetch=$shouldFetch pwd=`pwd`;

path_to_exe=$(which vcpkg.exe 2> /dev/null);
if [ ! -x "$path_to_exe" ] ; then
	if [ ! -d vcpkg ]; then
		git clone https://github.com/microsoft/vcpkg;
		./vcpkg/bootstrap-vcpkg.bat;
	fi;
	findExecutable vcpkg.exe `pwd`/vcpkg
fi;
if [ $shouldFetch -eq 1 ]; then
	vcpkg.exe install nlohmann-json --triplet x64-windows
	vcpkg.exe install fmt --triplet x64-windows
	vcpkg.exe install spdlog --triplet x64-windows
	vcpkg.exe install boost --triplet x64-windows
	#vcpkg.exe install protobuf[zlib]:x64-windows ./configure --disable-shared
fi;
#buildProto=$shouldFetch
buildProto=1
if [ ! -d protobuf ]; then  git clone https://github.com/Jde-cpp/protobuf.git; buildProto=1; fi;
function protocBuild
{
	type=sln;#lib;
	sharedLibs=$(if [ $type = "sln" ]; then echo "ON"; else echo "OFF"; fi)
	if [ ! -d "`pwd`/$type" ]; then
		mkdir $type; cd $type;
		#subDir=$([ "$type" = "Debug" ] && echo "/$type" || echo "");
		#cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=$type -Dprotobuf_WITH_ZLIB=ON -DZLIB_INCLUDE_DIR=$REPO_DIR/vcpkg/installed/x64-windows$subDir/include -DZLIB_LIB=$REPO_DIR/vcpkg/installed/x64-windows$subDir/lib ../..
		#cmake -G "Visual Studio 16 2019" -DCMAKE_BUILD_TYPE=$type -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_WITH_ZLIB=ON -DZLIB_INCLUDE_DIR=$REPO_DIR/vcpkg/installed/x64-windows/include -DZLIB_LIB=$REPO_DIR/vcpkg/installed/x64-windows$subDir/lib ../..
		#-DCMAKE_BUILD_TYPE=Release 
		cmake -G "Visual Studio 16 2019" -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_WITH_ZLIB=ON -DZLIB_INCLUDE_DIR=$REPO_DIR/vcpkg/installed/x64-windows/include -DZLIB_LIB=$REPO_DIR/vcpkg/installed/x64-windows/lib -Dprotobuf_MSVC_STATIC_RUNTIME=OFF -Dprotobuf_BUILD_SHARED_LIBS=$sharedLibs ../..
	else 
		cd $type; 
	fi;
	#nmake;
	#if [ ! -f Debug/libprotobufd.lib ]; then 
		baseCmd="msbuild.exe libprotobuf.vcxproj -p:Platform=x64 -maxCpuCount -nologo -v:q /clp:ErrorsOnly -p:Configuration"
		buildWindows2 "$baseCmd" libprotobuf.lib release;
		#cp Release/libprotobuf.lib $REPO_BASH/jde/Public/stage/Release/libprotobufd.lib;
		buildWindows2 "$baseCmd" libprotobufd.lib debug;
		#cp Debug/libprotobufd.lib $REPO_BASH/jde/Public/stage/Debug/libprotobufd.lib;
		buildWindows2 "msbuild.exe protoc.vcxproj -p:Platform=x64 -maxCpuCount -nologo -v:q /clp:ErrorsOnly -p:Configuration" protoc.exe release;
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
if [ $buildProto -eq 1 ]; then
	echo building protoc clean=$clean
	cd protobuf;
	if [ $shouldFetch -eq 1 ]; then git pull; fi;
	cd cmake;
	if [ $clean -eq 1 ]; then echo rm build; rm -r -f build; fi;
	moveToDir build;
	protocBuild;
	#protocBuild Release;
fi;

#protobufInclude=`pwd`/vcpkg/installed/x64-windows/include
#findExecutable protoc.exe `pwd`/vcpkg/installed/x64-windows/tools/protobuf 1
toBashDir $REPO_DIR REPO_BASH;
protobufInclude=$REPO_DIR/protobuf/src;
#findExecutable protoc.exe $REPO_BASH/protobuf/cmake/build/sln/Release;
findExecutable protoc.exe $REPO_BASH/jde/Public/stage/release;

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
fetchDefault Framework;
frameworkProtoc;
#echo "Building Framework";
build Framework 0;