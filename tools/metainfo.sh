#!/bin/bash
shopt -s nullglob

process=0

for f in *.c *.cpp *.m
do
	process=1
done

if [ $process -eq 1 ]
then

name=`pwd`
name=indigo_`basename $name`

if [ "$name" = "indigo_ccd_qhy" ]
then
	description="\"QHY Camera\""
else

if [ "$name" = "indigo_ccd_qhy2" ]
then
	description="\"QHY Camera2\""
else

if [ "$name" = "indigo_ccd_altair" ]
then
	description="\"AltairAstro Camera\""
else

if [ "$name" = "indigo_ccd_mallin" ]
then
	description="\"MallinCam Camera\""
else

if [ "$name" = "indigo_ccd_omegonpro" ]
then
	description="\"OmegonPro Camera\""
else

if [ "$name" = "indigo_ccd_rising" ]
then
	description="\"RisinCam Camera\""
else

if [ "$name" = "indigo_ccd_ssg" ]
then
	description="\"Orion StarShotG Camera\""
else

if [ "$name" = "indigo_ccd_ogma" ]
then
	description="\"OGMA Camera\""
else

if [ "$name" = "indigo_ccd_touptek" ]
then
	description="\"Touptek Camera\""
else
	description=`grep SET_DRIVER_INFO *.c *.cpp *.m | awk -F ',' '{ print $2 }' | awk '{$1=$1;print}'`

	if [ "`echo $description | cut -c1-1`" != "\"" ]
	then
		description=`grep "define $description" *.c *.cpp *.m *.h | awk '{$1="";$2=""}1'`
	fi
fi
fi
fi
fi
fi
fi
fi
fi
fi

version=`grep SET_DRIVER_INFO *.c *.cpp *.m | awk -F "," '{ print $4 }' | awk '{$1=$1;print}'`

if [ "`echo $version | cut -c1-1`" != "\"" ]
then
	version=`grep "define $version" *.c *.cpp *.m *.h | awk '{$1="";$2=""}1'`
fi

version=`echo $version | cut -c3-7`

echo \"$name\", $description, $version

fi
