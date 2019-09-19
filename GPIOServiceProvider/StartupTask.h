#pragma once

#include "pch.h"

namespace GPIOServiceProvider
{
    [Windows::Foundation::Metadata::WebHostHidden]
    public ref class StartupTask sealed : public Windows::ApplicationModel::Background::IBackgroundTask
    {
	private:
		Platform::Agile<Windows::ApplicationModel::Background::BackgroundTaskDeferral> m_serviceDeferral;
		Windows::ApplicationModel::AppService::AppServiceConnection ^m_serviceConnection;
		GPIOServiceListener::GPIOListener ^ m_pGPIOListener;
		GPIODriver::GPIOClientInOut ^ m_pGPIOClientInOut;
		Windows::System::Threading::ThreadPoolTimer ^m_timer;
		volatile bool m_cancelRequested = false;
    public:
		StartupTask();
        virtual void Run(Windows::ApplicationModel::Background::IBackgroundTaskInstance^ taskInstance);

	private:

		void OnCanceled(Windows::ApplicationModel::Background::IBackgroundTaskInstance ^sender, Windows::ApplicationModel::Background::BackgroundTaskCancellationReason reason);
		void OnstartStreaming(Platform::Object ^sender, Windows::Networking::Sockets::StreamSocket ^args);
		void OnstopStreaming(Platform::Object ^sender, int args);
		void OnRequestReceived(Windows::ApplicationModel::AppService::AppServiceConnection ^sender, Windows::ApplicationModel::AppService::AppServiceRequestReceivedEventArgs ^args);
		void Timer_Tick(Windows::System::Threading::ThreadPoolTimer^ timer);

    };
}
