#include "SQLConnector.h"
#include <iostream>
#include <string>
#include <mysql.h>

#pragma comment(lib, "libmySQL.lib")

using namespace std;


void SQLConnector::CheckConnect()
{
	MYSQL MySQL; 
	mysql_init(&MySQL);

	if (!mysql_real_connect(&MySQL, "localhost", "root", "qwe123", NULL, 3306, NULL, 0))
	{
		cout << "error\n";
	}
	else
	{
		cout << "success\n";
	}

	mysql_close(&MySQL);
}

bool SQLConnector::InsertChatLog(const string& clientIp, int clientPort, char cmd, const string& message)
{
	if (!Conn)
	{
		Conn = mysql_init(nullptr);
		if (!mysql_real_connect(Conn, "localhost", "root", "qwe123", "tcp_chat", 3306, NULL, 0))
		{
			cout << "[SQL] chat_log connect failed: " << mysql_error(Conn) << "\n";
			mysql_close(Conn);
			Conn = nullptr;
			return false;
		}
	}

	char cmdStr[2] = { cmd ? cmd : ' ', '\0' };

	// message는 클라이언트가 보낸 임의 문자열이라 따옴표가 섞이면 쿼리가 깨지거나
	// SQL 인젝션으로 이어질 수 있어 이스케이프 처리 (실제로 안 하면 깨지는 것 확인함)
	string escaped(message.size() * 2 + 1, '\0');
	unsigned long escapedLen = mysql_real_escape_string(Conn, &escaped[0], message.c_str(), message.size());
	escaped.resize(escapedLen);

	char query[2048];
	snprintf(query, sizeof(query),
		"INSERT INTO chat_log (client_ip, client_port, cmd, message) VALUES ('%s', %d, '%s', '%s')",
		clientIp.c_str(), clientPort, cmdStr, escaped.c_str());

	if (mysql_query(Conn, query))
	{
		cout << "[SQL] chat_log insert failed: " << mysql_error(Conn) << "\n";
		return false;
	}

	return true;
}

SQLConnector::~SQLConnector()
{
	if (Conn) mysql_close(Conn);
}
