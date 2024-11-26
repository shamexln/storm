#pragma once
#pragma comment(lib, "LogProvider.lib")
#include <string>

#include "Singleton.h"
#include "CLogHelper.h"
//class __declspec(dllimport) CSdcProvider;

/**
 * @brief Sample SDC provider for metric values, waveforms and alarms.
 */
namespace MedibusServer
{

    class LogProvider : public Singleton<LogProvider>
    {
    public:
        friend class Singleton<LogProvider>;
        static LogProvider& Instance() {
            static LogProvider instance;
            return instance;
        }


        void Init();
    	void LogFile(std::string msg, LevelEnum logLevel = LevelEnum::debug);

    	void LogFile(std::string msg, std::string instanceId, std::string sequenceId, std::string Info, const std::string& curValue);
        void LogFile(std::string msg, std::string instanceId, std::string sequenceId, const EssentialInfo& Info, const std::string& curValue);
        void LogFile(std::string msg, std::string instanceId, std::string sequenceId, const MetricInfo& Info, const std::string& curValue);
        void LogFile(std::string msg, std::string instanceId, std::string sequenceId, const DeviceInfo& Info, const std::string& curValue);
        void LogFile(std::string msg, std::string instanceId, std::string sequenceId, const AlarmInfo& Info, const std::string& curValue);

    private:
        LogProvider();
       // LogProvider() = default;
        ~LogProvider() = default;
        CLogHelper m_logHelper;
    };
}

