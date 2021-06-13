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
scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
if [[ -z $sourceBuild ]]; then source $scriptDir/source-build.sh; fi;
echo framework-build.sh clean=$clean shouldFetch=$shouldFetch;
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
if windows; then
	pushd `pwd` > /dev/null;
	moveToDir stage;
	moveToDir debug; cd ..;
	moveToDir release; popd  > /dev/null;
	if [ $shouldFetch -eq 1 ]; then
		vcpkg.exe install nlohmann-json --triplet x64-windows
		#vcpkg.exe install fmt --triplet x64-windows
		#vcpkg.exe install spdlog --triplet x64-windows
		vcpkg.exe install boost --triplet x64-windows
		#vcpkg.exe install protobuf[zlib]:x64-windows ./configure --disable-shared
	fi;
fi;
buildProto=$shouldFetch
buildProto=0
cd $REPO_BASH;
if [ ! -d protobuf ]; then  git clone https://github.com/Jde-cpp/protobuf.git; buildProto=1; fi;
if [ ! -d spdlog ]; then  git clone https://github.com/gabime/spdlog.git; fi;#buildSpd=1; fi;
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
	if [ $clean -eq 1 ]; then echo rm build; rm -r -f build; fi;
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

