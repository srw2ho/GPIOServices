#include "pch.h"
#include <ppl.h>
#include "ServiceChunkReceiver.h"
#include "GPIODriver.h"




using namespace GPIODriver;

using namespace concurrency;
using namespace Platform;
using namespace Windows::Networking;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;


using namespace StreamSocketComm;

namespace GPIOService
{


	ServiceChunkReceiver::ServiceChunkReceiver(Windows::Networking::Sockets::StreamSocket^ socket, GPIODriver::GPIOInOut^ pGPIOInOut) :SocketChunkReceiver(socket)
	{
		m_pGPIOInOut = pGPIOInOut;

	}


	ServiceChunkReceiver::~ServiceChunkReceiver()
	{

	}


	Windows::Storage::Streams::IBuffer^ ServiceChunkReceiver::createBufferfromDataQueue() {



		DataWriter^ writer = ref new DataWriter();


		return writer->DetachBuffer();
	}


	int ServiceChunkReceiver::DoProcessStringMsg(DataReader^ reader)
	{
		Platform::String^ rec = reader->ReadString(m_chunkBufferSize);
		std::vector<std::wstring> arr = GPIODriver::splitintoArray(rec->Data(), L" ");
		if (arr.size() > 0) {

			if (wcscmp(arr[0].c_str(), L"GPIOServiceClient.Start") == 0) { // Start Command
				if (arr.size() > 1) {
					wchar_t removechar = ' ';
					remove(arr[1], (wchar_t)removechar); // remove blanks form string
					if (wcscmp(arr[1].c_str(), L"UseMPack") == 0) { // Ue Mpack
						this->m_pGPIOInOut->setUseMpack(true);
					}
				}
				std::string command = "GPIOServiceClient.Started";
				Windows::Storage::Streams::IBuffer^ buf = SocketHelpers::createBufferfromSendData(command);
				m_acceptingData = true;
				this->SendData(buf);
			}
			else
			if (wcscmp(arr[0].c_str(), L"GPIOServiceClient.Stop") == 0) { // Stop Command
				std::string command = "GPIOServiceClient.Stopped";
				Windows::Storage::Streams::IBuffer^ buf = SocketHelpers::createBufferfromSendData(command);
				m_acceptingData = false;
				this->SendData(buf);
			}


	/*		if (arr[0].c_str() == (L"GPIOServiceClient.Start")) {
				std::string command = "GPIOServiceClient.Started";
				Windows::Storage::Streams::IBuffer^ buf = SocketHelpers::createBufferfromSendData(command);
				m_acceptingData = true;
				this->SendData(buf);

			}
			else if (rec == ("GPIOServiceMpackClient.Start")) {
				std::string command = "GPIOServiceClient.Started";
				Windows::Storage::Streams::IBuffer^ buf = SocketHelpers::createBufferfromSendData(command);
				this->m_pGPIOInOut->setUseMpack(true);
				m_acceptingData = true;
				this->SendData(buf);
			}
			else if (rec == ("GPIOServiceClient.Stop")) {
				std::string command = "GPIOServiceClient.Stopped";
				Windows::Storage::Streams::IBuffer^ buf = SocketHelpers::createBufferfromSendData(command);
				m_acceptingData = false;
				this->SendData(buf);
			}*/
		}




		return 1;

	}
	int ServiceChunkReceiver::DoProcessPayloadMsg(DataReader^ reader)
	{
		unsigned int len = reader->UnconsumedBufferLength;
		Platform::String^ rec = reader->ReadString(m_chunkBufferSize);
		m_pGPIOInOut->DoProcessCommand(rec);
		return 1;

	}

	int ServiceChunkReceiver::DoProcessPayloadMsgPack(DataReader^ reader)
	{
		unsigned int len = reader->UnconsumedBufferLength;
		auto byteArry = ref new Platform::Array<byte>(m_chunkBufferSize);

		reader->ReadBytes(byteArry);
		std::vector<byte> arr(byteArry->begin(), byteArry->end());
		bool ret = m_pGPIOInOut->parse_mpackObjects(arr);
		return ret ? 1 : -1;

	}

