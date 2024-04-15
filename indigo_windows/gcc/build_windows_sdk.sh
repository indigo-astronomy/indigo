#!/bin/bash

export SAVED_PATH=$PATH 

export VERSION=2.0-281

export PATH=/c/Qt/Tools/mingw810_32/bin/:$SAVED_PATH
export ARCH=x86

mingw32-make clean
mingw32-make bundle

export PATH=/c/Qt/Tools/mingw810_64/bin/:$SAVED_PATH
export ARCH=x64

mingw32-make clean
mingw32-make bundle
