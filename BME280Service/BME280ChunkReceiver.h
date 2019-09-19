#pragma once

#include <collection.h>
#include <ppltasks.h>
#include <wrl.h>
#include <robuffer.h>

#include "SocketEventSource.h"

namespace BME280Service
{

	class BME280ChunkReceiver
	{
		concurrency::cancellation_token_source* m_pSocketCancelTaskToken;

		int m_ChunkProcessing;
		unsigned int m_chunkBufferSize;

		int64 m_StartChunkReceived;
		int64 m_DeltaChunkReceived;
		unsigned int m_ReadBufferLen;


		BME280Service::SocketEventSource ^ m_EventSource;
		Windows::Networking::Sockets::StreamSocket^ m_socket;
		bool m_acceptingData;

		Microsoft::WRL::Wrappers::SRWLock m_lock;
//		concurrency::cancellation_token_source* m_pSocketCancelTaskToken;


	public:
		BME280ChunkReceiver(Windows::Networking::Sockets::StreamSocket^);
		virtual ~BME280ChunkReceiver();

		void Init();
		void ReceiveChunkLoop(
			Windows::Storage::Streams::DataReader^ reader,
			Windows::Networking::Sockets::StreamSocket^ socket);


		void CancelAsyncSocketOperation();
		BME280Service::SocketEventSource ^ getSocketEventSource() { return m_EventSource; };

	//	void setCancellationToken(concurrency::cancellation_token_source* token) { m_pSocketCancelTaskToken = token; };

		Windows::Networking::Sockets::StreamSocket^ getSocket() { return m_socket; };

		void BME280ChunkReceiver::SendData(Windows::Storage::Streams::IBuffer^ buffer);
	private:
		void DoProcessChunk(Windows::Storage::Streams::DataReader^ reader);
		Windows::Storage::Streams::IBuffer^ createBufferfromSendData(std::string& stringinfo);
	};

}