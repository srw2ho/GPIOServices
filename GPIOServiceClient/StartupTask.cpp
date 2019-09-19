#include "pch.h"
#include "StartupTask.h"

using namespace GPIOServiceClient;



using namespace GPIODriver;

using namespace GPIOServiceClient;

using namespace Platform;

using namespace Windows::ApplicationModel::Background;

using namespace Windows::ApplicationModel::AppService;

using namespace Windows::Foundation;

using namespace Windows::Foundation::Collections;

using namespace Windows::System::Threading;

using namespace Windows::Devices::Gpio;

using namespace Windows::Devices::Pwm;

using namespace StreamSocketComm;

using namespace Windows::Storage::Streams;
using namespace Windows::Networking::Sockets;

using namespace concurrency;


const UINT64 nano100SecInSec = (UINT64)10000000 * 1;

// The Background Application template is documented at http://go.microsoft.com/fwlink/?LinkID=533884&clcid=0x409
namespace GPIOServiceClient {




StartupTask::StartupTask()
{

	m_pServiceListener = ref new StreamSocketComm::SocketListener();
	m_RemoteComputer = "localhost";
	m_serviceConnection = nullptr;
	m_serviceDeferral = nullptr;
	m_bIsConnected = false;


	m_pCancelTaskToken = new concurrency::cancellation_token_source();

	m_pGPIOEventPackageQueue = new GPIODriver::GPIOEventPackageQueue();

	m_GPIOClientInOut = ref new GPIODriver::GPIOInOut(m_pGPIOEventPackageQueue);

	



	m_DisplayManager = ref new SSD1306Display::DisplayManager;


	m_bDisplayInit = false;
	m_DisplayAdress = (int)SSD1306Display::SSD1306_Addresses::SSD1306_Primaer;
	m_Temperature = -1;
	m_DistanceInZentimeter = -1;
	m_HC_SR04MeasTime = -1;

}


void StartupTask::Run(IBackgroundTaskInstance^ taskInstance)

{


	m_serviceDeferral = taskInstance->GetDeferral();


	taskInstance->Canceled += ref new Windows::ApplicationModel::Background::BackgroundTaskCanceledEventHandler(this, &GPIOServiceClient::StartupTask::OnCanceled);

	// Init Input Pins
	GPIOInputPin * pInput = new GPIOInputPin(m_pGPIOEventPackageQueue, 5, 0);
	pInput->setActivateOutputProcessing(true);
	m_GPIOClientInOut->addGPIOPin(pInput);
	pInput = new GPIOInputPin(m_pGPIOEventPackageQueue, 6, 0);
	pInput->setActivateOutputProcessing(true);
	m_GPIOClientInOut->addGPIOPin(pInput);
	pInput = new GPIOInputPin(m_pGPIOEventPackageQueue, 16, 0);
	pInput->setActivateOutputProcessing(true);
	m_GPIOClientInOut->addGPIOPin(pInput);
	pInput = new GPIOInputPin(m_pGPIOEventPackageQueue, 26, 0); // defekt
	pInput->setActivateOutputProcessing(true);
	m_GPIOClientInOut->addGPIOPin(pInput);
	pInput = new GPIOInputPin(m_pGPIOEventPackageQueue, 25, 0);//defekt
	pInput->setActivateOutputProcessing(true);
	m_GPIOClientInOut->addGPIOPin(pInput);
	pInput = new GPIOInputPin(m_pGPIOEventPackageQueue, 24, 0);
	pInput->setActivateOutputProcessing(true);
	m_GPIOClientInOut->addGPIOPin(pInput);


	m_PWMOutputPin = new GPIOPWMOutputPin(m_pGPIOEventPackageQueue, 22, 1); // LED Output GPIO21, InitValue = 1
	m_GPIOClientInOut->addGPIOPin(m_PWMOutputPin);

	m_PWMOutputPin->setActivateOutputProcessing(true);
	m_PWMOutputPin->setFrequency(50); // 40-1000Hz sind nur möglich
	m_PWMOutputPin->setSetValue(0.50);

// Output Input Pins

	GPIOOutputPin * pOutput = new GPIOOutputPin(m_pGPIOEventPackageQueue, 17, 1);
	pOutput->setActivateOutputProcessing(true);
	m_GPIOClientInOut->addGPIOPin(pOutput);

	pOutput = new GPIOOutputPin(m_pGPIOEventPackageQueue, 27, 1);
	pOutput->setActivateOutputProcessing(true);
	m_GPIOClientInOut->addGPIOPin(pOutput);
	/*
	pOutput = new GPIOOutputPin(m_pGPIOEventPackageQueue, 23, 1);
	pOutput->setActivateOutputProcessing(true);
	m_GPIOClientInOut->addGPIOPin(pOutput);

	pOutput = new GPIOOutputPin(m_pGPIOEventPackageQueue, 22, 1);
	pOutput->setActivateOutputProcessing(true);
//	pOutput->setPulseTimeinms(200);
	m_GPIOClientInOut->addGPIOPin(pOutput);

		*/

	pOutput = new GPIOOutputPin(m_pGPIOEventPackageQueue, 21, 1);
	pOutput->setActivateOutputProcessing(true);
	m_GPIOClientInOut->addGPIOPin(pOutput);


	// Init Input HC_SR04

	GPIOHCSR04* pGPIOHCSR04 = new GPIOHCSR04(m_pGPIOEventPackageQueue, 12, 23, 0); // Echo-Eingang = 12, Trigger = 20
	pGPIOHCSR04->setActivateOutputProcessing(true);
	m_GPIOClientInOut->addGPIOPin(pGPIOHCSR04);

	
	BME280Sensor* pBME280Sensor = new BME280Sensor(m_pGPIOEventPackageQueue, 0x76,0); // BME280_I2C_ADDR_PRIM
	//pBME280Sensor->setActivateOutputProcessing(true);
	pBME280Sensor->setActivateOutputProcessing(false);
	m_GPIOClientInOut->addGPIOPin(pBME280Sensor);
	

	m_pServiceListener->OnClientConnected += ref new Windows::Foundation::TypedEventHandler<Windows::Networking::Sockets::StreamSocket ^, int>(this, &GPIOServiceClient::StartupTask::OnOnClientConnected);

	m_pServiceListener->Failed += ref new Windows::Foundation::TypedEventHandler<Platform::Object ^, Platform::Exception ^>(this, &GPIOServiceClient::StartupTask::OnFailed);

	m_pServiceListener->stopStreaming += ref new Windows::Foundation::TypedEventHandler<Platform::Object ^, Platform::Exception ^>(this, &GPIOServiceClient::StartupTask::OnstopStreaming);

	auto tsk = InitDisplayDriver(m_DisplayAdress); // Initialisierung Display Async vor Kommunikation
	tsk.wait();


	m_pServiceListener->StartClientAsync(m_RemoteComputer, 3005);






	TimeSpan interval;

//	interval.Duration = 0.5 * 10000000L; // 0.5 sec PWM
	interval.Duration = 1 * 10000000L; // 1 sec PWM

	m_cancelRequested = false;
	timer = ThreadPoolTimer::CreatePeriodicTimer(ref new TimerElapsedHandler(this, &StartupTask::Timer_Tick), interval);


	startProcessingReadValues(); // Starting Processing


}

void StartupTask::Timer_Tick(ThreadPoolTimer^ timer)

{

	if (m_cancelRequested == false) {

		if (m_bIsConnected)
		{
			DoProcessingData();
		}

	}

	else {

		timer->Cancel();


		stopProcessingReadValues();


		m_GPIOClientInOut->deleteAllGPIOPins();

		if (m_serviceConnection != nullptr)
		{
			m_serviceConnection = nullptr;
		}
		
		if (m_pServiceListener != nullptr) {

			m_pServiceListener->CancelConnections();

			delete m_pServiceListener;
			m_pServiceListener = nullptr;
		}

		if (m_pCancelTaskToken != nullptr) {

			delete m_pCancelTaskToken;
		}
		if (m_pGPIOEventPackageQueue != nullptr) {

			delete m_pGPIOEventPackageQueue;
			m_pGPIOEventPackageQueue = nullptr;
		}

		if (m_DisplayManager != nullptr) {
			//	m_DisplayManager->Dispose();
			delete m_DisplayManager;

		}


		if (m_serviceDeferral != nullptr) {
			m_serviceDeferral->Complete();
			m_serviceDeferral = NULL;
		}

		timer = nullptr;

	}


	

}




void StartupTask::OnOnClientConnected(Windows::Networking::Sockets::StreamSocket ^sender, int args)
{// Client connection created --> Add Connection to Listener
	//throw ref new Platform::NotImplementedException();

	StreamSocket^ socket = sender;

	StreamSocketComm::SocketChunkReceiver* pServiceListener = new ServiceChunkReceiver(socket, this->m_GPIOClientInOut);

	SocketChunkReceiverWinRT ^ pBME280ChunkReceiverWinRT = ref new SocketChunkReceiverWinRT(pServiceListener); // wrapper for SocketChunkReceiver and its derived
	m_pServiceListener->AddChunkReceiver(pBME280ChunkReceiverWinRT);
	pBME280ChunkReceiverWinRT->geSocketChunkReceiver()->StartService(); // send "BME280Server.Start" - Copmmand to Client Station-> Start with Communication



}









void GPIOServiceClient::StartupTask::OnCanceled(Windows::ApplicationModel::Background::IBackgroundTaskInstance ^sender, Windows::ApplicationModel::Background::BackgroundTaskCancellationReason reason)
{
	m_cancelRequested = true;
}


void GPIOServiceClient::StartupTask::OnFailed(Platform::Object ^sender, Platform::Exception ^args)
{

	Windows::Foundation::TimeSpan interval;
	
	interval.Duration = 10 * nano100SecInSec; // after 10 sec Try Connect
	TimerElapsedHandler ^handler = ref new TimerElapsedHandler([this](ThreadPoolTimer ^timer) {
		// Try to Connect
		m_pServiceListener->StartClientAsync(m_RemoteComputer, 3005);
	});
	Windows::System::Threading::ThreadPoolTimer ^Timer;
	Timer = ThreadPoolTimer::CreateTimer(handler, interval);// einmaliger Erzeugen eines Timers

	m_bIsConnected = false;

}
void GPIOServiceClient::StartupTask::OnstopStreaming(Platform::Object ^sender, Platform::Exception ^args)
{ // Streaming
	Windows::Foundation::TimeSpan interval;

	interval.Duration = 10 * nano100SecInSec; // after 10 sec Try Connect
	TimerElapsedHandler ^handler = ref new TimerElapsedHandler([this](ThreadPoolTimer ^timer) {
		// Try to Connect
		m_pServiceListener->StartClientAsync(m_RemoteComputer, 3005);
	});
	Windows::System::Threading::ThreadPoolTimer ^Timer;
	Timer = ThreadPoolTimer::CreateTimer(handler, interval);// einmaliger Erzeugen eines Timers
	m_bIsConnected = false;

}

concurrency::task<bool> StartupTask::InitDisplayDriver(unsigned char adress)
{


	//	auto token = cancellation.get_token();

	auto task = m_DisplayManager->InitAsync((int)adress);

	auto rettsk = create_task(task).then([this, adress](bool bok) ->bool {
		try {
			bool IniOk = false;
			if (bok) {

				IniOk = m_DisplayManager->InitDisplay((int)adress);
				if (IniOk) {
					m_bDisplayInit = true;
				}

			}

			return IniOk;
		}
		catch (...)
		{
			if (bok) {


			}
			return false;
		}


	});

	return rettsk;
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
					if (!m_bIsConnected)
					{
					//	InitProcessingData();
						m_bIsConnected = true;
					}
					GPIOPin* pGPIOPin = (GPIOPin*)  pPacket->getAdditional();
					if (pGPIOPin != nullptr)
					{
						ProcessGPIOState(pGPIOPin);
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
			catch (Exception^ exception)
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
void StartupTask::DoDisplayInfo(int Adress)
{
	if (!m_bDisplayInit) return;

	wchar_t buffer[50];





	swprintf(&buffer[0], sizeof(buffer) / sizeof(buffer[0]), L"1:Dist: %5.2f %s", double (m_DistanceInZentimeter), L"cm");

	String^ line_1 = ref new Platform::String(buffer);

	swprintf(&buffer[0], sizeof(buffer) / sizeof(buffer[0]), L"1:Time: %5.2f %s", double(m_HC_SR04MeasTime/1000), L"ms");

	String^ line_2 = ref new Platform::String(buffer);

	swprintf(&buffer[0], sizeof(buffer) / sizeof(buffer[0]), L"1:Temp: %5.2f %s", double(m_Temperature), L"C");


	String^ line_3 = ref new Platform::String(buffer);


	String^ line_4 = L"...";


	bool bdoUpdate = m_DisplayManager->Update(Adress, line_1, line_2, line_3, line_4);

}
/*
void StartupTask::InitProcessingData()
{
	GPIOPin* pPin;

	GPIOs  gPIOs;
	// Input-Pins for Output Processing in aktiv setzen
	m_GPIOClientInOut->GetGPIPinsByTyp(gPIOs, GPIODriver::GPIOTyp::input);
	for (size_t i = 0; i < gPIOs.size(); i++) {
		gPIOs.at(i)->setActivateOutputProcessing(false);
	}

}
*/

void StartupTask::DoProcessingData()
{
	GPIOPin* pPin;
	/*
	pPin = m_GPIOClientInOut->getGPIOPinByPinNr(23);
	if (pPin != nullptr) pPin->setSetValue(!pPin->getSetValue());

	pPin = m_GPIOClientInOut->getGPIOPinByPinNr(22);
	if (pPin != nullptr) pPin->setSetValue(!pPin->getSetValue());
	*/

	pPin = m_GPIOClientInOut->getGPIOPinByPinNr(21);
	if (pPin != nullptr) pPin->setSetValue(!pPin->getSetValue());

	Platform::String^ state = m_GPIOClientInOut->GetGPIClientSendState(); // Status aller Ausgänge

	Windows::Storage::Streams::IBuffer^ buf = SocketHelpers::createPayloadBufferfromSendData(state);
	m_pServiceListener->SendDataToClients(buf);


	m_toggle = !m_toggle;

	//m_GPIOClientInOut->getGPIOPinValueByIdx(0);
}
void StartupTask::ProcessGPIOState(GPIODriver::GPIOPin* pGPIOPin)

{
	//m_GPIOClientInOut->Lock();
	//pGPIOPin->Lock();

	GPIODriver::GPIOPin* pGPIOOut = nullptr;
	double value;
	bool doSendNewState = false;
	switch (pGPIOPin->getGPIOTyp())
	{

		case GPIOTyp::input:
		{
			pGPIOPin->setActivateOutputProcessing(false);
			if (pGPIOPin->getPinNumber() == 24) {

				value = pGPIOPin->getPinValue() ? 1 : 0; // Relais low aktiv
				if (value == 0) {
					GPIODriver::GPIOOutputPin* pinOutPut = (GPIODriver::GPIOOutputPin*)  m_GPIOClientInOut->getGPIOPinByPinNr(17);
					if (pinOutPut != nullptr)
					{ // 1500 msec Output activate
					//	pinOutPut->setSetValue(0);
					//	pinOutPut->setPulseTimeinms(1500);
						pGPIOOut = pinOutPut;
					}
				}
			}
			else if (pGPIOPin->getPinNumber() == 25) {

				value = pGPIOPin->getPinValue() ? 1 : 0; // Relais low aktiv
				//if (value == 0) 
				{
					GPIODriver::GPIOOutputPin* pinOutPut = (GPIODriver::GPIOOutputPin*)  m_GPIOClientInOut->getGPIOPinByPinNr(27);
					if (pinOutPut != nullptr)
					{ // 1500 msec Output activate
					//	pinOutPut->setSetValue(!value);
						pGPIOOut = pinOutPut;
					}
				}
			}

			break;
		}
		case GPIOTyp::output:
		{
			break;
		}
		case GPIOTyp::PWM9685:
		case GPIOTyp::PWM:
		{
			if (pGPIOPin == m_PWMOutputPin)
			{
				if (m_PWMOutputPin->getSetValue() < 0.99)
				{
					m_PWMOutputPin->setSetValue(m_PWMOutputPin->getSetValue() + 0.1);
				}
				else {
					m_PWMOutputPin->setSetValue(0.5);
				}
				m_PWMOutputPin->setActivateOutputProcessing(false);

			}
				break;
		}
		case GPIOTyp::HC_SR04:
		{
			if (pGPIOPin->getPinNumber() == 12) {

				value = pGPIOPin->getPinValue(); // Zeit in s
				m_HC_SR04MeasTime = value;
				if (value == -1) // Es wurde kein Hindernis erkannt
				{

				}
				else // Zeit in µsec
				{
					double temp = 20; // fixed Temperature
					if (m_Temperature != -1) temp = m_Temperature;
			
					double VinMeterProSec = 331.5 + (0.6 * temp);
					m_DistanceInZentimeter = (VinMeterProSec * value)  / (10000 *2);
	//				DoDisplayInfo(m_DisplayAdress);

				}
				DoDisplayInfo(m_DisplayAdress);
			}

			break;
		}
		case GPIOTyp::BME280:
		{
			if (pGPIOPin->getPinNumber() == 0x76) {

				BME280Sensor*pBME280Sensor = (BME280Sensor *) pGPIOPin;
				double Humidity = pBME280Sensor->getHumidity();
				double Presssure = pBME280Sensor->getPressure();
				m_Temperature = pBME280Sensor->getTemperature();
	//			DoDisplayInfo(m_DisplayAdress);
			}

			break;
		}
		default:
		{
			break;
		}

	}

	if (pGPIOOut!=nullptr) {
		Platform::String^ state = pGPIOOut->GetGPIOPinClientSendCmd();
	//	Windows::Storage::Streams::IBuffer^ buf = SocketHelpers::createPayloadBufferfromSendData(state);
	//	m_pServiceListener->SendDataToClients(buf);
	}


//	pGPIOPin->UnLock();
//	m_GPIOClientInOut->UnLock();
}




}





