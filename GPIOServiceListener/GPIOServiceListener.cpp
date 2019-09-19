#include "pch.h"
#include "GPIOServiceListener.h"

using namespace concurrency;

using namespace Platform;
using namespace Platform::Collections;
using namespace std;
using namespace Windows::Foundation;
using namespace Windows::Media::Capture;
using namespace Windows::Networking;
using namespace Windows::Networking::Connectivity;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Core;

using namespace GPIOServiceListener;


GPIOListener::GPIOListener(GPIODriver::GPIOClientInOut^pGPIOClientInOut, Windows::ApplicationModel::AppService::AppServiceConnection  ^serviceConnection)
{
	m_listener = ref new Windows::Networking::Sockets::StreamSocketListener();
	//	m_pSocketCancelTaskToken = new concurrency::cancellation_token_source();
	m_pSocketCancelTaskToken = nullptr;
	//m_pOpenCVPackageCancelTaskToken = new concurrency::cancellation_token_source();

	m_pGPIOChunkReceiver = new GPIOChunkReceiver(pGPIOClientInOut,serviceConnection);
	m_serviceConnection = serviceConnection;

	m_pGPIOClientInOut = pGPIOClientInOut;


	m_port = -1;
}

IAsyncAction ^ GPIOListener::StartServerAsync(int port)
{

	if (this->m_port > 0) port = this->m_port;

	return create_async([this, port]()
	{

		StartListenerAsync();

		this->m_listener->Control->KeepAlive = false;



		return create_task(this->m_listener->BindServiceNameAsync(port == 0 ? L"" : port.ToString()))

			.then([this] {

			this->m_port = _wtoi(this->m_listener->Information->LocalPort->Data());


			if (this->m_port == 0)
			{
				throw ref new InvalidArgumentException(L"Failed to convert TCP port");
			}
			//	Trace("@%p bound socket listener to port %i", (void*)server, server->m_port);


		});


	});


}

void GPIOListener::CloseHTTPServerAsync()
{

	if (m_OnConnectionReceivedToken.Value > 0) {
		m_pSocketCancelTaskToken->cancel();
		m_listener->ConnectionReceived -= m_OnConnectionReceivedToken; //keine neue Connections zulassen
		m_pGPIOChunkReceiver->getSocketEventSource()->eventhandler -= m_GPIOChunkReceiverToken; //keine neue Connections zulassen

		m_OnConnectionReceivedToken.Value = 0;
	}


}

void GPIOListener::StartListenerAsync()
{

	if (m_OnConnectionReceivedToken.Value == 0)
	{
		if (m_pSocketCancelTaskToken != nullptr) {
			delete m_pSocketCancelTaskToken;
		}
		m_pSocketCancelTaskToken = new concurrency::cancellation_token_source();

		m_pGPIOChunkReceiver->setCancellationToken(m_pSocketCancelTaskToken);

		m_OnConnectionReceivedToken = m_listener->ConnectionReceived +=
			ref new TypedEventHandler<StreamSocketListener^, StreamSocketListenerConnectionReceivedEventArgs^>(
				this,
				&GPIOListener::OnConnectionReceived
				);

		m_GPIOChunkReceiverToken = m_pGPIOChunkReceiver->getSocketEventSource()->eventhandler += ref new GPIOServiceListener::NotificationEventhandler(this, &GPIOListener::OnStopStreaming);


	}
}

void GPIOListener::OnStopStreaming(Windows::Networking::Sockets::StreamSocket^ socket, int status)
{

	this->m_pGPIOChunkReceiver->DoResetCommand(0); // alle Ausgänge auf 0 setzen

	this->stopStreaming(this, status); // Callback to Caller

}

void GPIOListener::OnConnectionReceived(_In_ StreamSocketListener^ sender, _In_ StreamSocketListenerConnectionReceivedEventArgs^ e)
{

	DataReader^ reader = ref new DataReader(e->Socket->InputStream);
	reader->InputStreamOptions = InputStreamOptions::Partial;
	StreamSocket^ socket = e->Socket;

	this->startStreaming(this, socket); // Callback to Caller

	m_pGPIOChunkReceiver->Init();
	m_pGPIOChunkReceiver->ReceiveChunkLoop(reader, e->Socket);


}
