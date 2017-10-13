/*

 */
#include "mobilegt_util.h"

//// 使用网上copy的代码处理字符串
#include "get_config.h"

//int转换为字节bytes
void intToByte(int i, byte *bytes, int size /*= 4*/) {
	//byte[] bytes = new byte[4];
	memset(bytes, 0, sizeof (byte) * size);
	bytes[3] = (byte) (0xff & i);
	bytes[2] = (byte) ((0xff00 & i) >> 8);
	bytes[1] = (byte) ((0xff0000 & i) >> 16);
	bytes[0] = (byte) ((0xff000000 & i) >> 24);
}

//bytes转换int
int bytesToInt(const byte * bytes, int size /*= 4*/) {
	int addr = bytes[3] & 0xFF;
	addr |= ((bytes[2] << 8) & 0xFF00);
	addr |= ((bytes[1] << 16) & 0xFF0000);
	addr |= ((bytes[0] << 24) & 0xFF000000);
	return addr;
}

//EncryptDecryptHelper
//必须加上初始化,否则编译会在使用到该成员变量的地方提示undefine reference to single_Instance的错误
EncryptDecryptHelper * EncryptDecryptHelper::single_Instance = NULL;
EncryptDecryptHelper::EncryptDecryptHelper() {
	//构造函数是私有的  
	aesEncryption.SetKey(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
	aesDecryption.SetKey(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
}

//// 
//TunIPAddrPool::TunIPAddrPool(string netaddr, string netmask) : pool_netaddr(netaddr), pool_netmask(netmask) {
//// 未实现
//}

//// 
//// 构造函数
//// 读取assign_ip_recorder文件，初始化历史分配情况，保证同样的deviceId与分配的IP有一一对应关系
//// 
TunIPAddrPool::TunIPAddrPool(string assign_ip_recorder, CacheLogger & cLogger) : assign_ip_recorder(assign_ip_recorder), cLogger(cLogger) {
	mtx_tunaddr_pool.lock();
	const string FUN_NAME = "TunIPAddrPool->TunIPAddrPool";
	//log(log_level::DEBUG, FUN_NAME, "initial TunIPAddrPool");
	ifstream infile((PROC_DIR + assign_ip_recorder).c_str());
	if (!infile) {
		cout << "file open error.[" << PROC_DIR + assign_ip_recorder << "]" << endl;
	}
	//// ##ip_prefix.#ip_index2.#ip_index1=#deviceId
	//// #开头为注释
	//// 10.77.0.1=deviceId-1
	//// 10.77.0.2=deviceId-2
	//// ......
	string line, tunip, deviceId;
	while (getline(infile, line)) {
		if (AnalyseLine(line, tunip, deviceId)) {
			umap_deviceId_tunip[deviceId] = tunip;
			umap_tunip_deviceId[tunip] = deviceId;
			int new_index1, new_index2;
			auto pos = tunip.rfind(".");
			std::stringstream ss;
			ss << tunip.substr(pos + 1);
			ss >> new_index1;
			ip_index1 = ip_index1 >= new_index1 ? ip_index1 : new_index1;

			string t = tunip.substr(0, pos);
			pos = t.rfind(".");
			ss.clear();
			ss << t.substr(pos + 1);
			ss >> new_index2;
			ip_index2 = ip_index2 >= new_index2 ? ip_index2 : new_index2;
		}
	}
	ip_index1++; //所有已分配的ip再加1为第一个可分配IP
	if (ip_index1 > IP_INDEX1_MAX) {
		ip_index2++;
		ip_index1 = 1;
	}
	if (ip_index2 == IP_INDEX2_MAX && ip_index1 == IP_INDEX1_MAX) {
		//log(log_level::FATAL, FUN_NAME, "exceed ip_index1 & ip_index2. current ip_index1:" + to_string(ip_index1) + " ip_index2:" + to_string(ip_index2));
	}
	infile.close();
	//log(log_level::INFO, FUN_NAME, "initial ip_index1:" + to_string(ip_index1) + " ip_index2:" + to_string(ip_index2));
	mtx_tunaddr_pool.unlock();
}

//// 
void TunIPAddrPool::hibernateHistoryLog(string logFileName) {
	//未实现
}

////
//// TunIPAddrPool
//// 每次分配新的tunip给新的deviceId后立即将分配记录写入分配记录文件
//// 
string TunIPAddrPool::assignTunIPAddr(string deviceId) {
	const string FUN_NAME = "TunIPAddrPool::assignTunIPAddr";
	mtx_tunaddr_pool.lock();
	string tun_ip;
	auto f = umap_deviceId_tunip.find(deviceId);
	if (f == umap_deviceId_tunip.end()) {
		//log(log_level::DEBUG, FUN_NAME, "cannot find a assigned tun ip for deviceId[" + deviceId + "]");
		//// #ip_prefix.#ipindex2.#ipindex1
		//// example: 10.77.0.1 10.77.0.2 10.77.0.3 ...
		string newip = "";
		if (!exceedScope) {
			newip = ip_prefix + "." + to_string(ip_index2) + "." + to_string(ip_index1);
			//log(log_level::DEBUG, FUN_NAME, "assigned tun_ip is:[" + newip + "] current ip_index1:" + to_string(ip_index1) + " ip_index2:" + to_string(ip_index2));
			ip_index1++; //所有已分配的ip再加1为第一个可分配IP
			if (ip_index1 > IP_INDEX1_MAX) {
				ip_index2++;
				ip_index1 = 1;
			}
			if (ip_index2 == IP_INDEX2_MAX && ip_index1 == IP_INDEX1_MAX) {
				//log(log_level::FATAL, FUN_NAME, "exceed ip_index1 & ip_index2. current ip_index1:" + to_string(ip_index1) + " ip_index2:" + to_string(ip_index2));
				exceedScope = true;
			}

			umap_deviceId_tunip[deviceId] = newip;
			umap_tunip_deviceId[newip] = deviceId;
			//// update assign_ip_recorder
			ofstream outfile((PROC_DIR + assign_ip_recorder).c_str(), ios::app);
			streampos sp = outfile.tellp();
			if (sp <= 0)
				outfile << "#ip_prefix.index2.index1=deviceId" << endl;
			outfile << newip << "=" << deviceId << endl;
			outfile.close();
		} else {
			//log(log_level::ERROR, FUN_NAME, "cannot assign tun_ip. current ip_index1:" + to_string(ip_index1) + " ip_index2:" + to_string(ip_index2));
		}
		tun_ip = newip;
	} else {
		//log(log_level::DEBUG, FUN_NAME, "find a assigned tun ip.");
		tun_ip = f->second;
	}
	mtx_tunaddr_pool.unlock();
	return tun_ip;
}

//// 
//// 依据deviceId查询对应分配的tunip
//// 
string TunIPAddrPool::queryTunIPAddrByDeviceId(string deviceId) {
	string tun_ip;
	auto f = umap_deviceId_tunip.find(deviceId);
	if (f == umap_deviceId_tunip.end()) {
		;
	} else {
		tun_ip = f->second;
	}
	return tun_ip;
}

//// 
//// class PeerClient
////
PeerClient::PeerClient() {

}
PeerClient::PeerClient(string deviceId, string tun_addr, string internet_addr, int internet_port) : peer_deviceId(deviceId), peer_tun_ip(tun_addr), peer_internet_ip(internet_addr), peer_internet_port(internet_port) {

}
string PeerClient::getPeer_deviceId() {

	return peer_deviceId;
}

//// 
string PeerClient::getPeer_tun_ip() {

	return peer_tun_ip;
}

//// 
string PeerClient::getPeer_internet_ip() {

	return peer_internet_ip;
}

//// 
int PeerClient::getPeer_internet_port() {

	return peer_internet_port;
}

//// 
void PeerClient::setPeer_deviceId(string deviceId) {
	mtx_peerClient.lock();
	peer_deviceId = deviceId;
	mtx_peerClient.unlock();
}

//// 
void PeerClient::setPeer_internet_ip(string internet_ip) {
	mtx_peerClient.lock();
	peer_internet_ip = internet_ip;
	mtx_peerClient.unlock();
}

//// 
void PeerClient::setPeer_internet_port(int internet_port) {
	mtx_peerClient.lock();
	peer_internet_port = internet_port;
	mtx_peerClient.unlock();
}

//// 
void PeerClient::refreshRecentConnectTime() {
	mtx_recentConnectTime.lock();
	recentConnectTime = std::chrono::system_clock::now();
	mtx_recentConnectTime.unlock();
}

//// 
std::chrono::system_clock::time_point PeerClient::getRecentConnectTime() {

	return recentConnectTime;
}

//// 
void PeerClient::increaseDataPktCount_recv(int count) {
	mtx_dataPktCount_recv.lock();
	dataPktCount_recv += count;
	mtx_dataPktCount_recv.unlock();
}

//// 
void PeerClient::increaseDataPktCount_send(int count) {
	mtx_dataPktCount_send.lock();
	dataPktCount_send += count;
	mtx_dataPktCount_send.unlock();
}

//// 
int PeerClient::getDataPktCount_send() const {

	return dataPktCount_send;
}

//// 
int PeerClient::getDataPktCount_recv() const {
	return dataPktCount_recv;
}
void PeerClient::increaseCmdPktCount_send(int count) {
	mtx_cmdPktCount_send.lock();
	cmdPktCount_send += count;
	mtx_cmdPktCount_send.unlock();
}
void PeerClient::increaseCmdPktCount_recv(int count) {
	mtx_cmdPktCount_recv.lock();
	cmdPktCount_recv += count;
	mtx_cmdPktCount_recv.unlock();
}
int PeerClient::getCmdPktCount_send() const {
	return cmdPktCount_send;
}
int PeerClient::getCmdPktCount_recv() const {
	return cmdPktCount_recv;
}
void PeerClient::resetDataCount() {
	mtx_peerClient.lock();
	dataPktCount_send = 0;
	dataPktCount_recv = 0;
	cmdPktCount_send = 0;
	cmdPktCount_recv = 0;
	mtx_peerClient.unlock();
}

//// 
//// class PeerClientTable
//// 
//PeerClientTable * PeerClientTable::single_Instance = NULL;
PeerClientTable::PeerClientTable(CacheLogger & cLogger) : cLogger(cLogger) {
	//构造函数是私有的  

}

//// 析构函数 
PeerClientTable::~PeerClientTable() {
	mtx_peerClientTable.lock();
	const string FUN_NAME = "~PeerClientTable";
	cLogger.log(log_level::DEBUG, FUN_NAME, "destructor clear all peer client object.");
	//	if (single_Instance == NULL) {
	//		for (auto iter = umap_tunip_client.begin(); iter != umap_tunip_client.end(); iter++) {
	//			PeerClient * p_peerClient = iter->second;
	//			delete p_peerClient;
	//		}
	//		delete single_Instance;
	//	}
	umap_tunip_client.clear();
	umap_internetip_port_client.clear();
	mtx_peerClientTable.unlock();
}

//// 
//PeerClientTable * PeerClientTable::getInstance(CacheLogger & cLogger) {
//	if (single_Instance == NULL) //判断是否第一次调用  
//		single_Instance = new PeerClientTable(cLogger);
//	return single_Instance;
//}

//// 
PeerClient * PeerClientTable::getPeerClientByTunIP(string tun_ip) {
	if (umap_tunip_client.find(tun_ip) == umap_tunip_client.end()) {
		return NULL;
	} else {
		return umap_tunip_client[tun_ip];
	}
}

//int PeerClientTable::addPeerClient(string tun_ip, PeerClient * p_peerClient) {
//	int result = 0;
//	umap_tunip_client[tun_ip] = p_peerClient;
//	umap_internetip_client[p_peerClient->getPeer_internet_ip()] = p_peerClient;
//	result = 1;
//	return result;
//}
int PeerClientTable::addPeerClient(string deviceId, string tun_ip, string internet_ip, int internet_port) {
	mtx_peerClientTable.lock();
	PeerClient* ptr_oldClient = getPeerClientByTunIP(tun_ip);
	int result = 0;
	if (ptr_oldClient == NULL) {
		PeerClient * p_peerClient = new PeerClient(deviceId, tun_ip, internet_ip, internet_port);
		p_peerClient->refreshRecentConnectTime();
		p_peerClient->increaseCmdPktCount_recv();
		p_peerClient->increaseCmdPktCount_send(3);
		umap_tunip_client[tun_ip] = p_peerClient;
		string internetip_port = internet_ip + ":" + to_string(internet_port);
		umap_internetip_port_client[internetip_port] = p_peerClient;
		result = 1;
	} else {
		string old_internetip_port = ptr_oldClient->getPeer_internet_ip() + ":"
				+ to_string(ptr_oldClient->getPeer_internet_port());
		auto it = umap_internetip_port_client.find(old_internetip_port);
		if (it != umap_tunip_client.end())
			umap_internetip_port_client.erase(it);
		ptr_oldClient->refreshRecentConnectTime();
		ptr_oldClient->setPeer_deviceId(deviceId);
		ptr_oldClient->setPeer_internet_ip(internet_ip);
		ptr_oldClient->setPeer_internet_port(internet_port);
		string new_internetip_port = internet_ip + ":" + to_string(internet_port);
		umap_internetip_port_client[new_internetip_port] = ptr_oldClient;
		//ptr_oldClient->resetDataCount();
		ptr_oldClient->increaseCmdPktCount_recv();
		ptr_oldClient->increaseCmdPktCount_send(3);
	}
	mtx_peerClientTable.unlock();
	return result;
}

//// 
int PeerClientTable::deletePeerClient(string tun_ip) {
	mtx_peerClientTable.lock();
	const string FUN_NAME = "deletePeerClient";
	int result = 0;
	auto iter1 = umap_tunip_client.find(tun_ip);
	cLogger.log(log_level::DEBUG, FUN_NAME, "find tun_ip:" + tun_ip);
	if (iter1 != umap_tunip_client.end()) {
		PeerClient * p_peerClient = umap_tunip_client[tun_ip];
		string internetip_port = p_peerClient->getPeer_internet_ip() + ":" + to_string(p_peerClient->getPeer_internet_port());
		auto iter2 = umap_internetip_port_client.find(internetip_port);
		if (iter2 != umap_internetip_port_client.end()) {
			umap_internetip_port_client.erase(iter2);
		}
		umap_tunip_client.erase(iter1);
		cLogger.log(log_level::DEBUG, FUN_NAME, "begin delete p_peerClient");
		delete p_peerClient;
		result = 1;
	}
	mtx_peerClientTable.unlock();
	return result;
}

////
bool PeerClientTable::checkPeerInternetIPandPort(string internet_ip, int port) {
	const string FUN_NAME = "checkPeerInternetIPandPort";
	bool succeed = false;
	string internetip_port = internet_ip + ":" + to_string(port);
	if (umap_internetip_port_client.find(internetip_port) != umap_internetip_port_client.end()) {
		cLogger.log(log_level::DEBUG, FUN_NAME, "found " + internetip_port + ". begin check time");
		PeerClient * p_peerClient = umap_internetip_port_client[internetip_port];

		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
		std::chrono::system_clock::time_point recent = p_peerClient->getRecentConnectTime();
		//// 30minute * 60second/minute
		//// 最近连接时间不超过30分钟
		int tt = std::chrono::duration_cast<std::chrono::seconds>(now - recent).count();
		if (tt <= 30 * 60) {
			succeed = true;
			//超过10秒才更新
			if (tt > 10)
				p_peerClient->refreshRecentConnectTime();
			p_peerClient->increaseDataPktCount_send();
		}
	} else {
		cLogger.log(log_level::DEBUG, FUN_NAME, "not found: " + internetip_port);
	}
	return succeed;
}
unordered_map<string, PeerClient*> PeerClientTable::getUmap_tunip_client() const {
	return umap_tunip_client;
}

//// 
//// class PacketNode
//// 
PacketNode::PacketNode(int nodeIndex) : index(nodeIndex) {
	ptr = new char[MAX_LEN];
	pkt_len = 0;
}
PacketNode::~PacketNode() {
	delete[] ptr;
}
//得到报文接收到当前的持续时间
std::chrono::microseconds PacketNode::getPktNodeDurationMicroseconds() {
	auto pktNode_d = std::chrono::system_clock::now() - timestamp;
	return std::chrono::duration_cast<std::chrono::microseconds>(pktNode_d);
}
//添加一个处理时间跟踪记录
void PacketNode::addProcessTimeTrack(string track_description) {
	ProcessTimeTrack ptt;
	ptt.str_track_description = track_description;
	ptt.timestamp = std::chrono::system_clock::now();
	vec_process_TT.push_back(ptt);
}

//打印输出处理时间跟踪记录
string PacketNode::getStrProcessTimeTrack() {
	string str_track = "pktnode[" + to_string(index) + "] size(" + to_string(pkt_len) + ") processTT(microseconds) ";
	for (ProcessTimeTrack vec : vec_process_TT) {
		auto tt_d = vec.timestamp - timestamp;
		str_track += vec.str_track_description + ":" + to_string(std::chrono::duration_cast<std::chrono::microseconds>(tt_d).count()) + " ";
	}
	return str_track;
}
void PacketNode::clear() {
	pkt_len = 0;
	vec_process_TT.clear();
}
//// 
//// class PacketPool
//// 构造函数
//// 报文缓冲池, 构造初始化构建分配POOL_NODE_MAX_NUMBER个缓冲节点用于存放数据
PacketPool::PacketPool(CacheLogger & cLogger) : cLogger(cLogger) {
	const string FUN_NAME = "PacketPool";
	cLogger.log(log_level::DEBUG, FUN_NAME, "packet pool constructor.");
	for (int i = 0; i < POOL_NODE_MAX_NUMBER; i++) {
		ptr_pktNodePool[i] = new PacketNode(i);
		queue_producer.push(i); // 即:queue_producer.push(ptr_pktNodePool[i]->index);
	}
}

//// 
//// 析构函数,回收构造是分配的POOL_NODE_MAX_NUMBER个缓冲报文节点
PacketPool::~PacketPool() {
	for (int i = 0; i < POOL_NODE_MAX_NUMBER; i++) {
		delete ptr_pktNodePool[i];
	}
}

//// 得到一个用于接收网络数据报文的节点
PacketNode* PacketPool::produce() {
	const string FUN_NAME = "PacketPool-->produce()";
	int nodeIndex = -1;
	while (!SYSTEM_EXIT && nodeIndex < 0) {
		mtx_queue_producer.lock();
		if (!queue_producer.empty()) {
			nodeIndex = queue_producer.front();
			queue_producer.pop();
		}
		mtx_queue_producer.unlock();
		if (nodeIndex < 0) {
			cLogger.log(log_level::DEBUG, FUN_NAME, "produce_cv wait(lck(mtx_produce_cv)) ");
			std::unique_lock <std::mutex> lck(mtx_produce_cv);
			//produce_cv.wait(lck);
			produce_cv.wait_for(lck, std::chrono::microseconds{200});
		}
	}
	if (nodeIndex >= 0)
		return ptr_pktNodePool[nodeIndex];
	else
		return NULL;

}

//// 结点已接收完网络数据报文,加入待处理队列
void PacketPool::produceCompleted(PacketNode * const pkt_node) {
	mtx_queue_consumer.lock();
	queue_consumer.push(pkt_node->index);
	mtx_queue_consumer.unlock();
	consume_cv.notify_all(); //唤醒所有阻塞等待的消费线程
}
//// 结点接收的数据为空或用于接收数据的结点不需要接收数据,回收结点用于下次接收
void PacketPool::produceWithdraw(PacketNode * const pkt_node) {
	PacketPool::consumeCompleted(pkt_node);
}

//// 得到一个需要处理(即,已接收网络数据报文)的节点
PacketNode* PacketPool::consume() {
	const string FUN_NAME = "PacketPool-->consume()";
	int nodeIndex = -1;
	while (!SYSTEM_EXIT && nodeIndex < 0) {
		mtx_queue_consumer.lock();
		if (!queue_consumer.empty()) {
			nodeIndex = queue_consumer.front();
			queue_consumer.pop();
		}
		mtx_queue_consumer.unlock();
		if (nodeIndex < 0) {
			cLogger.log(log_level::DEBUG, FUN_NAME, "consume_cv wait(lck(mtx_consume_cv)) ");
			std::unique_lock <std::mutex> lck(mtx_consume_cv);
			//consume_cv.wait(lck); //阻塞等待
			consume_cv.wait_for(lck, std::chrono::microseconds{200}); //阻塞等待
		}
	}
	if (nodeIndex >= 0)
		return ptr_pktNodePool[nodeIndex];
	else
		return NULL;
}

//// 节点数据处理完毕,加入待处理队列
void PacketPool::consumeCompleted(PacketNode * const pkt_node) {
	//pkt_node->pkt_len = 0;
	pkt_node->clear();
	mtx_queue_producer.lock();
	queue_producer.push(pkt_node->index);
	mtx_queue_producer.unlock();
	produce_cv.notify_all(); // 条件变量,唤醒所有阻塞等待的生产线程.
}
int PacketPool::getRemainInConsumer() {
	return queue_consumer.size();
}
int PacketPool::getReaminInProducer() {
	return queue_producer.size();
}
void PacketPool::terminateProcess() {
	produce_cv.notify_all();
	consume_cv.notify_all();
}
