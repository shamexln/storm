#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <semaphore>
#include <stack>
#include <thread>
#include <typeinfo>

#include "LogProvider.h"
#include "serial/serial.h"
/**
 * The base State class declares methods that all Concrete State should
 * implement and also provides a backreference to the Context object, associated
 * with the State. This backreference can be used by States to transition the
 * Context to another State.
 */
const int BUFSZ = 100;

std::string GetErrorMessage(uint8_t errCode)
{
    std::string errMsg;
    switch (errCode)
    {
    case 0x01:
        errMsg = "Zero Or Span Of Any Component In Progress";
        break;
    case 0x02:
        errMsg = "Wrong Parameter";
        break;
    case 0x03:
        errMsg = "Wrong Unit";
        break;
    case 0x04:
        errMsg = "Agent Not Supported";
        break;
    case 0x08:
        errMsg = "Span Invalid Tag";
        break;
    case 0x10:
        errMsg = "Parameter Not Supported";
        break;
    case 0x11:
        errMsg = "Not Allowed At This Moment";
        break;
    case 0x12:
        errMsg = "Frame Not Supported";
        break;
    case 0x13:
        errMsg = "Rt Not Supported";
        break;
    case 0x14:
        errMsg = "Wrong Interval Base Time";
        break;
    case 0x15:
        errMsg = "Data Not Available Yet";
        break;
    case 0x20:
        errMsg = "Eeprom Access Failed";
        break;
    case 0x22:
        errMsg = "Non Volatile Memory Access Failed";
        break;
    case 0x31:
        errMsg = "Watertrap Is Full";
        break;
    case 0x60:
        errMsg = "Tpu Timeout";
        break;
    case 0x70:
        errMsg = "Wrong Parameter Set Order";
        break;
    case 0x71:
        errMsg = "Wrong Parameter Set Type";
        break;
    case 0x72:
        errMsg = "Wrong Parameter Set Value";
        break;
    case 0x73:
        errMsg = "Wrong Parameter Set Non-Zero";
        break;
    case 0x74:
        errMsg = "Checksum Failure";
        break;
    case 0x75:
        errMsg = "Verification Of New Parameter In Eeprom Failed";
        break;
    case 0x76:
        errMsg = "Wrong Parameter Number";
        break;
    case 0x77:
        errMsg = "Calibration Value Can Not Be Stored With This Command";
        break;
    case 0x78:
        errMsg = "Data Amount Out Of Range";
        break;
    case 0x79:
        errMsg = "Calibration Value Storage Failed Old Value Ok";
        break;
    case 0x7A:
        errMsg = "Calibration Value Storage Failed Old Corrupted";
        break;
    case 0x7B:
        errMsg = "Hardware Supervision Eeprom Access Failed";
        break;
    case 0x7C:
        errMsg = "Get Fail Software Error";
        break;
    case 0x90:
        errMsg = "Calibration Cancelled";
        break;
    case 0x91:
        errMsg = "No Calibration Data Available";
        break;
    case 0x92:
        errMsg = "Just Collecting Calibration Data";
        break;
    case 0x93:
        errMsg = "Calibration Data Transmitted";
        break;
    case 0xA0:
        errMsg = "Delay Time Is Zero";
        break;
    case 0xA1:
        errMsg = "Invalid Amount Of Parameters";
        break;
    case 0xA2:
        errMsg = "Factory Calibration Hardware Error";
        break;
    case 0xA3:
        errMsg = "Factory Calibration Warm-Up";
        break;
    case 0xA4:
        errMsg = "Data Not Available";
        break;
    case 0xA5:
        errMsg = "Parameter Error Zero Gas Type";
        break;
    case 0xA6:
        errMsg = "Parameter Error For Limit";
        break;
    case 0xA7:
        errMsg = "Parameter Error For Zero Mode";
        break;
    case 0xB0:
        errMsg = "Failed";
        break;
    case 0xC0:
        errMsg = "Subcomponent Not Available For This Purpose";
        break;
    case 0xC1:
        errMsg = "Sub Component Does Not Support This Mode";
        break;
    case 0xCE:
        errMsg = "Write Access Not Allowed";
        break;
    case 0xCF:
        errMsg = "Does Not Exist";
        break;
    case 0xFF:
        errMsg = "Unknown Command";
        break;
    default:
        errMsg = "No Error";
    }
    return errMsg;
}

class IObserver {
public:
    virtual ~IObserver() {};
    virtual void Update(std::vector<uint8_t> rddata, size_t sz) = 0;
};

class ISubject {
public:
    virtual ~ISubject() {};
    virtual void Attach(IObserver* observer) = 0;
    virtual void Detach(IObserver* observer) = 0;
    virtual void AttachNeedResponse(IObserver* observer) = 0;
    virtual void DetachNeedResponse() = 0;
    virtual void NotifyOne(std::vector<uint8_t> rddata, size_t sz) = 0;
    virtual void Notify(std::vector<uint8_t> rddata, size_t sz) = 0;
};

class Context;

class State :public IObserver {
    /**
     * @var Context
     */
protected:
    Context* context_;
    bool m_bIsAlreadySent{false};
    bool m_bIsDataReceived{ false };
public:
    virtual ~State()
	{
        std::cout << "State Destructor" << '\n';
    }

    void set_context(Context* context) {
        this->context_ = context;
    }
    virtual std::vector<uint8_t> GetCommand() = 0;
    virtual size_t GetRespondBytes() = 0;
    virtual uint32_t GetCommandId() = 0;
    virtual void HandleData() = 0;
    virtual void Register() = 0;
    virtual void SetAlreadySent(bool bAlreadySent)
    {
        m_bIsAlreadySent = bAlreadySent;
    }
    virtual bool IsAlreadySent()
    {
        return m_bIsAlreadySent;
    }
    virtual bool IsSingleCommand()
    {
        return true;
    }
    virtual bool IsContinuousCommand()
    {
        return false;
    }
    virtual bool IsDataReceived()
    {
        return m_bIsDataReceived;
    }

    virtual void SetDataReceived(bool bReceived)
    {
        m_bIsDataReceived = bReceived;
    }

    virtual void PrintData(const std::vector<uint8_t>& rddata)
    {
        for (auto element : rddata)
        {
            std::cout << std::setw(3) << std::hex << static_cast<int>(element);
           
            
        }
        std::cout << '\n';

        std::stringstream msg;
        for (auto element : rddata)
        {
            std::cout << std::setw(3) << std::hex << static_cast<int>(element);
            msg << std::setw(3) << std::hex << static_cast<int>(element);
        }
        MedibusServer::LogProvider::Instance().LogFile(msg.str());
    }

    
};


class Context : public ISubject{

public:
    Context(State* state) : state_(nullptr) {
        this->TransitionTo(state);
    }
    Context(uint16_t cmdId, std::shared_ptr<State> state) : state_(nullptr), m_state(state) {
        this->TransitionTo(cmdId, state);

    }


    ~Context() {
        t1.join();
        delete state_;
    }

    void Attach(IObserver* observer) override {
      
        auto itr = std::find(m_ListObservers.begin(), m_ListObservers.end(), observer);
        if(itr == m_ListObservers.end())
        {
            m_ListObservers.push_back(observer);
        }
        
    }
    void Detach(IObserver* observer) override {

    	m_ListObservers.remove(observer);

    }

    void AttachNeedResponse(IObserver* observer) override {
        while (!m_StackResponse.empty())
        {
            m_StackResponse.pop();
        }
        m_StackResponse.push(observer);
    }
    void DetachNeedResponse() override {
        if (!m_StackResponse.empty())
        {
            m_StackResponse.pop();
        }
    }

    void NotifyOne(std::vector<uint8_t> rddata, size_t sz) override {
        if (!m_StackResponse.empty())
        {
            m_StackResponse.top()->Update(rddata, sz);
        }
        
    }

    void Notify(std::vector<uint8_t> rddata, size_t sz) override {
        std::list<IObserver*>::iterator iterator = m_ListObservers.begin();
        while (iterator != m_ListObservers.end()) {
            if ((*iterator) != nullptr)
            {
            	(*iterator)->Update(rddata, sz);
            }
            
            ++iterator;
        }
    }

   /* void CheckModuleConnected(std::chrono::milliseconds lastTime)
    {
        auto t2 = std::thread(&Context::HandleModuleConnected, this, lastTime);
        t2.detach();

    }*/

    void Init()
    {
        t1 = std::thread(&Context::HandleResponseThread, this, 0);

    }

    size_t SendCmdSync(State* state)
    {
        size_t bytes_wrote = m_serial.write(state->GetCommand());
        std::unique_lock<std::mutex> lock(mutex_condition);
        condition.wait(lock);
        if (this->m_state->IsDataReceived())
        {
            this->m_state->SetAlreadySent(true);
        }
        else
        {
            this->m_state->SetAlreadySent(false);
        }
        return bytes_wrote;
    }

    size_t SendCmd(State* state)
    {
        size_t bytes_wrote = m_serial.write(state->GetCommand());
        return bytes_wrote;
    }

    size_t ReadRespond(State* state, std::vector<uint8_t>& rddata)
    {
        size_t bytes_read = m_serial.read(rddata, state->GetRespondBytes());
        return bytes_read;
    }

    size_t ReadRespond(State* state, uint8_t* rddata, size_t sz)
    {
        size_t bytes_read = m_serial.read(rddata, sz);
        return bytes_read;
    }

    void SetAutoZeroCondition(bool bSet)
    {
        m_bAutoZeroCondition = bSet;
    }

    bool GetAutoZeroCondition()
    {
        return m_bAutoZeroCondition;
    }

    void SetPneumaticsEnabled(bool bAvailable)
    {
        m_bPneumaticsEnabled = bAvailable;
    }

    bool GetPneumaticsEnabled()
    {
        return m_bPneumaticsEnabled;
    }

    void SetPAIAvailable(bool bAvailable)
    {
        m_bPAIAvailable = bAvailable;
    }

    bool GetPAIAvailable()
    {
        return m_bPAIAvailable;
    }

    void SetNeedsExternalData(bool bNeed)
    {
        m_bNeedsExternalData = bNeed;
    }

    bool GetNeedsExternalData()
    {
        return m_bNeedsExternalData;
    }

    void SetNeedsExternalDataValue(uint8_t hsp)
    {
        m_bHSP = hsp;
    }

    uint8_t GetNeedsExternalDataValue()
    {
        return m_bHSP;
    }

    void TransitionTo(State* state) {
        std::lock_guard<std::mutex> lock(m);
        //std::cout << "Context: Transition to " << typeid(*state).name() << ".\n";
        if (this->state_ != nullptr)
            delete this->state_;
        this->state_ = state;
        this->state_->set_context(this);
    }

    void TransitionTo(std::shared_ptr<State> state) {
        std::lock_guard<std::mutex> lock(m);
        //std::cout << "Context: Transition to " << typeid(state).name() << ".\n";

        this->m_state = state;
        this->m_state->set_context(this);
    }
   
    void TransitionTo(uint32_t cmdId, std::shared_ptr<State> state ) {
        std::lock_guard<std::mutex> lock(m);
        // first Detach the latest state, which saved in  this->m_state
        // ignore the flag, because if it doesn't exist in the list, nothing will happen
        if (this->m_state && this->m_state->IsSingleCommand())
        {
            DetachNeedResponse();
        }
        // continuous commands don't need call detach, always attach until application exit.
        std::cout << "Context: Transition to " << typeid(state).name() << ".\n";
        auto fnd = m_mapStates.find(cmdId);
        if (fnd == m_mapStates.end())
        {
            m_mapStates.emplace(std::make_pair(cmdId, state));
            this->m_state = state;
            // Attach the state according the flag
            if (this->m_state && this->m_state->IsSingleCommand())
            {
                AttachNeedResponse(this->m_state.get());
            }
            if (this->m_state && this->m_state->IsContinuousCommand())
            {
                Attach(this->m_state.get());
            }
        }
        else
        {
            this->m_state = fnd->second;
            if (this->m_state && this->m_state->IsSingleCommand() && !this->m_state->IsAlreadySent())
            {
                AttachNeedResponse(this->m_state.get());
            }
            
        }

        this->m_state->set_context(this);
    }

    void Request1() {
        std::lock_guard<std::mutex> lock(m);
        //this->m_state->Register();
        this->m_state->HandleData();
    }

    

private:

    // not use
	//void HandleModuleConnected(std::chrono::milliseconds lastTime)
	//{
	//	while (true)
	//	{
	//		std::chrono::milliseconds now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

	//		auto diffmilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime);
	//		if (diffmilliseconds.count() >= 150)
	//		{
	//			// no response from module
 //               std::unique_lock<std::mutex> lock(mutex_condition);
 //               condition.wait(lock);
	//		}
	//	}
	//}
        void HandleResponseThread(int i)
        {
            try
            {
                std::vector<uint8_t> data;
                const size_t ACKHEADLENGTH = 3;
                State* state;
                uint8_t rddata[BUFSZ]{};
                size_t sz;
                size_t ackdatalength = 0;
                size_t dataheadlength = 0;
                while (true)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    size_t bytes_read = m_serial.read(rddata, BUFSZ - 1);
                    if (bytes_read != 0)
                    {
                        // If there is any data come from serial, notify all mutex
                        std::lock_guard<std::mutex> lock(mutex_condition);
                        this->m_state->SetDataReceived(true);
                        condition.notify_all();
                    }
                    else
                    {
                        // If there is no data come from serial, notify all mutex
                        std::lock_guard<std::mutex> lock(mutex_condition);
                        this->m_state->SetDataReceived(false);
                        condition.notify_all();
                    }
                    data.insert(data.end(), rddata, rddata + bytes_read);
                    // first check the ACK Header (3 bytes), if not , wait until receiving all ACK Header .
                    dataheadlength += data.size();
                    if (dataheadlength < ACKHEADLENGTH)
                    {
                        continue;
                    }
                    dataheadlength = 0;
                    // second According to ACK Header (3 bytes), get parameter length(LB), wait until  receiving all parameter.
                    ackdatalength = data[2];
                    if (ackdatalength + ACKHEADLENGTH + 1 > data.size())
                    {
                        continue;
                    }

                    // here, we can get one full ack response.
                    // analysis the ack response to distribute it to observer
                    //
                    if (data[0] == 0x06)
                    {
                        // success
                        // send data from data to data + len(including cs)
                        std::vector<uint8_t> datatosend;
                        datatosend = std::vector<uint8_t>(data.begin(), data.begin() + ackdatalength + ACKHEADLENGTH + 1);

                        //if (data[1] == 0x12)
                        {
                            Notify(datatosend, ackdatalength + ACKHEADLENGTH + 1);
                        }
                        //else
                        {
                            NotifyOne(datatosend, ackdatalength + ACKHEADLENGTH + 1);
                        }

                        // remove the read data from cache
                        std::vector<uint8_t> tmp;
                        if (ackdatalength + ACKHEADLENGTH + 1 < data.size())
                        {
                            tmp = std::vector<uint8_t>(data.begin() + ackdatalength + ACKHEADLENGTH + 1, data.end());
                        }

                        data = tmp;
                    }
                    else if (data[0] == 0x15)
                    {
                        // fail
                        // send data from data to data + len(including cs)
                        NotifyOne(data, ackdatalength + ACKHEADLENGTH + 1);
                        Notify(data, ackdatalength + ACKHEADLENGTH + 1);
                        // remove the read data from cache
                        std::vector<uint8_t> tmp;
                        if (ackdatalength + ACKHEADLENGTH + 1 < data.size())
                        {
                            tmp = std::vector<uint8_t>(data.begin() + ackdatalength + ACKHEADLENGTH + 1, data.end());
                        }

                        data = tmp;
                    }

                }
            }
        	catch (...)
            {
                std::cout << "Handle Response Thread catch a exception. \n" << '\n';
            }
            

        }

