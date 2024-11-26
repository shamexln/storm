#include "LogProvider.h"

#include <string>
#include <windows.h>


namespace MedibusServer
{
	LogProvider::LogProvider()
	{
    
		HINSTANCE dllHandle = LoadLibrary(TEXT("LogProvider.dll"));
		Init();
    
	}

	void LogProvider::Init()
	{
		m_logHelper.Init();
	}
	
	void LogProvider::LogFile(std::string msg, LevelEnum logLevel)
	{
	    m_logHelper.LogFile(msg, static_cast<LevelEnum>(logLevel));
	}

}




