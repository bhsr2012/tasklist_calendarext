#pragma once
// ASCII ONLY. GBK sources will eat non-ascii lines.
// Probes are inert unless the environment variable TDL_PROF=1
#include <windows.h>
#include <stdio.h>

static bool TdlPEnabled()
{
	static int s_nEnabled = -1;

	if (s_nEnabled < 0)
	{
		char szBuf[8] = { 0 };
		s_nEnabled = (GetEnvironmentVariableA("TDL_PROF", szBuf, sizeof(szBuf)) > 0 &&
					  szBuf[0] == '1') ? 1 : 0;
	}

	return (s_nEnabled == 1);
}

static __int64 TdlPFreq()
{
	static __int64 f = 0;
	if (!f) { LARGE_INTEGER li; QueryPerformanceFrequency(&li); f = li.QuadPart; }
	return f;
}

static __int64 TdlPNow()
{
	LARGE_INTEGER li; QueryPerformanceCounter(&li); return li.QuadPart;
}

static void TdlPLogMs(const char* tag, double ms)
{
	if (!TdlPEnabled())
		return;

	FILE* fp = fopen("D:\\tmp\\tdl_prof3.log", "a");
	if (fp) { fprintf(fp, "%s\t%.1f ms\n", tag, ms); fclose(fp); }
}

static void TdlPLog(const char* tag, __int64 t0)
{
	TdlPLogMs(tag, (double)(TdlPNow() - t0) * 1000.0 / (double)TdlPFreq());
}

static void TdlPLogI(const char* tag, __int64 t0, int v)
{
	if (!TdlPEnabled())
		return;

	double ms = (double)(TdlPNow() - t0) * 1000.0 / (double)TdlPFreq();
	FILE* fp = fopen("D:\\tmp\\tdl_prof3.log", "a");
	if (fp) { fprintf(fp, "%s(%d)\t%.1f ms\n", tag, v, ms); fclose(fp); }
}