private:
    std::list<IObserver*> m_ListObservers;
    std::stack<IObserver*> m_StackResponse;
    int m_nNumThreads{ 1 };
    bool m_bPneumaticsEnabled{ false };
    bool m_bAutoZeroCondition{ false };
    bool m_bPAIAvailable{ false };
    bool m_bNeedsExternalData{ false };
    uint8_t m_bHSP{ 0x00 };
    std::shared_ptr<State> m_state;
    State* state_;
    std::map<uint16_t, std::shared_ptr<State>> m_mapStates;
    std::thread t1;
    serial::Serial m_serial{ "COM9", 19200, serial::Timeout::simpleTimeout(100) };
    std::mutex m;
    std::mutex mutex_condition;
    std::condition_variable condition;
};

/**
 * Concrete States implement various behaviors, associated with a state of the
 * Context.
 */

class StopContinuousDataState : public State {
public:
    ~StopContinuousDataState()
    {
        std::cout << "StopContinuousDataState Destructor" << '\n';
    }
    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
    std::chrono::milliseconds m_lastTime{ std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) };
};


class GetIntervalBaseTimeState : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};


// Transmit Device Component Information
// Vendor Code
class TransmitDeviceComponentInformation_VendorCode_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Transmit Device Component Information
// Serial Number
class TransmitDeviceComponentInformation_SerialNumber_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Transmit Device Component Information
// Hardware Revision
class TransmitDeviceComponentInformation_HardwareRevision_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Transmit Device Component Information
// Software Revision
class TransmitDeviceComponentInformation_SoftwareRevision_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Transmit Device Component Information
// Product Name
class TransmitDeviceComponentInformation_ProductName_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Transmit Device Component Information
// Part Number
class TransmitDeviceComponentInformation_PartNumber_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Adjust Time Information
class AdjustTimeInformationState : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Transmit Generic Module Features
class TransmitGenericModuleFeaturesState : public State {
public:
	
    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

//// Transmit Generic Module Features
//// Auto Zero Condition
//class TransmitGenericModuleFeatures_AutoZeroCondition_State : public State {
//public:
//
//    void HandleData() override;
//    std::vector<uint8_t> GetCommand() override;
//    size_t GetRespondBytes() override;
//    uint16_t GetCommandId() override;
//    void Register() override;
//    void Update(std::vector<uint8_t> rddata, size_t sz) override;
//    void SetAlreadySent(bool bAlreadySent) override;
//    bool IsAlreadySent() override;
//};

// Switch Breath Detection Mode
// Pgm Breath Detection
class SwitchBreathDetectionMode_PgmBreathDetection_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Switch Breath Detection Mode
// 	Pgm Breath Detection Auto Wakeup
class SwitchBreathDetectionMode_PgmBreathDetectionAutoWakeup_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};


// Switch Breath Detection Mode
// 	Auto-Wakeup After Breathphase1
class SwitchBreathDetectionMode_AutoWakeupAfterBreathphase1_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Switch Breath Detection Mode
// 	Auto-Wakeup After Breathphase2
class SwitchBreathDetectionMode_AutoWakeupAfterBreathphase2_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Switch Breath Detection Mode
// 	Auto-Wakeup After Breathphase3
class SwitchBreathDetectionMode_AutoWakeupAfterBreathphase3_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Switch Breath Detection Mode
// 	Auto-Wakeup After Breathphase4
class SwitchBreathDetectionMode_AutoWakeupAfterBreathphase4_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};


// Switch Breath Detection Mode
// 	Auto-Wakeup After Breathphase5
class SwitchBreathDetectionMode_AutoWakeupAfterBreathphase5_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};


// Transmit Patient Data
// FRAME_$12$0E - Parameter Detailed Status
class TransmitPatientData_120E_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
    bool IsSingleCommand() override;
    bool IsContinuousCommand() override;
};

// Measurement Mode
class MeasurementModeState : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
    std::chrono::milliseconds m_lastTime { std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) };
};

// Operating Mode
class OperatingModeState : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Switch Valves
// CMD_$61 - Switch Valves
class  SwitchValvesState : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};


// Switch Pump State
// CMD_$62 - Switch Pump
class  SwitchPumpState : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Select The Anesthetic Agent
// FRAME_$12$0E
class SelectTheAnestheticAgentState : public State{
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};


// Evaluate FRAME_$12$10
// FRAME_$12$10 - Physiologic Agent1
class Evaluate_1210_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Select Anesthetic Agent Type
// CMD_$1D - Select Anesthetic Agent Type
// Halothane
class  SelectAnestheticAgentType_Halothane_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};


// Provide the sensor module with required data
// Halothane
class  ProvideTheSensorModuleWithRequiredData_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};


// Accept External Parameter Data
// CMD_$1C - Accept External Parameter Data
// Halothane
class  AcceptExternalParameterData_UnknownAccuracy_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};


// Supervise Module Status
// Check Watertrap
class  SuperviseModuleStatus_120E_MSBit2_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};


// Supervise Module Status
// Watertrap disconnected?
class  SuperviseModuleStatus_120B_MSWBit5_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Supervise Module Status
// Watertrap full?
class  SuperviseModuleStatus_120B_MSWBit6_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Supervise Module Status
// Watertrap warning?
class  SuperviseModuleStatus_120E_MSWBit7_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Supervise Module Status
// Any Componeny Fail
class  SuperviseModuleStatus_120E_MSBit6_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Supervise Module Status
// Breath phase data available?
class  SuperviseModuleStatus_120E_MSBit5_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Supervise Module Status
// No respiration / Apnea?
class  SuperviseModuleStatus_120E_MSBit4_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Supervise Zero Request State
// Measurement Mode?
class  SuperviseZeroRequest_120E_OMS_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Zero In Progress State
// FRAME_$12$03,CO2_PS,N2O_PS, Bit5
class  ZeroInProgress_1203_CO2N2OPSBit5_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Zero In Progress State
// FRAME_$12$04,O2_PS, Bit5
class  ZeroInProgress_1204_O2PSBit5_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};


// Zero In Progress State
// FRAME_$12$10,A1_PS, Bit5
class  ZeroInProgress_1210_A1PSBit5_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Zero In Progress State
// FRAME_$12$11,A2_PS, Bit5
class  ZeroInProgress_1211_A2PSBit5_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Zero Request State
// FRAME_$12$0E,MS, Bit0
class  ZeroRequestState : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Handle Zero Request State
// FRAME_$12$0E,MS, Bit0
class  HandleZeroRequestState : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Init Zero State
// CMD_$20 - Initiate Zero
// Single Command
class  InitZeroState : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Get Units State
// FRAME_$12$12 - Parameter Unit Information
class  GetUnitsState : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Evaluate Connection Established
class  EvaluateConnectionEstablishedState : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Host Selectable Parameters
// FRAME_$12$0E, HSP
class  HostSelectableParameters_120E_HSP_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Parameter Availability Information
// FRAME_$12$0E, PAI
class ParameterAvailabilityInformation_120E_PAI_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Parameter Mode State
// FRAME_$12$03,CO2_PS, Bit6-7
class  ParameterMode_1203_CO2PSBit6Bit7_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Parameter Mode State
// FRAME_$12$03,N2O_PS, Bit6-7
class  ParameterMode_1203_N2OPSBit6Bit7_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Parameter Mode State
// FRAME_$12$04,O2_PS, Bit6-7
class  ParameterMode_1204_O2PSBit6Bit7_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};


// Parameter Mode State
// FRAME_$12$10,A1_PS, Bit6-7
class  ParameterMode_1210_A1PSBit6Bit7_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Parameter Mode State
// FRAME_$12$11,A2_PS, Bit6-7
class  ParameterMode_1211_A2PSBit6Bit7_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};



// Parameter Inop Information State
// FRAME_$12$0E,PII
class  ParameterInopInformation_120E_PII_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Measurement Mode State
// FRAME_$12$0E,OMS
class  MeasurementMode_120E_OMS_State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

// Occlusion State
// FRAME_$12$0E,MS, Bit1
class  Occlusion_120E_MSBit1__State : public State {
public:

    void HandleData() override;
    std::vector<uint8_t> GetCommand() override;
    size_t GetRespondBytes() override;
    uint32_t GetCommandId() override;
    void Register() override;
    void Update(std::vector<uint8_t> rddata, size_t sz) override;
};

void StopContinuousDataState::HandleData() {
    {
        if (IsAlreadySent())
        {
            return;
        }
        /*SetAlreadySent(true);*/
        //std::vector<uint8_t> rddata;
        uint8_t* rddata = new uint8_t[BUFSZ]();
        std::cout << "Handles StopContinuousData.\n";
        std::stringstream msg;
        msg << "Handles StopContinuousData.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());

    	std::chrono::milliseconds now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

    	auto diffmilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastTime);
    	if (diffmilliseconds.count() >= 150)
    	{
            m_lastTime = now;
            size_t bytes_wrote = this->context_->SendCmdSync(this);
    	}
        
    }
}


std::vector<uint8_t> StopContinuousDataState::GetCommand() {
        
     std::vector<uint8_t> data{ 0x10, 0x01, 0x19, 0xd6 };
     return data;
}

size_t StopContinuousDataState::GetRespondBytes() {
    return 4;
}

uint32_t StopContinuousDataState::GetCommandId()
{
    return 0x19;
}

void StopContinuousDataState::Register() {
    this->context_->AttachNeedResponse(this);
}

void StopContinuousDataState::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x19 && rddata[2] == 0x00)

    {
        std::cout << "Change to GetIntervalBaseTimeState.\n";
        std::stringstream msg;
        msg << "Change to GetIntervalBaseTimeState.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());
        auto ptr = std::make_shared<GetIntervalBaseTimeState>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);
    }

}


void GetIntervalBaseTimeState::HandleData() {
    {
        if (IsAlreadySent())
        {
            return;
        }
        /*SetAlreadySent(true);*/
        //std::vector<uint8_t> rddata;
        uint8_t* rddata = new uint8_t[BUFSZ]();
        std::cout << "Handles GetIntervalBaseTimeState.\n";
        std::stringstream msg;
        msg << "Handles GetIntervalBaseTimeState.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());
        size_t bytes_wrote = this->context_->SendCmdSync(this);

    }
}


std::vector<uint8_t> GetIntervalBaseTimeState::GetCommand() {

    std::vector<uint8_t> data{ 0x10, 0x02, 0x02, 0xff, 0xed };
    return data;
}

size_t GetIntervalBaseTimeState::GetRespondBytes() {
    return 6;
}

uint32_t GetIntervalBaseTimeState::GetCommandId()
{
    return 0x02;
}

void GetIntervalBaseTimeState::Register() {
    this->context_->AttachNeedResponse(this);
}

void GetIntervalBaseTimeState::Update(std::vector<uint8_t> rddata, size_t sz)
{
    size_t bytes_read = 0;
    if (rddata[0] == 0x06 && rddata[1] == 0x02 && rddata[2] == 0x02)
    {
        std::cout << "Change to GetIntervalBaseTimeState.\n";
        std::stringstream msg;
        msg << "Change to GetIntervalBaseTimeState.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());
        for (int i = 3; i <= 4; i++)
        {
            std::cout << rddata[i];
        }
        std::cout << '\n';
        auto ptr = std::make_shared<TransmitDeviceComponentInformation_VendorCode_State>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x02 && rddata[2] == 0x01)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        std::stringstream msg;
        msg << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        MedibusServer::LogProvider::Instance().LogFile(msg.str());
        std::cout << "Skip to TransmitDeviceComponentInformation_VendorCode_State.\n";
        msg << "Skip to TransmitDeviceComponentInformation_VendorCode_State.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());
        auto ptr = std::make_shared<TransmitDeviceComponentInformation_VendorCode_State>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);
    }

    PrintData(rddata);
}

void TransmitDeviceComponentInformation_VendorCode_State::HandleData() {
    {
        if (IsAlreadySent())
        {
            return;
        }
        /*SetAlreadySent(true);*/
        uint8_t* rddata = new uint8_t[BUFSZ]();
       // std::vector<uint8_t> rddata;
        std::cout << "Handles TransmitDeviceComponentInformation_VendorCode_State.\n";
        std::stringstream msg;
        msg << "Handles TransmitDeviceComponentInformation_VendorCode_State.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());
        size_t bytes_wrote = this->context_->SendCmdSync(this);
        
    }
}


std::vector<uint8_t> TransmitDeviceComponentInformation_VendorCode_State::GetCommand() {

    std::vector<uint8_t> data{ 0x10, 0x0a, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xdc };
    return data;
}

size_t TransmitDeviceComponentInformation_VendorCode_State::GetRespondBytes() {
    return 24;
}

uint32_t TransmitDeviceComponentInformation_VendorCode_State::GetCommandId()
{
    return 0x0a00;
}

void TransmitDeviceComponentInformation_VendorCode_State::Register() {
    this->context_->AttachNeedResponse(this);
}

void TransmitDeviceComponentInformation_VendorCode_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    size_t bytes_read = 0;
    if (rddata[0] == 0x06 && rddata[1] == 0x0a && rddata[2] == 0x14)
    {
        std::cout << "Change to TransmitDeviceComponentInformation_VendorCode_State.\n";
        std::stringstream msg;
        msg << "Change to TransmitDeviceComponentInformation_VendorCode_State.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());
        // sucess
        for (int i = 11; i <= 20; i++)
        {
            std::cout << rddata[i];
        }
        std::cout << '\n';

        // Is my response?
    	if ((GetCommandId() & 0x00ff) == rddata[21])
    	{
            auto ptr = std::make_shared<TransmitDeviceComponentInformation_SerialNumber_State>();
            this->context_->TransitionTo(ptr->GetCommandId(), ptr);
    	}

    }
    else if(rddata[0] == 0x15 && rddata[1] == 0x0a && rddata[2] == 0x01)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        std::stringstream msg;
        msg << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        MedibusServer::LogProvider::Instance().LogFile(msg.str());
        auto ptr = std::make_shared<StopContinuousDataState>();
        // if second time to create a smart point with the same raw point
        // TransitionTo will first find the command id related with last time smart point
        // the new smart point will be released, when this function is end.
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);
    }

    PrintData(rddata);
}

void TransmitDeviceComponentInformation_SerialNumber_State::HandleData() {
    {
        if (IsAlreadySent())
        {
            return;
        }
        SetAlreadySent(true);
        //std::vector<uint8_t> rddata;
        uint8_t* rddata = new uint8_t[BUFSZ]();
        std::cout << "Handles TransmitDeviceComponentInformation_SerialNumber_State.\n";
        std::stringstream msg;
        msg << "Handles TransmitDeviceComponentInformation_SerialNumber_State.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());
        size_t bytes_wrote = this->context_->SendCmd(this);
       
    }
}

// Command={10 0a 0a 00 00 00 00 00 00 00 00 01 db}
std::vector<uint8_t> TransmitDeviceComponentInformation_SerialNumber_State::GetCommand() {

    std::vector<uint8_t> data{ 0x10, 0x0a, 0x0a, 0x00, 0x0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xdb };
    return data;
}

