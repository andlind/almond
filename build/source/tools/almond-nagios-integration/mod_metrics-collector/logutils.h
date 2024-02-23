#ifndef LOGUTILS_H
#define LOGUTILS_H
 
#include <iostream>
#include <string>
#include <time.h>
 
namespace Util
{
    // Get current date/time, format is YYYY-MM-DD.HH:mm:ss
    const std::string CurrentDateTime()
    {
        time_t     now;
        struct tm  tstruct;
        char       buf[80];
	now = time(NULL);
        localtime_r(&now, &tstruct);
        strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
        return buf;
    }
}
#endif
