#pragma once

#include "pch.h"

#include "GPIOInOut.h"
#include "ServiceChunkReceiver.h"
#include "GPIOEventPackageQueue.h"
#include "BME280SensorExternal.h"



namespace GPIOService
{



    [Windows::Foundation::Metadata::WebHostHidden]
    public ref class StartupTask sealed : public Windows::ApplicationModel::Background::IBackgroundTask
    {
	private:
		Platform::Agile<Windows::ApplicationModel::Background::BackgroundTaskDeferral^> m_serviceDeferral;
	//	Windows::ApplicationModel::AppService::AppServiceConnection^ m_connection;
		GPIODriver::GPIOInOut^ m_pGPIOInOut;

		StreamSocketComm::SocketListener ^m_pServiceListener;

		Windows::System::Threading::ThreadPoolTimer ^m_timer;
		volatile bool m_cancelRequested = false;

		concurrency::cancellation_token_source* m_pCancelTaskToken;
		concurrency::task<void> m_ProcessingReadValuesTsk;

		GPIODriver::GPIOEventPackageQueue* m_pGPIOEventPackageQueue;


    public:
        virtual void Run(Windows::ApplicationModel::Background::IBackgroundTaskInstance^ taskInstance);
		StartupTask();
	private:

		void OnCreateConnection(
			_In_ Windows::Networking::Sockets::StreamSocketListener^ sender,
			_In_ Windows::Networking::Sockets::StreamSocketListenerConnectionReceivedEventArgs^ e
		);


		void OnTaskCanceled(Windows::ApplicationModel::Background::IBackgroundTaskInstance^ sender, Windows::ApplicationModel::Background::BackgroundTaskCancellationReason reason);
		void OnRequestReceived(Windows::ApplicationModel::AppService::AppServiceConnection ^sender, Windows::ApplicationModel::AppService::AppServiceRequestReceivedEventArgs ^args);


		void OnstopStreaming(Platform::Object ^sender, int args);
		void OnstartStreaming(Platform::Object ^sender, Windows::Networking::Sockets::StreamSocket ^args);

		void Timer_Tick(Windows::System::Threading::ThreadPoolTimer^ timer);
	//	void OnInputStateChanged(Platform::Object ^sender, Platform::String ^  args);
	//	void OnOutputStateChanged(Platform::Object ^sender, Platform::String ^args);
	//	void OnInputHCSR04StateChanged(Platform::Object ^sender, Platform::String ^args);


		Concurrency::task<void> doProcessReadValues();
		void startProcessingReadValues();
		void stopProcessingReadValues();
		void OnCreateBME280External(Platform::Object ^sender, int args);
		void OnFailed(Platform::Object ^sender, Platform::Exception ^args);
	};
}
