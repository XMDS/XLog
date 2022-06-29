#pragma once
#include <stdio.h>
#include <unistd.h>
#include <android/log.h>

enum LogPrio {
	LOG_UNKNOWN = 0,
	LOG_DEFAULT,
	LOG_VERBOSE,
	LOG_DEBUG,
	LOG_INFO,
	LOG_WARN,
	LOG_ERROR,
	LOG_FATAL,
	LOG_SILENT,
};

enum tType
{
	tLOCAL, //
	tUTC,
};

enum tStyle
{
	tDEFAULT,//%H:%M:%S
	tAll_1, //%Y-%m-%d %H:%M:%S
	tAll_2, //
};

class mylog
{
public:
	char LogPath[255];
	size_t LogSize;
	FILE* fLog;
	mylog(char* path);
	~mylog();
	void Write(const char* text);
	void WriteFormat(const char* format, va_list arg);
	void WriteFormat(const char* format, ...);
	void WriteAllInfo(int prio, const char* tag, const char* text, tType type, tStyle style);
	void WriteFormatAllInfo(int prio, const char* tag, tType type, tStyle style, const char* format, va_list arg);
	void WriteFormatAllInfo(int prio, const char* tag, tType type, tStyle style, const char* format, ...);
	size_t GetFileSize();
	void CreateNewLogFile(char* path);
	
private:
	static char* GetTime(char* buf, size_t len, tType type, tStyle style);

};