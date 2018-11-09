/*
 * 定义一套方法和协议用于同运行状态的vpnserver交互，用于查看vpnserver的运行状态，调整运行参数等
 * 交互通过一个特殊的配置文件完成，vpnserver的主线程定时查看该配置文件执行相应的动作，然后将结果存储到相应的结果文件
 * 1.查看运行状态
 * <verb><object><attribute><interval>
 * query 
 * 查看对象object
 * |-各处理线程(tunReceiver/tunDataProcess/tunnelReceiver/tunnelDataProcess)
 * |-当前已连接的客户端(peerClient)
 * |-报文缓冲池(tunIF_recv_packetPool/tunnel_recv_packetPool)
 * 
 * attribute
 * |-对象的属性
 *   |-并发线程数
 *   |-每个客户端的接收/处理报文数
 * interval
 * |-间隔时间
 * 2.调整运行参数(暂不实现)
 * 
 */

/* 
 * File:   mobilegt_vpnserver_inspection.h
 * Author: lenovo-pc
 *
 * Created on November 27, 2017, 10:24 AM
 */

#ifndef MOBILEGT_VPNSERVER_INSPECTION_H
#define MOBILEGT_VPNSERVER_INSPECTION_H



#endif /* MOBILEGT_VPNSERVER_INSPECTION_H */

