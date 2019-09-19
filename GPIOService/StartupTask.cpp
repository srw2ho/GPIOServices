#include "pch.h"
#include "StartupTask.h"

using namespace GPIOService;

using namespace Platform;
using namespace Windows::ApplicationModel::AppService;
using namespace Windows::ApplicationModel::Background;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace concurrency;

using namespace Windows::Storage::Streams;
using namespace Windows::Networking::Sockets;


using namespace Windows::System::Threading;

using namespace StreamSocketComm;

using namespace GPIODriver;

// The Background Application template is documented at http://go.microsoft.com/fwlink/?LinkID=533884&clcid=0x409
namespace GPIOService
{

	StartupTask::StartupTask() {
		m_pServiceListener = ref new StreamSocketComm::SocketListener();

		m_pServiceListener->OnCreateConnection +=
			ref new TypedEventHandler<StreamSocketListener^, StreamSocketListenerConnectionReceivedEventArgs^>(this, &StartupTask::OnCreateConnection);

		m_pServiceListener->Failed += ref new Windows::Foundation::TypedEventHandler<Platform::Object ^, Platform::Exception ^>(this, &GPIOService::StartupTask::OnFailed);
	
		m_serviceDeferral = nullptr;
		m_pGPIOInOut = nullptr;
		m_connection = nullptr;
		m_timer = nullptr;
		m_pCancelTaskToken = new concurrency::cancellation_token_source();

		m_pGPIOEventPackageQueue = new GPIODriver::GPIOEventPackageQueue();



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

		// Take a deferral so the service isn't terminated
		m_serviceDeferral = taskInstance->GetDeferral();

		auto appServiceTrigger = dynamic_cast<AppServiceTriggerDetails^>(taskInstance->TriggerDetails);

		taskInstance->Canceled += ref new BackgroundTaskCanceledEventHandler(this, &StartupTask::OnTaskCanceled);
	
		m_pGPIOInOut = ref new GPIODriver::GPIOInOut(m_pGPIOEventPackageQueue);
		bool bInit = m_pGPIOInOut->InitGPIO(); // Init

		if (bInit) {


			if (appServiceTrigger != nullptr) {
				if (String::operator==(appServiceTrigger->Name, L"GPIOService"))
				{
					auto details = dynamic_cast<AppServiceTriggerDetails^>(taskInstance->TriggerDetails);

					m_connection = details->AppServiceConnection;

					m_connection->RequestReceived += ref new Windows::Foundation::TypedEventHandler<Windows::ApplicationModel::AppService::AppServiceConnection ^, Windows::ApplicationModel::AppService::AppServiceRequestReceivedEventArgs ^>(this, &GPIOService::StartupTask::OnRequestReceived);

				}

			}


			m_pServiceListener->CreateListenerServerAsync(3005);

			TimeSpan interval;
			interval.Duration = 5 * 10000000L; // 5 sec : Time for Sending actual State

			m_timer = ThreadPoolTimer::CreatePeriodicTimer(ref new TimerElapsedHandler(this, &StartupTask::Timer_Tick), interval);

	//		m_pGPIOInOut->InputStateChanged += ref new Windows::Foundation::TypedEventHandler<Platform::Object ^, Platform::String ^>(this, &GPIOService::StartupTask::OnInputStateChanged);
			
	//		m_pGPIOInOut->OutputStateChanged += ref new Windows::Foundation::TypedEventHandler<Platform::Object ^, Platform::String ^>(this, &GPIOService::StartupTask::OnOutputStateChanged);
			
	//		m_pGPIOInOut->InputHCSR04StateChanged += ref new Windows::Foundation::TypedEventHandler<Platform::Object ^, Platform::String ^>(this, &GPIOService::StartupTask::OnInputHCSR04StateChanged);

			m_pGPIOInOut->CreateBME280External += ref new Windows::Foundation::TypedEventHandler<Platform::Object ^, int>(this, &GPIOService::StartupTask::OnCreateBME280External);
			startProcessingReadValues(); // Starting Processing

		}
		else {
			m_serviceDeferral->Complete();
		}


	}

