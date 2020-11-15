#pragma once
#ifndef BME280SOCKETCHUNK_RECEIVER_H_
#define BME280SOCKETCHUNK_RECEIVER_H_

#include <collection.h>
#include <ppltasks.h>
#include <wrl.h>
#include <robuffer.h>

#include "SocketListener.h"

#include "SocketChunkReceiver.h"

#include "GPIOInOut.h"

namespace GPIOService
{


	class ServiceChunkReceiver: public StreamSocketComm::SocketChunkReceiver
	{
		enum ConsumeDataType {
			Undef,
			Binary,
			String,
			MsgPack
		};


		GPIODriver::GPIOInOut^ m_pGPIOInOut;

		int m_ConsumeDataType;

	public:
		ServiceChunkReceiver(Windows::Networking::Sockets::StreamSocket^, GPIODriver::GPIOInOut^);
		virtual ~ServiceChunkReceiver();


	private:
		virtual void DoProcessChunk(Windows::Storage::Streams::DataReader^ reader);
		Windows::Storage::Streams::IBuffer^ createBufferfromDataQueue();
		int DoProcessChunkMsg(Windows::Storage::Streams::DataReader^ reader);
		int DoProcessStringMsg(Windows::Storage::Streams::DataReader^ reader);
		int DoProcessPayloadMsg(Windows::Storage::Streams::DataReader^ reader);
		int DoProcessPayloadMsgPack(Windows::Storage::Streams::DataReader^ reader);
		int SearchForChunkMsgLen(Windows::Storage::Streams::DataReader^ reader);
	};

}
#endif /**/