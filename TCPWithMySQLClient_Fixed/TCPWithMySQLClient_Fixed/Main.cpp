#define _CRT_SECURE_NO_WARNINGS


#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include <WinSock2.h>
#include <WS2tcpip.h>

#include "../../Socket_err.h"

#pragma comment(lib, "ws2_32")

char* SERVERIP = (char*)"127.0.0.1";
#define SERVERPORT 17325
#define BUFSIZE 50


int main()
{
	int Retval;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

	SOCKET ServerSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (ServerSocket == INVALID_SOCKET) err_quit("socket()");

	SOCKADDR_IN ServerSockAddr;
	memset(&ServerSockAddr, 0, sizeof(ServerSockAddr));
	ServerSockAddr.sin_family = AF_INET;
	ServerSockAddr.sin_port = htons(SERVERPORT);
	inet_pton(AF_INET, SERVERIP, &ServerSockAddr.sin_addr);

	Retval = connect(ServerSocket, (struct sockaddr*)&ServerSockAddr, sizeof(ServerSockAddr));
	if (Retval == SOCKET_ERROR) err_quit("connect()");

	char Buf[BUFSIZE + 1];
	const char* TestData[] = {
		"안녕하세요",
		"반가워요",
		"오늘따라 할 이야기가 많을 것 같네요",
		"저도 그렇네요",
	};

	for (int i = 0; i < 4; ++i)
	{
		memset(Buf, '#', sizeof(Buf));
		strncpy(Buf, TestData[i], strlen(TestData[i]));

		Retval = send(ServerSocket, Buf, BUFSIZE, 0);
		if (Retval == SOCKET_ERROR)
		{
			err_display("send()");
			break;
		}
		printf("[TCP 클라이언트] %d바이트를 보냈습니다. \n", Retval);

		Retval = recv(ServerSocket, Buf, BUFSIZE, MSG_WAITALL);
		if (Retval == SOCKET_ERROR)
		{
			err_display("recv()");
			break;
		}
		else if (Retval == 0) break;

		Buf[Retval] = '\0';
		printf("[TCP 클라이언트] %d바이트를 받았습니다.\n", Retval);
		printf("[받은 데이터] %s\n", Buf);
	}

	//int Len;

	//while (1)
	//{
	//	printf("\n[보낼 데이터] ");
	//	if (fgets(Buf, BUFSIZE + 1, stdin) == NULL) break;

	//	Len = (int)strlen(Buf);

	//	if (Buf[Len - 1] == '\n') Buf[Len - 1] = '\0';
	//	if (strlen(Buf) == 0) break;

	//	Retval = send(ServerSocket, Buf, (int)strlen(Buf), 0);
	//	if (Retval == SOCKET_ERROR)
	//	{
	//		err_display("send()");
	//		break;
	//	}

	//	printf("[TCP 클라이언트] %d바이트를 보냈습니다. \n", Retval);

	//	Retval = recv(ServerSocket, Buf, Retval, MSG_WAITALL);
	//	if (Retval == SOCKET_ERROR)
	//	{
	//		err_display("recv()");
	//		break;
	//	}
	//	else if (Retval == 0) break;

	//	Buf[Retval] = '\0';
	//	printf("[TCP 클라이언트] %d바이트를 받았습니다.\n", Retval);
	//	printf("[받은 데이터] %s\n", Buf);
	//}

	closesocket(ServerSocket);

	WSACleanup();

	return 0;
}