#pragma once
#include <stdarg.h>
#include <stdio.h>
#include <varargs.h>
#include <IMiniLog.h>
class FLog :
	public IMiniLog
{
public:

	virtual void LogV(const ELogType nType, const char* szFormat, ...) PRINTF_PARAMS(3, 0) {
		CHAR OutputBuf[4096];
		va_list args;
		va_start(args, szFormat);
		vsnprintf(OutputBuf, 4095, szFormat, args);
		::OutputDebugStringA(OutputBuf);
		printf("%s\n", OutputBuf);
		va_end(args);
	}
};

