
#include "mobilegt_util.h"


int build_parameters(char *parameters, int size, const string tun_ip, const map<string, string> & conf_m, CacheLogger & cLogger);
/*
 * 处理接收来自手机客户端的数据,然后转发给TUN interface,采用多线程机制并发处理
 * 1. 解密接收到数据客户端数据(加密数据)
 * 2. 转发
 */
int tunnelDataProcess(PacketPool & tunnelReceiver_packetPool, int fd_tun_interface, int socketfd_tunnel, TunIPAddrPool & tunip_pool, PeerClientTable & peerClientTable, const map<string, string> & conf_m, CacheLogger & cLogger) {
	const string FUN_NAME = "tunnelDataProcess";
	const int WARN_THRESHOLD = 10000; //微秒
	EncryptDecryptHelper * aes_helper = EncryptDecryptHelper::getInstance();
	struct timeval sTime, eTime;
	long exeTime;
	char decryptedPacket[2048];
	while (true) {
		PacketNode* pkt_node = tunnelReceiver_packetPool.consume();
		if (pkt_node == NULL) {
			if (SYSTEM_EXIT)
				break;
			//阻塞等待通知有数据
			//std::unique_lock <std::mutex> lck(tunnelReceiver_packetPool.mtx_cv);
			//tunnelReceiver_packetPool.produce_cv.wait(lck);
			//休眠然后重试
			//this_thread::sleep_for(chrono::milliseconds(10)); //std::this_thread;std::chrono;
			continue;
		}
		pkt_node->addProcessTimeTrack("consumeBegin");
		//process pkt_node里面的数据

		cLogger.log(log_level::DEBUG, FUN_NAME, "consume tunnel recv node. pkt_node index["
				+ to_string(pkt_node->index) + "] remain:" + to_string(tunnelReceiver_packetPool.getRemainInConsumer()));
		char* packet = pkt_node->ptr;
		int packet_len = pkt_node->pkt_len;
		bool dropPacket = false; //是否丢弃报文,缺省不丢弃
		if (packet[0] != 0) {
			//检查客户端地址是否已在记录表中,不在则丢弃报文
			pkt_node->addProcessTimeTrack("consumeCheckPeerBegin");
			if (!peerClientTable.checkPeerInternetIPandPort(pkt_node->pkt_internetAddr, pkt_node->pkt_internetPort))
				dropPacket = true;
			pkt_node->addProcessTimeTrack("consumeCheckPeerCompleted");
			if (!dropPacket) {

				cLogger.log(log_level::DEBUG, FUN_NAME, "begin process data cipher packet. pkt_length:"
						+ to_string(packet_len));
				gettimeofday(&sTime, NULL);
				CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aes_helper->aesDecryption, aes_helper->iv);
				gettimeofday(&eTime, NULL);
				exeTime = (eTime.tv_sec - sTime.tv_sec)*1000000 + (eTime.tv_usec - sTime.tv_usec); //exeTime 单位是微秒
				if (exeTime > WARN_THRESHOLD)
					cLogger.log(log_level::WARN, FUN_NAME, "decrypt init exeTime:" + to_string(exeTime) + " microseconds");

				//// decrypt packet;
				std::string strDecryptedText = "";

				/*
				16进制输出接收到的加密数据报文
				for(int i=0;i<length;i++)
					cout <<hex<< (0xFF & static_cast<byte>(packet[i])) << " ";
				cout << endl;
				 */
				gettimeofday(&sTime, NULL);
				CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(strDecryptedText), CryptoPP::BlockPaddingSchemeDef::ZEROS_PADDING);
				stfDecryptor.Put((byte*) packet + 1, packet_len - 1);
				stfDecryptor.MessageEnd();

				int iDecryptedTextSize = strDecryptedText.size();
				byte lb[4]; //添入报文大小
				for (int i = 0; i < 4; i++) {
					lb[i] = static_cast<byte> (strDecryptedText[i]);
					//std::cout << hex << (0xFF & static_cast<byte>(strDecryptedText[i])) << " ";
				}

				int datalength = bytesToInt(lb);

				cLogger.log(log_level::DEBUG, FUN_NAME, "decrypted plain packet. packet_length--data_length:" + to_string(iDecryptedTextSize) + "--" + to_string(datalength));
				/*
				for(int i=0;i<4;i++) {
				//lb[i]=static_cast<byte>(strDecryptedText[i]);
				std::cout << hex << (0xFF & static_cast<byte>(strDecryptedText[i])) << " ";
				}
				cout << endl;
				 */
				for (int i = 0; i < datalength; i++) {
					decryptedPacket[i] = static_cast<byte> (strDecryptedText[i + 4]);
					//	std::cout << hex << (0xFF & static_cast<byte>(strDecryptedText[i+4])) << " ";
				}
				gettimeofday(&eTime, NULL);
				exeTime = (eTime.tv_sec - sTime.tv_sec)*1000000 + (eTime.tv_usec - sTime.tv_usec); //exeTime 单位是微秒
				if (exeTime > WARN_THRESHOLD)
					cLogger.log(log_level::WARN, FUN_NAME, "decrypt packet exeTime:" + to_string(exeTime));
				// Write the incoming packet to the fd_tun_interfacd.

				cLogger.log(log_level::DEBUG, FUN_NAME, "write to fd_tun_interface. decryptedPacket datalength is:"
						+ to_string(datalength));
				pkt_node->addProcessTimeTrack("writeBegin");
				write(fd_tun_interface, decryptedPacket, datalength);
				pkt_node->addProcessTimeTrack("writeCompleted");
			}
		} else {
			//不是数据报文,而是建立建立连接命令报文,检查约定的密钥是否正确,正确则记录该客户端,并向该客户端发送响应报文
			//手机客户端命令报文数据格式为[0][sharedsecret][:][deviceId]

			cLogger.log(log_level::DEBUG, FUN_NAME, "process cmd packet from " + pkt_node->pkt_internetAddr
					+ ":" + to_string(pkt_node->pkt_internetPort) + ". pkt_length:" + to_string(packet_len));
			string str_recv_pkt1, str_recv_deviceId, str_recv_secret;
			str_recv_pkt1.append(&packet[1], packet_len - 1); //skip packet[0]
			string secret_delimiter = ":";
			str_recv_secret = str_recv_pkt1.substr(0, str_recv_pkt1.find(secret_delimiter));
			str_recv_deviceId = str_recv_pkt1.substr(str_recv_pkt1.find(secret_delimiter) + 1);
			//cLogger.log(log_level::DEBUG, FUN_NAME, "recv_secret is[" + str_recv_secret + "]. recvDeviceId is [" + str_recv_deviceId + "] deviceid length:" + to_string(str_recv_deviceId.length()));

			cLogger.log(log_level::DEBUG, FUN_NAME, "recv_secret is[*****]. recvDeviceId is [" + str_recv_deviceId + "] deviceid length:" + to_string(str_recv_deviceId.length()));
			if (str_recv_secret == secret) {
				//验证通过,记录客户端IP,端口信息等
				string deviceId = str_recv_deviceId;
				string tun_ip = tunip_pool.assignTunIPAddr(deviceId);
				string internet_ip = pkt_node->pkt_internetAddr;
				int internet_port = pkt_node->pkt_internetPort;
				//PeerClient * peerClient = new PeerClient(deviceId, tun_ip, internet_ip, internet_port);
				//peerClientTable.addPeerClient(tun_ip, peerClient);
				cLogger.log(log_level::INFO, FUN_NAME, "add PeerClient[" + deviceId + ":" + tun_ip + "-"
						+ internet_ip + ":" + to_string(internet_port) + "]");
				peerClientTable.addPeerClient(deviceId, tun_ip, internet_ip, internet_port);
				//验证通过,发送响应报文,包括tun私有地址,dns地址等信息,发送三次
				char parameters[1024];
				build_parameters(parameters, sizeof (parameters), tun_ip, conf_m, cLogger);
				sockaddr_in peer_cli_addr;
				memset(&peer_cli_addr, 0, sizeof (peer_cli_addr));
				peer_cli_addr.sin_family = AF_INET;
				peer_cli_addr.sin_port = htons(pkt_node->pkt_internetPort);
				inet_pton(AF_INET, pkt_node->pkt_internetAddr.c_str(), &peer_cli_addr.sin_addr);
				for (int i = 0; i < 3; ++i) {
					sendto(socketfd_tunnel, parameters, sizeof (parameters), MSG_NOSIGNAL, (sockaddr *) & peer_cli_addr, sizeof (peer_cli_addr));
				}
			}
		}
		if (dropPacket) {
			cLogger.log(log_level::WARN, FUN_NAME, "drop Packet.");
			//pkt_node->addProcessTimeTrack("dropPacket");
		}
		pkt_node->addProcessTimeTrack("consumeCompleted");
		cLogger.log(log_level::INFO, FUN_NAME, pkt_node->getStrProcessTimeTrack());
		tunnelReceiver_packetPool.consumeCompleted(pkt_node);
	}
	stringstream ss;
	ss << this_thread::get_id();
	cLogger.log(log_level::FATAL, FUN_NAME, "thread[" + ss.str() + "] exit!");
	return 0;
}

