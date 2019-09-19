#pragma once

#include <vector>
#include <list>
#include <memory>
#include <sstream>

#include <collection.h>
#include <ppltasks.h>
#include <wrl.h>
#include <robuffer.h>

#include "GPIOChunkReceiver.h"


namespace GPIOServiceListener
{
	enum  NotificationEventReason {
		EndRequest = 0,
		CanceledbyUser,
		CanceledbyError
	};



	delegate void NotificationEventhandler(Windows::Networking::Sockets::StreamSocket^ socket, int status);

	// interface that has an event and a function to invoke the event  
	interface struct EventInterface {
	public:
		event NotificationEventhandler ^ eventhandler;
		void fire(Windows::Networking::Sockets::StreamSocket^ socket, int status);
	};


	// class that implements the interface event and function  
	ref class SocketEventSource : public EventInterface {
	public:

		virtual event NotificationEventhandler^ eventhandler;

		virtual void fire(Windows::Networking::Sockets::StreamSocket^ socket, int status)
		{
			eventhandler(socket, status);

		}
	};


	class GPIOChunkReceiver
	{
	private:
		concurrency::cancellation_token_source* m_pSocketCancelTaskToken;

		int m_ChunkProcessing;
		unsigned int m_chunkBufferSize;

		int64 m_StartChunkReceived;
		int64 m_DeltaChunkReceived;
		unsigned int m_ReadBufferLen;
		SocketEventSource ^ m_EventSource;
		GPIODriver::GPIOClientInOut^ m_pGPIOClientInOut;
		Windows::ApplicationModel::AppService::AppServiceConnection ^m_serviceConnection;
		Windows::Networking::Sockets::StreamSocket^ m_socket;
		Microsoft::WRL::Wrappers::SRWLock m_lock;
	public:
		GPIOChunkReceiver(GPIODriver::GPIOClientInOut^pGPIOClientInOut,Windows::ApplicationModel::AppService::AppServiceConnection  ^serviceConnection);

		virtual ~GPIOChunkReceiver();
		void setSocket(Windows::Networking::Sockets::StreamSocket^ socket) {m_socket = socket;};
		void Init();
		void ReceiveChunkLoop(Windows::Storage::Streams::DataReader^ reader,Windows::Networking::Sockets::StreamSocket^ socket);

		void setCancellationToken(concurrency::cancellation_token_source* token) { m_pSocketCancelTaskToken = token; };

		SocketEventSource ^ getSocketEventSource() { return m_EventSource; };

		void DoProcessCommand(Platform::String^ doProcessCmd);
		void DoResetCommand(int resetValue);
	private:

		unsigned char* GetData(Windows::Storage::Streams::IBuffer^ buffer);
		std::vector<unsigned char> ConvertBufferToByteVector(Windows::Storage::Streams::IBuffer^ buffer);
		void DoProcessChunk(Windows::Storage::Streams::DataReader^ reader);
		int SearchForChunkMsgLen(Windows::Storage::Streams::DataReader^ reader);
		int DoProcessChunkMsg(Windows::Storage::Streams::DataReader^ reader);
		Windows::Storage::Streams::IBuffer^ createBufferfromSendData(Platform::String^ stringToSend);
		void SendData(Platform::String^ info);

	};
}
