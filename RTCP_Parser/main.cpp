#define _CRT_SECURE_NO_WARNINGS
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct RTCP_Header
{
	unsigned short rc:5;
	unsigned short padding:1;
	unsigned short version:2; //1 char
	unsigned short payloadtype:8; //2 char
	unsigned short length; //3,4 char
	unsigned int ssrc; //5,6,7,8 char
};

struct Rtcp_SR
{
	unsigned int  NtpTimestampMSW;
	unsigned int  NtpTimestampLSW;
	unsigned int  RtpTimestamp;
	unsigned int  SenderPacketCnt;
	unsigned int  SenderOctetCnt;
};

void parse_udp_packet(unsigned char* packet, int packet_length)
{	
	int offset = 0;
	while (offset < packet_length)
	{
		RTCP_Header* pHeader =  (RTCP_Header*)(packet + offset);
		printf("ssrc: 0x%08x ver: %d, padding: %d, rc: %d, type: %d, paloadlen: %d\n", 
			ntohl(pHeader->ssrc), pHeader->version, pHeader->padding, pHeader->rc, pHeader->payloadtype, 4 * ntohs(pHeader->length));

		if(pHeader->payloadtype == 200)
		{
			Rtcp_SR* pstSR = (Rtcp_SR*)(packet + offset + 8);

			printf("octetCnt:%d, packetCnt: %d\n", 
				ntohl(pstSR->SenderOctetCnt), ntohl(pstSR->SenderPacketCnt));
		}

		offset += 4 * ntohs(pHeader->length) + 4;
	}

	return;
}

int main(int argc, char* argv[]) {
	// 假设这里有一个包含多个RTCP数据包的UDP数据包
	unsigned char udp_packet[2048] = { 0 };
	int udp_length = sizeof(udp_packet) / sizeof(udp_packet[0]);
	FILE* fRtcpData = fopen(argv[1], "r");
	int nPackSize = fread(udp_packet, 1, udp_length, fRtcpData);

	// 解析UDP数据包中的RTCP数据包
	parse_udp_packet(udp_packet, nPackSize);

	return 0;
}