size_t TransmitDeviceComponentInformation_SerialNumber_State::GetRespondBytes() {
    return 24;
}

uint32_t TransmitDeviceComponentInformation_SerialNumber_State::GetCommandId()
{
    return 0x0a01;
}


void TransmitDeviceComponentInformation_SerialNumber_State::Register() {
    this->context_->AttachNeedResponse(this);
}

void TransmitDeviceComponentInformation_SerialNumber_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    size_t bytes_read = 0;
    if (rddata[0] == 0x06 && rddata[1] == 0x0a && rddata[2] == 0x14)
    {
        std::cout << "Change to TransmitDeviceComponentInformation_HardwareRevision_State.\n";
        std::stringstream msg;
        msg << "Change to TransmitDeviceComponentInformation_HardwareRevision_State.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());
        // sucess
       
        for (int i = 11; i <= 20; i++)
        {
            std::cout << rddata[i];
        }
        std::cout << '\n';

        // Is my response?
        if ((GetCommandId() & 0x00ff) == rddata[21])
        {
            auto ptr = std::make_shared<TransmitDeviceComponentInformation_HardwareRevision_State>();
            this->context_->TransitionTo(ptr->GetCommandId(), ptr);
        }
      
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x0a && rddata[2] == 0x01)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        std::stringstream msg;
        msg << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        MedibusServer::LogProvider::Instance().LogFile(msg.str());
        auto ptr = std::make_shared<StopContinuousDataState>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);
    }

    PrintData(rddata);
}

void TransmitDeviceComponentInformation_HardwareRevision_State::HandleData() {
    {
        if (IsAlreadySent())
        {
            return;
        }
        SetAlreadySent(true);
        //std::vector<uint8_t> rddata;
        uint8_t* rddata = new uint8_t[BUFSZ]();
        std::cout << "Handles TransmitDeviceComponentInformation_HardwareRevision_State.\n";
        std::stringstream msg;
        msg << "Handles TransmitDeviceComponentInformation_HardwareRevision_State.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());
        size_t bytes_wrote = this->context_->SendCmd(this);
    }
}


std::vector<uint8_t> TransmitDeviceComponentInformation_HardwareRevision_State::GetCommand() {

    std::vector<uint8_t> data{ 0x10, 0x0a, 0x0a, 0x00, 0x0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xda };
    return data;
}

size_t TransmitDeviceComponentInformation_HardwareRevision_State::GetRespondBytes() {
    return 24;
}

uint32_t TransmitDeviceComponentInformation_HardwareRevision_State::GetCommandId()
{
    return 0x0a02;
}

void TransmitDeviceComponentInformation_HardwareRevision_State::Register() {
    this->context_->AttachNeedResponse(this);
}

void TransmitDeviceComponentInformation_HardwareRevision_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x0a && rddata[2] == 0x14)
    {
        std::cout << "Change to TransmitDeviceComponentInformation_SoftwareRevision_State.\n";
        std::stringstream msg;
        msg << "Change to TransmitDeviceComponentInformation_SoftwareRevision_State.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());
        // sucess
        
        for (int i = 11; i <= 20; i++)
        {
            std::cout << rddata[i];
        }
        std::cout << '\n';

        // Is my response?
        if ((GetCommandId() & 0x00ff) == rddata[21])
        {
            auto ptr = std::make_shared<TransmitDeviceComponentInformation_SoftwareRevision_State>();
            this->context_->TransitionTo(ptr->GetCommandId(), ptr);
        }

    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x0a && rddata[2] == 0x01)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        std::stringstream msg;
        msg << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        MedibusServer::LogProvider::Instance().LogFile(msg.str());
        auto ptr = std::make_shared<StopContinuousDataState>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);
    }

    PrintData(rddata);
}


void TransmitDeviceComponentInformation_SoftwareRevision_State::HandleData() {
    {
        if (IsAlreadySent())
        {
            return;
        }
        SetAlreadySent(true);
        //std::vector<uint8_t> rddata;
        uint8_t* rddata = new uint8_t[BUFSZ]();
        std::cout << "Handles TransmitDeviceComponentInformation_SoftwareRevision_State.\n";
        std::stringstream msg;
        msg << "Handles TransmitDeviceComponentInformation_SoftwareRevision_State.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());
        size_t bytes_wrote = this->context_->SendCmd(this);
    }
}


std::vector<uint8_t> TransmitDeviceComponentInformation_SoftwareRevision_State::GetCommand() {

    std::vector<uint8_t> data{ 0x10, 0x0a, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xd9 };
    return data;
}

size_t TransmitDeviceComponentInformation_SoftwareRevision_State::GetRespondBytes() {
    return 24;
}

uint32_t TransmitDeviceComponentInformation_SoftwareRevision_State::GetCommandId()
{
    return 0x0a03;
}

void TransmitDeviceComponentInformation_SoftwareRevision_State::Register() {
    this->context_->AttachNeedResponse(this);
}

void TransmitDeviceComponentInformation_SoftwareRevision_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x0a && rddata[2] == 0x14)
    {
        std::cout << "Change to TransmitDeviceComponentInformation_ProductName_State.\n";
        std::stringstream msg;
        msg << "Change to TransmitDeviceComponentInformation_ProductName_State.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());
        // sucess
        
        for (int i = 11; i <= 20; i++)
        {
            std::cout << rddata[i];
        }
        std::cout << '\n';

        // Is my response?
        if ((GetCommandId() & 0x00ff) == rddata[21])
        {
            auto ptr = std::make_shared<TransmitDeviceComponentInformation_ProductName_State>();
            this->context_->TransitionTo(ptr->GetCommandId(), ptr);
        }
        
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x0a && rddata[2] == 0x01)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        std::stringstream msg;
        msg << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        MedibusServer::LogProvider::Instance().LogFile(msg.str());
        auto ptr = std::make_shared<StopContinuousDataState>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);
    }

    PrintData(rddata);
}


void TransmitDeviceComponentInformation_ProductName_State::HandleData() {
    {
        if (IsAlreadySent())
        {
            return;
        }
        SetAlreadySent(true);
        //std::vector<uint8_t> rddata;
        uint8_t* rddata = new uint8_t[BUFSZ]();
        std::cout << "Handles TransmitDeviceComponentInformation_ProductName_State.\n";
        std::stringstream msg;
        msg << "Handles TransmitDeviceComponentInformation_ProductName_State.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());
        size_t bytes_wrote = this->context_->SendCmd(this);
        
    }
}

// Command={10 0a 0a 00 00 00 00 00 00 00 00 05 d7}
std::vector<uint8_t> TransmitDeviceComponentInformation_ProductName_State::GetCommand() {

    std::vector<uint8_t> data{ 0x10, 0x0a, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0xd7 };
    return data;
}

size_t TransmitDeviceComponentInformation_ProductName_State::GetRespondBytes() {
    return 24;
}

uint32_t TransmitDeviceComponentInformation_ProductName_State::GetCommandId()
{
    return 0x0a05;
}

void TransmitDeviceComponentInformation_ProductName_State::Register() {
    this->context_->AttachNeedResponse(this);
}

void TransmitDeviceComponentInformation_ProductName_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x0a && rddata[2] == 0x14)
    {
        std::cout << "Change to TransmitDeviceComponentInformation_PartNumber_State.\n";
        std::stringstream msg;
        msg << "Change to TransmitDeviceComponentInformation_PartNumber_State.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());
        // sucess
        
        for (int i = 11; i <= 20; i++)
        {
            std::cout << rddata[i];
        }
        std::cout << '\n';

        // Is my response?
        if ((GetCommandId() & 0x00ff) == rddata[21])
        {
            auto ptr = std::make_shared<TransmitDeviceComponentInformation_PartNumber_State>();
            this->context_->TransitionTo(ptr->GetCommandId(), ptr);
        }

    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x0a && rddata[2] == 0x01)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        std::stringstream msg;
        msg << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        MedibusServer::LogProvider::Instance().LogFile(msg.str());

        std::cout << "Skip to TransmitDeviceComponentInformation_PartNumber_State.\n";
        msg << "Skip to TransmitDeviceComponentInformation_PartNumber_State.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());

        auto ptr = std::make_shared<TransmitDeviceComponentInformation_PartNumber_State>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);
    }
}


void TransmitDeviceComponentInformation_PartNumber_State::HandleData() {
    {
        if (IsAlreadySent())
        {
            return;
        }
        SetAlreadySent(true);
        //std::vector<uint8_t> rddata;
        uint8_t* rddata = new uint8_t[BUFSZ]();
        std::cout << "Handles TransmitDeviceComponentInformation_PartNumber_State.\n";
        std::stringstream msg;
        msg << "Handles TransmitDeviceComponentInformation_PartNumber_State.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());
        size_t bytes_wrote = this->context_->SendCmd(this);
        
    }
}


std::vector<uint8_t> TransmitDeviceComponentInformation_PartNumber_State::GetCommand() {

    std::vector<uint8_t> data{ 0x10, 0x0a, 0x0a, 0x00, 0x0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0xd6 };
    return data;
}

size_t TransmitDeviceComponentInformation_PartNumber_State::GetRespondBytes() {
    return 24;
}

uint32_t TransmitDeviceComponentInformation_PartNumber_State::GetCommandId()
{
    return 0x0a06;
}

void TransmitDeviceComponentInformation_PartNumber_State::Register() {
    this->context_->AttachNeedResponse(this);
}

void TransmitDeviceComponentInformation_PartNumber_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x0a && rddata[2] == 0x14)
    {
        std::cout << "Change to TransmitDeviceComponentInformation_PartNumber_State.\n";
        std::stringstream msg;
        msg << "Change to TransmitDeviceComponentInformation_PartNumber_State.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());

        // sucess
        
        for (int i = 11; i <= 20; i++)
        {
            std::cout << rddata[i];
        }
        std::cout << '\n';

        // Is my response?
        if ((GetCommandId() & 0x00ff) == rddata[21])
        {
            auto ptr = std::make_shared<AdjustTimeInformationState>();
            this->context_->TransitionTo(ptr->GetCommandId(), ptr);
        }
        
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x0a && rddata[2] == 0x01)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        std::stringstream msg;
        msg << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        MedibusServer::LogProvider::Instance().LogFile(msg.str());
        auto ptr = std::make_shared<StopContinuousDataState>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);
    }

    PrintData(rddata);
}


void AdjustTimeInformationState::HandleData() {
    {
        if (IsAlreadySent())
        {
            return;
        }
        SetAlreadySent(true);
        //std::vector<uint8_t> rddata;
        uint8_t* rddata = new uint8_t[BUFSZ]();
        std::cout << "Handles AdjustTimeInformationState.\n";
        std::stringstream msg;
        msg << "Handles AdjustTimeInformationState.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());
        size_t bytes_wrote = this->context_->SendCmd(this);
    }
}


std::vector<uint8_t> AdjustTimeInformationState::GetCommand() {

    std::vector<uint8_t> data{ 0x10, 0x09, 0x2b, 0x01, 0x02, 0x03, 0x04, 0x05, 0x18, 0x00, 0x00, 0x95 };
    return data;
}

size_t AdjustTimeInformationState::GetRespondBytes() {
    // the size of ACK or NAK?
    return 4;
}

uint32_t AdjustTimeInformationState::GetCommandId()
{
    return 0x2b;
}

void AdjustTimeInformationState::Register() {
    this->context_->AttachNeedResponse(this);
}

void AdjustTimeInformationState::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x2b && rddata[2] == 0x00)
    {
        std::cout << "Change to TransmitGenericModuleFeaturesState.\n";
        std::stringstream msg;
        msg << "Change to TransmitGenericModuleFeaturesState.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());

        // sucess

        auto ptr = std::make_shared<TransmitGenericModuleFeaturesState>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);

    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x2b && rddata[2] == 0x01)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        std::stringstream msg;
        msg << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        MedibusServer::LogProvider::Instance().LogFile(msg.str());
        auto ptr = std::make_shared<StopContinuousDataState>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);
    }
}

// CMD_$2C - Transmit Generic Module Features
void TransmitGenericModuleFeaturesState::HandleData() {
    {
        if (IsAlreadySent())
        {
            return;
        }
        SetAlreadySent(true);
        //std::vector<uint8_t> rddata;
        uint8_t* rddata = new uint8_t[BUFSZ]();
        std::cout << "Handles TransmitGenericModuleFeaturesState.\n";
        std::stringstream msg;
        msg << "Handles TransmitGenericModuleFeaturesState.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());

        size_t bytes_wrote = this->context_->SendCmd(this);
    }
}


std::vector<uint8_t> TransmitGenericModuleFeaturesState::GetCommand() {

    std::vector<uint8_t> data{ 0x10, 0x01, 0x2c, 0xc3 };
    return data;
}

size_t TransmitGenericModuleFeaturesState::GetRespondBytes() {
    return 8;
}

uint32_t TransmitGenericModuleFeaturesState::GetCommandId()
{
    return 0x2c12;
}


void TransmitGenericModuleFeaturesState::Register() {
    this->context_->AttachNeedResponse(this);
}

// CMD_$2C - Transmit Generic Module Features
void TransmitGenericModuleFeaturesState::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x2c && rddata[2] == 0x04)
    {
        // sucess
        // bit1
        if ((rddata[6] & 0x02) == 0x02)
        {
            // bit2
            // pneumatic component available
            if ((rddata[6] & 0x04) == 0x04)
            {
                this->context_->SetPneumaticsEnabled(true);
            }
            else
            {
                // pneumatic component not available
                this->context_->SetPneumaticsEnabled(false);
            }
        }
        else
        {
            // pneumatic component not available
            this->context_->SetPneumaticsEnabled(false);
        }

        // bit0
        // 	ZERO_CTRL - Zero Control
        if ((rddata[6] & 0x01) == 0x01)
        {
            this->context_->SetAutoZeroCondition(false);
        }
        else
        {
            this->context_->SetAutoZeroCondition(true);
        }

        auto ptr = std::make_shared<SwitchBreathDetectionMode_PgmBreathDetection_State>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);

    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x2c && rddata[2] == 0x01)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        std::stringstream msg;
        msg << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        MedibusServer::LogProvider::Instance().LogFile(msg.str());

        std::cout << "Skip to SwitchBreathDetectionMode_PgmBreathDetection_State.\n";
        msg << "Skip to SwitchBreathDetectionMode_PgmBreathDetection_State.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());

        auto ptr = std::make_shared<SwitchBreathDetectionMode_PgmBreathDetection_State>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);
    }

    PrintData(rddata);
}


