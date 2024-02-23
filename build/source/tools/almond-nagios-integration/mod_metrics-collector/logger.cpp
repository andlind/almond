#undef __STRICT_ANSI__
#include "logger.h"
#include "logger.h"
#include "logutils.h"
const string CLogger::m_sFileName = "/var/log/metrics_collector.log";
CLogger* CLogger::m_pThis = NULL;
ofstream CLogger::m_Logfile;
CLogger::CLogger()
{
 
}
CLogger* CLogger::GetLogger(){
    	if (m_pThis == NULL){
        	m_pThis = new CLogger();
        	m_Logfile.open(m_sFileName.c_str(), ios::out | ios::app);
	}
	return m_pThis;
}
 
void CLogger::Log(const char * format, ...)
{
    char* sMessage = NULL;
    int nLength = 512;
    va_list args;
    va_start(args, format);
    //  Return the number of characters in the string referenced the list of arguments.
    // _vscprintf doesn't count terminating '\0' (that's why +1)
    //nLength = _vscprintf(format, args) + 1;
    //nLength = 256;
    sMessage = new char[nLength];
    //vsprintf_s(sMessage, nLength, format, args);
    vsnprintf(sMessage, nLength, format, args);
    //vsprintf(sMessage, format, args);
    if (!m_Logfile.is_open()) {
	    m_Logfile.open(m_sFileName.c_str(), ios::out | ios::app);
    }
    m_Logfile << Util::CurrentDateTime() << ":\t";
    m_Logfile << sMessage << "\n";
    va_end(args);
    m_Logfile.close();

    delete [] sMessage;
}

void CLogger::Log(const string& sMessage)
{
    if (!m_Logfile.is_open()) {
	    m_Logfile.open(m_sFileName.c_str(), ios::out | ios::app);
    }
    m_Logfile <<  Util::CurrentDateTime() << ":\t";
    m_Logfile << sMessage << "\n";
    m_Logfile.close();
}

CLogger& CLogger::operator<<(const string& sMessage)
{
    if (!m_Logfile.is_open()) {
            m_Logfile.open(m_sFileName.c_str(), ios::out | ios::app);
    }
    m_Logfile << "\n" << Util::CurrentDateTime() << ":\t";
    m_Logfile << sMessage << "\n";
    m_Logfile.close();
    return *this;
}
