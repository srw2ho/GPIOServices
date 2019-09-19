#include "pch.h"
#include <ppl.h>
#include "ServiceChunkReceiver.h"




using namespace GPIODriver;

using namespace concurrency;
using namespace Platform;
using namespace Windows::Networking;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;


using namespace StreamSocketComm;

namespace GPIOService
{


	ServiceChunkReceiver::ServiceChunkReceiver(Windows::Networking::Sockets::StreamSocket^socket, GPIODriver::GPIOInOut^ pGPIOInOut) :SocketChunkReceiver(socket)
	{
		m_pGPIOInOut = pGPIOInOut;

	}


	ServiceChunkReceiver::~ServiceChunkReceiver()
	{

	}


	Windows::Storage::Streams::IBuffer^ ServiceChunkReceiver::createBufferfromDataQueue() {



		DataWriter ^ writer = ref new DataWriter();
		

		return writer->DetachBuffer();
	}



	

	void ServiceChunkReceiver::DoProcessChunk(DataReader^ reader)
	{
		bool doReadBuffer = false;

		unsigned int nread = 6;
		if (reader->UnconsumedBufferLength > nread)
			// 4 bytes for length and 2 bytes for verifikation
		{
			unsigned int readBufferLen = reader->ReadUInt32();

			byte checkByte1 = reader->ReadByte();
			byte checkByte2 = reader->ReadByte();

			if ((checkByte1 == 0x55) && ((checkByte2 == 0x55) || (checkByte2 == 0x50)))  // prufen, ob stream richtig ist
			{
	//		if ((checkByte1 == 0x55) && (checkByte2 == 0x55)) { // prufen, ob stream richtig ist
			// Do Processing - Message

				unsigned int _len = reader->UnconsumedBufferLength;


				if (reader->UnconsumedBufferLength < readBufferLen) doReadBuffer = true;
				else {
					if (checkByte2 == 0x55) {
					}
					else if (checkByte2 == 0x50) {

					}

					Platform::String^  rec = reader->ReadString(readBufferLen);
					if (rec == ("GPIOServiceClient.Start")) {
						std::string command = "GPIOServiceClient.Started";
						Windows::Storage::Streams::IBuffer^ buf = SocketHelpers::createBufferfromSendData(command);
						m_acceptingData = true;
						this->SendData(buf);

					}
					else if (rec == ("GPIOServiceClient.Stop")) {
						std::string command = "GPIOServiceClient.Stopped";
						Windows::Storage::Streams::IBuffer^ buf = SocketHelpers::createBufferfromSendData(command);
						m_acceptingData = false;
						this->SendData(buf);
					}
					else
					if (rec == ("GPIOServiceClient.GetData")) {

						String^ state = m_pGPIOInOut->GetGPIState();

						Windows::Storage::Streams::IBuffer^ buf = SocketHelpers::createPayloadBufferfromSendData(state);

				//		Windows::Storage::Streams::IBuffer^ buf = createBufferfromDataQueue();
						this->SendData(buf);
					}
					else
					if (rec == ("GPIOServiceClient.LifeCycle")) {
						std::string command = "GPIOServiceClient.LifeCycle";
						Windows::Storage::Streams::IBuffer^ buf = SocketHelpers::createBufferfromSendData(command);
						this->SendData(buf);
					}
					else
					{
						m_pGPIOInOut->DoProcessCommand(rec);
			//			String^ state = m_pGPIOInOut->GetGPIState();

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

}