//// Transmit Generic Module Features
//// Auto Zero Condition
//void TransmitGenericModuleFeatures_AutoZeroCondition_State::HandleData()
//{
//    if (IsAlreadySent())
//    {
//        return;
//    }
//    SetAlreadySent(true);
//    //std::vector<uint8_t> rddata;
//    uint8_t* rddata = new uint8_t[BUFSZ]();
//    std::cout << "Handles TransmitGenericModuleFeatures_AutoZeroCondition_State.\n";
//
//    size_t bytes_wrote = this->context_->SendCmd(this);
//}
//
//std::vector<uint8_t> TransmitGenericModuleFeatures_AutoZeroCondition_State::GetCommand() {
//
//    std::vector<uint8_t> data{ 0x10, 0x01, 0x2c, 0xc3 };
//    return data;
//}
//
//size_t TransmitGenericModuleFeatures_AutoZeroCondition_State::GetRespondBytes() {
//    return 8;
//}
//
//uint16_t TransmitGenericModuleFeatures_AutoZeroCondition_State::GetCommandId()
//{
//    return 0x2c00;
//}
//
//
//void TransmitGenericModuleFeatures_AutoZeroCondition_State::Register() {
//    this->context_->Attach(this);
//}
//
//
//// CMD_$2C - Transmit Generic Module Features
//void TransmitGenericModuleFeatures_AutoZeroCondition_State::Update(std::vector<uint8_t> rddata, size_t sz)
//{
//    if (rddata[0] == 0x06 && rddata[1] == 0x2c && rddata[2] == 0x04)
//    {
//        // sucess
//        // bit0
//        // 	ZERO_CTRL - Zero Control
//        if ((rddata[6] & 0x01) == 0x01)
//        {
//            this->context_->SetAutoZeroCondition(false);
//        }
//        else
//        {
//            this->context_->SetAutoZeroCondition(true);
//        }
//
//        auto ptr = std::make_shared<SwitchBreathDetectionMode_PgmBreathDetection_State>();
//        this->context_->TransitionTo(ptr->GetCommandId(), ptr);
//    }
//    else if (rddata[0] == 0x15 && rddata[1] == 0x2c && rddata[2] == 0x01)
//    {
//        // fail
//        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
//        std::cout << "Skip to SwitchBreathDetectionMode_PgmBreathDetection_State.\n";
//        auto ptr = std::make_shared<SwitchBreathDetectionMode_PgmBreathDetection_State>();
//        this->context_->TransitionTo(ptr->GetCommandId(), ptr);
//    }
//
//    PrintData(rddata);
//}
//
//
//void TransmitGenericModuleFeatures_AutoZeroCondition_State::SetAlreadySent(bool bAlreadySent)
//{
//    m_bIsAlreadySent = bAlreadySent;
//}
//
//bool TransmitGenericModuleFeatures_AutoZeroCondition_State::IsAlreadySent()
//{
//    return m_bIsAlreadySent;
//}

// Switch Breath Detection Mode
// Pgm Breath Detection
// CMD_$1E - Switch Breath Detection Mode
void SwitchBreathDetectionMode_PgmBreathDetection_State::HandleData() {
    {
        if (IsAlreadySent())
        {
            return;
        }
        SetAlreadySent(true);
        //std::vector<uint8_t> rddata;
        uint8_t* rddata = new uint8_t[BUFSZ]();
        std::cout << "Handles SwitchBreathDetectionModeState.\n";
        std::stringstream msg;
        msg << "Handles SwitchBreathDetectionModeState.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());


        size_t bytes_wrote = this->context_->SendCmd(this);
    }
}

// Command={10 02 1e 01 cf}
std::vector<uint8_t> SwitchBreathDetectionMode_PgmBreathDetection_State::GetCommand() {

    std::vector<uint8_t> data{ 0x10, 0x02, 0x1e, 0x01, 0xcf };
    return data;
}

size_t SwitchBreathDetectionMode_PgmBreathDetection_State::GetRespondBytes() {
    return 4;
}

// CommandId=0x1e01
uint32_t SwitchBreathDetectionMode_PgmBreathDetection_State::GetCommandId()
{
    return 0x1e01;
}


void SwitchBreathDetectionMode_PgmBreathDetection_State::Register() {
    this->context_->AttachNeedResponse(this);
}

// CMD_$1E - Switch Breath Detection Mode
void SwitchBreathDetectionMode_PgmBreathDetection_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x1e && rddata[2] == 0x00)
    {
        std::cout << "Change the state of the context to SwitchBreathDetectionMode_PgmBreathDetection_State.\n";
        std::stringstream msg;
        msg << "Change the state of the context to SwitchBreathDetectionMode_PgmBreathDetection_State.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());

        // sucess
        auto ptr = std::make_shared<SwitchBreathDetectionMode_PgmBreathDetectionAutoWakeup_State>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x1e && rddata[2] == 0x01)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        auto ptr = std::make_shared<StopContinuousDataState>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);
    }

    PrintData(rddata);
}

// Switch Breath Detection Mode
// Pgm Breath Detection Auto Wakeup
void SwitchBreathDetectionMode_PgmBreathDetectionAutoWakeup_State::HandleData() {
    {
        if (IsAlreadySent())
        {
            return;
        }
        SetAlreadySent(true);
        //std::vector<uint8_t> rddata;
        uint8_t* rddata = new uint8_t[BUFSZ]();
        std::cout << "Handles SwitchBreathDetectionModeState.\n";
        std::stringstream msg;
        msg << "Handles SwitchBreathDetectionModeState.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());
        size_t bytes_wrote = this->context_->SendCmd(this);
        
    }
}

// Command={10 02 1e 02 ce}
std::vector<uint8_t> SwitchBreathDetectionMode_PgmBreathDetectionAutoWakeup_State::GetCommand() {

    std::vector<uint8_t> data{ 0x10, 0x02, 0x1e, 0x02, 0xce };
    return data;
}

size_t SwitchBreathDetectionMode_PgmBreathDetectionAutoWakeup_State::GetRespondBytes() {
    return 4;
}

// CommandId=0x1e02
uint32_t SwitchBreathDetectionMode_PgmBreathDetectionAutoWakeup_State::GetCommandId()
{
    return 0x1e02;
}

void SwitchBreathDetectionMode_PgmBreathDetectionAutoWakeup_State::Register() {
    this->context_->AttachNeedResponse(this);
}


void SwitchBreathDetectionMode_PgmBreathDetectionAutoWakeup_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x1e && rddata[2] == 0x00)
    {
        std::cout << "Change the state of the context to SwitchBreathDetectionMode_AutoWakeupAfterBreathphase1_State.\n";
        std::stringstream msg;
        msg << "Change the state of the context to SwitchBreathDetectionMode_AutoWakeupAfterBreathphase1_State.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());

        // sucess
        auto ptr = std::make_shared<SwitchBreathDetectionMode_AutoWakeupAfterBreathphase1_State>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x1e && rddata[2] == 0x01)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        std::stringstream msg;
        msg << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        MedibusServer::LogProvider::Instance().LogFile(msg.str());

        auto ptr = std::make_shared<StopContinuousDataState>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);
    }

    PrintData(rddata);
}

// Switch Breath Detection Mode
// Auto-Wakeup After Breathphase1
void SwitchBreathDetectionMode_AutoWakeupAfterBreathphase1_State::HandleData() {
    {
        if (IsAlreadySent())
        {
            return;
        }
        SetAlreadySent(true);
        //std::vector<uint8_t> rddata;
        uint8_t* rddata = new uint8_t[BUFSZ]();
        std::cout << "Handles SwitchBreathDetectionMode_AutoWakeupAfterBreathphase1_State.\n";
        std::stringstream msg;
        msg << "Handles SwitchBreathDetectionMode_AutoWakeupAfterBreathphase1_State.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());
        size_t bytes_wrote = this->context_->SendCmd(this);
      
    }
}

// Command={10 02 1e 05 cb}
std::vector<uint8_t> SwitchBreathDetectionMode_AutoWakeupAfterBreathphase1_State::GetCommand() {

    std::vector<uint8_t> data{ 0x10, 0x02, 0x1e, 0x05, 0xcb };
    return data;
}

size_t SwitchBreathDetectionMode_AutoWakeupAfterBreathphase1_State::GetRespondBytes() {
    return 4;
}

// CommandId=0x1e05
uint32_t SwitchBreathDetectionMode_AutoWakeupAfterBreathphase1_State::GetCommandId()
{
    return 0x1e05;
}


void SwitchBreathDetectionMode_AutoWakeupAfterBreathphase1_State::Register() {
    this->context_->AttachNeedResponse(this);
}

void SwitchBreathDetectionMode_AutoWakeupAfterBreathphase1_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x1e && rddata[2] == 0x00)
    {
        std::cout << "Change the state of the context to SwitchBreathDetectionMode_AutoWakeupAfterBreathphase2_State.\n";
        std::stringstream msg;
        msg << "Change the state of the context to SwitchBreathDetectionMode_AutoWakeupAfterBreathphase2_State.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());

        // sucess
        auto ptr = std::make_shared<SwitchBreathDetectionMode_AutoWakeupAfterBreathphase2_State>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr );
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x1e && rddata[2] == 0x01)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        std::stringstream msg;
        msg << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        MedibusServer::LogProvider::Instance().LogFile(msg.str());

        auto ptr = std::make_shared<StopContinuousDataState>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);
    }

    PrintData(rddata);
}


// Switch Breath Detection Mode
// Auto-Wakeup After Breathphase2
void SwitchBreathDetectionMode_AutoWakeupAfterBreathphase2_State::HandleData() {
    {
        if (IsAlreadySent())
        {
            return;
        }
        SetAlreadySent(true);
        //std::vector<uint8_t> rddata;
        uint8_t* rddata = new uint8_t[BUFSZ]();
        std::cout << "Handles SwitchBreathDetectionMode_AutoWakeupAfterBreathphase2_State.\n";
        std::stringstream msg;
        msg << "Handles SwitchBreathDetectionMode_AutoWakeupAfterBreathphase2_State.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());
        size_t bytes_wrote = this->context_->SendCmd(this);
        
    }
}

// Command={10 02 1e 06 ca}
std::vector<uint8_t> SwitchBreathDetectionMode_AutoWakeupAfterBreathphase2_State::GetCommand() {

    std::vector<uint8_t> data{ 0x10, 0x02, 0x1e, 0x06, 0xca };
    return data;
}

size_t SwitchBreathDetectionMode_AutoWakeupAfterBreathphase2_State::GetRespondBytes() {
    return 4;
}

// CommandId=0x1e06
uint32_t SwitchBreathDetectionMode_AutoWakeupAfterBreathphase2_State::GetCommandId()
{
    return 0x1e06;
}

void SwitchBreathDetectionMode_AutoWakeupAfterBreathphase2_State::Register() {
    this->context_->AttachNeedResponse(this);
}


void SwitchBreathDetectionMode_AutoWakeupAfterBreathphase2_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x1e && rddata[2] == 0x00)
    {
        std::cout << "Change the state of the context to SwitchBreathDetectionMode_AutoWakeupAfterBreathphase3_State.\n";
        std::stringstream msg;
        msg << "Change the state of the context to SwitchBreathDetectionMode_AutoWakeupAfterBreathphase3_State.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());

        // sucess
        auto ptr = std::make_shared<SwitchBreathDetectionMode_AutoWakeupAfterBreathphase3_State>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr );
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x1e && rddata[2] == 0x01)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        std::stringstream msg;
        msg << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        MedibusServer::LogProvider::Instance().LogFile(msg.str());

        auto ptr = std::make_shared<StopContinuousDataState>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);
    }

    PrintData(rddata);
}

// Switch Breath Detection Mode
// Auto-Wakeup After Breathphase3
void SwitchBreathDetectionMode_AutoWakeupAfterBreathphase3_State::HandleData() {
    {
        if (IsAlreadySent())
        {
            return;
        }
        SetAlreadySent(true);
        //std::vector<uint8_t> rddata;
        uint8_t* rddata = new uint8_t[BUFSZ]();
        std::cout << "Handles SwitchBreathDetectionMode_AutoWakeupAfterBreathphase3_State.\n";
        std::stringstream msg;
        msg << "Handles SwitchBreathDetectionMode_AutoWakeupAfterBreathphase3_State.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());
        size_t bytes_wrote = this->context_->SendCmd(this);
        
    }
}

// Command={10 02 1e 07 c9}
std::vector<uint8_t> SwitchBreathDetectionMode_AutoWakeupAfterBreathphase3_State::GetCommand() {

    std::vector<uint8_t> data{ 0x10, 0x02, 0x1e, 0x07, 0xc9 };
    return data;
}

size_t SwitchBreathDetectionMode_AutoWakeupAfterBreathphase3_State::GetRespondBytes() {
    return 4;
}

// CommandId=0x1e07
uint32_t SwitchBreathDetectionMode_AutoWakeupAfterBreathphase3_State::GetCommandId()
{
    return 0x1e07;
}

void SwitchBreathDetectionMode_AutoWakeupAfterBreathphase3_State::Register() {
    this->context_->AttachNeedResponse(this);
}


void SwitchBreathDetectionMode_AutoWakeupAfterBreathphase3_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x1e && rddata[2] == 0x00)
    {
        std::cout << "Change the state of the context to SwitchBreathDetectionMode_AutoWakeupAfterBreathphase4_State.\n";
        std::stringstream msg;
        msg << "Change the state of the context to SwitchBreathDetectionMode_AutoWakeupAfterBreathphase4_State.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());

        // sucess
        auto ptr = std::make_shared<SwitchBreathDetectionMode_AutoWakeupAfterBreathphase4_State>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr );
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x1e && rddata[2] == 0x01)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        std::stringstream msg;
        msg << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        MedibusServer::LogProvider::Instance().LogFile(msg.str());

        auto ptr = std::make_shared<StopContinuousDataState>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);
    }

    PrintData(rddata);
}


// Switch Breath Detection Mode
// Auto-Wakeup After Breathphase4
void SwitchBreathDetectionMode_AutoWakeupAfterBreathphase4_State::HandleData() {
    {
        if (IsAlreadySent())
        {
            return;
        }
        SetAlreadySent(true);
        //std::vector<uint8_t> rddata;
        uint8_t* rddata = new uint8_t[BUFSZ]();
        std::cout << "Handles SwitchBreathDetectionMode_AutoWakeupAfterBreathphase4_State.\n";
        std::stringstream msg;
        msg << "Handles SwitchBreathDetectionMode_AutoWakeupAfterBreathphase4_State.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());
        size_t bytes_wrote = this->context_->SendCmd(this);
        
    }
}

// Command={10 02 1e 08 c8}
std::vector<uint8_t> SwitchBreathDetectionMode_AutoWakeupAfterBreathphase4_State::GetCommand() {

    std::vector<uint8_t> data{ 0x10, 0x02, 0x1e, 0x08, 0xc8 };
    return data;
}

size_t SwitchBreathDetectionMode_AutoWakeupAfterBreathphase4_State::GetRespondBytes() {
    return 5;
}

// CommandId=0x1e08
uint32_t SwitchBreathDetectionMode_AutoWakeupAfterBreathphase4_State::GetCommandId()
{
    return 0x1e08;
}

void SwitchBreathDetectionMode_AutoWakeupAfterBreathphase4_State::Register() {
    this->context_->AttachNeedResponse(this);
}


void SwitchBreathDetectionMode_AutoWakeupAfterBreathphase4_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x1e && rddata[2] == 0x00)
    {
        std::cout << "Change the state of the context to SwitchBreathDetectionMode_AutoWakeupAfterBreathphase4_State.\n";
        std::stringstream msg;
        msg << "Change the state of the context to SwitchBreathDetectionMode_AutoWakeupAfterBreathphase4_State.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());

        // sucess
        auto ptr = std::make_shared<SwitchBreathDetectionMode_AutoWakeupAfterBreathphase5_State>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr );
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x1e && rddata[2] == 0x01)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        std::stringstream msg;
        msg << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        MedibusServer::LogProvider::Instance().LogFile(msg.str());

        auto ptr = std::make_shared<StopContinuousDataState>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);
    }

    PrintData(rddata);
}

