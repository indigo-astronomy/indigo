#!/bin/bash
shopt -s nullglob

name=`pwd`
name=indigo_`basename $name`
description=`grep SET_DRIVER_INFO *.c *.cpp *.m | awk -F ',' '{ print $2 }' | awk '{$1=$1;print}'`
version=`grep SET_DRIVER_INFO *.c *.cpp *.m | awk -F "," '{ print $4 }' | awk '{$1=$1;print}'`

if [ "`echo $description | cut -c1-1`" != "\"" ]
then
	description=`grep "define $description" *.c *.cpp *.m *.h | awk '{$1="";$2=""}1'`
fi

if [ "`echo $version | cut -c1-1`" != "\"" ]
then
	version=`grep "define $version" *.c *.cpp *.m *.h | awk '{$1="";$2=""}1'`
fi

version=`echo $version | cut -c3-7`

echo \"$name\", $description, $version
