#pragma once
// ASCII ONLY
// Probes are inert unless the environment variable TDL_PROF=1
#include <windows.h>
#include <stdio.h>

static bool CalPEnabled()
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

static __int64 CalPFreq()
{
	static __int64 f = 0;
	if (!f) { LARGE_INTEGER li; QueryPerformanceFrequency(&li); f = li.QuadPart; }
	return f;
}

static __int64 CalPNow()
{
	LARGE_INTEGER li; QueryPerformanceCounter(&li); return li.QuadPart;
}

static void CalPLog(const char* tag, __int64 t0)
{
	if (!CalPEnabled())
		return;

	double ms = (double)(CalPNow() - t0) * 1000.0 / (double)CalPFreq();
	FILE* fp = fopen("D:\\tmp\\tdl_prof3.log", "a");
	if (fp) { fprintf(fp, "  %s\t%.1f ms\n", tag, ms); fclose(fp); }
}