// Switch Breath Detection Mode
// Auto-Wakeup After Breathphase5
void SwitchBreathDetectionMode_AutoWakeupAfterBreathphase5_State::HandleData() {
    {
        if (IsAlreadySent())
        {
            return;
        }
        SetAlreadySent(true);
        //std::vector<uint8_t> rddata;
        uint8_t* rddata = new uint8_t[BUFSZ]();
        std::cout << "Handles SwitchBreathDetectionMode_AutoWakeupAfterBreathphase5_State.\n";
        std::stringstream msg;
        msg << "Handles SwitchBreathDetectionMode_AutoWakeupAfterBreathphase5_State.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());

        size_t bytes_wrote = this->context_->SendCmd(this);
    }
}

// Command={10 02 1e 09 c7}
std::vector<uint8_t> SwitchBreathDetectionMode_AutoWakeupAfterBreathphase5_State::GetCommand() {

    std::vector<uint8_t> data{ 0x10, 0x02, 0x1e, 0x09, 0xc7 };
    return data;
}

size_t SwitchBreathDetectionMode_AutoWakeupAfterBreathphase5_State::GetRespondBytes() {
    return 5;
}

// CommandId=0x1e09
uint32_t SwitchBreathDetectionMode_AutoWakeupAfterBreathphase5_State::GetCommandId()
{
    return 0x1e09;
}


void SwitchBreathDetectionMode_AutoWakeupAfterBreathphase5_State::Register() {
    this->context_->AttachNeedResponse(this);
}

void SwitchBreathDetectionMode_AutoWakeupAfterBreathphase5_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x1e && rddata[2] == 0x00)
    {
        std::cout << "Change the state of the context to TransmitPatientData_120E_State.\n";
        std::stringstream msg;
        msg << "Change the state of the context to TransmitPatientData_120E_State.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());

        // sucess
        auto ptr = std::make_shared<TransmitPatientData_120E_State>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x1e && rddata[2] == 0x01)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        std::stringstream msg;
        msg << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        MedibusServer::LogProvider::Instance().LogFile(msg.str());

        auto ptr = std::make_shared<StopContinuousDataState>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);
    }

    PrintData(rddata);
}

// Transmit Patient Data
// FRAME_$12$0E - Parameter Detailed Status
void TransmitPatientData_120E_State::HandleData() {
    {
        if (IsAlreadySent())
        {
            return;
        }
        SetAlreadySent(true);
        std::vector<uint8_t> data;
        uint8_t* rddata = new uint8_t[BUFSZ]();
        std::cout << "Handles TransmitPatientDataState.\n";
        std::stringstream msg;
        msg << "Handles TransmitPatientDataState.\n";
        MedibusServer::LogProvider::Instance().LogFile(msg.str());

        size_t bytes_wrote = this->context_->SendCmd(this);
        //size_t bytes_read = this->context_->ReadRespond(this, rddata, BUFSZ - 1);
        //// cache the read data to vector data
        //data.insert(data.end(), rddata, rddata + bytes_read);
        //if (rddata[0] == 0x06 && rddata[1] == 0x12)
        //{
        //    std::cout << "Change the state of the context to .\n";
        //    // sucess

        //    if (data[13] == 0x0e)
        //    {
        //        if (data[12] != 0x00)
        //        {
        //            // fail
        //            this->context_->TransitionTo(new MeasurementModeState);
        //        }
        //        else
        //        {
        //            // success
        //            this->context_->TransitionTo(new OperatingModeState);
        //        }
        //        

        //    }
        //    else
        //    {
        //        // not 0x0e, continue to read respond
        //        
        //    }

        //}
        //else if (bytes_read != 0 && rddata[0] == 0x15 && rddata[1] == 0x03 && rddata[2] == 0x01)
        //{
        //    // fail
        //    
        //}

    }
}

// Command={10 0d 12 00 00 00 00 00 0f 68 18 40 1f 00 3c a7}
std::vector<uint8_t> TransmitPatientData_120E_State::GetCommand() {

    //std::vector<uint8_t> data{ 0x10, 0x0d, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x40, 0x05, 0x00, 0x3c, 0x10  };
    // all data
    std::vector<uint8_t> data{ 0x10, 0x0d, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x68, 0x18, 0x40, 0x1f, 0x00, 0x3c, 0xa7 };
    return data;
}

size_t TransmitPatientData_120E_State::GetRespondBytes() {
    return 28;
}

uint32_t TransmitPatientData_120E_State::GetCommandId()
{
    return 0x120e00;
}

void TransmitPatientData_120E_State::Register() {
    this->context_->AttachNeedResponse(this);
}


void TransmitPatientData_120E_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x12)
    {
        // sucess
        if (rddata[13] == 0x0e)
        {
            // check HSP for ProvideTheSensorModuleWithRequiredDataState
            if (rddata[7] & 0xde)
            {
                this->context_->SetNeedsExternalData(true);
                this->context_->SetNeedsExternalDataValue(rddata[7]);
            }
            else
            {
                this->context_->SetNeedsExternalData(false);
                this->context_->SetNeedsExternalDataValue(rddata[7]);
            }

            if (rddata[12] != 0x00)
            {
                // fail
                std::cout << "Fail with switch to MeasurementModeState: " << '\n';

                auto ptr = std::make_shared<MeasurementModeState>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
            else
            {
                // success
                std::cout << "Change the state of the context to TransmitPatientData_120E_State.\n";
                auto ptr = std::make_shared<OperatingModeState>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
        }
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x1e && rddata[2] == 0x01)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        std::cout << "Skip to OperatingModeState.\n";
        auto ptr = std::make_shared<OperatingModeState>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);
    }

    PrintData(rddata);
}

bool TransmitPatientData_120E_State::IsSingleCommand()
{
    return true;
}

bool TransmitPatientData_120E_State::IsContinuousCommand()
{
    return false;
}


// Measurement Mode
void MeasurementModeState::HandleData() {
    {
        if (IsAlreadySent())
        {
            return;
        }
        SetAlreadySent(true);
        //std::vector<uint8_t> rddata;
        uint8_t* rddata = new uint8_t[BUFSZ]();
        std::cout << "Handles OperatingModeState.\n";
        std::chrono::milliseconds now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
        
        auto diffmilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastTime);
        if (diffmilliseconds.count() >= 1000 )
        {
            m_lastTime = now;
            size_t bytes_wrote = this->context_->SendCmd(this);
        }
        
        //size_t bytes_read = this->context_->ReadRespond(this, rddata, BUFSZ - 1);
        //if (bytes_read == GetRespondBytes() && rddata[0] == 0x06 && rddata[1] == 0x03 && rddata[2] == 0x01)
        //{
        //    std::cout << "Change the state of the context to .\n";
        //    // success
        //    if (rddata[3] == 0x00)
        //    {
        //        this->context_->TransitionTo(new OperatingModeState);
        //    }
        //    else
        //    {
        //        // fail
        //        std::cout << "Still not measurement mode: " << GetErrorMessage(rddata[3]) << '\n';
        //        this->context_->TransitionTo(new MeasurementModeState);
        //    }
        //    
        //}
        //else if (bytes_read != 0 && rddata[0] == 0x15 && rddata[1] == 0x03 && rddata[2] == 0x01)
        //{
        //    // fail
        //    std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        //    this->context_->TransitionTo(new MeasurementModeState);
        //}
    }
}


std::vector<uint8_t> MeasurementModeState::GetCommand() {

    std::vector<uint8_t> data{ 0x10, 0x02, 0x03, 0x00, 0xeb };
    return data;
}

size_t MeasurementModeState::GetRespondBytes() {
    return 5;
}

uint32_t MeasurementModeState::GetCommandId()
{
    return 0x0300;
}

void MeasurementModeState::Register() {
    this->context_->AttachNeedResponse(this);
}

void MeasurementModeState::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x03 && rddata[2] == 0x01)
    {
       
        // success
        if (rddata[3] == 0x00)
        {
            std::cout << "Change the state of the context to OperatingModeState.\n";
            auto ptr = std::make_shared<OperatingModeState>();
            this->context_->TransitionTo(ptr->GetCommandId(), ptr);
        }
        else
        {
            // fail
            std::cout << "Still not measurement mode: " << GetErrorMessage(rddata[3]) << '\n';
            std::cout << "Change the state of the context to MeasurementModeState.\n";
            auto ptr = std::make_shared<MeasurementModeState>();
            this->context_->TransitionTo(ptr->GetCommandId(), ptr);
        }

    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x03 && rddata[2] == 0x01)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        auto ptr = std::make_shared<StopContinuousDataState>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);
    }

    PrintData(rddata);
}

// Operating Mode
void OperatingModeState::HandleData() {
    {
        if(IsAlreadySent())
        {
            return;
        }
        SetAlreadySent(true);
        std::vector<uint8_t> data;
        uint8_t* rddata = new uint8_t[BUFSZ]();
        std::cout << "Handles OperatingModeState.\n";

        size_t bytes_wrote = this->context_->SendCmd(this);
        //size_t bytes_read = this->context_->ReadRespond(this, rddata, BUFSZ - 1);
        //data.insert(data.end(), rddata, rddata + bytes_read);
        //if (bytes_read == GetRespondBytes() && rddata[0] == 0x06 && rddata[1] == 0x03 && rddata[2] == 0x01)
        //{
        //    std::cout << "Change the state of the context to .\n";
        //    // success
        //    if (rddata[3] != 0x00)
        //    {
        //        this->context_->TransitionTo(new MeasurementModeState);
        //    }
        //    
        //}
        //else if (bytes_read != 0 && rddata[0] == 0x15 && rddata[1] == 0x03 && rddata[2] == 0x01)
        //{
        //    // fail
        //    std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        //    this->context_->TransitionTo(new StopContinuousDataState);
        //}
    }
}


std::vector<uint8_t> OperatingModeState::GetCommand() {

    std::vector<uint8_t> data{ 0x10, 0x02, 0x03, 0x00, 0xeb };
    return data;
}

size_t OperatingModeState::GetRespondBytes() {
    return 5;
}

uint32_t OperatingModeState::GetCommandId()
{
    return 0x0301;
}


void OperatingModeState::Register() {
    this->context_->AttachNeedResponse(this);
}

void OperatingModeState::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x03 && rddata[2] == 0x01)
    {
        // success
        if (rddata[3] == 0x00)
        {
            std::cout << "Change the state of the context SwitchValvesState.\n";
            auto ptr = std::make_shared<SwitchValvesState>();
            this->context_->TransitionTo(ptr->GetCommandId(), ptr);
        }

    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x03 && rddata[2] == 0x01)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        auto ptr = std::make_shared<StopContinuousDataState>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);
    }

    PrintData(rddata);
}


// Switch Valves State
// CMD_$61 - Switch Valves
void SwitchValvesState::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    std::vector<uint8_t> data;
    uint8_t* rddata = new uint8_t[BUFSZ]();
    std::cout << "Handles SwitchValvesState.\n";

    size_t bytes_wrote = this->context_->SendCmd(this);
}

// VP - Valve Position
// Sample Gas1
std::vector<uint8_t> SwitchValvesState::GetCommand()
{
    std::vector<uint8_t> data{ 0x10, 0x02, 0x61, 0x00, 0x8d };
    return data;
}

size_t SwitchValvesState::GetRespondBytes()
{
    return 4;
}

uint32_t SwitchValvesState::GetCommandId()
{
    return 0x6100;
}

void SwitchValvesState::Register()
{
    this->context_->AttachNeedResponse(this);
}

void SwitchValvesState::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x61 && rddata[2] == 0x00)
    {
        // success
        std::cout << "Change the state of the context SwitchPumpState.\n";
        auto ptr = std::make_shared<SwitchPumpState>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);

    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x62 && rddata[2] == 0x01)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        auto ptr = std::make_shared<StopContinuousDataState>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);
    }

    PrintData(rddata);
}


// Switch Valves State
// CMD_$61 - Switch Valves
void SwitchPumpState::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    std::vector<uint8_t> data;
    uint8_t* rddata = new uint8_t[BUFSZ]();
    std::cout << "Handles SwitchPumpState.\n";

    size_t bytes_wrote = this->context_->SendCmd(this);
}

// PF - Pump Flow
// High Flow
std::vector<uint8_t> SwitchPumpState::GetCommand()
{
    std::vector<uint8_t> data{ 0x10, 0x02, 0x62, 0x02, 0x8a };
    return data;
}

size_t SwitchPumpState::GetRespondBytes()
{
    return 4;
}

uint32_t SwitchPumpState::GetCommandId()
{
    return 0x6202;
}

void SwitchPumpState::Register()
{
    this->context_->AttachNeedResponse(this);
}

void SwitchPumpState::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x62 && rddata[2] == 0x00)
    {
        // success
        std::cout << "Change the state of the context SelectTheAnestheticAgentState.\n";
        auto ptr = std::make_shared<SelectTheAnestheticAgentState>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);

    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x62 && rddata[2] == 0x01)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        auto ptr = std::make_shared<StopContinuousDataState>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);
    }

    PrintData(rddata);
}


// Handle Continuous Command
// SelectTheAnestheticAgentState
// FRAME_$12$0E - Parameter Detailed Status
void SelectTheAnestheticAgentState::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    Register();
}

std::vector<uint8_t> SelectTheAnestheticAgentState::GetCommand()
{
    return std::vector<uint8_t>();
}

size_t SelectTheAnestheticAgentState::GetRespondBytes()
{
    return 0;
}

// CommandId=0x120e0401
uint32_t SelectTheAnestheticAgentState::GetCommandId()
{
    return 0x120e0401;
}

void SelectTheAnestheticAgentState::Register()
{
    this->context_->AttachNeedResponse(this);
}

void SelectTheAnestheticAgentState::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x12)
    {
        if (rddata[13] == 0x0e )
        {
            // PAI available?
            if (rddata[4] & 0x0c)
            {
                // success
                std::cout << "PAI is available .\n";
                this->context_->SetPAIAvailable(true);
                std::cout << "Change the state of the context Evaluate_1210_State.\n";
                auto ptr = std::make_shared<Evaluate_1210_State>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
            // PAI not available
            else
            {
                // success
                std::cout << "Change the state of the context.\n";
                auto ptr = std::make_shared<ProvideTheSensorModuleWithRequiredData_State>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
        }
       
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x1e && rddata[2] == 0x01)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
    }

    PrintData(rddata);
}


void Evaluate_1210_State::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    std::cout << "Handles Evaluate_1210_State.\n";
    Register();
}

std::vector<uint8_t> Evaluate_1210_State::GetCommand()
{
    return std::vector<uint8_t>();
}

size_t Evaluate_1210_State::GetRespondBytes()
{
    return 0;
}

uint32_t Evaluate_1210_State::GetCommandId()
{
    return 0x121009;
}

void Evaluate_1210_State::Register()
{
    this->context_->AttachNeedResponse(this);
}

void Evaluate_1210_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x12)
    {
        
        if (rddata[13] == 0x10 && (rddata[9] & 0x03) == 0x0)
        {
            // NAIF
            // success
            std::cout << "Change the state of the context SelectAnestheticAgentType_Halothane_State.\n";
            auto ptr = std::make_shared<SelectAnestheticAgentType_Halothane_State>();
            this->context_->TransitionTo(ptr->GetCommandId(), ptr);
        }

        else if (rddata[13] == 0x10 && (rddata[9] & 0x02))
        {
            // DAIF
            // success
            std::cout << "Change the state of the context ProvideTheSensorModuleWithRequiredData_State.\n";
            auto ptr = std::make_shared<ProvideTheSensorModuleWithRequiredData_State>();
            this->context_->TransitionTo(ptr->GetCommandId(), ptr);
        }
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x1e && rddata[2] == 0x01)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
    }

    PrintData(rddata);
}

