clean=${1:-0};
fetch=${2:-1};
set disable-completion on;
windows() { [[ -n "$WINDIR" ]]; }

source ./source-build.sh


path_to_exe=$(which vcpkg.exe 2> /dev/null);
if [ ! -x "$path_to_exe" ] ; then
	if [ ! -d vcpkg ]; then
		git clone https://github.com/microsoft/vcpkg;
		./vcpkg/bootstrap-vcpkg.bat;
	fi;
	findExecutable vcpkg.exe `pwd`/vcpkg
fi;
if [ $fetch -eq 1 ]; then
	vcpkg.exe install nlohmann-json --triplet x64-windows
	vcpkg.exe install fmt --triplet x64-windows
	vcpkg.exe install spdlog --triplet x64-windows
	vcpkg.exe install boost --triplet x64-windows

	vcpkg.exe install protobuf[zlib]:x64-windows
fi;
findExecutable protoc.exe `pwd`/vcpkg/installed/x64-windows/tools/protobuf
protobufInclude=`pwd`/vcpkg/installed/x64-windows/include

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
