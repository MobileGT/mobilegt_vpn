#/bin/bash

netstat -rn
sudo iptables -t nat -L
sudo ifconfig -a

IF_NAME=eth0
TUN_NAME=tun0
TUN_ADDR=10.77.255.254
#TUN_ADDR=10.77.0.2
#TUN_DSTADDR=10.77.0.1
#PRIVATE_ADDR=10.77.0.1

ADDR_PREFIX=10.77.0.0/16

sudo sysctl -w net.ipv4.ip_forward=1
sudo iptables -t nat -D POSTROUTING -s $ADDR_PREFIX -o $IF_NAME -j MASQUERADE
sudo iptables -t nat -A POSTROUTING -s $ADDR_PREFIX -o $IF_NAME -j MASQUERADE
#sudo iptables -t nat -A POSTROUTING -s $ADDR_PREFIX -o $IF_NAME -j MASQUERADE -j LOG --log-level 7 --log-prefix "nat postrouting:"

sudo ip tuntap del dev $TUN_NAME mode tun
sudo ip tuntap add dev $TUN_NAME mode tun
#sudo ifconfig $TUN_NAME $TUN_ADDR dstaddr $TUN_DSTADDR up
sudo ifconfig $TUN_NAME $TUN_ADDR up
sudo route add -net $ADDR_PREFIX dev $TUN_NAME
#route del -net ...
#route del -host ...

echo "===========================split line=========================="
netstat -rn
sudo iptables -t nat -L

#设置dumpcap无需sudo方式
ls -l /usr/bin/dumpcap
sudo chmod 4755 /usr/bin/dumpcap
ls -l /usr/bin/dumpcap	