	int ServiceChunkReceiver::SearchForChunkMsgLen(DataReader^ reader)
	{
		unsigned int len = reader->UnconsumedBufferLength;
		m_chunkBufferSize = -1;
		unsigned int nread = 6;
		if (reader->UnconsumedBufferLength >= nread)
		{
			bool wrongdata = true;
			m_ConsumeDataType = ConsumeDataType::Undef;
			unsigned int readBufferLen = reader->ReadUInt32();

			byte checkByte1 = reader->ReadByte();
			byte checkByte2 = reader->ReadByte();

			if (checkByte1 == 0x55)  // prufen, ob stream richtig ist
			{
				wrongdata = false;
				if (checkByte2 == 0x55) {
					m_ConsumeDataType = ConsumeDataType::String;
				}
				else
					if (checkByte2 == 0x50) {
						m_ConsumeDataType = ConsumeDataType::Binary;
					}
					else
						if (checkByte2 == 0x51) {
							m_ConsumeDataType = ConsumeDataType::MsgPack;
						}
						else {
							wrongdata = true;
						}

			}



			if (wrongdata)
			{ // alles auslesen, neu aufsetzen
				IBuffer^ chunkBuffer = reader->ReadBuffer(reader->UnconsumedBufferLength);
			}
			else {
				m_chunkBufferSize = readBufferLen;
			}

		}

		len = reader->UnconsumedBufferLength;
		return m_chunkBufferSize;
	}

	void ServiceChunkReceiver::DoProcessChunk(DataReader^ reader)
	{
		int chunkMsgLen = 0;
		bool doReadNewData = false;
		while (!doReadNewData)
		{
			switch (m_ChunkProcessing)
			{

			case 0: // search Chunk-Msg-Len
			{

				int nChunkProcess = SearchForChunkMsgLen(reader);

				if (nChunkProcess > 0) {
					m_ChunkProcessing = 1;
					int64 time = 0;
					m_DeltaChunkReceived = time - m_StartChunkReceived;
					m_StartChunkReceived = time;


				}
				else if (nChunkProcess == -1) { // read no buffer
					doReadNewData = true;
				}
				break;
			}
			case 1: // Process-Chunk
			{
				int nChunkProcess = DoProcessChunkMsg(reader);
				if (nChunkProcess == 1) // read new data use same buffer
				{
					m_ChunkProcessing = 0;
				}
				else if (nChunkProcess == -1) // read new buffer
				{
					m_ChunkProcessing = 0;
					doReadNewData = true;
				}
				else if (nChunkProcess == 0) // 
				{// read further data
					doReadNewData = true;
				}
				break;


			}
			default:
			{

			}

			}
		}



	}

	int ServiceChunkReceiver::DoProcessChunkMsg(DataReader^ reader)
	{
		if (m_chunkBufferSize == -1) return -1;

		unsigned int _len = reader->UnconsumedBufferLength;

		if (m_chunkBufferSize == 0)
		{
			return 0;
		}
		if (reader->UnconsumedBufferLength < m_chunkBufferSize) {
			return 0;
		}

		int retValue = -1;
		if (m_ConsumeDataType == ConsumeDataType::String)
		{
			retValue = DoProcessStringMsg(reader);
		}
		else 	if (m_ConsumeDataType == ConsumeDataType::Binary)
		{
			retValue = DoProcessPayloadMsg(reader);
		}
		else 	if (m_ConsumeDataType == ConsumeDataType::MsgPack)
		{
			retValue = DoProcessPayloadMsgPack(reader);
		}

		return retValue; // read new data
	}

	//void ServiceChunkReceiver::DoProcessChunk(DataReader^ reader)
	//{
	//	bool doReadBuffer = false;

	//	unsigned int nread = 6;
	//	if (reader->UnconsumedBufferLength > nread)
	//		// 4 bytes for length and 2 bytes for verifikation
	//	{
	//		m_ConsumeDataType = ConsumeDataType::Undef;

	//		unsigned int readBufferLen = reader->ReadUInt32();

	//		byte checkByte1 = reader->ReadByte();
	//		byte checkByte2 = reader->ReadByte();

	//		if (checkByte1 == 0x55)  // prufen, ob stream richtig ist
	//		{
	//			m_chunkBufferSize = readBufferLen;
	//			if (checkByte2 == 0x55) {
	//				m_ConsumeDataType = ConsumeDataType::String;
	//			}
	//			else if (checkByte2 == 0x50) {
	//				m_ConsumeDataType = ConsumeDataType::Binary;
	//			}
	//			else if (checkByte2 == 0x51) {
	//				m_ConsumeDataType = ConsumeDataType::MsgPack;
	//			}
	//			else
	//			{
	//				doReadBuffer = true;
	//			}
	//		}

	//		if (doReadBuffer)
	//		{ // alles auslesen
	//			IBuffer^ chunkBuffer = reader->ReadBuffer(reader->UnconsumedBufferLength);
	//		}
	//		else {
	//			DoProcessChunkMsg(reader);
	//		}

	//	}

	//}

}