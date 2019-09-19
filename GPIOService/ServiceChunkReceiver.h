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
		GPIODriver::GPIOInOut^ m_pGPIOInOut;

	public:
		ServiceChunkReceiver(Windows::Networking::Sockets::StreamSocket^, GPIODriver::GPIOInOut^);
		virtual ~ServiceChunkReceiver();


	private:
		virtual void DoProcessChunk(Windows::Storage::Streams::DataReader^ reader);
		Windows::Storage::Streams::IBuffer^ createBufferfromDataQueue();
	};

}
#endif /**/