void SelectAnestheticAgentType_Halothane_State::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    std::vector<uint8_t> data;
    uint8_t* rddata = new uint8_t[BUFSZ]();
    std::cout << "Handles SelectAnestheticAgentType_Halothane_State.\n";

    size_t bytes_wrote = this->context_->SendCmd(this);
}

std::vector<uint8_t> SelectAnestheticAgentType_Halothane_State::GetCommand()
{
    std::vector<uint8_t> data{ 0x10, 0x03, 0x1d, 0x01, 0x00, 0xcf };
    return data;
}

size_t SelectAnestheticAgentType_Halothane_State::GetRespondBytes()
{
    return size_t();
}

uint32_t SelectAnestheticAgentType_Halothane_State::GetCommandId()
{
    return 0x1d01;
}

void SelectAnestheticAgentType_Halothane_State::Register()
{
    this->context_->AttachNeedResponse(this);
}

void SelectAnestheticAgentType_Halothane_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x1d && rddata[2] == 0x00)
    {
        // success
        std::cout << "Change the state of the context ProvideTheSensorModuleWithRequiredData_State.\n";
        auto ptr = std::make_shared<ProvideTheSensorModuleWithRequiredData_State>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);

    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x1d && rddata[2] == 0x01)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        auto ptr = std::make_shared<StopContinuousDataState>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);
    }

    PrintData(rddata);
}

void ProvideTheSensorModuleWithRequiredData_State::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    std::cout << "Handles ProvideTheSensorModuleWithRequiredData_State.\n";
    Register();
}


std::vector<uint8_t> ProvideTheSensorModuleWithRequiredData_State::GetCommand()
{
    return std::vector<uint8_t>();
}
size_t ProvideTheSensorModuleWithRequiredData_State::GetRespondBytes()
{
    return size_t();
}

// CommandId=0x120e07
uint32_t ProvideTheSensorModuleWithRequiredData_State::GetCommandId()
{
    return 0x120e07;
}
void ProvideTheSensorModuleWithRequiredData_State::Register()
{
    this->context_->AttachNeedResponse(this);
}
void ProvideTheSensorModuleWithRequiredData_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x12)
    {
        if (rddata[13] == 0x0e)
        {
            // check HSP for checking needs the module external data
            if (rddata[7] & 0xde)
            {
                // yes, need
                 // success
                std::cout << "Needs External Data .\n";

                std::cout << "Change the state of the context AcceptExternalParameterData_UnknownAccuracy_State.\n";
                auto ptr = std::make_shared<AcceptExternalParameterData_UnknownAccuracy_State>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
            else
            {
               // no, not need
                // success
                std::cout << "Not Needs External Data .\n";

                std::cout << "Change the state of the context SuperviseModuleStatus_120E_MSBit2_State.\n";
                 auto ptr = std::make_shared<SuperviseModuleStatus_120E_MSBit2_State>();
                 this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
        }
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x12 && rddata[2] == 0x09)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
    }

    PrintData(rddata);
}

void AcceptExternalParameterData_UnknownAccuracy_State::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    std::vector<uint8_t> data;
    uint8_t* rddata = new uint8_t[BUFSZ]();
    std::cout << "Handles AcceptExternalParameterData_UnknownAccuracy_State.\n";

    size_t bytes_wrote = this->context_->SendCmd(this);
}

// Command={10 06 1c df 0a 02 e3}
// Command={10 06 1c df 0a 01 e3}
std::vector<uint8_t> AcceptExternalParameterData_UnknownAccuracy_State::GetCommand()
{
    std::vector<uint8_t> data{ 0x10, 0x06, 0x1c, 0xdf, 0x0a, 0x02, 0xe3};
    return data;
}

size_t AcceptExternalParameterData_UnknownAccuracy_State::GetRespondBytes()
{
    return size_t();
}

// CommandId=0x06
uint32_t AcceptExternalParameterData_UnknownAccuracy_State::GetCommandId()
{
    return 0x06;
}

void AcceptExternalParameterData_UnknownAccuracy_State::Register()
{
    this->context_->AttachNeedResponse(this);
}

void AcceptExternalParameterData_UnknownAccuracy_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x1c && rddata[2] == 0x00)
    {
        // success
        std::cout << "Change the state of the context SuperviseModuleStatus_120E_MSBit2_State.\n";
         auto ptr = std::make_shared<SuperviseModuleStatus_120E_MSBit2_State>();
         this->context_->TransitionTo(ptr->GetCommandId(), ptr);

    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x1c && rddata[2] == 0x01)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
       /* auto ptr = std::make_shared<StopContinuousDataState>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);*/
    }

    PrintData(rddata);
}

void SuperviseModuleStatus_120E_MSBit2_State::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    std::cout << "Handles SuperviseModuleStatus_120E_MSBit2_State.\n";
    Register();
}
std::vector<uint8_t> SuperviseModuleStatus_120E_MSBit2_State::GetCommand()
{
    return std::vector<uint8_t>();
}
size_t SuperviseModuleStatus_120E_MSBit2_State::GetRespondBytes()
{
    return size_t();
}
uint32_t SuperviseModuleStatus_120E_MSBit2_State::GetCommandId()
{
    return 0x120e02;
}
void SuperviseModuleStatus_120E_MSBit2_State::Register()
{
    this->context_->AttachNeedResponse(this);
}
void SuperviseModuleStatus_120E_MSBit2_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x12)
    {
        if (rddata[13] == 0x0e)
        {
            // check Watertrap? yes
            if (rddata[14] & 0x04)
            {
                // yes
                std::cout << "Change the state of the context SuperviseModuleStatus_120B_MSWBit5_State.\n";
                auto ptr = std::make_shared<SuperviseModuleStatus_120B_MSWBit5_State>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
            // check Watertrap? no
            else 
            {
                // Is Any Component Fail?
                 auto ptr = std::make_shared<SuperviseModuleStatus_120E_MSBit6_State>();
                 this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
        }

    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x12 && rddata[2] == 0x09)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
    }

    PrintData(rddata);
}

void SuperviseModuleStatus_120B_MSWBit5_State::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    std::cout << "Handles SuperviseModuleStatus_120B_MSWBit5_State.\n";
    Register();
}

std::vector<uint8_t> SuperviseModuleStatus_120B_MSWBit5_State::GetCommand()
{
    return std::vector<uint8_t>();
}

size_t SuperviseModuleStatus_120B_MSWBit5_State::GetRespondBytes()
{
    return size_t();
}

// CommandId=0x120b05
uint32_t SuperviseModuleStatus_120B_MSWBit5_State::GetCommandId()
{
    return 0x120b05;
}

void SuperviseModuleStatus_120B_MSWBit5_State::Register()
{
    this->context_->AttachNeedResponse(this);
}

void SuperviseModuleStatus_120B_MSWBit5_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    // Is Watertrap disconnected
    if (rddata[0] == 0x06 && rddata[1] == 0x12)
    {
        if (rddata[13] == 0x0b)
        {
            // MSW, bit5
            if (rddata[3] & 0x20)
            {
                // Watertrap is disconnected.
                // Do not affect values
                // Display warning message to check watertrap.
                std::cout << "Display warning message to check watertrap .\n";
                std::cout << "Leave gas labels and values unchanged at this point .\n";
                std::cout << "Change the state of the context SuperviseModuleStatus_120E_MSBit6_State.\n";
                auto ptr = std::make_shared<SuperviseModuleStatus_120E_MSBit6_State>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);

            }
            else
            {
                std::cout << "Change the state of the context SuperviseModuleStatus_120B_MSWBit6_State.\n";
                auto ptr = std::make_shared<SuperviseModuleStatus_120B_MSWBit6_State>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
        }
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x12 && rddata[2] == 0x09)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
    }
}


void SuperviseModuleStatus_120B_MSWBit6_State::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    std::cout << "Handles SuperviseModuleStatus_120B_MSWBit6_State.\n";
    Register();
}

std::vector<uint8_t> SuperviseModuleStatus_120B_MSWBit6_State::GetCommand()
{
    return std::vector<uint8_t>();
}

size_t SuperviseModuleStatus_120B_MSWBit6_State::GetRespondBytes()
{
    return size_t();
}

uint32_t SuperviseModuleStatus_120B_MSWBit6_State::GetCommandId()
{
    return 0x120b06;
}

void SuperviseModuleStatus_120B_MSWBit6_State::Register()
{
    this->context_->AttachNeedResponse(this);
}

void SuperviseModuleStatus_120B_MSWBit6_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    // Watertrap full?
    if (rddata[0] == 0x06 && rddata[1] == 0x12)
    {
        if (rddata[13] == 0x0b)
        {
            // MSW, bit5
            if (rddata[3] & 0x40)
            {
                // Watertrap is full.
                // Replace wa
                // tertrap.
                // Do not affect values.
                std::cout << "Display warning message, that watetrap is full. \n";
                std::cout << "Leave gas labels and values unchanged at this point .\n";
                std::cout << "Change the state of the context SuperviseModuleStatus_120E_MSBit6_State.\n";
                auto ptr = std::make_shared<SuperviseModuleStatus_120E_MSBit6_State>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);

            }
            else
            {
                std::cout << "Change the state of the context SuperviseModuleStatus_120E_MSWBit7_State.\n";
                auto ptr = std::make_shared<SuperviseModuleStatus_120E_MSWBit7_State>();


            	this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
        }
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x12 && rddata[2] == 0x09)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
    }
}

void SuperviseModuleStatus_120E_MSWBit7_State::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    std::cout << "Handles SuperviseModuleStatus_120E_MSWBit7_State.\n";
    Register();
}

std::vector<uint8_t> SuperviseModuleStatus_120E_MSWBit7_State::GetCommand()
{
    return std::vector<uint8_t>();
}

size_t SuperviseModuleStatus_120E_MSWBit7_State::GetRespondBytes()
{
    return size_t();
}

uint32_t SuperviseModuleStatus_120E_MSWBit7_State::GetCommandId()
{
    return 0x120b07;
}

void SuperviseModuleStatus_120E_MSWBit7_State::Register()
{
    this->context_->AttachNeedResponse(this);
}

void SuperviseModuleStatus_120E_MSWBit7_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    // Watertrap warning?
    if (rddata[0] == 0x06 && rddata[1] == 0x12)
    {
        if (rddata[13] == 0x0b)
        {
            // MSW, bit5
            if (rddata[3] & 0x80)
            {
                // Watertrap will be full soon.
                // DO not affect values.
                std::cout << "Display warning message to check watertrap level. \n";
                std::cout << "Leave gas labels and values unchanged at this point .\n";
                std::cout << "Change the state of the context SuperviseModuleStatus_120E_MSBit6_State.\n";
                auto ptr = std::make_shared<SuperviseModuleStatus_120E_MSBit6_State>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);

            }
            else
            {
                // SW-Bug.
                // Handle as unspecific pneumatics error.
                std::cout << "Display warning message to check pneumatics. \n";
                std::cout << "Leave gas labels and values unchanged at this point .\n";
                std::cout << "Change the state of the context SuperviseModuleStatus_120E_MSBit6_State.\n";
                auto ptr = std::make_shared<SuperviseModuleStatus_120E_MSBit6_State>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
        }
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x12 && rddata[2] == 0x09)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
    }
}

void SuperviseModuleStatus_120E_MSBit6_State::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    std::cout << "Handles SuperviseModuleStatus_120E_MSBit6_State.\n";
    Register();
}

std::vector<uint8_t> SuperviseModuleStatus_120E_MSBit6_State::GetCommand()
{
    return std::vector<uint8_t>();
}

size_t SuperviseModuleStatus_120E_MSBit6_State::GetRespondBytes()
{
    return size_t();
}

// CommandId=0x120e06
uint32_t SuperviseModuleStatus_120E_MSBit6_State::GetCommandId()
{
    return 0x120e06;
}

void SuperviseModuleStatus_120E_MSBit6_State::Register()
{
    this->context_->AttachNeedResponse(this);
}

void SuperviseModuleStatus_120E_MSBit6_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    // Any Component Fail
    if (rddata[0] == 0x06 && rddata[1] == 0x12)
    {
        if (rddata[13] == 0x0e)
        {
            // check Watertrap? 
            if (rddata[14] & 0x40)
            {
                // yes
                // Display warning message, that a hardware failure is present
                std::cout << "Display warning message, that a hardware failure is present. \n";
                std::cout << "Leave gas labels and values unchanged at this point .\n";
                std::cout << "Change the state of the context SuperviseModuleStatus_120E_MSBit5_State.\n";
                auto ptr = std::make_shared<SuperviseModuleStatus_120E_MSBit5_State>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
            else
            {
                // no
                std::cout << "Change the state of the context SuperviseModuleStatus_120E_MSBit5_State.\n";
                auto ptr = std::make_shared<SuperviseModuleStatus_120E_MSBit5_State>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
        }
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x12 && rddata[2] == 0x09)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
    }
}

void SuperviseModuleStatus_120E_MSBit5_State::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    std::cout << "Handles SuperviseModuleStatus_120E_MSBit5_State.\n";
    Register();
}

std::vector<uint8_t> SuperviseModuleStatus_120E_MSBit5_State::GetCommand()
{
    return std::vector<uint8_t>();
}

size_t SuperviseModuleStatus_120E_MSBit5_State::GetRespondBytes()
{
    return size_t();
}

uint32_t SuperviseModuleStatus_120E_MSBit5_State::GetCommandId()
{
    return 0x120e05;
}

void SuperviseModuleStatus_120E_MSBit5_State::Register()
{
    this->context_->AttachNeedResponse(this);
}

void SuperviseModuleStatus_120E_MSBit5_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x12)
    {
        if (rddata[13] == 0x0e)
        {
            if (rddata[14] & 0x20)
            {
                // yes
                std::cout << "Frame data contain breath phase related data which can be evaluated for e.g. alarm handling. \n";
                std::cout << "Change the state of the context SuperviseModuleStatus_120E_MSBit4_State.\n";
                auto ptr = std::make_shared<SuperviseModuleStatus_120E_MSBit4_State>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
            else
            {
                // no
                std::cout << "Frame data contain realtime values(same as corresponding parameter RT_X in RTDATA). \n";
                std::cout << "Change the state of the context SuperviseModuleStatus_120E_MSBit4_State.\n";
                auto ptr = std::make_shared<SuperviseModuleStatus_120E_MSBit4_State>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
        }
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x12 && rddata[2] == 0x09)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
    }
}

void SuperviseModuleStatus_120E_MSBit4_State::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    std::cout << "Handles SuperviseModuleStatus_120E_MSBit4_State.\n";
    Register();
}

std::vector<uint8_t> SuperviseModuleStatus_120E_MSBit4_State::GetCommand()
{
    return std::vector<uint8_t>();
}

size_t SuperviseModuleStatus_120E_MSBit4_State::GetRespondBytes()
{
    return size_t();
}

uint32_t SuperviseModuleStatus_120E_MSBit4_State::GetCommandId()
{
    return 0x120e0402;
}

void SuperviseModuleStatus_120E_MSBit4_State::Register()
{
    this->context_->AttachNeedResponse(this);
}

