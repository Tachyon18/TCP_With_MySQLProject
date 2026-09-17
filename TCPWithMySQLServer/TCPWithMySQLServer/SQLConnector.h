#pragma once

#include <iostream>
#include <string>
//#include <my_global.h>
#include <mysql.h>

class SQLConnector
{
	MYSQL* Conn = nullptr;

public:

	void CheckConnect();
	bool InsertChatLog(const std::string& clientIp, int clientPort, char cmd, const std::string& message);

	~SQLConnector();
};

