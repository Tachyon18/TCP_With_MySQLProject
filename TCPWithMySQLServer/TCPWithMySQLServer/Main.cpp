#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>

#include <WinSock2.h>
#include <WS2tcpip.h>

#include "SQLConnector.h"

#include "../../Socket_err.h"

#pragma comment(lib, "ws2_32")

using namespace std;

#define SERVERPORT 17325
#define BUFSIZE 512

void InterpretMessage(char* buffer, int BufferLength, char* Addr, SOCKADDR_IN ClientSocket, SQLConnector& SQL);

DWORD WINAPI ProcessClient(LPVOID arg)
{
	SOCKET ClientSocket = (SOCKET)arg;
	int Retval;
	struct sockaddr_in ClientSockAddr;
	char Addr[INET_ADDRSTRLEN];
	int AddrLen;
	char Buf[BUFSIZE + 1];

	AddrLen = sizeof(ClientSockAddr);
	getpeername(ClientSocket, (struct sockaddr*)&ClientSockAddr, &AddrLen);
	inet_ntop(AF_INET, &ClientSockAddr.sin_addr, Addr, sizeof(Addr));

	// 클라이언트(스레드)마다 자기 전용 DB 커넥션을 가짐 — mysql 커넥션은 여러 스레드가
	// 동시에 공유해서 쓰도록 만들어져 있지 않으므로, 스레드 간에 공유하지 않음.
	SQLConnector SQL;

	while (1)
	{
		Retval = recv(ClientSocket, Buf, BUFSIZE, 0);
		if (Retval == SOCKET_ERROR)
		{
			err_display("recv()");
			break;
		}
		else if (Retval == 0) break;

		InterpretMessage(Buf, Retval, Addr, ClientSockAddr, SQL);

		Retval = send(ClientSocket, Buf, Retval, 0);
		if (Retval == SOCKET_ERROR)
		{
			err_display("send()");
			break;
		}

	}

	closesocket(ClientSocket);
	printf("[TCP 종료] 클라이언트 종료 : IP 주소 = %s , 포트 번호 = %d\n", Addr, ntohs(ClientSockAddr.sin_port));

	return 0;
}

void InterpretMessage(char* buffer, int BufferLength, char* Addr, SOCKADDR_IN ClientSocket, SQLConnector& SQL)
{
	string Temp = "";
	string Messages;
	char cmd = 0;

	// buffer[i] != '#' 만으로 끝을 찾으면, '#'로 끝나지 않는(=프로토콜을 지키지 않은)
	// 메시지가 들어왔을 때 수신 버퍼 경계를 넘어 계속 읽는 버퍼 오버리드가 생긴다.
	for (int i = 0; i < BufferLength && buffer[i] != '#'; i++)
	{
		if ((buffer[i] == '|' && (cmd == 0)))
		{
			cmd = Temp[0];
			Temp = "";
		}
		else if ((buffer[i] == '|') && (cmd != 0))
		{
			Messages.append(Temp);
			Temp = "";
		}
		else
		{
			Temp = Temp + buffer[i];
		}
	}

	Messages = Messages + "\0";

	if (cmd == 'C')
	{
		printf("[TCP/%s : %d] : Client Function Called!\n", Addr, ntohs(ClientSocket.sin_port));
	}
	else if (cmd == 'Q')
	{

	}

	int Length = Messages.size();

	char* Message = new char[Length];

	strcpy(Message, Messages.c_str());

	printf("[TCP/%s : %d] %s\n", Addr, ntohs(ClientSocket.sin_port), Message);

	SQL.InsertChatLog(Addr, ntohs(ClientSocket.sin_port), cmd, Messages);

	delete[] Message;

}

int main()
{
	SQLConnector SQL;

	SQL.CheckConnect();

	int Retval;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

	printf("[알림] WinSock 초기화 성공\n");

	SOCKET ServerSocket = socket(AF_INET, SOCK_STREAM, 0);

	if (ServerSocket == INVALID_SOCKET) err_quit("socket()");
	printf("[알림] Socket 생성 성공\n");

	SOCKADDR_IN ServerSockAddr;
	memset(&ServerSockAddr, 0, sizeof(ServerSockAddr));
	ServerSockAddr.sin_family = PF_INET;
	ServerSockAddr.sin_addr.s_addr = INADDR_ANY;
	ServerSockAddr.sin_port = htons(SERVERPORT);

	Retval = bind(ServerSocket, (SOCKADDR*)&ServerSockAddr, sizeof(ServerSockAddr));
	if (Retval == SOCKET_ERROR) err_quit("bind()");

	Retval = listen(ServerSocket, SOMAXCONN);
	if (Retval == SOCKET_ERROR) err_quit("listen()");

	SOCKET ClientSocket;
	SOCKADDR_IN ClientSockAddr;
	int AddrLen;
	HANDLE hThread;

	while (1)
	{
		AddrLen = sizeof(ClientSockAddr);
		ClientSocket = accept(ServerSocket, (struct sockaddr*)&ClientSockAddr, &AddrLen);
		if (ClientSocket == INVALID_SOCKET)
		{
			err_display("accept()");
			break;
		}

		char Addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &ClientSockAddr.sin_addr, Addr, sizeof(Addr));
		printf("\n[TCP 접속] 클라이언트 접속: IP 주소=%s , 포트 번호=%d\n", Addr, ntohs(ClientSockAddr.sin_port));

		hThread = CreateThread(NULL, 0, ProcessClient, (LPVOID)ClientSocket, 0, NULL);
		if (hThread == NULL) { closesocket(ClientSocket); }
		else { CloseHandle(hThread); }
	}

	closesocket(ServerSocket);

	WSACleanup();

	return 0;
}