void SuperviseModuleStatus_120E_MSBit4_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    // Breath phase data available?
    if (rddata[0] == 0x06 && rddata[1] == 0x12)
    {
        if (rddata[13] == 0x0e)
        {
            if (rddata[14] & 0x10)
            {
                // yes
                std::cout << "Meaning as \"No respiration\":no breathing cycles detectable. \n";
                std::cout << "Meaning as \"Apnea\":A previously detected breathing activity has timed out. \n";
                std::cout << "Change the state of the context SuperviseZeroRequest_120E_OMS_State.\n";
                auto ptr = std::make_shared<SuperviseZeroRequest_120E_OMS_State>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
            else
            {
                // no
                std::cout << "Breathing activity on the sample line. \n";
                std::cout << "Change the state of the context SuperviseZeroRequest_120E_OMS_State.\n";
                auto ptr = std::make_shared<SuperviseZeroRequest_120E_OMS_State>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
        }
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x12 && rddata[2] == 0x09)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
    }
}

void SuperviseZeroRequest_120E_OMS_State::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    std::cout << "Handles SuperviseZeroRequest_120E_OMS_State.\n";
    Register();
}

std::vector<uint8_t> SuperviseZeroRequest_120E_OMS_State::GetCommand()
{
    return std::vector<uint8_t>();
}

size_t SuperviseZeroRequest_120E_OMS_State::GetRespondBytes()
{
    return size_t();
}

// CommandId=0x120e1201
uint32_t SuperviseZeroRequest_120E_OMS_State::GetCommandId()
{
    return 0x120e1201;
}

void SuperviseZeroRequest_120E_OMS_State::Register()
{
    this->context_->AttachNeedResponse(this);
}

void SuperviseZeroRequest_120E_OMS_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x12)
    {
        if (rddata[13] == 0x0e)
        {
            if (rddata[12] == 0x00)
            {
                // yes
                std::cout << "Change the state of the context ZeroInProgress_1203_CO2N2OPSBit5_State. \n";
                 auto ptr = std::make_shared<ZeroInProgress_1203_CO2N2OPSBit5_State>();
                 this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
            else
            {
                // no
                std::cout << "Change the state of the context HandleZeroState.\n";
                auto ptr = std::make_shared<HandleZeroRequestState>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
        }
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x12 && rddata[2] == 0x09)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
    }
}

void ZeroInProgress_1203_CO2N2OPSBit5_State::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    std::cout << "Handles ZeroInProgress_1203_CO2N2OPSBit5_State.\n";
    Register();
}

std::vector<uint8_t> ZeroInProgress_1203_CO2N2OPSBit5_State::GetCommand()
{
    return std::vector<uint8_t>();
}

size_t ZeroInProgress_1203_CO2N2OPSBit5_State::GetRespondBytes()
{
    return size_t();
}

uint32_t ZeroInProgress_1203_CO2N2OPSBit5_State::GetCommandId()
{
    return 0x120305;
}

void ZeroInProgress_1203_CO2N2OPSBit5_State::Register()
{
    this->context_->AttachNeedResponse(this);
}

void ZeroInProgress_1203_CO2N2OPSBit5_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x12)
    {
        if (rddata[13] == 0x03)
        {
            if (rddata[11] & 0x20 || rddata[12] & 0x20)
            {
                // yes
                std::cout << "Change the state of the context HandleZeroRequestState. \n";
                auto ptr = std::make_shared<HandleZeroRequestState>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
            else
            {
                // no
                std::cout << "Change the state of the context ZeroInProgress_1204_O2PSBit5_State.\n";
                auto ptr = std::make_shared<ZeroInProgress_1204_O2PSBit5_State>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
        }
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x12 && rddata[2] == 0x09)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
    }
}

void ZeroInProgress_1204_O2PSBit5_State::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    std::cout << "Handles ZeroInProgress_1204_O2PSBit5_State.\n";
    Register();
}

std::vector<uint8_t> ZeroInProgress_1204_O2PSBit5_State::GetCommand()
{
    return std::vector<uint8_t>();
}

size_t ZeroInProgress_1204_O2PSBit5_State::GetRespondBytes()
{
    return size_t();
}

// CommandId=0x120405
uint32_t ZeroInProgress_1204_O2PSBit5_State::GetCommandId()
{
    return 0x120405;
}

void ZeroInProgress_1204_O2PSBit5_State::Register()
{
    this->context_->AttachNeedResponse(this);
}

void ZeroInProgress_1204_O2PSBit5_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x12)
    {
        if (rddata[13] == 0x04)
        {
            if (rddata[11] & 0x20)
            {
                // yes
                std::cout << "Change the state of the context ZeroRequestState. \n";
                auto ptr = std::make_shared<HandleZeroRequestState>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
            else
            {
                // no
                std::cout << "Change the state of the context ZeroInProgress_1210_A1PSBit5_State.\n";
                auto ptr = std::make_shared<ZeroInProgress_1210_A1PSBit5_State>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
        }
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x12 && rddata[2] == 0x09)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
    }
}

void ZeroInProgress_1210_A1PSBit5_State::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    std::cout << "Handles ZeroInProgress_1210_A1PSBit5_State.\n";
    Register();
}

std::vector<uint8_t> ZeroInProgress_1210_A1PSBit5_State::GetCommand()
{
    return std::vector<uint8_t>();
}

size_t ZeroInProgress_1210_A1PSBit5_State::GetRespondBytes()
{
    return size_t();
}

// CommandId=0x121005
uint32_t ZeroInProgress_1210_A1PSBit5_State::GetCommandId()
{
    return 0x121005;
}

void ZeroInProgress_1210_A1PSBit5_State::Register()
{
    this->context_->AttachNeedResponse(this);
}

void ZeroInProgress_1210_A1PSBit5_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x12)
    {
        if (rddata[13] == 0x10)
        {
            if (rddata[11] & 0x20)
            {
                // yes
                std::cout << "Change the state of the context ZeroRequestState. \n";
                auto ptr = std::make_shared<HandleZeroRequestState>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
            else
            {
                // no
                std::cout << "Change the state of the context ZeroInProgress_1211_A2PSBit5_State.\n";
                auto ptr = std::make_shared<ZeroInProgress_1211_A2PSBit5_State>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
        }
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x12 && rddata[2] == 0x09)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
    }
}

void ZeroInProgress_1211_A2PSBit5_State::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    std::cout << "Handles ZeroInProgress_1211_A2PSBit5_State.\n";
    Register();
}

std::vector<uint8_t> ZeroInProgress_1211_A2PSBit5_State::GetCommand()
{
    return std::vector<uint8_t>();
}

size_t ZeroInProgress_1211_A2PSBit5_State::GetRespondBytes()
{
    return size_t();
}

// CommandId=0x121105
uint32_t ZeroInProgress_1211_A2PSBit5_State::GetCommandId()
{
    return 0x121105;
}

void ZeroInProgress_1211_A2PSBit5_State::Register()
{
    this->context_->AttachNeedResponse(this);
}

void ZeroInProgress_1211_A2PSBit5_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x12)
    {
        if (rddata[13] == 0x11)
        {
            if (rddata[12] & 0x20)
            {
                // yes
                std::cout << "Change the state of the context ZeroRequestState. \n";
                auto ptr = std::make_shared<HandleZeroRequestState>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
            else
            {
                // no
                std::cout << "Change the state of the context ZeroRequestState.\n";
                auto ptr = std::make_shared<ZeroRequestState>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
        }
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x12 && rddata[2] == 0x09)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
    }
}

void ZeroRequestState::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    std::cout << "Handles ZeroRequestState.\n";
    Register();
}

std::vector<uint8_t> ZeroRequestState::GetCommand()
{
    return std::vector<uint8_t>();
}

size_t ZeroRequestState::GetRespondBytes()
{
    return size_t();
}

// CommandId=0x120e1200
uint32_t ZeroRequestState::GetCommandId()
{
    return 0x120e1200;
}

void ZeroRequestState::Register()
{
    this->context_->AttachNeedResponse(this);
}

void ZeroRequestState::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x12)
    {
        if (rddata[13] == 0x11)
        {
            if (rddata[12] & 0x20)
            {
                // yes
                std::cout << "Change the state of the context HandleZeroRequestState. \n";
                auto ptr = std::make_shared<HandleZeroRequestState>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
            else
            {
                // no
                std::cout << "Change the state of the context HandleZeroRequestState.\n";
                auto ptr = std::make_shared<HandleZeroRequestState>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
        }
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x12 && rddata[2] == 0x09)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
    }
}

void HandleZeroRequestState::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    std::vector<uint8_t> data;
    uint8_t* rddata = new uint8_t[BUFSZ]();
    std::cout << "Handles HandleZeroRequestState.\n";
    size_t bytes_wrote = this->context_->SendCmdSync(this);

}

// Command={10 01 2c c3}
std::vector<uint8_t> HandleZeroRequestState::GetCommand()
{
    std::vector<uint8_t> data{ 0x10, 0x01, 0x2c, 0xc3};
    return data;
}

size_t HandleZeroRequestState::GetRespondBytes()
{
    return size_t();
}

uint32_t HandleZeroRequestState::GetCommandId()
{
    return 0x2c06;
}

void HandleZeroRequestState::Register()
{
    this->context_->AttachNeedResponse(this);
}

void HandleZeroRequestState::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x2c && rddata[2] == 0x04)
    {

        // bit0
        // 	ZERO_CTRL - Zero Control
        if ((rddata[6] & 0x01) == 0x01)
        {
            // no
            std::cout << "Message to the user to prepare mainstream sensor for zeroing. \n";
            std::cout << "Wait until confirmation of user.\n";
            std::string strAnswer;
            getline(std::cin, strAnswer);
            while (strAnswer == "n")
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
            // get yes confirmation
            std::cout << "Change the state of the context InitZeroState.\n";
            auto ptr = std::make_shared<InitZeroState>();
            this->context_->TransitionTo(ptr->GetCommandId(), ptr);
        }
        else
        {
            // yes
            std::cout << "Change the state of the context InitZeroState.\n";
            auto ptr = std::make_shared<InitZeroState>();
            this->context_->TransitionTo(ptr->GetCommandId(), ptr);
        }

    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x2c && rddata[2] == 0x01)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
    }

    PrintData(rddata);
}

void InitZeroState::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    //std::vector<uint8_t> rddata;
    uint8_t* rddata = new uint8_t[BUFSZ]();
    std::cout << "Handles InitZeroState.\n";

    size_t bytes_wrote = this->context_->SendCmd(this);
}

// Command={10 0b 20 00 00 01 00 00 00 00 00 01 00 c3}
std::vector<uint8_t> InitZeroState::GetCommand()
{
    std::vector<uint8_t> data{ 0x10, 0x0b, 0x20, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0xc3 };
    return data;
}

size_t InitZeroState::GetRespondBytes()
{
    return size_t();
}

// CommandId=0x20010100
uint32_t InitZeroState::GetCommandId()
{
    return 0x20010100;
}

void InitZeroState::Register()
{
    this->context_->AttachNeedResponse(this);
}

void InitZeroState::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x20 && rddata[2] == 0x00)
    {
        std::cout << "Change to GetUnitsState.\n";
        
        auto ptr = std::make_shared<GetUnitsState>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x20 && rddata[2] == 0x01)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
        std::cout << "Skip to GetUnitsState.\n";
        auto ptr = std::make_shared<GetUnitsState>();
        this->context_->TransitionTo(ptr->GetCommandId(), ptr);
    }

    PrintData(rddata);
}

void GetUnitsState::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    std::cout << "Handles GetUnitsState.\n";
    Register();
}

std::vector<uint8_t> GetUnitsState::GetCommand()
{
    return std::vector<uint8_t>();
}

size_t GetUnitsState::GetRespondBytes()
{
    return size_t();
}

// CommandId=0x1212
uint32_t GetUnitsState::GetCommandId()
{
    return 0x1212;
}

void GetUnitsState::Register()
{
    this->context_->AttachNeedResponse(this);
}

void GetUnitsState::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x12)
    {
        if (rddata[13] == 0x12)
        {
            // CO2_U - Co2 Parameter Unit
            if (rddata[3] & 0x05)
            {
                // Atps Mmhg
                std::cout << "CO2_U - Co2 Parameter Unit is Atps Mmhg. \n";
                std::cout << "Change the state of the context EvaluateConnectionEstablishedState.\n";
                auto ptr = std::make_shared<EvaluateConnectionEstablishedState>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
            else if (rddata[3] == 0x00)
            {
                // Ats Vol
                std::cout << "CO2_U - Co2 Parameter Unit is Ats Vol. \n";
                std::cout << "Change the state of the context EvaluateConnectionEstablishedState.\n";
                auto ptr = std::make_shared<EvaluateConnectionEstablishedState>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
            // N2O_U - N2o Parameter Unit
            if (rddata[4] & 0x05)
            {
                // Atps Mmhg
                std::cout << "N2o Parameter Unit is Atps Mmhg. \n";
                std::cout << "Change the state of the context EvaluateConnectionEstablishedState.\n";
                auto ptr = std::make_shared<EvaluateConnectionEstablishedState>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
            else if (rddata[4] == 0x00)
            {
                // Ats Vol
                std::cout << "N2o Parameter Unit is Ats Vol. \n";
                std::cout << "Change the state of the context EvaluateConnectionEstablishedState.\n";
                auto ptr = std::make_shared<EvaluateConnectionEstablishedState>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }

            // A1_U - Agent1 Parameter Unit
            if (rddata[5] & 0x05)
            {
                // Atps Mmhg
                std::cout << "A1_U - Agent1 Parameter Unit is Atps Mmhg. \n";
                std::cout << "Change the state of the context EvaluateConnectionEstablishedState.\n";
                auto ptr = std::make_shared<EvaluateConnectionEstablishedState>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
            else if (rddata[5] == 0x00)
            {
                // Ats Vol
                std::cout << "A1_U - Agent1 Parameter Unit is Ats Vol. \n";
                std::cout << "Change the state of the context EvaluateConnectionEstablishedState.\n";
                auto ptr = std::make_shared<EvaluateConnectionEstablishedState>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }

            // A2_U - Agent2 Parameter Unit
            if (rddata[6] & 0x05)
            {
                // Atps Mmhg
                std::cout << "A2_U - Agent2 Parameter Unit is Atps Mmhg. \n";
                std::cout << "Change the state of the context EvaluateConnectionEstablishedState.\n";
                auto ptr = std::make_shared<EvaluateConnectionEstablishedState>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
            else if (rddata[6] == 0x00)
            {
                // Ats Vol
                std::cout << "A2_U - Agent2 Parameter Unit is Ats Vol. \n";
                std::cout << "Change the state of the context EvaluateConnectionEstablishedState.\n";
                auto ptr = std::make_shared<EvaluateConnectionEstablishedState>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }

            // O2_U - O2 Parameter Unit
            if (rddata[7] & 0x05)
            {
                // Atps Mmhg
                std::cout << "O2_U - O2 Parameter Unit is Atps Mmhg. \n";
                std::cout << "Change the state of the context EvaluateConnectionEstablishedState.\n";
                auto ptr = std::make_shared<EvaluateConnectionEstablishedState>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
            else if (rddata[7] == 0x00)
            {
                // Ats Vol
                std::cout << "O2_U - O2 Parameter Unit is Ats Vol. \n";
                std::cout << "Change the state of the context EvaluateConnectionEstablishedState.\n";
                auto ptr = std::make_shared<EvaluateConnectionEstablishedState>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
        }
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x12 && rddata[2] == 0x09)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
    }

    PrintData(rddata);
}

void EvaluateConnectionEstablishedState::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    std::cout << "Handles EvaluateConnectionEstablishedState.\n";
    Register();
}

std::vector<uint8_t> EvaluateConnectionEstablishedState::GetCommand()
{
    return std::vector<uint8_t>();
}

size_t EvaluateConnectionEstablishedState::GetRespondBytes()
{
    return size_t();
}

uint32_t EvaluateConnectionEstablishedState::GetCommandId()
{
    return 0x2c0601;
}

void EvaluateConnectionEstablishedState::Register()
{
    this->context_->AttachNeedResponse(this);
}

void EvaluateConnectionEstablishedState::Update(std::vector<uint8_t> rddata, size_t sz)
{
    std::cout << "Change the state of the context HostSelectableParameters_120E_HSP_State.\n";
    auto ptr = std::make_shared<HostSelectableParameters_120E_HSP_State>();
    this->context_->TransitionTo(ptr->GetCommandId(), ptr);
}

void HostSelectableParameters_120E_HSP_State::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    std::cout << "Handles HostSelectableParameters_120E_HSP_State.\n";
    Register();
}

std::vector<uint8_t> HostSelectableParameters_120E_HSP_State::GetCommand()
{
    return std::vector<uint8_t>();
}

size_t HostSelectableParameters_120E_HSP_State::GetRespondBytes()
{
    return size_t();
}

// CommandId=0x120e0701
uint32_t HostSelectableParameters_120E_HSP_State::GetCommandId()
{
    return 0x120e0701;
}

void HostSelectableParameters_120E_HSP_State::Register()
{
    this->context_->AttachNeedResponse(this);
}

void HostSelectableParameters_120E_HSP_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x12)
    {
        if (rddata[13] == 0x0e)
        {
            // check HSP for checking needs the module external data
            if (rddata[7] & 0xde)
            {
                // yes, need
                 // success
                std::cout << "Needs External Data .\n";

                std::cout << "This parameter is not measured by the sensor module but it must be provided by the host.\n";
                std::cout << "Show that the parameter is not installed on the sensor module.\n";
                
            }
            else
            {
                // no, not need
                 // success
                std::cout << "Not Needs External Data .\n";

                std::cout << "Change the state of the context ParameterAvailabilityInformation_120E_PAI_State.\n";
                auto ptr = std::make_shared<ParameterAvailabilityInformation_120E_PAI_State>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
        }
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x12 && rddata[2] == 0x09)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
    }

    PrintData(rddata);
}

void ParameterAvailabilityInformation_120E_PAI_State::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    std::cout << "Handles HostSelectableParameters_120E_HSP_State.\n";
    Register();
}

