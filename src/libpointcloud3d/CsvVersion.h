#ifndef _CSV_VERSION_H
#define _CSV_VERSION_H

#include <string>
#include <time.h>
using namespace std;

#define LIB_VERSION "1.0.2"

class CsvVersion
{
public: 
	static const bool IS_TEST_VESION = true;
	static string getVersion()
	{
		char stringVersion[128];
		char stdDate[128] = { 0 }; getReleaseTime(stdDate);

		sprintf(stringVersion, "%s.%s", LIB_VERSION, stdDate);

		if (true == IS_TEST_VESION)
			return std::string(stringVersion) + "B";
		else
			return std::string(stringVersion);
	}

	static string getReleaseTime(char *stdDate = NULL)
	{
		struct tm lpCompileTime;
		
		char Mmm[4] = "Jan";
		sscanf(__DATE__, "%4s %d %d", Mmm,
			&lpCompileTime.tm_mday, &lpCompileTime.tm_year);
		//lpCompileTime.tm_year -= 1900;

		std::string str = Mmm;

		if (str == "Jan")
		{
			lpCompileTime.tm_mon = 1;
		}
		else if (str == "Feb")
		{
			lpCompileTime.tm_mon = 2;
		}
		else if (str == "Mar")
		{
			lpCompileTime.tm_mon = 3;
		}
		else if (str == "Apr")
		{
			lpCompileTime.tm_mon = 4;
		}
		else if (str == "May")
		{
			lpCompileTime.tm_mon = 5;
		}
		else if (str == "Jun")
		{
			lpCompileTime.tm_mon = 6;
		}
		else if (str == "Jul")
		{
			lpCompileTime.tm_mon = 7;
		}
		else if (str == "Aug")
		{
			lpCompileTime.tm_mon = 8;
		}
		else if (str == "Sep")
		{
			lpCompileTime.tm_mon = 9;
		}
		else if (str == "Oct")
		{
			lpCompileTime.tm_mon = 10;
		}
		else if (str == "Nov")
		{
			lpCompileTime.tm_mon = 11;
		}
		else if (str == "Dec")
		{
			lpCompileTime.tm_mon = 12;
		}
		else if (str == "Feb")
		{
			lpCompileTime.tm_mon = 0;
		}

		sscanf(__TIME__, "%d:%d:%d", &lpCompileTime.tm_hour,
			&lpCompileTime.tm_min, &lpCompileTime.tm_sec);
		lpCompileTime.tm_isdst = lpCompileTime.tm_wday = lpCompileTime.tm_yday = 0;

		char szBuf[64] = { 0 };
		sprintf(szBuf, "%04d-%02d-%02d %02d:%02d", lpCompileTime.tm_year, lpCompileTime.tm_mon, lpCompileTime.tm_mday, lpCompileTime.tm_hour, lpCompileTime.tm_min);
		std::string str1 = szBuf;

		if (NULL != stdDate) {
			sprintf(stdDate, "%02d%02d%02d", lpCompileTime.tm_year%100, lpCompileTime.tm_mon, lpCompileTime.tm_mday);
		}

		return str1;
	}
};

#endif //_CSV_VERSION_H