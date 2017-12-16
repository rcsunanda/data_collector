#!/bin/bash

#change limits
ulimit -c unlimited		#for core file generation
ulimit -n 12000			#file descriptor limit

count=0

while true
do
	#useful variables
	dt=$(date '+%d/%m/%Y %H:%M:%S');
	dt_append=$(date '+%d_%m_%Y');
	count=$((count+1))
	backup_directory="../backup-$dt_append-$count"
	
	#run program
	echo "Starting data_recorder program on $dt (count = $count)"
	./data_recorder configuration.txt
	
	retValue=$?
	echo "data_recorder program exited on $dt (count = $count, return value = $retValue)"
	
	#create backup directory and move files if error occurred after startup
	if [ $retValue -ne 10 ];
	then
		echo "Moving log and core files to $backup_directory"
		mkdir $backup_directory
		mv *.log $backup_directory
		mv core* $backup_directory
		mv nohup* $backup_directory
	fi
	

	#sleep 5 minutes for port to be ready
	sleep 300
done