	void StartupTask::Timer_Tick(ThreadPoolTimer^ timer)
	{

		if (m_cancelRequested == false) {
			// Sind f+r alle Geräte die Initislisierungen beendet?
			//if (m_pBME280WinIoTWrapper->getInitCount() == 0) 
			{
				
				bool bok = true;
				// Send Data to All Connected Clients
				// Get States fro all connected GPIO-Pins
				String^ state = m_pGPIOInOut->GetGPIState();
				if (state->Length() > 0) {
					Windows::Storage::Streams::IBuffer^ buf = SocketHelpers::createPayloadBufferfromSendData(state);
					m_pServiceListener->SendDataToClients(buf);
				}
				if (!bok) m_cancelRequested = true;
			}
		}
		else {

			// Stop Timer
			timer->Cancel();



			stopProcessingReadValues();

			if (m_pGPIOInOut != nullptr) {
				//	m_pGPIOInOut->ResetGPIOPins(); Reset mit Low oder Hight: ist jedesmal anders
				m_pGPIOInOut->deleteAllGPIOPins(); // hier gehen die Ausgänge auf Undefined
				delete m_pGPIOInOut;
				m_pGPIOInOut = nullptr;
			}

			// Indicate that the background task has completed.

			// Cancel Connections

					// Cancel Connections
			if (m_pServiceListener != nullptr)
			{
				m_pServiceListener->CancelConnections();
				m_pServiceListener = nullptr;
			}

		

			if (m_connection != nullptr) {
				delete m_connection;
				m_connection = nullptr;
			}

			if (m_pCancelTaskToken != nullptr) {

				delete m_pCancelTaskToken;
			}
			if (m_pGPIOEventPackageQueue != nullptr) {

				delete m_pGPIOEventPackageQueue;
				m_pGPIOEventPackageQueue = nullptr;
			}

			if (m_serviceDeferral != nullptr)
			{
				// Complete the service deferral
				m_serviceDeferral->Complete();
				m_serviceDeferral = nullptr;
			}

		

			timer = nullptr;
		}




	}
	void StartupTask::StartupTask::OnTaskCanceled(IBackgroundTaskInstance^ sender, BackgroundTaskCancellationReason reason)
	{
		m_cancelRequested = true;

	}


	void StartupTask::OnCreateConnection(_In_ StreamSocketListener^ sender, _In_ StreamSocketListenerConnectionReceivedEventArgs^ e)
	{


		DataReader^ reader = ref new DataReader(e->Socket->InputStream);
		reader->InputStreamOptions = InputStreamOptions::Partial;
		StreamSocket^ socket = e->Socket;


		StreamSocketComm::SocketChunkReceiver* pServiceChunkReceiver = new ServiceChunkReceiver(socket, m_pGPIOInOut);
		SocketChunkReceiverWinRT ^ pBME280ChunkReceiverWinRT = ref new SocketChunkReceiverWinRT(pServiceChunkReceiver); // wrapper for SocketChunkReceiver and its derived
		m_pServiceListener->AddChunkReceiver(pBME280ChunkReceiverWinRT);
		pServiceChunkReceiver->ReceiveChunkLoop(reader, e->Socket); // Do Receive into Loop


	}

	void GPIOService::StartupTask::OnFailed(Platform::Object ^sender, Platform::Exception ^args)
	{// Failed Connection

	}


	void StartupTask::OnRequestReceived(AppServiceConnection^ sender, AppServiceRequestReceivedEventArgs^ args)
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
						if (args->Request->Message->HasKey(L"GPIO.Command")) {
							auto cmdObject = args->Request->Message->Lookup(L"GPIO.Command");
							if (cmdObject != nullptr) {
								String^ command = safe_cast<String^>(cmdObject);
								if (command != L"") {
								
									m_pGPIOInOut->DoProcessCommand(command);
									String^ state = m_pGPIOInOut->GetGPIState();
									responseMessage->Insert(L"GPIO.State", state);
								}


							}
						};

