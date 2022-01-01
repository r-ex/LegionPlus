#include "pch.h"
#include "Logger.h"

std::string AddTimestamp(std::string msg)
{
	return std::string("[" + Utils::GetISOTimestamp() + "] ") + msg;
}

void Logger::Info(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	std::string sFmt(fmt);

	sFmt = AddTimestamp("[I] " + sFmt);

	vprintf(sFmt.c_str(), args);

	va_end(args);
}

void Logger::Info(std::string msg)
{
	msg = AddTimestamp("[I] " + msg);
	printf(msg.c_str());
}

void Logger::Warning(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	std::string sFmt(fmt);

	sFmt = AddTimestamp("[W] " + sFmt);

	vprintf(sFmt.c_str(), args);

	va_end(args);
}

void Logger::Warning(std::string msg)
{
	msg = AddTimestamp("[W] " + msg);
	printf(msg.c_str());
}

Logger g_Logger;