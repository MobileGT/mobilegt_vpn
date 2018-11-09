#/bin/bash

usage() {
	echo "example:"
	echo "    $0 start"
	echo "    $0 stop"
	echo "    $0 status"
	echo "    $0 update"
}

if [ ! $# -eq 1 ];then
	usage
	exit 0
fi

SERVER_NAME=Alibaba
MOBILEGT_HOME="/home/rywang/vpnserver"
SRC_DIR="/home/rywang/.netbeans/remote/222.16.4.76/lenovo-pc-Windows-x86_64/D/GitHub"
SCRIPT_SRC_DIR="$SRC_DIR/mobilegt_vpn"
PROG_SRC_DIR="$SRC_DIR/mobilegt_vpn/dist/Debug/GNU-Linux"
TUN_IF_NAME="tun0"
stop_tunnel_IP="222.16.4.76"
stop_tunnel_PORT="8000"
DATA_HOME="$MOBILEGT_HOME/pcap_data"
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

PROG_DST_DIR="$MOBILEGT_HOME/bin"
TAG_FILE="$MOBILEGT_HOME/proc/mobilegt.tag"
KEYWORD_MOBILEGT=mobilegt_vpn

CMD_DUMPCAP="/usr/bin/dumpcap"
CMD_MKDIR="/bin/mkdir"
CMD_NC="/bin/nc"
CMD_PING="/bin/ping"
CMD_PS="/bin/ps"
CMD_GREP="/bin/grep"
CMD_AWK="/usr/bin/awk"
CMD_CP="/bin/cp"
CMD_ECHO="/bin/echo"
CMD_SLEEP="/bin/sleep"
CMD_KILL="/bin/kill"
if [ ! -d $DATA_DIR ]; then
	$CMD_MKDIR $DATA_DIR
fi
if [ ! -d $LOG_DIR ]; then
	$CMD_MKDIR $LOG_DIR
fi

if [ $1 == "start" ];then
	$CMD_ECHO "start mobilegt vpn server..."
	$CMD_ECHO "$PROG_DST_DIR/mobilegt_vpn -f $PROG_DST_DIR/mobilegt_vpn.cfg &"
	$PROG_DST_DIR/mobilegt_vpn -f $PROG_DST_DIR/mobilegt_vpn.cfg &
	
	$CMD_ECHO "start mobilegt vpn server completed."

	$CMD_ECHO "start capture interface:$TUN_IF_NAME packet...... .pcap data in $DATA_DIR"
	#dumpcap -i $TUN_IF_NAME -b duration:$DURA_SEC -P -w $DATA_DIR/1.pcap
	$CMD_DUMPCAP -i $TUN_IF_NAME -b filesize:$FILESIZE_KB -b duration:$DURA_SEC -P -w $DATA_DIR/$SERVER_NAME.pcap &
	$CMD_ECHO "start capture packet completed."
	$CMD_ECHO

elif [ $1 == "stop" ];then
	$CMD_ECHO "stop mobilegt vpn server..."
	$CMD_ECHO 0 > $TAG_FILE
	$CMD_SLEEP 3
	$CMD_NC -vuz $stop_tunnel_IP $stop_tunnel_PORT
	$CMD_SLEEP 3
	$CMD_PING -c 1 -W 1 -I $TUN_IF_NAME $stop_tunnel_IP
	$CMD_SLEEP 3
	PID=`$CMD_PS -ef | $CMD_GREP $KEYWORD_MOBILEGT | $CMD_GREP -v 'grep' | $CMD_AWK '{print $2}'`
	if [ -z "$PID" ];then
		$CMD_ECHO "    NO running mobilegt vpn server."
	else
		$CMD_ECHO "    kill PID:$PID"
        $CMD_KILL -9 $PID
	fi
	$CMD_ECHO "STOP mobilegt vpn server completed."
	
	$CMD_ECHO "STOP capture interface:$TUN_IF_NAME packet......"
	$PROG_DST_DIR/stopCapture.sh $TUN_IF_NAME
	$CMD_ECHO "STOP capture packet process completed."
	$CMD_ECHO
elif [ $1 == "status" ];then
	PID=`$CMD_PS -ef | $CMD_GREP $KEYWORD_MOBILEGT | $CMD_GREP -v "grep" | $CMD_AWK '{print $2}'`
	if [ -z "$PID" ];then
		$CMD_ECHO "NO $KEYWORD_MOBILEGT running."
		$CMD_ECHO
	else
		$CMD_ECHO "$KEYWORD_MOBILEGT is running."
		$CMD_ECHO
        $CMD_PS -ef | $CMD_GREP $KEYWORD_MOBILEGT | $CMD_GREP -v "grep"
		$CMD_ECHO
	fi 
elif [ $1 == "update" ];then
	$CMD_ECHO "start update vpnserver..."
	$CMD_ECHO "$CMD_CP $SCRIPT_SRC_DIR/mobilegt_vpn.sh|mobilegt_preprocess.sh|stopCapture.sh|mobilegt_vpn.cfg $PROG_DST_DIR/"
	$CMD_CP $SCRIPT_SRC_DIR/mobilegt_vpn.sh $PROG_DST_DIR
	$CMD_CP $SCRIPT_SRC_DIR/mobilegt_preprocess.sh $PROG_DST_DIR
	$CMD_CP $SCRIPT_SRC_DIR/stopCapture.sh $PROG_DST_DIR/
	$CMD_CP $SCRIPT_SRC_DIR/mobilegt_vpn.cfg $PROG_DST_DIR/

	$CMD_ECHO "$CMD_CP $PROG_SRC_DIR/mobilegt_vpn $PROG_DST_DIR                                                   
	$CMD_CP $PROG_SRC_DIR/mobilegt_vpn $PROG_DST_DIR/

	$CMD_ECHO "update vpnserver completed."
else
	usage
fi