						if (args->Request->Message->HasKey(L"GPIO.State")) {
							auto cmdObject = args->Request->Message->Lookup(L"GPIO.State");
							if (cmdObject != nullptr) {
								String^ command = safe_cast<String^>(cmdObject);
								if (command != L"") {

									m_pGPIOInOut->DoProcessCommand(command);
									String^ state = m_pGPIOInOut->GetGPIState();
									responseMessage->Insert(L"GPIO.State", state);
								}


							}
						};


						if (args->Request->Message->HasKey(L"GPIO.Initialize")) {
							auto cmdObject = args->Request->Message->Lookup(L"GPIO.Initialize");
							if (cmdObject != nullptr) {
								int nInitialize = safe_cast<int>(cmdObject);
								if (nInitialize>=0) {
									bool bInit = m_pGPIOInOut->InitGPIO(); // Init
									if (bInit) {

									}
								}
			

							}
						};
						if (args->Request->Message->HasKey(L"GPIO.ResetOut")) {
							auto cmdObject = args->Request->Message->Lookup(L"GPIO.ResetOut");
							if (cmdObject != nullptr) {
								int nInitialize = safe_cast<int>(cmdObject);
								if (nInitialize>=0) 
								{
									bool bInit = m_pGPIOInOut->ResetGPIOPins();
										 // Init
									if (bInit) {

									}
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



Concurrency::task<void> StartupTask::doProcessReadValues()
{
	auto token = m_pCancelTaskToken->get_token();
	auto tsk = create_task([this, token]() -> void
	{
		bool dowhile = true;
		DWORD waitTime = 500; // 500 msec warten
		while (dowhile) {
			try {
				GPIOEventPackage* pPacket = nullptr;
				m_pGPIOEventPackageQueue->waitForPacket(&pPacket, waitTime);
				if (pPacket != nullptr)
				{
					Platform::String ^ message = pPacket->getEventMsg();

					if (message->Length() > 0) {
						// Sending to all connected clients
						Windows::Storage::Streams::IBuffer^ buf = SocketHelpers::createPayloadBufferfromSendData(message);
						m_pServiceListener->SendDataToClients(buf);
					}

					delete pPacket;
				}

				if (token.is_canceled()) {
					cancel_current_task();
				}

			}
			catch (task_canceled&)
			{
				dowhile = false;

			}
			catch (Exception^ exception	)
			{
				dowhile = false;
			}

		}



	}, token);

	return tsk;
}



void StartupTask::startProcessingReadValues()
{

	m_ProcessingReadValuesTsk = create_task(doProcessReadValues()).then([this](task<void> previous)
	{

		try {
			previous.get();
		}
		catch (Exception^ exception)
		{

		}

	});


}


void StartupTask::stopProcessingReadValues()
{

	try {

		m_pGPIOEventPackageQueue->cancelwaitForPacket();
		if (m_pCancelTaskToken != nullptr) {
			m_pCancelTaskToken->cancel();
		}
//		Sleep(100);

		m_ProcessingReadValuesTsk.wait(); // wait for ended
	
	}
	catch (Exception^ exception)
	{
		bool b = true;

	}


}



void GPIOService::StartupTask::OnCreateBME280External(Platform::Object ^sender, int args)
{
	GPIOService::BME280SensorExternal* pBME280IoTDriverWrapper = new  GPIOService::BME280SensorExternal(m_pGPIOEventPackageQueue,args,0);
	m_pGPIOInOut->addGPIOPin(pBME280IoTDriverWrapper);

	auto tsk = pBME280IoTDriverWrapper->InitDriver(args);
	
	
	create_task(tsk).then([this, pBME280IoTDriverWrapper, args](task<bool> value)->void {

		try {
			bool ok = value.get();
			if (ok)
			{

			}
			else
			{

			}

		}

		catch (task_canceled const &) // cancel by User
		{
			bool bcancel = true;
		}
		catch (Exception^ exception)
		{

		}
	}).wait();
	
	bool bOK = true;

}
}







