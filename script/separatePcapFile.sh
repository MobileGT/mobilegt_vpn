#!/bin/bash

TUNIP_ASSIGN_FILE=/home/rywang/vpnserver/proc/tunip.assign
PCAPFILE_ROOT=/home/rywang/vpnserver/pcap_data
CMD_TSHARK=/usr/bin/tshark
CMD_CAPINFOS=/usr/bin/capinfos
CMD_MKDIR=/bin/mkdir
CMD_LS=/bin/ls
CMD_AWK=/usr/bin/awk
CMD_RM=/bin/rm
CMD_MV=/bin/mv

echo "===================================="
echo "Begin separatePcapFile `date`"
cd $PCAPFILE_ROOT
#delete tiny file and create time 30 min ago
#if the size of the pcap file is 24,then the file is empty.
#find . -size -25c -a -name '*.pcap' -a -cmin +30 
find . -size -25c -a -name '*.pcap' -a -cmin +30 -print0 | xargs -0 /bin/rm 2>>/dev/null

PCAPFILE_LIST=`$CMD_LS -t *.pcap 2>>/dev/null`
#read tunip.assign file then create deviceid directory

declare -a tunip_array
declare -a deviceid_array
count=1
for line_tunip in `cat $TUNIP_ASSIGN_FILE`
do
	#echo $line_tunip
	find_numberSign=`echo $line_tunip | grep '#'`
	if [ "$find_numberSign" == "" ];then
		find_equalsSign=`echo $line_tunip | grep '='`
		deviceid=''
		if [ "$find_equalsSign" = "" ];then
			//not contains =,skip
			continue
		else
			OLD_IFS="$IFS"
			IFS="="
			field=($line_tunip)	
			tunip=${field[0]}
			tunip_array[$count]=$tunip
			deviceid=${field[1]}
			deviceid_array[$count]=$deviceid
			IFS="$OLD_IFS"
			count=$((count+1))
		fi
	else
		#contain #,skip
		#echo "skip comment line:$line_tunip"
		continue
	fi

	if [ ! -d $deviceid ]; then
		#echo $CMD_MKDIR $deviceid
		$CMD_MKDIR $deviceid
	fi
done

#echo ${tunip_array[*]}
#echo count:$count
index=0
#delPcap="True"
delPcap="False"
for ORIG_FILE in $PCAPFILE_LIST
do
	sleep 1
	dataresult=""
	orig_file_size=`$CMD_LS -l $ORIG_FILE | $CMD_AWK '{ print $5}'`
	if [ $index -eq 0 ]; then
		#skip the first file(the newest file)
		echo "skip $ORIG_FILE"
		index=1
		continue
	fi
	echo "process $ORIG_FILE start. `date`"
	$CMD_CAPINFOS -sc $ORIG_FILE
	cc=1
	ipFilter=""
	updateNum=0
	while [ $cc -lt $count ]
	do
		tunip=${tunip_array[$cc]}
		deviceid=${deviceid_array[$cc]}
		#tshark -r 1_00001_20171012232310.pcap -Y 'ip.addr==10.77.0.3' -w tt2.pcap -F pcap
		#echo $CMD_TSHARK -r $ORIG_FILE -Y ip.addr==$tunip -w $deviceid/$ORIG_FILE -F pcap 2>>/dev/null
		$CMD_TSHARK -r $ORIG_FILE -Y "ip.addr==$tunip" -w $deviceid/$ORIG_FILE -F pcap 2>>/dev/null
		if [ -z "$ipFilter" ]; then
			ipFilter="ip.dst!=$tunip && ip.src!=$tunip"
		else
			ipFilter="$ipFilter && ip.dst!=$tunip && ip.src!=$tunip"
		fi
		#echo "$ipFilter && ip.addr==$tunip/16"
		filesize=`$CMD_LS -l $deviceid/$ORIG_FILE 2>>/dev/null | $CMD_AWK '{ print $5}' 2>>/dev/null`
		#echo "orig_file_size:$orig_file_size----->filesize:$filesize"
		if [ -z "$filesize" ]; then
			echo "filesize is null"
		elif [ $filesize -le 24 ]; then
			$CMD_RM $deviceid/$ORIG_FILE
			echo -n .
		elif [ $filesize -ge $((orig_file_size/10)) ]; then
			echo "need update file.orig_file_size:$orig_file_size--->finshedFilesize[$((++updateNum))]:$filesize"
			#echo "$CMD_TSHARK -r $ORIG_FILE -Y \"ip.dst!=$tunip && ip.src!=$tunip && ip.addr==$tunip/16\" -w ${ORIG_FILE}_new -F pcap 2>>/dev/null"
			echo "$CMD_TSHARK -r $ORIG_FILE -Y \"$ipFilter && ip.addr==$tunip/16\" -w ${ORIG_FILE}_new -F pcap 2>>/dev/null"
			$CMD_TSHARK -r $ORIG_FILE -Y "$ipFilter && ip.addr==$tunip/16" -w ${ORIG_FILE}_new -F pcap 2>>/dev/null
			if [ ! -f ${ORIG_FILE}_bak ]; then
				echo $CMD_MV $ORIG_FILE ${ORIG_FILE}_bak
				$CMD_MV $ORIG_FILE ${ORIG_FILE}_bak
			fi
			$CMD_CAPINFOS -sc ${ORIG_FILE}_new
			#echo $CMD_MV ${ORIG_FILE}_new $ORIG_FILE
			$CMD_MV ${ORIG_FILE}_new $ORIG_FILE
			dataresult=${dataresult}"|${deviceid}-${tunip}"
		else
			echo "don't need update file.orig_file_size:$orig_file_size--->finshedFilesize:$filesize"
			dataresult=${dataresult}"|${deviceid}-${tunip}"
		fi
		cc=$((cc+1))
	done
	echo "data in:${dataresult}"
	if [ "$delPcap" == "True" ]; then
		echo $CMD_RM $ORIG_FILE ${ORIG_FILE}_bak
		$CMD_RM ${ORIG_FILE} ${ORIG_FILE}_bak
	elif [ -f ${ORIG_FILE}_bak ]; then
		echo "restore pcap file"
		$CMD_MV ${ORIG_FILE}_bak ${ORIG_FILE}
	fi
	echo "process $ORIG_FILE completed! `date`"
done

echo "End separatePcapFile. `date`"
echo "===================================="
#if false;then
#	block comment
#fi
#ls -alrt *.pcap
