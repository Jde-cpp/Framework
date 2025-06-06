#!/bin/bash
jdeDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )/../.." && pwd )"
#if ! source $jdeDir/Framework/scripts/common-error.sh; then exit 1; fi;  findExecutable false was triggering
windows() { [[ -n "$WINDIR" ]]; }

export PS4='+(${BASH_SOURCE}:${LINENO}): ${FUNCNAME[0]:+${FUNCNAME[0]}(): }'

startIndex() {
  x="${1%%$2*}"
  [[ "$x" = "$1" ]] && echo -1 || echo "${#x}"
}

# function anyDir {
# 	echo a;
# 	path=$1;
# 	echo $path
# 	index=$(startIndex "$path" /*/);
# 	if [ $index -ne -1 ]; then
# 		echo hi;
# 	fi;
# 	echo $index;
# }

function findExecutable {
	exe=$1;
	defaultPath=$2;
	exitFailure=${3:-1};
	local path_to_exe=$(which "$exe" 2> /dev/null);
	if [ ! -x "$path_to_exe" ]; then
		if  [[ -x "${defaultPath//\\}/$exe" ]]; then
     		PATH=${defaultPath//\\}:$PATH;
		else
			if [ $exitFailure -eq 1 ]; then
				echo `pwd`;
				echo common.sh:?? can not find "${defaultPath//\\}/$exe";
				exit 1;
			fi;
		fi;
	fi;
}

function toBashDir {
	windowsDir=$1;
  	local -n _bashDir=$2
  	_bashDir=${windowsDir/:/}; _bashDir=${_bashDir//\\//}; _bashDir=${_bashDir/C/c};
	if [[ ${_bashDir:0:1} != "/" ]]; then _bashDir=/$_bashDir; fi;
}
if windows; then
	toBashDir $REPO_DIR REPO_BASH;
	if [[ -z $JDE_DIR ]]; then JDE_DIR=$jdeDir; fi;
	toBashDir $JDE_DIR JDE_BASH;
else
	REPO_BASH=$REPO_DIR;
	JDE_BASH=$JDE_DIR
fi;

function moveToDir {
	if [ ! -d $1 ]; then mkdir $1; fi;
	cd $1;
};
function toWinDir {
	bashDir=$1;
	local -n _winDir=$2
	_winDir=${bashDir////\\};
	if [[ $_winDir == \\c\\* ]]; then _winDir=c:${_winDir:2};
	elif [[ $_winDir == \"\\c\\* ]]; then _winDir=\"c:${_winDir:3}; fi;
}

function fetchFile {
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

function fetch {
	fetchDir $1 $2 $shouldFetch; #if [ $? -ne 0 ]; then exit $?; fi;
	cd $2;
	if [ -d source ];then cd source; fi;
}

function fetchDir {
	local branch=${1}; local dir=${2}; local fetch=${3};
	url=$([ ! -z "$jde_token" ] && echo $jde_token@ || echo "");
	if [ ! -d $dir ]; then
		git clone -b $branch --single-branch https://"$url"github.com/Jde-cpp/$dir.git -q;
	else
		cd $dir;
		if  [[ $fetch -eq 1 ]]; then
			git pull -q; if [ $? -ne 0 ]; then echo fetch failed:  `pwd`; exit 1; fi;
			git checkout $branch -q; if [ $? -ne 0 ]; then echo checkout failed:  `pwd`; exit 1; fi;
		fi;
		cd ..;
	fi;
}

function addHard {
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

function addHardDir {
	local dir=$1;
	local sourceDir=$2/$1;
	moveToDir $dir;
	for filename in $sourceDir/*; do
		if [ -f $filename ]; then addHard $(basename "$filename") $sourceDir;
		elif [ -d $filename ]; then addHardDir $(basename "$filename") $sourceDir; fi;
	done;
	cd ..;
}


function mklink {
	local file=$1;
	local fetchLocation=$2;
	if [ -f $file ]; then rm $file; fi;
	if windows; then
		toWinDir "$fetchLocation" _source;
		if [ ! -f "$_source/$file" ]; then echo $PS4 $_source/$file not found; exit 1; fi;
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
function linkFileAbs {
	local original=$1;#TwsSocketClient64.vcxproj._user
	local link=$2; #TwsSocketClient64.vcxproj.user
	if [ -f $link ]; then exit 0; fi;
	if windows; then
		toWinDir $original originalWin;
		toWinDir $link linkWin;
		cmd <<< "mklink $linkWin $originalWin " > /dev/null;  #need to send in quoted, linkFile is already quoting.
	else
		if [ -L $file ]; then rm $file; fi;
		ln -s $original $link;
	fi;
}

function linkFile {
	local original=$1;#TwsSocketClient64.vcxproj._user
	local link=$2; #TwsSocketClient64.vcxproj.user
	if [ -f $link ]; then exit 0; fi;
	if windows; then
		toWinDir "`pwd`" _pwd;
		echo linkFileAbs start;
		linkFileAbs \"`pwd`/$original\" \"`pwd`/$link\" #"
		if [ ! -f "$link" ]; then echo file $link not found; echo `pwd`; echo linkFileAbs \"`pwd`/$original\" \"`pwd`/$link\";  exit 1; fi; #"
	else
 		if [ -L $file ]; then rm $file; fi;
		ln -s $original $link;
	fi;
}

function removeJson {
	echo start;
	local file=$1;
	local test=$2;
	local update=$3;
	echo test=${test//\"/\\\"};
	echo file=$file;
	items=$(eval jq \"${test//\"/\\\"}\" $file;)
	echo items=$items;
	if [ ! -z "$items" ]; then
		(eval "jq \"${update//\"/\\\"}\" $file") > temp.json; if [ $? -ne 0 ]; then echo `pwd`; echo jq \"${update//\"/\\\"}\" $file $file; exit 1; fi;
		mv temp.json $file;
	fi;
}

function addJson {
	echo start;
	local file=$1;
	local test=$2;
	local update=$3;
	echo test=${test//\"/\\\"};
	echo file=$file;
	items=$(eval jq \"${test//\"/\\\"}\" $file;)
	echo items=$items;
	if [ -z "$items" ]; then
		(eval "jq \"${update//\"/\\\"}\" $file") > temp.json; if [ $? -ne 0 ]; then echo `pwd`; echo jq \"${update//\"/\\\"}\" $file $file; fi;#exit 1; fi;
		mv temp.json $file;
	fi;
}