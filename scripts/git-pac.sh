#!/bin/bash
pac=${1};  #jde-blockly
if [[ -z $pac ]]; then echo personal access token is first arg.; exit 1; fi;
#

function add
{
	local dir=../$1;
	if [[ ! -d $dir ]]; then echo $dir not found.; exit 1; fi;
	#echo git remote set-url origin https://jde-cpp:$pac@github.com/Jde-cpp/Blockly.git;
	cd $dir;
	git remote set-url origin https://jde-cpp:$pac@github.com/Jde-cpp/$1.git;
}
#if [[ -d ../Blockly3 ]]; then echo $dir not found.; exit 1; fi;
#add Blockly;
#add MarketLibrary;
add TwsWebSocket;
