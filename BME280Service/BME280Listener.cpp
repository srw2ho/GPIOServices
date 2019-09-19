#include "pch.h"
#include "BME280Listener.h"


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

namespace BME280Service
{



	BME280Listener::BME280Listener()
	{
			m_pBME280ChunkConnectionList = new std::list<BME280ChunkReceiver*>();
	}


	BME280Listener::~BME280Listener()
	{
		DeleteAllConnections();
		delete m_pBME280ChunkConnectionList;
	}

	void BME280Listener::StartListener()
	{

		if (m_ChunkReceiverToken.Value == 0)
		{
			CancelConnections();

			m_ChunkReceiverToken = m_listener->ConnectionReceived +=
				ref new TypedEventHandler<StreamSocketListener^, StreamSocketListenerConnectionReceivedEventArgs^>(
					this,
					&BME280Listener::OnConnectionReceived
					);

		}
	}

	void BME280Listener::StopListener()
	{

		if (m_ChunkReceiverToken.Value > 0) {
			CancelConnections();
			m_listener->ConnectionReceived -= m_ChunkReceiverToken; //keine neue Connections zulassen
			m_ChunkReceiverToken.Value = 0;
		}


	}

	void BME280Listener::CancelConnections()
	{
		auto lock = m_lock.LockExclusive();

		// cancel all pending Connections from getted Type
		while (!m_pBME280ChunkConnectionList->empty())
		{
			BME280ChunkReceiver*pConn = m_pBME280ChunkConnectionList->back();
			m_pBME280ChunkConnectionList->pop_back();

			pConn->CancelAsyncSocketOperation();

		}
	//	m_connections.UnLock();
	}

	void BME280Listener::DeleteAllConnections(void) {
		auto lock = m_lock.LockExclusive();

		while (!m_pBME280ChunkConnectionList->empty())
		{
			BME280ChunkReceiver*pConn = m_pBME280ChunkConnectionList->back();
			m_pBME280ChunkConnectionList->pop_back();
			delete pConn;

		}


	}


	bool BME280Listener::DeleteConnectionBySocket(Windows::Networking::Sockets::StreamSocket^ socket) {
		bool found = false;
		auto lock = m_lock.LockExclusive();
		std::list<BME280ChunkReceiver*>::iterator it;

		for (it = m_pBME280ChunkConnectionList->begin(); it != m_pBME280ChunkConnectionList->end(); it++)
		{
			BME280ChunkReceiver*pConn = *it;
			if (pConn->getSocket() == socket)
			{
				m_pBME280ChunkConnectionList->remove(pConn);
				delete pConn;
				found = true;
				break;
			}

		}

		return found;

	}

	IAsyncAction ^ BME280Listener::CreateListenerServerAsync(int port)
	{

		return create_async([this, port]()
		{

			StartListener();

			this->m_listener->Control->KeepAlive = false;


			return create_task(this->m_listener->BindServiceNameAsync(port == 0 ? L"" : port.ToString()))

				.then([this] {

				this->m_port = _wtoi(this->m_listener->Information->LocalPort->Data());


				if (this->m_port == 0)
				{
					throw ref new InvalidArgumentException(L"Failed to convert TCP port");
				}
			

			});


		});


	}


	void BME280Listener::OnConnectionReceived(_In_ StreamSocketListener^ sender, _In_ StreamSocketListenerConnectionReceivedEventArgs^ e)
	{

		auto lock = m_lock.LockExclusive();

		DataReader^ reader = ref new DataReader(e->Socket->InputStream);
		reader->InputStreamOptions = InputStreamOptions::Partial;
		StreamSocket^ socket = e->Socket;

		this->startStreaming(this, socket); // Callback to MediaPlayder Source


		BME280ChunkReceiver* pBME280ChunkReceiver = new BME280ChunkReceiver(socket);
		pBME280ChunkReceiver->getSocketEventSource()->eventhandler += ref new BME280Service::NotificationEventhandler(this, &BME280Listener::OnStopStreaming);
		pBME280ChunkReceiver->Init();
		pBME280ChunkReceiver->ReceiveChunkLoop(reader, e->Socket);
		m_pBME280ChunkConnectionList->push_back(pBME280ChunkReceiver);


	}

	void BME280Listener::OnStopStreaming(Windows::Networking::Sockets::StreamSocket^ socket, int status)
	{

		DeleteConnectionBySocket(socket);
		this->stopStreaming(this, status); // Callback to MediaPlayder Source


	}

	void BME280Listener::SendDataToClients(Windows::Storage::Streams::IBuffer^ buffer)
	{

		auto lock = m_lock.LockExclusive();

		std::list<BME280ChunkReceiver*>::iterator it;
		for (it = m_pBME280ChunkConnectionList->begin(); it != m_pBME280ChunkConnectionList->end(); it++)
		{
			BME280ChunkReceiver*pConn = *it;
			pConn->SendData(buffer);
		}


	}

}