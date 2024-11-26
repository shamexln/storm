#pragma once
// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the LOGPROVIDER_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// LOGPROVIDER_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef LOGPROVIDER_EXPORTS
#define LOGPROVIDER_API __declspec(dllexport)
#else
#define LOGPROVIDER_API __declspec(dllimport)
#endif
#include <string>






namespace MedibusServer
{
	class AlarmInfo;
	class DeviceInfo;
	class MetricInfo;
	class EssentialInfo;
	class LogProvider;

	enum class LOGPROVIDER_API  LevelEnum {
		trace = 0,
		debug = 1,
		info = 2,
		warn = 3,
		err = 4,
		critical = 5,
		off = 6,
		n_levels
	};

	class LOGPROVIDER_API CLogHelper
	{
	public:
		CLogHelper();
		~CLogHelper();
		void Init();
		void LogFile(std::string msg, LevelEnum logLevel = LevelEnum::debug);

		void LogFile(std::string msg, std::string instanceId, std::string sequenceId, std::string Info, const std::string& curValue);
		void LogFile(std::string msg, std::string instanceId, std::string sequenceId, const EssentialInfo& Info, const std::string& curValue);
		void LogFile(std::string msg, std::string instanceId, std::string sequenceId, const MetricInfo& Info, const std::string& curValue);
		void LogFile(std::string msg, std::string instanceId, std::string sequenceId, const DeviceInfo& Info, const std::string& curValue);

		void LogFile(std::string msg, std::string instanceId, std::string sequenceId, const AlarmInfo& Info, const std::string& curValue);
	private:
		LogProvider* m_pLog;
	};
}


