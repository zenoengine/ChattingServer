#include "Logger.h"

Logger::Logger()
{
}

Logger::~Logger()
{
}

void Logger::Init(const char * fileName)
{
	mpFile = fopen(fileName, "a");
	InitializeCriticalSection(&mCS);

}

void Logger::Clear()
{
	fclose(mpFile);
	DeleteCriticalSection(&mCS);
}

void Logger::WriteLog(const char * text)
{
	EnterCriticalSection(&mCS);
	fprintf(mpFile, text);
	fprintf(mpFile, "\n");
	LeaveCriticalSection(&mCS);
}
