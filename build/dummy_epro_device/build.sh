#!/bin/bash

if [ "$1" == cmake ]; then
    #Delete all files except the compile script
    find ! -name 'build.sh' ! -name 'configuration.txt' -type f -exec rm -f {} +
    cmake ../../dummy_epro_device/ -DCMAKE_BUILD_TYPE=Debug
    make clean
	make VERBOSE=1
    exit 0
fi

if [ "$1" == clean ]; then 
    make clean
    make
    #make VERBOSE=1
    exit 0
fi


if [ "$1" == make ]; then 
    make
    #make VERBOSE=1
    exit 0
fi

echo "Invalid arguments"
