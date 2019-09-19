#include "pch.h"
#include "BME280ChunkReceiver.h"
#include <ppl.h>



using namespace concurrency;
using namespace Platform;
using namespace Windows::Networking;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;

namespace BME280Service
{



	BME280ChunkReceiver::BME280ChunkReceiver(Windows::Networking::Sockets::StreamSocket^socket)
	{
		m_EventSource = ref new SocketEventSource();
		m_socket = socket;
		m_acceptingData = false;
		m_pSocketCancelTaskToken = new concurrency::cancellation_token_source();

	}


	BME280ChunkReceiver::~BME280ChunkReceiver()
	{
		delete m_pSocketCancelTaskToken;
	}

	void BME280ChunkReceiver::Init()
	{
	
		m_acceptingData = false;
	}

	void BME280ChunkReceiver::CancelAsyncSocketOperation() {
		m_acceptingData = false;
		m_pSocketCancelTaskToken->cancel();


	}


	void BME280ChunkReceiver::SendData(Windows::Storage::Streams::IBuffer^ buffer)
	{

		if (buffer->Length == 0) return;

		if ( !m_acceptingData)
		{
			return;
		}

		auto lock = m_lock.LockExclusive();

	//	Windows::Storage::Streams::IBuffer^ buffer = createBufferfromSendData(info);

		auto token = m_pSocketCancelTaskToken->get_token();
		// Write the locally buffered data to the network.

		auto StoreAsync = m_socket->OutputStream->WriteAsync(buffer);

		create_task(StoreAsync).then([this](unsigned int bytesStored)
		{
			if (bytesStored > 0) {
				//	Trace("@%p completed sending HTTP part @%p", (void*)this, (void*)buffer);

				auto lock = m_lock.LockExclusive();
				return m_socket->OutputStream->FlushAsync();
			}
			else
			{ // when nothing is sended -> cancel
	
				cancel_current_task();
			}


		}, token).then([this](task<bool> previos) {
			bool bflushed = false;
			try
			{
				bflushed = previos.get();
			}

			catch (task_canceled const &) // cancel by User
			{
				return;
			}
			catch (Exception^ exception) {
				return;
			}

			catch (...)
			{
				return;
			}

		});
	}

	Windows::Storage::Streams::IBuffer^ BME280ChunkReceiver::createBufferfromSendData(std::string& stringinfo) {


		DataWriter^ writer = ref new DataWriter();
		// Write first the length of the string a UINT32 value followed up by the string. The operation will just store 
		// the data locally.
		Platform::String^ stringToSend = StringFromAscIIChars((char*)stringinfo.c_str());

		writer->WriteUInt32(writer->MeasureString(stringToSend));
		writer->WriteByte(0x55);	// CheckByte 1 for verification
		writer->WriteByte(0x55);	// CheckByte 2
		writer->WriteString(stringToSend);

		return writer->DetachBuffer();

	}
	void BME280ChunkReceiver::DoProcessChunk(DataReader^ reader)
	{
		bool doReadBuffer = false;

		unsigned int nread = 6;
		if (reader->UnconsumedBufferLength > nread)
			// 4 bytes for length and 2 bytes for verifikation
		{
			unsigned int readBufferLen = reader->ReadUInt32();

			byte checkByte1 = reader->ReadByte();
			byte checkByte2 = reader->ReadByte();

			if ((checkByte1 == 0x55) && (checkByte2 == 0x55)) { // prufen, ob stream richtig ist
			// Do Processing - Message

				unsigned int _len = reader->UnconsumedBufferLength;

				if (reader->UnconsumedBufferLength < readBufferLen) doReadBuffer = true;
				else {
					Platform::String^  rec = reader->ReadString(readBufferLen);
					if (rec == ("BME280Server.Start")) {
						std::string command = "BME280Server.Started";
						Windows::Storage::Streams::IBuffer^ buf = createBufferfromSendData(command);
						m_acceptingData = true;
						this->SendData(buf);

					}
					else if (rec == ("BME280Server.Stop")) {
						std::string command = "BME280Server.Stopped";
						Windows::Storage::Streams::IBuffer^ buf = createBufferfromSendData(command);
						m_acceptingData = false;
						this->SendData(buf);
					}
					else
					{
						//		DoProcessGPIOCommand(rec);
					}

				}
			}
			else doReadBuffer = true;
			if (doReadBuffer)
			{ // alles auslesen
				IBuffer^  chunkBuffer = reader->ReadBuffer(reader->UnconsumedBufferLength);
			}

		}

	}

	void BME280ChunkReceiver::ReceiveChunkLoop(Windows::Storage::Streams::DataReader^ reader, Windows::Networking::Sockets::StreamSocket^ socket)
	{
		// Read first 4 bytes (length of the subsequent string).
		auto token = m_pSocketCancelTaskToken->get_token();
		unsigned int readBufferLen = 4096;
		create_task(reader->LoadAsync(readBufferLen), token).then([this, reader, socket](task<unsigned int> size) {


			unsigned int readlen = 0;
			readlen = size.get();
			if (readlen == 0) {
				cancel_current_task();
			}
			else {
				DoProcessChunk(reader);
			}


		}).then([this, reader, socket](task<void> previous) {

			try {
				previous.get();
				ReceiveChunkLoop(reader, socket);
			}
			catch (task_canceled&)
			{
				auto lock = m_lock.LockExclusive();
				this->getSocketEventSource()->fire(socket, BME280Service::NotificationEventReason::CanceledbyUser);
				delete socket;
				m_socket = nullptr;

			}
			catch (Exception^ exception) {
				auto lock = m_lock.LockExclusive();
				this->getSocketEventSource()->fire(socket, BME280Service::NotificationEventReason::CanceledbyError);
				delete socket;
				m_socket = nullptr;
			}

			catch (...)
			{
				auto lock = m_lock.LockExclusive();
				this->getSocketEventSource()->fire(socket, BME280Service::NotificationEventReason::CanceledbyError);
				delete socket;
				m_socket = nullptr;

			}
		});



	}
}