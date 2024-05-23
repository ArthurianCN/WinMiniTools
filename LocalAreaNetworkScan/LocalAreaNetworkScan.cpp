#include <stdlib.h>
#include <stdio.h>

#include "pcap.h"
#include "packet32.h"
#include <ntddndis.h>

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"wpcap.lib")
#pragma comment(lib,"Packet.lib")


#define ETH_ARP         0x0806		//��̫��֡���ͱ�ʾ�������ݵ����ͣ�����ARP�����Ӧ����˵�����ֶε�ֵΪx0806
#define ARP_HARDWARE    1			//Ӳ�������ֶ�ֵΪ��ʾ��̫����ַ
#define ETH_IP          0x0800		//Э�������ֶα�ʾҪӳ���Э���ַ����ֵΪx0800��ʾIP��ַ
#define ARP_REQUEST     1			//ARP����
#define ARP_REPLY       2			//ARPӦ��



#define DBG_INFO(format, ...) \
		fprintf(stdout, "[info] [%d %s]" format "\n", __LINE__, __FUNCTION__, ##__VA_ARGS__);

#define DBG_WARNNING(format, ...) \
	fprintf(stdout, "[warnning] [%d %s]" format "\n", __LINE__, __FUNCTION__, ##__VA_ARGS__);

#pragma pack(push, 1)

// ��̫֡ͷ���ṹ�壬��14�ֽ�
struct EthernetHeader
{
	u_char DestMAC[6];					//Ŀ��MAC��ַ 6�ֽ�
	u_char SourMAC[6];					//ԴMAC��ַ 6�ֽ�
	u_short EthType;					//��һ��Э�����ͣ���0x0800������һ����IPЭ�飬0x0806Ϊarp  2�ֽ�
};


// ARPͷ���ṹ��28�ֽ�
struct Arpheader {
	unsigned short HardwareType;		//Ӳ������
	unsigned short ProtocolType;		//Э������
	unsigned char HardwareAddLen;		//Ӳ����ַ����
	unsigned char ProtocolAddLen;		//Э���ַ����
	unsigned short OperationField;		//�����ֶ�
	unsigned char SMac[6];				//Դmac��ַ
	unsigned int SIp;					//Դip��ַ
	unsigned char DestMacAdd[6];		//Ŀ��mac��ַ
	unsigned int DestIpAdd;				//Ŀ��ip��ַ
};

//arp���ṹ
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

// ��ȡ������ip������
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
			char ip_str[INET_ADDRSTRLEN]; // ����һ���㹻��Ļ��������洢IP��ַ�ַ���  
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


// ������ͨ��ip�����������������IP����  
int calculateSubnetHosts(const char *ip_str, const char *mask_str) {  
	// ��IP��ַ����������ת��Ϊ32λ�޷�������  
	unsigned int ip = inet_addr(ip_str);  
	unsigned int mask = inet_addr(mask_str);  

	// ���ת���Ƿ�ɹ�  
	if (ip == INADDR_NONE || mask == INADDR_NONE) {  
		perror("inet_addr failed");  
		exit(EXIT_FAILURE);  
	}  

	// ��ȡ�������ֵ�λ����������������0��λ����  
	int host_bits = 0;  
	unsigned int mask_inv = ~mask; // ȡ���õ��������ֵ�����  
	while (mask_inv) {  
		if (mask_inv & 1) {  
			host_bits++;  
		}  
		mask_inv >>= 1;  
	}  

	// ���������е�IP�����������������ַ�͹㲥��ַ��  
	int subnet_hosts = (1 << host_bits) - 2;  
	return subnet_hosts;  
}  


/* ���������ȫ�����ܵ�IP��ַ����ARP������߳� */
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
	// Ŀ�ĵ�ַΪ�㲥��ַ
	memset(eh.DestMAC, 0xff, 6);
	// Դmac��ַΪ����
	memcpy(eh.SourMAC, mac, 6);
	eh.EthType = htons(ETH_ARP);

	Arpheader ah;
	memcpy(ah.SMac, mac, 6);
	memset(ah.DestMacAdd, 0x00, 6);
	
	ah.HardwareType = htons(ARP_HARDWARE);
	ah.ProtocolType = htons(ETH_IP);
	ah.HardwareAddLen = 6;
	ah.ProtocolAddLen = 4;
	// ���󷽵�IP��ַΪ�����IP��ַ
	ah.SIp = inet_addr(ip);
	ah.OperationField = htons(ARP_REQUEST);

	// ��ȡ�����������ip����
	int nMaxIpNum = calculateSubnetHosts(ip, netmask);
	
	//�������������ip��ַ��Ӧ���豸�ķ���arp����
	for (int i = 0; i < nMaxIpNum; i++) 
	{
		// arp���ĵ�����
		ah.DestIpAdd = htonl(hisip + i);
		// ����һ��ARP����
		unsigned char sendbuf[42] = { 0 }; //arp���ṹ��С
		memcpy(sendbuf, &eh, sizeof(eh));
		memcpy(sendbuf + sizeof(eh), &ah, sizeof(ah));
		if (pcap_sendpacket(adhandle, sendbuf, 42) != 0) 
		{
			DBG_WARNNING("PacketSendPacket in getmine Error: 0x%08x\n", GetLastError());
		}
	}

	// ��һ������ʱ�䣬����ĳ���豸��û��������߽��̾��˵���
	Sleep(3000);
	g_bFlag = true;
	return 0;
}


int main()
{
	char errbuf[PCAP_ERRBUF_SIZE] = { 0 };
	pcap_if_t* devices = NULL;
	// ��ȡ�豸�б�
	if (pcap_findalldevs(&devices, errbuf) == -1) {
		fprintf(stderr, "Error in pcap_findalldevs: %s\n", errbuf);
		getchar();
		getchar();

		return -1;
	}
	
	// ��ӡ��ȡ�������������豸����Ϣ��ip��ַ
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

	printf("��ѡ���豸��(1-%d)", nDevCnt);
	
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
	unsigned char szLocalMac[7] = { 0 };          //����MAC��ַ
	// ��ȡ�����豸��IP�����룬����֮��Ļ�ʧ��
	if(false == GetSelfAddr(dev, szLocalIp, szNetMask))
	{
		DBG_WARNNING("GetSelfAddr fail");
		getchar();
		getchar();

		return -1;
	}

	// ��ȡ�����豸mac��ַ
	if(false == get_mac_from_pcap_dev_name(dev->name, szLocalMac))
	{
		DBG_WARNNING("get_mac_from_pcap_dev_name fail");
		getchar();
		getchar();

		return -1;
	}

	// ��һ��pcap�Ķ��󣬺������еĲ��������������������
	pcap_t *pPcapHandle = pcap_open_live(dev->name, BUFSIZ, 1, 1000, errbuf);
	if(NULL == pPcapHandle)
	{
		fprintf(stderr,"\nUnable to open the adapter.[%s]\n", errbuf);
		getchar();
		getchar();

		return -2;
	}
	
	struct bpf_program fcode;
	// ���ɹ�������
	if(pcap_compile(pPcapHandle, &fcode, "arp", 1, 0xffffff) < 0)
	{
		fprintf(stderr,"\nError compiling filter: wrong syntax.\n");
		pcap_close(pPcapHandle);
		getchar();
		getchar();

		return -3;
	}

	// �����ɺõĹ��������󶨵�ǰ��򿪵�pcap������
	if(pcap_setfilter(pPcapHandle, &fcode)<0)
	{
		fprintf(stderr,"\nError setting the filter\n");
		pcap_close(pPcapHandle);
		getchar();
		getchar();

		return -4;
	}

	printf("������IP: %s, MAC:%02x-%02x-%02x-%02x-%02x-%02x\n", 
		szLocalIp, szLocalMac[0], szLocalMac[1], szLocalMac[2], szLocalMac[3], szLocalMac[4], szLocalMac[5]);
	struct sparam sp;
	sp.adhandle = pPcapHandle;
	sp.ip = szLocalIp;
	sp.mac = (unsigned char *)szLocalMac;
	sp.netmask = szNetMask;

	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) SendArpPacket, &sp, 0, NULL);

	struct pcap_pkthdr * pkt_header;
	while (true) {
		// ��cpuʱ��Ƭ�ó�ȥ���ٴν����Ŷӣ���ֹcpu���
		Sleep(0);
		if (g_bFlag) {
			//DBG_INFO("��ȡMAC��ַ���");
			break;
		}

		const u_char *pkt_data = NULL;
		int res = pcap_next_ex(pPcapHandle, &pkt_header, &pkt_data);
		if (1 != res)
		{
			// �ɹ��᷵��1��0�������ʱ���û�����ݰ���������˵�
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

		// ����arp�Ļظ����Ĳ��������ǹ��˵������񱾻���ѯ������
		if (rArpPacket->ah.OperationField != htons(ARP_REPLY))
		{
			continue;
		}

		// ��arpͷ�������Դip��Դmac
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



