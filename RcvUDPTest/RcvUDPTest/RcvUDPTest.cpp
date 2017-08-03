// RcvUDPTest.cpp : Defines the entry point for the console application.
// https://stackoverflow.com/questions/14665543/how-do-i-receive-udp-packets-with-winsock-in-c
#include "stdafx.h"
#include <stdio.h>
#include <winsock2.h>
#include <string.h>
#pragma comment(lib, "Ws2_32.lib")

//#define UDP_PORT 27072
#define UDP_PORT 26090


void error(int sock, const char *msg)
{
	if (sock > 0) {
		closesocket(sock);
	}
	WSACleanup();
	perror(msg);
	exit(0);
}

int main()
{
	WSAData data;
	WSAStartup(MAKEWORD(2, 2), &data);

	int sock, length, n;
	int fromlen;
	struct sockaddr_in server;
	struct sockaddr_in from;
	char buf[1024];
	unsigned short serverPort = UDP_PORT;
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) error(sock, "Opening socket");
	length = sizeof(server);
	memset(&server, 0, length);
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(serverPort);
	if (bind(sock, (struct sockaddr *)&server, length)<0) error(sock, "binding");
	fromlen = sizeof(struct sockaddr_in);
	while (1)
	{
		n = recvfrom(sock, buf, 1024, 0, (struct sockaddr *)&from, &fromlen);
		if (n<0) error(sock, "recvfrom");
		//write(1,"Received a datagram: ", 21);
		//write(1,buf,n);
		printf("Received a datagram: %s", buf);
		//n = sendto(sock, "Got your message\n", 17, 0, (struct sockaddr *)&from, fromlen);
		//if (n<0) error(sock, "sendto");
	}
	closesocket(sock);
	WSACleanup();
	return 0;
}

