#/bin/bash

usage() {
	echo "example:"
	echo "    $0 start"
	echo "    $0 stop"
	echo "    $0 status"
}

if [ ! $# -eq 1 ];then
	usage
	exit 0
fi

MOBILEGT_HOME="/home/rywang/vpnserver"	
DATA_HOME="$MOBILEGT_HOME/pcap_data"
TUN_IF_NAME="tun0"
#duration:value switch to the next file after value seconds have elapsed, even if the current file is not completely filled up.
#filesize:value switch to the next file after it reaches a size of value kB. 
#Note that the filesize is limited to a maximum value of 2 GiB.
#5minute=300second;15minute=900second;30minute=1800second;1hour=60minute=3600second
#DURA_SEC=300
DURA_SEC=1800
#DURA_SEC=60
#1M=1000kB;200M=200000kB
FILESIZE_KB=200000
DATA_DIR=$DATA_HOME
LOG_DIR="$MOBILEGT_HOME/log"
	
SCRIPT_SRC_DIR="/home/rywang/.netbeans/remote/222.16.4.76/lenovo-pc-Windows-x86_64/D/GitHub/mobilegt_vpn"
PROG_SRC_DIR="/home/rywang/.netbeans/remote/222.16.4.76/lenovo-pc-Windows-x86_64/D/GitHub/mobilegt_vpn/dist/Debug/GNU-Linux"
PROG_DST_DIR="$MOBILEGT_HOME/bin"
TAG_FILE="$MOBILEGT_HOME/proc/mobilegt.tag"
KEYWORD_MOBILEGT=mobilegt_vpn

if [ ! -d $DATA_DIR ]; then
	mkdir $DATA_DIR
fi
if [ ! -d $LOG_DIR ]; then
	mkdir $LOG_DIR
fi

if [ $1 == "start" ];then
	echo "start mobilegt vpn server..."
	CMD_CP="/bin/cp"
	echo "$CMD_CP $SCRIPT_SRC_DIR/mobilegt_vpn.sh|mobilegt_preprocess.sh|stopCapture.sh|mobilegt_vpn.cfg $PROG_DST_DIR/"
	$CMD_CP $SCRIPT_SRC_DIR/mobilegt_vpn.sh $PROG_DST_DIR
	$CMD_CP $SCRIPT_SRC_DIR/mobilegt_preprocess.sh $PROG_DST_DIR
	$CMD_CP $SCRIPT_SRC_DIR/stopCapture.sh $PROG_DST_DIR/
	$CMD_CP $SCRIPT_SRC_DIR/mobilegt_vpn.cfg $PROG_DST_DIR/

	echo "$CMD_CP $PROG_SRC_DIR/mobilegt_vpn $PROG_DST_DIR/"
	$CMD_CP $PROG_SRC_DIR/mobilegt_vpn $PROG_DST_DIR/

	echo "$PROG_DST_DIR/mobilegt_vpn -f $PROG_DST_DIR/mobilegt_vpn.cfg &"
	$PROG_DST_DIR/mobilegt_vpn -f $PROG_DST_DIR/mobilegt_vpn.cfg &
	
	echo "start mobilegt vpn server completed."

	echo "start capture interface:$TUN_IF_NAME packet...... .pcap data in $DATA_DIR"
	#dumpcap -i $TUN_IF_NAME -b duration:$DURA_SEC -P -w $DATA_DIR/1.pcap
	dumpcap -i $TUN_IF_NAME -b filesize:$FILESIZE_KB -b duration:$DURA_SEC -P -w $DATA_DIR/1.pcap &
	echo "start capture packet completed."
	echo

elif [ $1 == "stop" ];then
	echo "stop mobilegt vpn server..."
	tunnel_IP="222.16.4.76"
	tunnel_PORT="8000"	
	CMD_NC="/bin/nc"
	CMD_PING="/bin/ping"
	echo 0 > $TAG_FILE
	sleep 3
	$CMD_NC -vuz $tunnel_IP $tunnel_PORT
	sleep 3
	$CMD_PING -c 1 -W 1 -I $TUN_IF_NAME $tunnel_IP
	sleep 3
	PID=`ps -ef | grep $KEYWORD_MOBILEGT | grep -v 'grep' | awk '{print $2}'`
	if [ -z "$PID" ];then
		echo "    NO running mobilegt vpn server."
	else
		echo "    kill PID:$PID"
        kill -9 $PID
	fi
	echo "STOP mobilegt vpn server completed."
	
	echo "STOP capture interface:$TUN_IF_NAME packet......"
	$PROG_DST_DIR/stopCapture.sh $TUN_IF_NAME
	echo "STOP capture packet process completed."
	echo
elif [ $1 == "status" ];then
	PID=`ps -ef | grep $KEYWORD_MOBILEGT | grep -v "grep" | awk '{print $2}'`
	if [ -z "$PID" ];then
		echo "NO $KEYWORD_MOBILEGT running."
		echo
	else

		echo "$KEYWORD_MOBILEGT is running."
		echo
        ps -ef | grep $KEYWORD_MOBILEGT | grep -v "grep"
		echo
	fi 
else
	usage
fi

