#!/bin/bash
#
#decrypt file each hour
#0 * * * * /home/liuzhen/decrypt.sh
#
HOME_AESFile=/home/liuzhen
BACKUP_AESFile=$HOME_AESFile/backup

JAVA_CMD=//home/jdk1.8.0_91/bin/java
CLASSNAME=test.AES

UNZIP_CMD=/usr/bin/unzip
MKDIR_CMD=/bin/mkdir
RM_CMD=/bin/rm
MV_CMD=/bin/mv
CMD_DATE=/bin/date
CMD_STAT=/usr/bin/stat

CUR_TIME=`$CMD_DATE '+%s'`

cd $HOME_AESFile
if [ ! -d "$BACKUP_AESFile" ]; then 
	$MKDIR_CMD "$BACKUP_AESFile"
fi 
#check new AES file list
AESFilelist=`ls *.zip\(AES\)`

#loop descrypt aesFile
for aesFile in $AESFilelist
do
	modifyTime_aes=`$CMD_STAT -c %Y $aesFile`
	if [ $[ $CUR_TIME - $modifyTime_aes ] -gt 10 ];then
		deFile=${aesFile%\(AES\)}
		echo descrypt $HOME_AESFile/$aesFile to $HOME_AESFile/$deFile
		$JAVA_CMD $CLASSNAME $HOME_AESFile/$aesFile $HOME_AESFile/$deFile
		sleep 1
	
		echo "$UNZIP_CMD $deFile & $RM_CMD $deFile"
		$UNZIP_CMD $deFile
		$RM_CMD $deFile
		sleep 1
	
		echo $MV_CMD $aesFile $BACKUP_AESFile/
		$MV_CMD $aesFile $BACKUP_AESFile/
		sleep 2
	fi
done


