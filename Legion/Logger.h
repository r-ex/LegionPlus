#pragma once
class Logger
{
public:
	static void Info(const char* fmt, ...);
	static void Info(std::string msg);

	static void Warning(const char* fmt, ...);
	static void Warning(std::string msg);
};

extern Logger g_Logger;