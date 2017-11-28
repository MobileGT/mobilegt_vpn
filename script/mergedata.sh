#!/bin/bash

#VPNSERVER=amazon
#VPNSERVER=mazon
VPNSERVER=Alibaba
OWNER=liuzhen
GROUPER=liuzhen
TARGET_DIR=/home/liuzhen/mergedata
SOURCE_SOCKET=/home/liuzhen/files
SOURCE_PCAP=/home/ubuntu/vpnserver/pcap_data
CMD_CHOWN=/bin/chown
CMD_LS=/bin/ls
CMD_MKDIR=/bin/mkdir
CMD_DATE=/bin/date
CMD_MV=/bin/mv
CMD_STAT=/usr/bin/stat
CMD_ECHO="/bin/echo"
WAIT_SECOND=5

BEGIN_FLAG=1
if [ ! -d $TARGET_DIR ]; then
	$CMD_ECHO $CMD_MKDIR $TARGET_DIR
	$CMD_MKDIR $TARGET_DIR
fi

#FILENAME=810EBLS28TDL20160625213910.socket
#20160625213910.socket  ------ 21
#DEVICEID=810EBLS28TDL
#2016.10.10 modify
#FILENAME=Alibaba_810EBLS28TDL20161010161947.socket
#FILENAME=amazon_94346c3720161010175501.socket
#FILENAME=VPNHOST _ DEVICEID YEAR MM DD HH MM SS
CUR_TIME=`TZ='Asia/Shanghai' $CMD_DATE '+%s'`
cd $SOURCE_SOCKET
SOCKETFileList=`$CMD_LS $VPNSERVER\_*.socket 2>>/dev/null`
for socketFile in $SOCKETFileList
do
	if [ BEGIN_FLAG ]; then
		$CMD_ECHO ================
		$CMD_ECHO `TZ='Asia/Shanghai' $CMD_DATE`
		BEGIN_FLAG=0
	fi
	LEN=`expr ${#socketFile} - 21`
	#echo LEN:$LEN
	VPNHOST_DEVICEID=${socketFile:0:$LEN}
	DEVICEID=${VPNHOST_DEVICEID#*_}
	#VPNHOST=${VPNHOST_DEVICEID%_*}
	DATE=${socketFile:$LEN:8}
	if [ ! -d $TARGET_DIR/$DEVICEID/$DATE ]; then
		$CMD_ECHO $CMD_MKDIR -p $TARGET_DIR/$DEVICEID/$DATE
		$CMD_MKDIR -p $TARGET_DIR/$DEVICEID/$DATE
	fi

	modifyTime=`$CMD_STAT -c %Y $socketFile`
	#if modifyTime > CUR_TIME $WAIT_SECOND second then move file
	#echo $[ $CUR_TIME - $modifyTime ]
	#echo $WAIT_SECNOD
	if [ $[ $CUR_TIME - $modifyTime ] -gt $WAIT_SECOND ];then 
		#echo $[ $CUR_TIME - $modifyTime ]
		$CMD_ECHO $CMD_MV $socketFile $TARGET_DIR/$DEVICEID/
		$CMD_MV $socketFile $TARGET_DIR/$DEVICEID/$DATE
	fi
	#/home/ubuntu/vpnserver/pcap_data/44cd4ccc/1_00020_20160821175323.pcap
	PCAPFileList=`$CMD_LS $SOURCE_PCAP/$DEVICEID/*_$DATE*.pcap 2>>/dev/null`
	for pcapFile in $PCAPFileList
	do
		modifyTime_pcap=`$CMD_STAT -c %Y $pcapFile`
		if [ $[ $CUR_TIME - $modifyTime_pcap ] -gt $WAIT_SECOND ];then
              	$CMD_ECHO $CMD_MV $pcapFile $TARGET_DIR/$DEVICEID/
              	$CMD_MV $pcapFile $TARGET_DIR/$DEVICEID/$DATE
	    fi
	done
done

#echo $CMD_CHOWN -R $OWNER:$GROUPER $TARGET_DIR
$CMD_CHOWN -R $OWNER:$GROUPER $TARGET_DIR
