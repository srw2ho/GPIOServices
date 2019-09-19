#pragma once

#include "pch.h"

namespace GPIOShutDownMangaging
{
    [Windows::Foundation::Metadata::WebHostHidden]
    public ref class StartupTask sealed : public Windows::ApplicationModel::Background::IBackgroundTask
    {
		Platform::Agile<Windows::ApplicationModel::Background::BackgroundTaskDeferral^> m_serviceDeferral;

		Windows::Devices::Gpio::GpioPin^ m_pin;
		UINT64 m_StartEventTime;
		bool m_ActiveLow;

    public:
        virtual void Run(Windows::ApplicationModel::Background::IBackgroundTaskInstance^ taskInstance);
		StartupTask();
		void OnValueChanged(UINT64 TimeinYsec, Windows::Devices::Gpio::GpioPinValueChangedEventArgs ^args);
	private:
		void OnTaskCanceled(Windows::ApplicationModel::Background::IBackgroundTaskInstance^ sender, Windows::ApplicationModel::Background::BackgroundTaskCancellationReason reason);

		static void OnValueInputChanged(Windows::Devices::Gpio::GpioPin ^sender, Windows::Devices::Gpio::GpioPinValueChangedEventArgs ^args);

	

    };
}
