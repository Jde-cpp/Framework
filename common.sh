#!/bin/bash
# if [ ! -z "$1" ]; then echo set file - $1; file=$1; else echo unset file; unset file; fi;
# #file=${1:-""}; #source-build.sh
# fetchLocation=${2}; #jde/Framework
# gitTarget=${3};#Jde-cpp/Framework/master
# #set disable-completion on;
# echo gitTarget=$gitTarget;
# echo fetchLocation=$fetchLocation;
# echo file=$file
windows() { [[ -n "$WINDIR" ]]; }

function moveToDir
{
	if [ ! -d $1 ]; then mkdir $1; fi;
	cd $1;
};
function toBashDir()
{
    windowsDir=$1;
    local -n _bashDir=$2
    _bashDir=/${windowsDir/:/}; _bashDir=${_bashDir//\\//}; 
}
function toWinDir()
{
    bashDir=$1;
    local -n _winDir=$2
    _winDir=${bashDir////\\}; _winDir=${_winDir/\\c/c:};
}
function fetch
{
    file2=${1};
    fetchLocation2=${2};
    gitTarget2=${3};
    if [ ! -f $file2 ]; then
        echo no $file2;
        if [ -f $fetchLocation2/$file2 ]; then
            echo $fetchLocation2/$file2 found;
            mklink $file2 $fetchLocation2
        else
            curl https://raw.githubusercontent.com/$gitTarget2/$file2 -o $file2
        fi;
    fi;
}
function mklink
{
    _file=$1;#TwsSocketClient64.vcxproj
    _fetchLocation=$2;
    rm -f $_file;
    if windows; then
        toWinDir "$_fetchLocation" source;
        toWinDir "`pwd`" destination;

        #link=`pwd`; link=${link////\\}; link=${link/\\c/c:};
        ##cmd <<< "mklink $link\\$_file ${_fetchLocation////\\}\\$_file" > /dev/null;
        #echo "mklink $destination\\$_file $source\\$_file";
        cmd <<< "mklink \"$destination\\$_file\" \"$source\\$_file\"" > /dev/null;
    else
        ln -s $_fetchLocation/$_file .;
    fi;
}