std::vector<uint8_t> ParameterAvailabilityInformation_120E_PAI_State::GetCommand()
{
    return std::vector<uint8_t>();
}

size_t ParameterAvailabilityInformation_120E_PAI_State::GetRespondBytes()
{
    return size_t();
}

// CommandId=0x120e0403
uint32_t ParameterAvailabilityInformation_120E_PAI_State::GetCommandId()
{
    return 0x120e0403;
}

void ParameterAvailabilityInformation_120E_PAI_State::Register()
{
    this->context_->AttachNeedResponse(this);
}

void ParameterAvailabilityInformation_120E_PAI_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x12)
    {
        if (rddata[13] == 0x0e)
        {
            // PAI available?
            if (rddata[4] & 0x0c)
            {
                std::cout << "PAI is available .\n";
                this->context_->SetPAIAvailable(true);
                std::cout << "Change the state of the context Evaluate_1210_State.\n";
                auto ptr = std::make_shared<Evaluate_1210_State>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
            // PAI not available
            else
            {
                std::cout << "PAI is not available .\n";
                std::cout << "Parameter is not available.\n";
                std::cout << "That means, it is not installed in the module.\n";
                std::cout << "Show that the parameter is not installed on the sensor module.\n";
            }
        }

    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x1e && rddata[2] == 0x01)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
    }

    PrintData(rddata);
}


void ParameterMode_1203_CO2PSBit6Bit7_State::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    std::cout << "Handles ParameterMode_1203_CO2PSBit6Bit7_State.\n";
    Register();
}

std::vector<uint8_t> ParameterMode_1203_CO2PSBit6Bit7_State::GetCommand()
{
    return std::vector<uint8_t>();
}

size_t ParameterMode_1203_CO2PSBit6Bit7_State::GetRespondBytes()
{
    return size_t();
}

// CommandId=0x12031106
uint32_t ParameterMode_1203_CO2PSBit6Bit7_State::GetCommandId()
{
    return 0x12031106;
}

void ParameterMode_1203_CO2PSBit6Bit7_State::Register()
{
    this->context_->AttachNeedResponse(this);
}

void ParameterMode_1203_CO2PSBit6Bit7_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x12)
    {
        if (rddata[13] == 0x03)
        {
            if ((rddata[11] & 0x03) == 0x03)
            {
                // not available, yes
                std::cout << "CO2_PS Parameter is not available.\n";
                std::cout << "Show that the parameter is not installed on the sensor module.\n";
            }
            else
            {
                // not available, no
                std::cout << "Change the state of the context ParameterMode_1203_N2OPSBit6Bit7_State.\n";
                auto ptr = std::make_shared<ParameterMode_1203_N2OPSBit6Bit7_State>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
        }
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x12 && rddata[2] == 0x09)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
    }

    PrintData(rddata);
}



void ParameterMode_1203_N2OPSBit6Bit7_State::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    std::cout << "Handles ParameterMode_1203_N2OPSBit6Bit7_State.\n";
    Register();
}

std::vector<uint8_t> ParameterMode_1203_N2OPSBit6Bit7_State::GetCommand()
{
    return std::vector<uint8_t>();
}

size_t ParameterMode_1203_N2OPSBit6Bit7_State::GetRespondBytes()
{
    return size_t();
}

// CommandId=0x12031206
uint32_t ParameterMode_1203_N2OPSBit6Bit7_State::GetCommandId()
{
    return 0x12031206;
}

void ParameterMode_1203_N2OPSBit6Bit7_State::Register()
{
    this->context_->AttachNeedResponse(this);
}

void ParameterMode_1203_N2OPSBit6Bit7_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x12)
    {
        if (rddata[13] == 0x03)
        {
            if ((rddata[12] & 0x03) == 0x03)
            {
                // not available, yes
                std::cout << "N2O_PS Parameter is not available.\n";
                std::cout << "Show that the parameter is not installed on the sensor module.\n";
            }
            else
            {
                // not available, no
                std::cout << "Change the state of the context ParameterMode_1204_O2PSBit6Bit7_State.\n";
                auto ptr = std::make_shared<ParameterMode_1204_O2PSBit6Bit7_State>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
        }
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x12 && rddata[2] == 0x09)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
    }

    PrintData(rddata);
}

void ParameterMode_1204_O2PSBit6Bit7_State::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    std::cout << "Handles ParameterMode_1204_O2PSBit6Bit7_State.\n";
    Register();
}

std::vector<uint8_t> ParameterMode_1204_O2PSBit6Bit7_State::GetCommand()
{
    return std::vector<uint8_t>();
}

size_t ParameterMode_1204_O2PSBit6Bit7_State::GetRespondBytes()
{
    return size_t();
}

// CommandId=0x12041106
uint32_t ParameterMode_1204_O2PSBit6Bit7_State::GetCommandId()
{
    return 0x12041106;
}

void ParameterMode_1204_O2PSBit6Bit7_State::Register()
{
    this->context_->AttachNeedResponse(this);
}

void ParameterMode_1204_O2PSBit6Bit7_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x12)
    {
        if (rddata[13] == 0x04)
        {
            if ((rddata[11] & 0x03) == 0x03)
            {
                // not available, yes
                std::cout << "O2_PS Parameter is not available.\n";
                std::cout << "Show that the parameter is not installed on the sensor module.\n";
            }
            else
            {
                // not available, no
                std::cout << "Change the state of the context ParameterMode_1210_A1PSBit6Bit7_State.\n";
                auto ptr = std::make_shared<ParameterMode_1210_A1PSBit6Bit7_State>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
        }
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x12 && rddata[2] == 0x09)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
    }

    PrintData(rddata);
}

void ParameterMode_1210_A1PSBit6Bit7_State::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    std::cout << "Handles ParameterMode_1210_A1PSBit6Bit7_State.\n";
    Register();
}

std::vector<uint8_t> ParameterMode_1210_A1PSBit6Bit7_State::GetCommand()
{
    return std::vector<uint8_t>();
}

size_t ParameterMode_1210_A1PSBit6Bit7_State::GetRespondBytes()
{
    return size_t();
}

// CommandId=0x12101206
uint32_t ParameterMode_1210_A1PSBit6Bit7_State::GetCommandId()
{
    return 0x12101206;
}

void ParameterMode_1210_A1PSBit6Bit7_State::Register()
{
    this->context_->AttachNeedResponse(this);
}

void ParameterMode_1210_A1PSBit6Bit7_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x12)
    {
        if (rddata[13] == 0x10)
        {
            if ((rddata[12] & 0x03) == 0x03)
            {
                // not available, yes
                std::cout << "A1_PS Parameter is not available.\n";
                std::cout << "Show that the parameter is not installed on the sensor module.\n";
            }
            else
            {
                // not available, no
                std::cout << "Change the state of the context ParameterMode_1203_CO2PSBit6Bit7_State.\n";
                auto ptr = std::make_shared<ParameterMode_1203_N2OPSBit6Bit7_State>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
        }
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x12 && rddata[2] == 0x09)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
    }

    PrintData(rddata);
}

void ParameterMode_1211_A2PSBit6Bit7_State::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    std::cout << "Handles ParameterMode_1211_A2PSBit6Bit7_State.\n";
    Register();
}

std::vector<uint8_t> ParameterMode_1211_A2PSBit6Bit7_State::GetCommand()
{
    return std::vector<uint8_t>();
}

size_t ParameterMode_1211_A2PSBit6Bit7_State::GetRespondBytes()
{
    return size_t();
}

// CommandId=0x12111206
uint32_t ParameterMode_1211_A2PSBit6Bit7_State::GetCommandId()
{
    return 0x12111206;
}

void ParameterMode_1211_A2PSBit6Bit7_State::Register()
{
    this->context_->AttachNeedResponse(this);
}

void ParameterMode_1211_A2PSBit6Bit7_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x12)
    {
        if (rddata[13] == 0x11)
        {
            if ((rddata[12] & 0x03) == 0x03)
            {
                // not available, yes
                std::cout << "A2_PS Parameter is not available.\n";
                std::cout << "Show that the parameter is not installed on the sensor module.\n";
            }
            else
            {
                // not available, no
                std::cout << "Change the state of the context ParameterInopInformation_120E_PII_State.\n";
                auto ptr = std::make_shared<ParameterInopInformation_120E_PII_State>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
        }
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x12 && rddata[2] == 0x09)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
    }

    PrintData(rddata);
}

void ParameterInopInformation_120E_PII_State::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    std::cout << "Handles ParameterInopInformation_120E_PII_State.\n";
    Register();
}

std::vector<uint8_t> ParameterInopInformation_120E_PII_State::GetCommand()
{
    return std::vector<uint8_t>();
}

size_t ParameterInopInformation_120E_PII_State::GetRespondBytes()
{
    return size_t();
}

// CommandId=0x120e0501
// 0x120e05 already exists
uint32_t ParameterInopInformation_120E_PII_State::GetCommandId()
{
    return 0x120e0501;
}

void ParameterInopInformation_120E_PII_State::Register()
{
    this->context_->AttachNeedResponse(this);
}

void ParameterInopInformation_120E_PII_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x12)
    {
        if (rddata[13] == 0x0e)
        {
            if (rddata[6] & 0x1f)
            {
                // yes
                std::cout << "Parameter is not operable.\n";
                std::cout << "It is installed but is has a technical failure. It will probably not cover from its failure.\n";
                std::cout << "Show that the parameter has an INOP condition and needs maintenance activities.\n";
            }
            else
            {
                // no
                std::cout << "Change the state of the context ParameterInopInformation_120E_PII_State.\n";
                auto ptr = std::make_shared<ParameterInopInformation_120E_PII_State>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
        }
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x12 && rddata[2] == 0x09)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
    }

    PrintData(rddata);
}

void MeasurementMode_120E_OMS_State::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    std::cout << "Handles SuperviseZeroRequest_120E_OMS_State.\n";
    Register();
}

std::vector<uint8_t> MeasurementMode_120E_OMS_State::GetCommand()
{
    return std::vector<uint8_t>();
}

size_t MeasurementMode_120E_OMS_State::GetRespondBytes()
{
    return size_t();
}

// CommandId=0x120e1202
uint32_t MeasurementMode_120E_OMS_State::GetCommandId()
{
    return 0x120e1202;
}

void MeasurementMode_120E_OMS_State::Register()
{
    this->context_->AttachNeedResponse(this);
}

void MeasurementMode_120E_OMS_State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x12)
    {
        if (rddata[13] == 0x0e)
        {
            if (rddata[12] == 0x00)
            {
                // yes
                std::cout << "Change the state of the context ZeroInProgress_1203_CO2N2OPSBit5_State. \n";
                auto ptr = std::make_shared<Occlusion_120E_MSBit1__State>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
            else
            {
                // no
                std::cout << "Module is in standby mode.\n";
            }
        }
    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x12 && rddata[2] == 0x09)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
    }
}

void Occlusion_120E_MSBit1__State::HandleData()
{
    if (IsAlreadySent())
    {
        return;
    }
    SetAlreadySent(true);
    std::cout << "Handles Occlusion_120E_MSBit1__State.\n";
    Register();
}

std::vector<uint8_t> Occlusion_120E_MSBit1__State::GetCommand()
{
    return std::vector<uint8_t>();
}
size_t Occlusion_120E_MSBit1__State::GetRespondBytes()
{
    return size_t();
}

// CommandId=0x120e01
uint32_t Occlusion_120E_MSBit1__State::GetCommandId()
{
    return 0x120e01;
}


void Occlusion_120E_MSBit1__State::Register()
{
    this->context_->AttachNeedResponse(this);
}


void Occlusion_120E_MSBit1__State::Update(std::vector<uint8_t> rddata, size_t sz)
{
    if (rddata[0] == 0x06 && rddata[1] == 0x12)
    {
        if (rddata[13] == 0x0e)
        {

            if (rddata[14] & 0x02)
            {
                // yes
                std::cout << "Change the state of the context SuperviseModuleStatus_120B_MSWBit5_State.\n";
                auto ptr = std::make_shared<SuperviseModuleStatus_120B_MSWBit5_State>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }

            else
            {
                auto ptr = std::make_shared<SuperviseModuleStatus_120E_MSBit6_State>();
                this->context_->TransitionTo(ptr->GetCommandId(), ptr);
            }
        }

    }
    else if (rddata[0] == 0x15 && rddata[1] == 0x12 && rddata[2] == 0x09)
    {
        // fail
        std::cout << "Fail with error message: " << GetErrorMessage(rddata[3]) << '\n';
    }

    PrintData(rddata);
}


/**
 * The client code.
 */
void ClientCode() {
    /*Context* context = new Context(new StopContinuousDataState);*/
    auto ptr = std::make_shared<StopContinuousDataState>();
    Context* context = new Context(ptr->GetCommandId(), ptr );
    context->Init();
    while(1)
    {
        context->Request1();
    }

    delete context;
}

int main() {
    ClientCode();
    return 0;
}


