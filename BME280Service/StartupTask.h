#pragma once

#include "pch.h"
#include "BME280WinIoTWrapper.h"



namespace BME280Service
{
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class StartupTask sealed : public Windows::ApplicationModel::Background::IBackgroundTask
	{
		BME280Service::BME280WinIoTWrapper* m_pBME280WinIoTWrapper;
		BME280Service::BME280Listener ^m_pBME280Listener;
		Platform::Agile<Windows::ApplicationModel::Background::BackgroundTaskDeferral^> m_serviceDeferral;

	public:
		virtual void Run(Windows::ApplicationModel::Background::IBackgroundTaskInstance^ taskInstance);
		StartupTask();


	protected:
		void OnTaskCanceled(Windows::ApplicationModel::Background::IBackgroundTaskInstance^ sender, Windows::ApplicationModel::Background::BackgroundTaskCancellationReason reason);

	};
}