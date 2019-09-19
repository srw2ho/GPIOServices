#include "..\GPIOService\StartupTask.h"
#include "..\GPIOService\StartupTask.h"
#include "..\GPIOService\StartupTask.h"
#include "..\GPIOService\StartupTask.h"
#include "pch.h"
#include "StartupTask.h"

using namespace GPIOServiceProvider;


using namespace Platform;

using namespace Windows::ApplicationModel::AppService;

using namespace Windows::ApplicationModel::Background;

using namespace Windows::Devices::Gpio;

using namespace Windows::Foundation;

using namespace Windows::Foundation::Collections;

using namespace Windows::System::Threading;

using namespace Platform;

using namespace Windows::ApplicationModel::Background;

using namespace concurrency;

// The Background Application template is documented at http://go.microsoft.com/fwlink/?LinkID=533884&clcid=0x409



StartupTask::StartupTask()
{

	m_serviceConnection = nullptr;
	m_pGPIOListener = nullptr;
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



	m_serviceConnection = ref new AppServiceConnection();

	m_serviceConnection->PackageFamilyName = L"GPIOService-uwp_cctb7d6fhpv4a";

	m_serviceConnection->AppServiceName = L"GPIOService";

	taskInstance->Canceled += ref new Windows::ApplicationModel::Background::BackgroundTaskCanceledEventHandler(this, &GPIOServiceProvider::StartupTask::OnCanceled);


	auto connectTask = Concurrency::create_task(m_serviceConnection->OpenAsync());

	connectTask.then([this](AppServiceConnectionStatus connectStatus)

	{

		if (AppServiceConnectionStatus::Success == connectStatus)

		{

			auto message = ref new ValueSet();

			int initialize = 0;
			message->Insert(L"GPIO.Initialize", initialize);

	
			auto messageTask = Concurrency::create_task(m_serviceConnection->SendMessageAsync(message));


			messageTask.then([this](AppServiceResponse ^appServiceResponse)

			{

				if ((appServiceResponse->Status == AppServiceResponseStatus::Success) &&

					(appServiceResponse->Message != nullptr) &&

					(appServiceResponse->Message->HasKey(L"Response")))

				{

					auto response = appServiceResponse->Message->Lookup(L"Response");

					String^ responseMessage = safe_cast<String^>(response);

					if (String::operator==(responseMessage, L"Success"))
					{
						m_pGPIOClientInOut = ref new  GPIODriver::GPIOClientInOut();
						m_pGPIOListener = ref new GPIOServiceListener::GPIOListener(m_pGPIOClientInOut, m_serviceConnection);
						m_pGPIOListener->StartServerAsync(8003);

						m_pGPIOListener->startStreaming += ref new Windows::Foundation::TypedEventHandler<Platform::Object ^, Windows::Networking::Sockets::StreamSocket ^>(this, &StartupTask::OnstartStreaming);

						m_pGPIOListener->stopStreaming += ref new Windows::Foundation::TypedEventHandler<Platform::Object ^, int>(this, &StartupTask::OnstopStreaming);

						//m_serviceConnection->RequestReceived += ref new Windows::Foundation::TypedEventHandler<Windows::ApplicationModel::AppService::AppServiceConnection ^, Windows::ApplicationModel::AppService::AppServiceRequestReceivedEventArgs ^>(this, &GPIOServiceProvider::StartupTask::OnRequestReceived);

						/*
						TimeSpan interval;
						interval.Duration = 500 * 1000 * 10;
						m_cancelRequested = false;
						m_timer = ThreadPoolTimer::CreatePeriodicTimer(ref new TimerElapsedHandler(this, &StartupTask::Timer_Tick), interval);
						*/

					}

				}

			});

		}

		else

		{

			m_serviceDeferral->Complete();

		}

	});

}
void GPIOServiceProvider::StartupTask::OnCanceled(Windows::ApplicationModel::Background::IBackgroundTaskInstance ^sender, Windows::ApplicationModel::Background::BackgroundTaskCancellationReason reason)
{
	m_cancelRequested = true;

	if (m_pGPIOListener != nullptr) {

		m_pGPIOListener->CloseHTTPServerAsync();
		delete m_pGPIOListener;
	}

	if (m_pGPIOClientInOut != nullptr) {

		m_pGPIOClientInOut->deleteAllGPIOPins();

		delete m_pGPIOClientInOut;
	}




	if (m_serviceDeferral != nullptr)
	{
		m_serviceDeferral->Complete();
	}

	if (m_serviceConnection != nullptr) { // close connection
		delete m_serviceConnection;
		m_serviceConnection = nullptr;
	}
}

void StartupTask::Timer_Tick(ThreadPoolTimer^ timer)
{

	if (m_cancelRequested == false) {

		// Request Status from GPIOService
	}
	else {

		timer->Cancel();

		//

		// Indicate that the background task has completed.

		//
		BackgroundTaskDeferral^ deferral = m_serviceDeferral.Get();
		deferral->Complete();

		//	serviceDeferral->Complete();
		if (m_serviceConnection != nullptr)
		{
			m_serviceConnection = nullptr;
		}


	}




}



void StartupTask::OnstartStreaming(Platform::Object ^sender, Windows::Networking::Sockets::StreamSocket ^args)
{

}


void StartupTask::OnstopStreaming(Platform::Object ^sender, int args)
{
	if (m_pGPIOClientInOut != nullptr) {
		m_pGPIOClientInOut->deleteAllGPIOPins();

	}

}


void GPIOServiceProvider::StartupTask::OnRequestReceived(Windows::ApplicationModel::AppService::AppServiceConnection ^sender, Windows::ApplicationModel::AppService::AppServiceRequestReceivedEventArgs ^args)
{
	// Get a deferral so we can use an awaitable API to respond to the message
	auto messageDeferral = args->GetDeferral();

	auto input = args->Request->Message;
	if (args->Request->Message != nullptr)
	{

		auto responseMessage = ref new ValueSet();

		if (args->Request->Message != nullptr)
		{
			{

				try

				{
					

					if (args->Request->Message->HasKey(L"GPIO.State")) {
						auto cmdObject = args->Request->Message->Lookup(L"GPIO.State");
						if (cmdObject != nullptr) {
							String^ command = safe_cast<String^>(cmdObject);
							if (command != L"") {

								m_pGPIOClientInOut->DoProcessCommand(command);
							
							

							}


						}
					};


					responseMessage->Insert(L"Response", L"Success");


				}

				catch (InvalidCastException^ ic)

				{

					responseMessage->Insert(L"Response", L"Request Failed:Invalid cast exception occurred.");

				}

				catch (Exception^ e)

				{

					responseMessage->Insert(L"Response", L"Request Failed:Unknown exception occurred.");

				}

			}



		}

		else

		{

			responseMessage->Insert(L"Response", L"Failed: Request message is empty.");

		}

		// Send the response asynchronously
		create_task(args->Request->SendResponseAsync(responseMessage)).then([messageDeferral](AppServiceResponseStatus status)
		{
			messageDeferral->Complete();
		});
	}
	else {
		messageDeferral->Complete();
	}

}


