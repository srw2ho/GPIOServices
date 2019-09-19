#include "pch.h"
#include "TimeConversion.h"
#include "StartupTask.h"

using namespace GPIOShutDownMangaging;

using namespace Platform;
using namespace Windows::ApplicationModel::Background;

using namespace Windows::Devices;


using namespace Windows::Devices::Gpio;

using namespace Microsoft::IoT::Lightning::Providers;

using namespace  Windows::System;


// The Background Application template is documented at http://go.microsoft.com/fwlink/?LinkID=533884&clcid=0x409





const UINT64 nano100SecInSec = (UINT64)10000000 * 1;

StartupTask^ glob_StartupTask = nullptr;

StartupTask::StartupTask() {

	m_serviceDeferral == nullptr;
	m_pin = nullptr;
	m_StartEventTime = 0;
	m_ActiveLow = false;

}

void StartupTask::Run(IBackgroundTaskInstance^ taskInstance)
{
    // 
    // TODO: Insert code to perform background work
    //
    // If you start any asynchronous methods here, prevent the task
    // from closing prematurely by using BackgroundTaskDeferral as
    // described in http://aka.ms/backgroundtaskdeferral
    //

	m_serviceDeferral = taskInstance->GetDeferral();
	taskInstance->Canceled += ref new BackgroundTaskCanceledEventHandler(this, &StartupTask::OnTaskCanceled);

	

	//	if (LightningProvider::IsLightningEnabled)
	if (Microsoft::IoT::Lightning::Providers::LightningProvider::IsLightningEnabled)
	{
		Windows::Devices::LowLevelDevicesController::DefaultProvider = LightningProvider::GetAggregateProvider();

	}
	Windows::Devices::Gpio::GpioController ^ GPIOController  = GpioController::GetDefault();;


	int pinNo = 24;


	GpioOpenStatus  status;
	GPIOController->TryOpenPin(pinNo, GpioSharingMode::Exclusive, &m_pin, &status);
	if (m_pin != nullptr) {
		if (status == GpioOpenStatus::PinOpened)
		{
	
			if (m_pin->IsDriveModeSupported(GpioPinDriveMode::Input)) {
				m_pin->SetDriveMode(GpioPinDriveMode::Input);

		
				Windows::Foundation::TimeSpan interval;

				long Msec = nano100SecInSec / 1000; // 1Sec / 1000 = 1msec
				interval.Duration = 50 * Msec; // 50 msec: Entprellzeit
				interval.Duration = interval.Duration;
				m_pin->DebounceTimeout = interval;

				m_pin->ValueChanged += ref new Windows::Foundation::TypedEventHandler<Windows::Devices::Gpio::GpioPin ^, Windows::Devices::Gpio::GpioPinValueChangedEventArgs ^>(&GPIOShutDownMangaging::StartupTask::OnValueInputChanged);

				int value = (m_pin->Read() == Windows::Devices::Gpio::GpioPinValue::High) ? 1 : 0;
				glob_StartupTask = this;
		
				if (value == 1) {
					m_ActiveLow = true;
				}
				else
				{
					m_ActiveLow = false;
				}
			}
		}
	}

}

void GPIOShutDownMangaging::StartupTask::OnValueChanged(UINT64 TimeinYsec, Windows::Devices::Gpio::GpioPinValueChangedEventArgs ^args)
{
	bool RisingEdge;
	bool doStartTime = false;
	bool doEndTime = false;

	if (args->Edge == GpioPinEdge::RisingEdge) { RisingEdge = true; }
	else if (args->Edge == GpioPinEdge::FallingEdge) { RisingEdge = false; }

	if (m_ActiveLow && RisingEdge) {
		doEndTime = true;
	}
	else if (m_ActiveLow && !RisingEdge) {
		doStartTime = true;
	}

	else if (!m_ActiveLow && RisingEdge) {
		doStartTime = true;

	}
	else if (!m_ActiveLow && !RisingEdge) {
		doEndTime = true;
	}

	if (doStartTime) {
		m_StartEventTime = ConversionHelpers::TimeConversion::getActualUnixTime();
	}
	else if (doEndTime) {
		if (m_StartEventTime!=0){
		
			UINT64 deltaTime = TimeinYsec - m_StartEventTime;
			Windows::Foundation::TimeSpan TimeSpan;
			TimeSpan.Duration = 0;

			if ((deltaTime > 3 * nano100SecInSec) && (deltaTime < 7 * nano100SecInSec))
			{//  Zeit von 3 Sek bis 7 Sek. -> neu Starten

				ShutdownManager::BeginShutdown(ShutdownKind::Restart, TimeSpan);
			}
			else if ((deltaTime >= 7 * nano100SecInSec))
			{ // Ab 7 Sekunden herunterfahren
				ShutdownManager::BeginShutdown(ShutdownKind::Shutdown, TimeSpan);
			}
		}
		m_StartEventTime = 0;
	}



}


void GPIOShutDownMangaging::StartupTask::OnValueInputChanged(Windows::Devices::Gpio::GpioPin ^sender, Windows::Devices::Gpio::GpioPinValueChangedEventArgs ^args)
{
	UINT64 actTime = ConversionHelpers::TimeConversion::getActualUnixTime();

	glob_StartupTask->OnValueChanged(actTime, args);

	/*
	bool RisingEdge;
	bool doStartTime = false;
	bool doEndTime = false;

	if (args->Edge == GpioPinEdge::RisingEdge) {RisingEdge = true;}
	else if (args->Edge == GpioPinEdge::FallingEdge) {RisingEdge = false;}
	
	if (glob_ActiveLow && RisingEdge) {
		doEndTime = true;
	}
	else if (glob_ActiveLow && !RisingEdge) {
		doStartTime = true;
	}

	else if (!glob_ActiveLow && RisingEdge) {
		doStartTime = true;

	}
	else if (!glob_ActiveLow && !RisingEdge) {
		doEndTime = true;
	}

	if (doStartTime) {
		glob_StartEventTime = ConversionHelpers::TimeConversion::getActualUnixTime();
	}
	else if (doEndTime) {
		UINT64 actTime = ConversionHelpers::TimeConversion::getActualUnixTime();

		UINT64 deltaTime = actTime - glob_StartEventTime;
		Windows::Foundation::TimeSpan TimeSpan;
		TimeSpan.Duration = 0;

		if ((deltaTime > 3 * nano100SecInSec) && (deltaTime < 7 * nano100SecInSec))
		{//  Zeit von 3 Sek bis 7 Sek. -> neu Starten

			ShutdownManager::BeginShutdown(ShutdownKind::Restart, TimeSpan);
		}
		else if ((deltaTime >= 7 * nano100SecInSec))
		{ // Ab 7 Sekunden herunterfahren
			ShutdownManager::BeginShutdown(ShutdownKind::Shutdown, TimeSpan);
		}
		glob_StartEventTime = actTime;
	}
	*/

}



void GPIOShutDownMangaging::StartupTask::OnTaskCanceled(IBackgroundTaskInstance^ sender, BackgroundTaskCancellationReason reason)
{
	if (m_serviceDeferral != nullptr)
	{
		// Complete the service deferral
		m_serviceDeferral->Complete();
		m_serviceDeferral = nullptr;
	}

	if (m_pin != nullptr) {
		delete m_pin;
		m_pin = nullptr;
	}
}
