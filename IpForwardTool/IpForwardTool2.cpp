/* 获取所有的路由，同时支持ipv4和ipv6，支持vista及以上 */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

/* Note: could also use malloc() and free() */

int main()
{
	PMIB_IPFORWARD_TABLE2 pIpForwardTable2 = (PMIB_IPFORWARD_TABLE2) MALLOC(sizeof (MIB_IPFORWARD_TABLE2));
    if (pIpForwardTable2 == NULL) {
        printf("Error allocating memory\n");
        return 1;
    }

    if(NO_ERROR != GetIpForwardTable2(AF_UNSPEC, &pIpForwardTable2))
	{
		printf("GetIpForwardTable2 fail. gle: 0x%08x\n", GetLastError());
		FREE(pIpForwardTable2);
		return 1;
    }
    
    printf("\tNumber of entries: %d\n", (int) pIpForwardTable2->NumEntries);
    for (int i = 0; i < (int) pIpForwardTable2->NumEntries; i++) {
		printf("\nRoute[%d]\n", i);
		char szGatewayIp[INET6_ADDRSTRLEN] = { 0 };
		char szDestIp[INET6_ADDRSTRLEN] = { 0 };
		// NET_LUID          InterfaceLuid;
		printf("\tIfType: %d ", (int)pIpForwardTable2->Table[i].InterfaceLuid.Info.IfType);
		switch(pIpForwardTable2->Table[i].InterfaceLuid.Info.IfType)
		{
		case IF_TYPE_OTHER:
			printf("Some other type of network interface\n");
			break;

		case IF_TYPE_ETHERNET_CSMACD:
			printf("An Ethernet network interface\n");
			break;

		case IF_TYPE_ISO88025_TOKENRING:
			printf("A token ring network interface\n");
			break;
		case IF_TYPE_PPP:
			printf("A PPP network interface\n");
			break;
		case IF_TYPE_SOFTWARE_LOOPBACK:
			printf("A software loopback network interface\n");
			break;
		case IF_TYPE_ATM:
			printf("An ATM network interface\n");
			break;
		case IF_TYPE_IEEE80211:
			printf("An IEEE 802.11 wireless network interface\n");
			break;
		case IF_TYPE_TUNNEL:
			printf("A tunnel type encapsulation network interface\n");
			break;
		case IF_TYPE_IEEE1394:
			printf("An IEEE 1394 (Firewire) high performance serial bus network interface\n");
			break;
		default:
			break;
		}

		// NET_IFINDEX       InterfaceIndex;
		printf("\tInterfaceIndex: %d\n", (int)pIpForwardTable2->Table[i].InterfaceIndex);

		// IP_ADDRESS_PREFIX DestinationPrefix;
		printf("\tDestinationPrefix PrefixLength: %d ", (int)pIpForwardTable2->Table[i].DestinationPrefix.PrefixLength);
		switch(pIpForwardTable2->Table[i].DestinationPrefix.Prefix.si_family)
		{
		case AF_INET:
			strcpy_s(szDestIp, sizeof (szDestIp), inet_ntoa(pIpForwardTable2->Table[i].DestinationPrefix.Prefix.Ipv4.sin_addr));
			printf("[ipv4] addr: %s\n", szDestIp);
			break;

		case AF_INET6:
			inet_ntop(AF_INET6, &pIpForwardTable2->Table[i].DestinationPrefix.Prefix.Ipv6.sin6_addr, szDestIp, INET6_ADDRSTRLEN);
			printf("[ipv6] addr: %s\n", szDestIp);
			break;

		default:
			printf("\n");
			break;
		}

		// SOCKADDR_INET     NextHop;
		printf("\tNextHop ");
		switch(pIpForwardTable2->Table[i].NextHop.si_family)
		{
		case AF_INET:
			strcpy_s(szGatewayIp, sizeof (szGatewayIp), inet_ntoa(pIpForwardTable2->Table[i].NextHop.Ipv4.sin_addr));
			printf("[ipv4] NextHop: %s\n", szGatewayIp);
			break;

		case AF_INET6:
			inet_ntop(AF_INET6, &pIpForwardTable2->Table[i].NextHop.Ipv6.sin6_addr, szGatewayIp, INET6_ADDRSTRLEN);
			printf("[ipv6] NextHop: %s\n", szGatewayIp);
			
			break;

		default:
			printf("\n");
			break;
		}

		// UCHAR             SitePrefixLength;
		printf("\tSitePrefixLength: %d\n", (int)pIpForwardTable2->Table[i].SitePrefixLength);
		
		// ULONG   ValidLifetime;
		printf("\tValidLifetime: %d\n", (int)pIpForwardTable2->Table[i].ValidLifetime);
		
		// ULONG   PreferredLifetime;
		printf("\tPreferredLifetime: %d\n", (int)pIpForwardTable2->Table[i].PreferredLifetime);
		
		// ULONG   Metric;
		printf("\tMetric: %d\n", (int)pIpForwardTable2->Table[i].Metric);
		
		// NL_ROUTE_PROTOCOL Protocol;
		printf("\tProtocol: %d ", (int)pIpForwardTable2->Table[i].Protocol);
		switch (pIpForwardTable2->Table[i].Protocol) {
		case MIB_IPPROTO_OTHER:
			printf("other\n");
			break;
		case MIB_IPPROTO_LOCAL:
			printf("local interface\n");
			break;
		case MIB_IPPROTO_NETMGMT:
			printf("static route set through network management \n");
			break;
		case MIB_IPPROTO_ICMP:
			printf("result of ICMP redirect\n");
			break;
		case MIB_IPPROTO_EGP:
			printf("Exterior Gateway Protocol (EGP)\n");
			break;
		case MIB_IPPROTO_GGP:
			printf("Gateway-to-Gateway Protocol (GGP)\n");
			break;
		case MIB_IPPROTO_HELLO:
			printf("Hello protocol\n");
			break;
		case MIB_IPPROTO_RIP:
			printf("Routing Information Protocol (RIP)\n");
			break;
		case MIB_IPPROTO_IS_IS:
			printf
				("Intermediate System-to-Intermediate System (IS-IS) protocol\n");
			break;
		case MIB_IPPROTO_ES_IS:
			printf("End System-to-Intermediate System (ES-IS) protocol\n");
			break;
		case MIB_IPPROTO_CISCO:
			printf("Cisco Interior Gateway Routing Protocol (IGRP)\n");
			break;
		case MIB_IPPROTO_BBN:
			printf("BBN Internet Gateway Protocol (IGP) using SPF\n");
			break;
		case MIB_IPPROTO_OSPF:
			printf("Open Shortest Path First (OSPF) protocol\n");
			break;
		case MIB_IPPROTO_BGP:
			printf("Border Gateway Protocol (BGP)\n");
			break;
		case MIB_IPPROTO_NT_AUTOSTATIC:
			printf("special Windows auto static route\n");
			break;
		case MIB_IPPROTO_NT_STATIC:
			printf("special Windows static route\n");
			break;
		case MIB_IPPROTO_NT_STATIC_NON_DOD:
			printf
				("special Windows static route not based on Internet standards\n");
			break;
		default:
			printf("UNKNOWN Proto value\n");
			break;
		}
		
		// BOOLEAN Loopback;
		printf("\tLoopback: %d\n", (int)pIpForwardTable2->Table[i].Loopback);
		
		// BOOLEAN AutoconfigureAddress;
		printf("\tAutoconfigureAddress: %d\n", (int)pIpForwardTable2->Table[i].AutoconfigureAddress);
		
		// BOOLEAN Publish;
		printf("\tPublish: %d\n", (int)pIpForwardTable2->Table[i].Publish);
		
		// BOOLEAN Immortal;
		printf("\tImmortal: %d\n", (int)pIpForwardTable2->Table[i].Immortal);
		
		// ULONG   Age;
		printf("\tAge: %d\n", (int)pIpForwardTable2->Table[i].Age);
		
		// NL_ROUTE_ORIGIN   Origin;
		printf("\tOrigin: %d ", (int)pIpForwardTable2->Table[i].Age);
		switch(pIpForwardTable2->Table[i].Origin)
		{
		case NlroManual:
			printf("A result of manual configuration\n");
			break;

		case NlroWellKnown:
			printf("A well-known route\n");
			break;
		case NlroDHCP:
			printf("A result of DHCP configuration\n");
			break;
		case NlroRouterAdvertisement:
			printf("The result of router advertisement\n");
			break;
		case Nlro6to4:
			printf("A result of 6to4 tunneling\n");
			break;

		default:
			printf("\n");
			break;
		}
    }
    FREE(pIpForwardTable2);
    return 0;
    
}