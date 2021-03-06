#!/bin/bash
windows() { [[ -n "$WINDIR" ]]; }
t=$(readlink -f "${BASH_SOURCE[0]}"); commonBuild=$(basename "$t"); unset t;

function toBashDir
{
	windowsDir=$1;
   local -n _bashDir=$2
   _bashDir=${windowsDir/:/}; _bashDir=${_bashDir//\\//}; _bashDir=${_bashDir/C/c};
	if [[ ${_bashDir:0:1} != "/" ]]; then _bashDir=/$_bashDir; fi;
}
if windows; then
	toBashDir $REPO_DIR REPO_BASH;
else
	REPO_BASH=$REPO_DIR;
fi;

function moveToDir
{
	if [ ! -d $1 ]; then mkdir $1; fi;
	cd $1;
};
function toWinDir
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

function addHard
{
	local file=$1;#TwsSocketClient64.vcxproj
	local fetchLocation=$2;
	if [ -f $file ]; then rm $file; fi;
	if windows; then
		toWinDir "$fetchLocation" _source;
		toWinDir "`pwd`" _destination;
		cmd <<< "mklink /H \"$_destination\\$file\" \"$_source\\$file\" " > /dev/null; #"
		if [ $? -ne 0 ]; then
			echo `pwd`;
			echo cmd <<< "mklink \"$_destination\\$file\" \"$_source\\$file\" "; #"
			exit 1;
		fi;
	else
		ln $fetchLocation/$file .;
	fi;
};

function addHardDir
{
	#echo addHardDir;
	local dir=$1;
	local sourceDir=$2/$1;
	moveToDir $dir;
	#echo $sourceDir/$dir;
	for filename in $sourceDir/*; do
		#echo $filename;
		if [ -f $filename ]; then addHard $(basename "$filename") $sourceDir;
		elif [ -d $filename ]; then addHardDir $(basename "$filename") $sourceDir; fi;
	done;
	cd ..;
}


function mklink
{
	local file=$1;#TwsSocketClient64.vcxproj
	local fetchLocation=$2;
	if [ -f $file ]; then rm $file; fi;
	if windows; then
		toWinDir "$fetchLocation" _source;
		if [ ! -f $_source ]; then echo $_source not found; exit 1; fi;
		toWinDir "`pwd`" _destination;
		cmd <<< "mklink \"$_destination\\$file\" \"$_source\\$file\" " > /dev/null;  #"
		if [ $? -ne 0 ]; then
			echo `pwd`;
			echo cmd <<< "mklink \"$_destination\\$file\" \"$_source\\$file\" "; #"
			exit 1;
		fi;
	else
 	if [ -L $file ]; then rm $file; fi;
		ln -s $fetchLocation/$file .;
	fi;
}
function linkFileAbs
{
	local original=$1;#TwsSocketClient64.vcxproj._user
	local link=$2; #TwsSocketClient64.vcxproj.user
	if [ -f $link ]; then exit 0; fi;
	if windows; then
		# echo Hi;
		toWinDir $original originalWin;
		toWinDir $link linkWin;
		#echo cmd  "mklink $linkWin $originalWin "; #
		cmd <<< "mklink $linkWin $originalWin " > /dev/null;  #need to send in quoted, linkFile is already quoting.
		#echo if [ ! -f $link ];
		#if [ ! -f $link ]; then echo file not found; echo `pwd`; echo cmd  "linkFileAbs $original $link ";  exit 1; fi; #"
		#echo success;
	else
		if [ -L $file ]; then rm $file; fi;
		ln -s $original $link;
	fi;
}

function linkFile
{
	local original=$1;#TwsSocketClient64.vcxproj._user
	local link=$2; #TwsSocketClient64.vcxproj.user
	if [ -f $link ]; then exit 0; fi;
	if windows; then
		toWinDir "`pwd`" _pwd;
		linkFileAbs \"`pwd`/$original\" \"`pwd`/$link\" #"
		if [ ! -f "$link" ]; then echo file not found; echo `pwd`; echo linkFileAbs \"`pwd`/$original\" \"`pwd`/$link\";  exit 1; fi; #"
		#if [ $? -ne 0 ]; then exit 1; fi;
	else
 		if [ -L $file ]; then rm $file; fi;
		ln -s $original $link;
	fi;
}
