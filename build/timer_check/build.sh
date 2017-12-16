#!/bin/bash

if [ "$1" == cmake ]; then
    #Delete all files except the compile script
    find ! -name 'build.sh' ! -name 'configuration.txt' ! -name 'eProData*' ! -name '*.log' ! -name '*.sh' ! -name 'core*' ! -name 'nohup*' -type f -exec rm -f {} +
    cmake ../../timer_check/ -DCMAKE_BUILD_TYPE=Debug
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
