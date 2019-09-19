#pragma once

#include "pch.h"

#include "GPIOInOut.h"
#include "ServiceChunkReceiver.h"
#include "GPIOEventPackageQueue.h"

using namespace SSD1306Display;

namespace GPIOServiceClient

{

	[Windows::Foundation::Metadata::WebHostHidden]

	public ref class StartupTask sealed : public Windows::ApplicationModel::Background::IBackgroundTask

	{
	private:
		volatile bool m_cancelRequested = false;
		bool m_toggle=false;
		GPIODriver::GPIOInOut^ m_GPIOClientInOut;

		GPIODriver::GPIOPWMOutputPin* m_PWMOutputPin;

		StreamSocketComm::SocketListener ^m_pServiceListener;
		Platform::String^ m_RemoteComputer;

		concurrency::cancellation_token_source* m_pCancelTaskToken;
		concurrency::task<void> m_ProcessingReadValuesTsk;

		bool m_bIsConnected;

		bool m_bDisplayInit;
		int m_DisplayAdress;
		GPIODriver::GPIOEventPackageQueue* m_pGPIOEventPackageQueue;
		SSD1306Display::DisplayManager ^ m_DisplayManager;
		double m_Temperature;
		double m_DistanceInZentimeter;
		double m_HC_SR04MeasTime;

	public:

		StartupTask();
		virtual void Run(Windows::ApplicationModel::Background::IBackgroundTaskInstance^ taskInstance);

	
	private:

		void OnCanceled(Windows::ApplicationModel::Background::IBackgroundTaskInstance ^sender, Windows::ApplicationModel::Background::BackgroundTaskCancellationReason reason);

		Platform::Agile<Windows::ApplicationModel::Background::BackgroundTaskDeferral> m_serviceDeferral;

		Windows::ApplicationModel::AppService::AppServiceConnection ^m_serviceConnection;

		Windows::System::Threading::ThreadPoolTimer ^timer;

		Platform::String ^command;


		void DoProcessingData();
	//	void InitProcessingData();

		void Timer_Tick(Windows::System::Threading::ThreadPoolTimer^ timer);
		void OnOnClientConnected(Windows::Networking::Sockets::StreamSocket ^sender, int args);

		void OnFailed(Platform::Object ^sender, Platform::Exception ^args);
		void OnstopStreaming(Platform::Object ^sender, Platform::Exception ^args);

		Concurrency::task<void> doProcessReadValues();
		void startProcessingReadValues();
		void stopProcessingReadValues();

		void ProcessGPIOState(GPIODriver::GPIOPin* pGPIOPin);

		concurrency::task<bool> InitDisplayDriver(unsigned char adress);
		void DoDisplayInfo(int Adress);

	//	void OnInputStateChanged(Platform::Object ^sender, int args);
	//	void OnOutputStateChanged(Platform::Object ^sender, int args);

	//	void OnHCSR04StateChanged(Platform::Object ^sender, int args);
	};

}