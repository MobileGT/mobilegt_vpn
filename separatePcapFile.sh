#!/bin/bash

TUNIP_ASSIGN_FILE=/home/rywang/vpnserver/proc/tunip.assign
PCAPFILE_ROOT=/home/rywang/vpnserver/pcap_data
CMD_TSHARK=/usr/bin/tshark
CMD_MKDIR=/bin/mkdir
CMD_LS=/bin/ls
CMD_AWK=/usr/bin/awk
CMD_RM=/bin/rm

cd $PCAPFILE_ROOT
#read tunip.assign file then create deviceid directory
for line_tunip in `cat $TUNIP_ASSIGN_FILE`
do
	#echo $line_tunip
	find_numberSign=`echo $line_tunip | grep '#'`
	if [ "$find_numberSign" == "" ]
	then
		find_equalsSign=`echo $line_tunip | grep '='`
		deviceid=''
		if [ "$find_equalsSign" = "" ] 
		then
			//not contains =,skip
			continue
		else
			OLD_IFS="$IFS"
			IFS="="
			field=($line_tunip)	
			tunip=${field[0]}
			deviceid=${field[1]}
			IFS="$OLD_IFS"
		fi
	else
		#contain #,skip
		#echo "skip comment line:$line_tunip"
		continue
	fi

	if [ ! -d $deviceid ]; then
		echo $CMD_MKDIR $deviceid
		$CMD_MKDIR $deviceid
	fi
	for ORIG_FILE in `ls *.pcap`
	do
		#tshark -r 1_00001_20171012232310.pcap -Y 'ip.addr==10.77.0.3' -w tt2.pcap -F pcap
		echo $CMD_TSHARK -r $ORIG_FILE -Y ip.addr==$tunip -w $deviceid/$ORIG_FILE -F pcap
		$CMD_TSHARK -r $ORIG_FILE -Y ip.addr==$tunip -w $deviceid/$ORIG_FILE -F pcap
		filesize=`$CMD_LS -l $deviceid/$ORIG_FILE | $CMD_AWK '{ print $5}'`
		if [ $filesize -le 24 ]
		then
			$CMD_RM $deviceid/$ORIG_FILE
		fi
	done
done

#if false;then
#	block comment
#fi

#ls -alrt *.pcap
index=0
for ORIG_FILE in `ls -t *.pcap`
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
