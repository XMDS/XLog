#include "mylog.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const int8_t NUM_MB = 1;
static const size_t FILE_SIZE = 1048576;
static constexpr size_t MAX_FILE_SIZE = FILE_SIZE * NUM_MB;

mylog::mylog(char* path)
{
	fLog = nullptr;
	LogSize = MAX_FILE_SIZE;
	memset(LogPath, NULL, sizeof(LogPath));
	CreateNewLogFile(path);
}

mylog::~mylog()
{
	fclose(fLog);
}

void mylog::Write(const char* text)
{
	if (GetFileSize() > LogSize)
	{
		fclose(fLog);
		static int8_t id = 1;
		char s[1];
		sprintf(s, "%d", ++id);
		char* name = new char[255];
		strcpy(name, LogPath);
		if (id > 99) {
			Write("log file limiti");
			exit(1);
		}
		name[strlen(name) - (id == 2 ? 4 : (id > 10) ? 6 : 5)] = '\0';
		strcat(name, s);
		strcat(name, ".log");
		if (access(name, F_OK) == 0) 
			remove(name);
		CreateNewLogFile(name);
		delete name;
	}
	fputs(text, fLog);
	fputs("\n", fLog);
	fflush(fLog);
}

void mylog::WriteFormat(const char* format, va_list arg)
{
	char text[255];
	memset(text, NULL, sizeof(text));
	vsnprintf(text, sizeof(text) - 1, format, arg);
	Write(text);
}

void mylog::WriteFormat(const char* format, ...)
{
	char text[255];
	memset(text, NULL, sizeof(text));
	va_list lst;
	va_start(lst, format);
	vsnprintf(text, sizeof(text) - 1, format, lst);
	va_end(lst);
	Write(text);
}

char* mylog::GetTime(char* buf, size_t len, tType type, tStyle style)
{
	timeval tv;
	tm* time;
	gettimeofday(&tv, NULL);
	if (type == tLOCAL)
		time = localtime(&(tv.tv_sec));
	else if (type == tUTC)
		time = gmtime(&(tv.tv_sec));
	else
		return nullptr;
	
	if (style == tDEFAULT)
		strftime(buf, len, "%H:%M:%S", time);
	else if (style == tAll_1)
		strftime(buf, len, "%Y-%m-%d %H:%M:%S", time);
	else if (style == tAll_2) {
		char* t = asctime(time);
		t[strlen(t) - 1] = '\0';
		strcpy(buf, t);
	}
	else 
		return nullptr;
	
	return buf;
}

void mylog::WriteAllInfo(int prio, const char* tag, const char* text, tType type, tStyle style)
{
	char p[0x10], time[0x40];
	switch (prio)
	{
	case LOG_VERBOSE:
		strcpy(p, "[VERBOSE]");
		break;
	case LOG_DEBUG:
		strcpy(p, "[DEBUG]");
		break;
	case LOG_INFO:
		strcpy(p, "[INFO]");
		break;
	case LOG_WARN:
		strcpy(p, "[WARN]");
		break;
	case LOG_ERROR:
		strcpy(p, "[ERROR]");
		break;
	case LOG_FATAL:
		strcpy(p, "[FATAL]");
		break;
	case LOG_SILENT:
		strcpy(p, "[SILENT]");
		break;
	case LOG_UNKNOWN:
	case LOG_DEFAULT:
	default:
		strcpy(p, "");
		break;
	}
	WriteFormat("%s[%s][%s] %s", p, GetTime(time, sizeof(time), type, style), tag, text);
}

void mylog::WriteFormatAllInfo(int prio, const char* tag, tType type, tStyle style, const char* format, va_list arg)
{
	char text[255];
	memset(text, NULL, sizeof(text));
	vsnprintf(text, sizeof(text) - 1, format, arg);
	WriteAllInfo(prio, tag, text, type, style);
}

void mylog::WriteFormatAllInfo(int prio, const char* tag, tType type, tStyle style, const char* format, ...)
{
	char text[255];
	memset(text, NULL, sizeof(text));
	va_list lst;
	va_start(lst, format);
	vsnprintf(text, sizeof(text) - 1, format, lst);
	va_end(lst);
	WriteAllInfo(prio, tag, text, type, style);
}

size_t mylog::GetFileSize()
{
	auto pos = ftell(fLog);
	fseek(fLog, 0, SEEK_END);
	auto size = ftell(fLog);
	fseek(fLog, pos, SEEK_SET);
	return size;
}

void mylog::CreateNewLogFile(char* path)
{
	strcpy(LogPath, path);
	fLog = fopen(LogPath, "wt+");
	if (fLog == NULL) {
		__android_log_write(ANDROID_LOG_ERROR, "mylog", "File open failed! ! !");
		exit(1);
	}
}