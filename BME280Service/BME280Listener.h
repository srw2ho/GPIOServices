#pragma once
#include <vector>
#include <list>
#include <memory>
#include <sstream>

#include <collection.h>
#include <ppltasks.h>
#include <wrl.h>
#include <robuffer.h>
#include <BME280ChunkReceiver.h>

namespace BME280Service
{


	//class BME280Listener
	public ref class BME280Listener sealed

	{
		Windows::Networking::Sockets::StreamSocketListener^ m_listener;
		int m_port;

		Windows::Foundation::EventRegistrationToken m_ChunkReceiverToken;

		Windows::Foundation::EventRegistrationToken m_BME280ChunkReceiveToken;

		std::list<BME280ChunkReceiver*> * m_pBME280ChunkConnectionList;
		Microsoft::WRL::Wrappers::SRWLock m_lock;

	//	BME280ChunkReceiver* m_pBME280ChunkReceiver;
	public:
		BME280Listener();
		virtual ~BME280Listener();

		Windows::Foundation::IAsyncAction ^ CreateListenerServerAsync(int port);
		void StartListener();
		void StopListener();
		void CancelConnections();
		void DeleteAllConnections(void);

		event Windows::Foundation::TypedEventHandler<Platform::Object^, Windows::Networking::Sockets::StreamSocket^  >^ startStreaming;
		event Windows::Foundation::TypedEventHandler<Platform::Object^, int>^ stopStreaming;

		void SendDataToClients(Windows::Storage::Streams::IBuffer^ buffer);
	protected:
		void OnConnectionReceived(
			_In_ Windows::Networking::Sockets::StreamSocketListener^ sender,
			_In_ Windows::Networking::Sockets::StreamSocketListenerConnectionReceivedEventArgs^ e
		);

		void OnStopStreaming(Windows::Networking::Sockets::StreamSocket^ socket, int status);
		bool DeleteConnectionBySocket(Windows::Networking::Sockets::StreamSocket^ socket);
	};

}