//build_parameters()方法构建参数,将tun接口的配置数据发送给通过验证的客户端
//example:
//参数主要有tun的地址,dns的地址,mtu
//sudo ./ToyVpnServer_encrypt $TUN_NAME $VPN_PORT $SECRET $CaptureCMD -m 1400 -a $PRIVATE_ADDR 32 -d $DNS_ADDR -r 0.0.0.0 0
//	"  -m <MTU> for the maximum transmission unit\n"
//	"  -a <address> <prefix-length> for the private address\n"
//	"  -r <address> <prefix-length> for the forwarding route\n"
//	"  -d <address> for the domain name server\n"

//m,<MTU> a,<PRIVATE_ADDR>,32 <d>,<DNS_ADDR> r,<0.0.0.0>,<0>
int build_parameters(char *parameters, int size, const string tun_ip, const map<string, string> & conf_m, CacheLogger & cLogger) {
	// Well, for simplicity, we just concatenate them (almost) blindly.
	int offset = 0;
	const string FUN_NAME = "build_parameters";
	string str_MTU, str_DNS;
	map<string, string>::const_iterator cit = conf_m.find("MTU");
	if (cit != conf_m.end()) {
		str_MTU = cit->second;
	} else {
		cLogger.log(log_level::FATAL, FUN_NAME, "cannot found MTU configure");
	}
	cit = conf_m.find("DNS_ADDR");
	if (cit != conf_m.end()) {
		str_DNS = cit->second;
	} else {
		cLogger.log(log_level::FATAL, FUN_NAME, "cannot found DNS_ADDR configure");
	}
	string str_ROUTE = "r,0.0.0.0,0";
	string delimiter = " ";
	string str_parameter = "m," + str_MTU + delimiter + "a," + tun_ip + ",32" + delimiter + "d," + str_DNS + delimiter + str_ROUTE;

	// Append the delimiter and the parameter.
	int length = str_parameter.length();
	memcpy(&parameters[offset + 1], str_parameter.c_str(), length);
	offset += 1 + length;
	// Fill the rest of the space with spaces.
	memset(&parameters[offset], ' ', size - offset);

	// Control messages always start with zero.
	parameters[0] = 0;
}