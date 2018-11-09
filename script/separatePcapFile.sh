#!/bin/bash

TUNIP_ASSIGN_FILE=/home/rywang/vpnserver/proc/tunip.assign
PCAPFILE_ROOT=/home/rywang/vpnserver/pcap_data
CMD_TSHARK=/usr/bin/tshark
CMD_MKDIR=/bin/mkdir
CMD_LS=/bin/ls
CMD_AWK=/usr/bin/awk
CMD_RM=/bin/rm
CMD_RMDIR=/bin/rmdir
CMD_ECHO=/bin/echo
CMD_FIND=/usr/bin/find
CMD_CAT=/bin/cat
CMD_GREP=/bin/grep

cd $PCAPFILE_ROOT
PCAPFILE_LIST=`$CMD_LS -t *.pcap 2>>/dev/null`
#read tunip.assign file then create deviceid directory
for line_tunip in `$CMD_CAT $TUNIP_ASSIGN_FILE`
do
	#echo $line_tunip
	find_numberSign=`$CMD_ECHO $line_tunip | $CMD_GREP '#'`
	if [ "$find_numberSign" == "" ]
	then
		find_equalsSign=`$CMD_ECHO $line_tunip | $CMD_GREP '='`
		deviceid=''
		if [ "$find_equalsSign" = "" ] 
		then
			//not contains =,skip
			continue
		else
			#set Internal Field Separator,first save original IFS
			OLD_IFS="$IFS"
			IFS="="
			field=($line_tunip)	
			tunip=${field[0]}
			deviceid=${field[1]}
			#restore IFS
			IFS="$OLD_IFS"
		fi
	else
		#contain #,skip
		#echo "skip comment line:$line_tunip"
		continue
	fi

	if [ ! -d $deviceid ]; then
		$CMD_ECHO $CMD_MKDIR $deviceid
		$CMD_MKDIR $deviceid
	fi
	index=0
	for ORIG_FILE in $PCAPFILE_LIST
	do
		if [ $index -eq 0 ]
		then
			#skip the first file(the newest file)
			index=1
			continue
		fi	
		#tshark -r 1_00001_20171012232310.pcap -Y 'ip.addr==10.77.0.3' -w tt2.pcap -F pcap
		$CMD_ECHO $CMD_TSHARK -r $ORIG_FILE -Y ip.addr==$tunip -w $deviceid/$ORIG_FILE -F pcap
		$CMD_TSHARK -r $ORIG_FILE -Y ip.addr==$tunip -w $deviceid/$ORIG_FILE -F pcap
		filesize=`$CMD_LS -l $deviceid/$ORIG_FILE 2>>/dev/null | $CMD_AWK '{ print $5}'`
		#if .pcap file is empty then delete it
		if [ $filesize -le 24 ]
		then
			$CMD_RM $deviceid/$ORIG_FILE
		fi
	done

done

#if false;then
#	block comment
#fi

#delete all original .pcap file except the newest .pcap file
#ls -alrt *.pcap
index=0
for ORIG_FILE in $PCAPFILE_LIST
do
	if [ $index -eq 0 ]
	then
		#skip the first file(the newest file)
		index=1
		#echo skip $ORIG_FILE
	else
		$CMD_RM $ORIG_FILE
		#echo $CMD_RM $ORIG_FILE
	fi
done

#delete empty directory
do
    for deviceid in `$CMD_FIND $PCAPFILE_ROOT -type d`
    do
        if [ 0 -eq `$CMD_FIND $deviceid -type f 2>/dev/null | wc -l` ]
        then
            $CMD_RMDIR $deviceid
        fi
    done
done