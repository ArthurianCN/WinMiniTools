#include <stdlib.h>
#include <stdio.h>

#include "pcap.h"
#include "packet32.h"
#include <ntddndis.h>

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"wpcap.lib")
#pragma comment(lib,"Packet.lib")


#define ETH_ARP         0x0806		//以太网帧类型表示后面数据的类型，对于ARP请求或应答来说。该字段的值为x0806
#define ARP_HARDWARE    1			//硬件类型字段值为表示以太网地址
#define ETH_IP          0x0800		//协议类型字段表示要映射的协议地址类型值为x0800表示IP地址
#define ARP_REQUEST     1			//ARP请求
#define ARP_REPLY       2			//ARP应答



#define DBG_INFO(format, ...) \
		fprintf(stdout, "[info] [%d %s]" format "\n", __LINE__, __FUNCTION__, ##__VA_ARGS__);

#define DBG_WARNNING(format, ...) \
	fprintf(stdout, "[warnning] [%d %s]" format "\n", __LINE__, __FUNCTION__, ##__VA_ARGS__);

#pragma pack(push, 1)

// 以太帧头部结构体，共14字节
struct EthernetHeader
{
	u_char DestMAC[6];					//目的MAC地址 6字节
	u_char SourMAC[6];					//源MAC地址 6字节
	u_short EthType;					//上一层协议类型，如0x0800代表上一层是IP协议，0x0806为arp  2字节
};


// ARP头部结构，28字节
struct Arpheader {
	unsigned short HardwareType;		//硬件类型
	unsigned short ProtocolType;		//协议类型
	unsigned char HardwareAddLen;		//硬件地址长度
	unsigned char ProtocolAddLen;		//协议地址长度
	unsigned short OperationField;		//操作字段
	unsigned char SMac[6];				//源mac地址
	unsigned int SIp;					//源ip地址
	unsigned char DestMacAdd[6];		//目的mac地址
	unsigned int DestIpAdd;				//目的ip地址
};

//arp包结构
struct ArpPacket {
	struct EthernetHeader ed;
	struct Arpheader ah;
};

struct sparam {
	pcap_t *adhandle;
	char *ip;
	unsigned char *mac;
	char *netmask;
};

#pragma pack(pop)

bool g_bFlag = false;

// 获取本机的ip和掩码
bool GetSelfAddr(pcap_if_t* pIf, char* ip_addr, char* ip_netmask = NULL)
{
	for (pcap_addr_t* a = pIf->addresses; a; a = a->next)
	{
		if(!a->addr)
		{
			continue;
		}

		if(AF_INET != a->addr->sa_family)
		{
			continue;
		}

		if(ip_addr)
		{
			char ip_str[INET_ADDRSTRLEN]; // 创建一个足够大的缓冲区来存储IP地址字符串  
			inet_ntop(AF_INET, &(((sockaddr_in *)a->addr)->sin_addr), ip_str, INET_ADDRSTRLEN);
			memcpy(ip_addr, ip_str, 16);
		}
		
		if(ip_netmask)
		{
			if(!a->netmask)
			{
				return false;
			}

			char netmaskstr[INET_ADDRSTRLEN] = { 0 };
			inet_ntop(AF_INET, &(((sockaddr_in *)a->netmask)->sin_addr), netmaskstr, INET_ADDRSTRLEN);
			memcpy(ip_netmask, netmaskstr, 16);
		}
	}

	if(0 == strlen(ip_addr))
	{
		return false;
	}

	return true;
}


bool get_mac_from_pcap_dev_name(char *name, unsigned char *mac)
{
    LPADAPTER lpAdapter = PacketOpenAdapter(name);
    if (!lpAdapter || (lpAdapter->hFile == INVALID_HANDLE_VALUE))
	{
		DBG_WARNNING("PacketOpenAdapter fail");
        return false;
	}

    PPACKET_OID_DATA  OidData = (PPACKET_OID_DATA)malloc(6 + sizeof(PACKET_OID_DATA));
    if (OidData == NULL){
        PacketCloseAdapter(lpAdapter);
        return false;
    }

    OidData->Oid = OID_802_3_CURRENT_ADDRESS;

    OidData->Length = 6;
    ZeroMemory(OidData->Data, 6);

    BOOLEAN	Status = PacketRequest(lpAdapter, FALSE, OidData);
    if(!Status)
	{
		DBG_WARNNING("PacketRequest fail");
		free(OidData);
		PacketCloseAdapter(lpAdapter);
		return false;
	}

    memcpy(mac, OidData->Data,6);

    free(OidData);
    PacketCloseAdapter(lpAdapter);
    return true;
}


// 函数：通过ip和子网掩码计算子网IP数量  
int calculateSubnetHosts(const char *ip_str, const char *mask_str) {  
	// 将IP地址和子网掩码转换为32位无符号整数  
	unsigned int ip = inet_addr(ip_str);  
	unsigned int mask = inet_addr(mask_str);  

	// 检查转换是否成功  
	if (ip == INADDR_NONE || mask == INADDR_NONE) {  
		perror("inet_addr failed");  
		exit(EXIT_FAILURE);  
	}  

	// 提取主机部分的位数（即子网掩码中0的位数）  
	int host_bits = 0;  
	unsigned int mask_inv = ~mask; // 取反得到主机部分的掩码  
	while (mask_inv) {  
		if (mask_inv & 1) {  
			host_bits++;  
		}  
		mask_inv >>= 1;  
	}  

	// 计算子网中的IP数量（不包括网络地址和广播地址）  
	int subnet_hosts = (1 << host_bits) - 2;  
	return subnet_hosts;  
}  


/* 向局域网内全部可能的IP地址发送ARP请求包线程 */
DWORD WINAPI SendArpPacket(LPVOID lpThis)
{
	struct sparam *spara = (struct sparam *) lpThis;
	
	char *ip = spara->ip;
	unsigned char *mac = spara->mac;
	char *netmask = spara->netmask;
	
	unsigned long myip = inet_addr(ip), mynetmask = inet_addr(netmask);
	unsigned long hisip = htonl((myip & mynetmask));
	pcap_t *adhandle = spara->adhandle;

	EthernetHeader eh;
	// 目的地址为广播地址
	memset(eh.DestMAC, 0xff, 6);
	// 源mac地址为本机
	memcpy(eh.SourMAC, mac, 6);
	eh.EthType = htons(ETH_ARP);

	Arpheader ah;
	memcpy(ah.SMac, mac, 6);
	memset(ah.DestMacAdd, 0x00, 6);
	
	ah.HardwareType = htons(ARP_HARDWARE);
	ah.ProtocolType = htons(ETH_IP);
	ah.HardwareAddLen = 6;
	ah.ProtocolAddLen = 4;
	// 请求方的IP地址为自身的IP地址
	ah.SIp = inet_addr(ip);
	ah.OperationField = htons(ARP_REQUEST);

	// 获取局域网内最大ip数量
	int nMaxIpNum = calculateSubnetHosts(ip, netmask);
	
	//向局域网内所有ip地址对应的设备的发送arp请求
	for (int i = 0; i < nMaxIpNum; i++) 
	{
		// arp报文的里面
		ah.DestIpAdd = htonl(hisip + i);
		// 构造一个ARP请求
		unsigned char sendbuf[42] = { 0 }; //arp包结构大小
		memcpy(sendbuf, &eh, sizeof(eh));
		memcpy(sendbuf + sizeof(eh), &ah, sizeof(ah));
		if (pcap_sendpacket(adhandle, sendbuf, 42) != 0) 
		{
			DBG_WARNNING("PacketSendPacket in getmine Error: 0x%08x\n", GetLastError());
		}
	}

	// 给一个缓冲时间，避免某个设备还没处理完这边进程就退掉了
	Sleep(3000);
	g_bFlag = true;
	return 0;
}


int main()
{
	char errbuf[PCAP_ERRBUF_SIZE] = { 0 };
	pcap_if_t* devices = NULL;
	// 获取设备列表
	if (pcap_findalldevs(&devices, errbuf) == -1) {
		fprintf(stderr, "Error in pcap_findalldevs: %s\n", errbuf);
		getchar();
		getchar();

		return -1;
	}
	
	// 打印获取到的所有网络设备的信息及ip地址
	int nDevCnt = 0;
	for(auto item = devices; item; item = item->next)
	{
		++nDevCnt;
		char szDevIp[INET_ADDRSTRLEN] = { 0 };
		GetSelfAddr(item, szDevIp);
		printf("%d ", nDevCnt);
		// printf("name: %s ", item->name);
		printf("description: %s ", item->description);
		printf("ip: %s", szDevIp);
		printf("\n");
	}

	printf("请选择设备：(1-%d)", nDevCnt);
	
	int nChoose = 0;
	scanf("%d", &nChoose);
	
	if(nChoose<1||nChoose>nDevCnt){
		printf("number out of range\n");
		pcap_freealldevs(devices);
		getchar();
		getchar();

		return -1;
	}
	
	pcap_if_t* dev = devices;
	for(int i = 1; i < nChoose && dev; ++i)
	{
		 dev = dev->next;
	}

	char szLocalIp[17] = { 0 }, szNetMask[17] = { 0 }; 
	unsigned char szLocalMac[7] = { 0 };          //本机MAC地址
	// 获取网卡设备的IP和掩码，蓝牙之类的会失败
	if(false == GetSelfAddr(dev, szLocalIp, szNetMask))
	{
		DBG_WARNNING("GetSelfAddr fail");
		getchar();
		getchar();

		return -1;
	}

	// 获取网卡设备mac地址
	if(false == get_mac_from_pcap_dev_name(dev->name, szLocalMac))
	{
		DBG_WARNNING("get_mac_from_pcap_dev_name fail");
		getchar();
		getchar();

		return -1;
	}

	// 打开一个pcap的对象，后续所有的操作都会基于这个对象进行
	pcap_t *pPcapHandle = pcap_open_live(dev->name, BUFSIZ, 1, 1000, errbuf);
	if(NULL == pPcapHandle)
	{
		fprintf(stderr,"\nUnable to open the adapter.[%s]\n", errbuf);
		getchar();
		getchar();

		return -2;
	}
	
	struct bpf_program fcode;
	// 生成过滤条件
	if(pcap_compile(pPcapHandle, &fcode, "arp", 1, 0xffffff) < 0)
	{
		fprintf(stderr,"\nError compiling filter: wrong syntax.\n");
		pcap_close(pPcapHandle);
		getchar();
		getchar();

		return -3;
	}

	// 将生成好的过滤条件绑定到前面打开的pcap对象上
	if(pcap_setfilter(pPcapHandle, &fcode)<0)
	{
		fprintf(stderr,"\nError setting the filter\n");
		pcap_close(pPcapHandle);
		getchar();
		getchar();

		return -4;
	}

	printf("本机：IP: %s, MAC:%02x-%02x-%02x-%02x-%02x-%02x\n", 
		szLocalIp, szLocalMac[0], szLocalMac[1], szLocalMac[2], szLocalMac[3], szLocalMac[4], szLocalMac[5]);
	struct sparam sp;
	sp.adhandle = pPcapHandle;
	sp.ip = szLocalIp;
	sp.mac = (unsigned char *)szLocalMac;
	sp.netmask = szNetMask;

	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) SendArpPacket, &sp, 0, NULL);

	struct pcap_pkthdr * pkt_header;
	while (true) {
		// 把cpu时间片让出去，再次进行排队，防止cpu飙升
		Sleep(0);
		if (g_bFlag) {
			//DBG_INFO("获取MAC地址完成");
			break;
		}

		const u_char *pkt_data = NULL;
		int res = pcap_next_ex(pPcapHandle, &pkt_header, &pkt_data);
		if (1 != res)
		{
			// 成功会返回1，0代表这个时间段没有数据包，这里过滤掉
			if(res != 0)
			{
				DBG_WARNNING("pcap_next_ex fail. res: %d", res);
			}

			continue;
		}
		
		ArpPacket *rArpPacket = (ArpPacket *) pkt_data;
		
		if (rArpPacket->ed.EthType != htons(ETH_ARP)) 
		{
			continue;
		}

		// 不是arp的回复报文不处理，就是过滤掉别人像本机查询的请求
		if (rArpPacket->ah.OperationField != htons(ARP_REPLY))
		{
			continue;
		}

		// 从arp头部里面的源ip和源mac
		char szIpAddr[INET_ADDRSTRLEN] = { 0 };
		inet_ntop(AF_INET, &(rArpPacket->ah.SIp), szIpAddr, INET_ADDRSTRLEN);
		printf("IP:%s MAC:", szIpAddr);
		for (int i = 0; i < 6; i++) 
		{
			printf("%02x", rArpPacket->ah.SMac[i]);
			if(i != 5)
			{
				printf("-");
			}
		}
		printf("\n");
	}
	
	pcap_close(pPcapHandle);
	getchar();
	getchar();

	return 0;
}



