
#example:
#    ./stopCapture tun0
CMD_ECHO="/bin/echo"
CMD_KILL="/bin/kill"
CMD_PS="/bin/ps"
CMD_GREP="/bin/grep"
CMD_AWK="/usr/bin/awk"

if [ ! $# -eq 1 ];then
    $CMD_ECHO "example:"
    $CMD_ECHO "$0 tun0"
    exit 0
fi

IF_NAME=$1
TSHARK_NAME="tshark"
DUMPCAP_NAME="dumpcap"
#DEVICE_ID=$2

if [ $1 != "all" -a $1 != "ALL" -a $1 != "All" ];then
    #ps -ef | grep $CMD_NAME | grep $IF_NAME
    PID=`$CMD_PS -ef | $CMD_GREP $TSHARK_NAME | $CMD_GREP $IF_NAME | $CMD_GREP -v "grep" | $CMD_AWK '{print $2}'`
    PID2=`$CMD_PS -ef | $CMD_GREP $DUMPCAP_NAME | $CMD_GREP $IF_NAME | $CMD_GREP -v "grep" | $CMD_AWK '{print $2}'`
else
    PID=`$CMD_PS -ef | $CMD_GREP $TSHARK_NAME | $CMD_GREP -v "grep" | $CMD_AWK '{print $2}'`
    PID2=`$CMD_PS -ef | $CMD_GREP $DUMPCAP_NAME | $CMD_GREP -v "grep" | $CMD_AWK '{print $2}'`
fi

if [ -z "$PID2" ];then
    $CMD_ECHO "    NO running $CMD_NAME2 process."
else
    $CMD_ECHO "    kill $CMD_NAME2 PID2:$PID2"
    #cat password | sudo -S kill -9 $PID2
    $CMD_KILL -9 $PID2
fi

if [ -z "$PID" ];then
    $CMD_ECHO "    NO running $CMD_NAME process."
else
    $CMD_ECHO "    kill $CMD_NAME PID:$PID"
    #cat password | sudo -S kill -9 $PID
    $CMD_KILL -9 $PID
fi

