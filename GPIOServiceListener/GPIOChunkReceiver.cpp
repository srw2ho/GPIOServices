#pragma once
#include "pch.h"
#include "GPIOChunkReceiver.h"


using namespace GPIOServiceListener;
using namespace GPIODriver;
using namespace concurrency;
using namespace Windows::ApplicationModel::AppService;
using namespace Platform;
using namespace Platform::Collections;
using namespace std;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Media::Capture;
using namespace Windows::Networking;
using namespace Windows::Networking::Connectivity;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Core;

#define CHK(statement)  {HRESULT _hr = (statement); if (FAILED(_hr)) { throw ref new Platform::COMException(_hr); };}

#define CHUNK_LENGTH_FIELD_LEN	6
#define CHUNK_FOOTER_FIELD_LEN	0



GPIOChunkReceiver::GPIOChunkReceiver(GPIODriver::GPIOClientInOut^pGPIOClientInOut, Windows::ApplicationModel::AppService::AppServiceConnection  ^serviceConnection)
{

	m_serviceConnection = serviceConnection;
	m_pSocketCancelTaskToken = nullptr;


	m_StartChunkReceived = 0;
	m_DeltaChunkReceived = 0;


	m_ChunkProcessing = 0;


	m_EventSource = ref new SocketEventSource;
	m_pGPIOClientInOut = pGPIOClientInOut;

	m_socket = nullptr;


}

void GPIOChunkReceiver::Init() {
	m_ChunkProcessing = 0;
	m_StartChunkReceived = 0;
	m_DeltaChunkReceived = 0;
	m_ReadBufferLen = 640 * 500;
}

GPIOChunkReceiver::~GPIOChunkReceiver()
{
	


}



//convert IBuffer to byte array

std::vector<unsigned char> GPIOChunkReceiver::ConvertBufferToByteVector(Windows::Storage::Streams::IBuffer^ buffer) {

	auto reader = ::Windows::Storage::Streams::DataReader::FromBuffer(buffer);

	std::vector<unsigned char> data(reader->UnconsumedBufferLength);
	//	Array<unsigned char>^ value = ref new Array<unsigned char>(100);

	if (!data.empty())
		reader->ReadBytes(::Platform::ArrayReference<unsigned char>(&data[0], (unsigned int)data.size()));
	//	reader->ReadBytes(value);

	return data;
}



unsigned char* GPIOChunkReceiver::GetData(Windows::Storage::Streams::IBuffer^ buffer)
{
	unsigned char* bytes = nullptr;
	Microsoft::WRL::ComPtr<::Windows::Storage::Streams::IBufferByteAccess> bufferAccess;
	CHK(((IUnknown*)buffer)->QueryInterface(IID_PPV_ARGS(&bufferAccess)));
	CHK(bufferAccess->Buffer(&bytes));
	return bytes;
}



int GPIOChunkReceiver::SearchForChunkMsgLen(DataReader^ reader)
{
	m_chunkBufferSize = -1;
	unsigned int nread = CHUNK_LENGTH_FIELD_LEN;
	if (reader->UnconsumedBufferLength > nread)
		// 4 bytes for length and 2 bytes for verifikation
	{
		unsigned int readBufferLen = reader->ReadUInt32();

		byte checkByte1 = reader->ReadByte();
		byte checkByte2 = reader->ReadByte();

		if ((checkByte1 == 0x55) && (checkByte2 == 0x55)) { // prufen, ob stream richtig ist
			m_chunkBufferSize = readBufferLen;
			
		}
		else
		{ // alles auslesen
			IBuffer^  chunkBuffer = reader->ReadBuffer(reader->UnconsumedBufferLength);
		}

	}

	return m_chunkBufferSize;
}
int GPIOChunkReceiver::DoProcessChunkMsg(DataReader^ reader)
{
	if (m_chunkBufferSize == -1) return -1;

	unsigned int _len = reader->UnconsumedBufferLength;

	if (reader->UnconsumedBufferLength < m_chunkBufferSize + CHUNK_FOOTER_FIELD_LEN) return 0;

	Platform::String^  rec = reader->ReadString(m_chunkBufferSize);

//	std::wstring doProcessCmd;
//	doProcessCmd.clear();
	if (rec == ("GPIOServiceClient.Start")) {
		/*
		doProcessCmd.append(L"GPO.18 = 0;");
		doProcessCmd.append(L"GPO.07 = 0;");
		doProcessCmd.append(L"GPO.05 = 0;");
		doProcessCmd.append(L"GPI.23 = 0;");// for Input
		*/
		SendData("GPIOServiceClient.Started"); // send data back

	}
	else if (rec == ("GPIOServiceClient.Stop")) {
		/*
		doProcessCmd.append(L"GPO.18 = 0;");
		doProcessCmd.append(L"GPO.07 = 0;");
		doProcessCmd.append(L"GPO.05 = 0;");
		*/
		SendData("GPIOServiceClient.Stopped"); // send data back
	}
	else {

//		Platform::String^ doProcessCmdstr = ref new Platform::String(doProcessCmd.c_str());
		DoProcessCommand(rec);

		/*
		std::vector<std::wstring> splitlines = GPIOServiceListener::splitintoArray(rec->Data(), L";");
		for (int i = 0; i < splitlines.size(); i++) {
			std::wstring line = splitlines.at(i);

			if (wcsncmp(line.c_str(), L"FaceRecognized", wcslen(L"FaceRecognized") )==0 ) {

				std::vector<std::wstring> splitvalues = GPIOServiceListener::splitintoArray(line, L":");
				if (splitvalues.size() > 1) {
					int value = _wtoi(splitvalues[1].c_str());
					if (value>0) doProcessCmd.append(L"GPO.18 = 1;");
					else doProcessCmd.append(L"GPO.18 = 0;");
				}
			}
	
			else if (wcsncmp(line.c_str(), L"FaceTracked", wcslen(L"FaceTracked")) == 0) {

				std::vector<std::wstring> splitvalues = GPIOServiceListener::splitintoArray(line, L":");
				if (splitvalues.size() > 1) {
					int value = _wtoi(splitvalues[1].c_str());
					if (value > 0) doProcessCmd.append(L"GPO.7 = 1;");
					else doProcessCmd.append(L"GPO.7 = 0;");
				}
			}
			else if (wcsncmp(line.c_str(), L"MotionDetected", wcslen(L"MotionDetected")) == 0) {
		
				std::vector<std::wstring> splitvalues = GPIOServiceListener::splitintoArray(line, L":");
				if (splitvalues.size() > 1) {
					int value = _wtoi(splitvalues[1].c_str());
					if (value > 0) doProcessCmd.append(L"GPO.5 = 1;");
					else doProcessCmd.append(L"GPO.5 = 0;");
				}
			}

		}
		*/



	}



	return 1;
}

void GPIOChunkReceiver::DoResetCommand(int resetValue)

	{


		auto message = ref new ValueSet();

		message->Insert(L"GPIO.ResetOut", resetValue);
		//m_toggle = !m_toggle;
		auto messageTask = Concurrency::create_task(m_serviceConnection->SendMessageAsync(message));
		messageTask.then([this](AppServiceResponse ^appServiceResponse)

		{

			if ((appServiceResponse->Status == AppServiceResponseStatus::Success) && (appServiceResponse->Message != nullptr))
			{
				if (appServiceResponse->Message->HasKey(L"Response")) {
					auto response = appServiceResponse->Message->Lookup(L"Response");

					String^ responseMessage = safe_cast<String^>(response);

					if (String::operator==(responseMessage, L"Success"))
					{

						

					}
				}


			}

		});

	}
void GPIOChunkReceiver::DoProcessCommand(Platform::String^ doProcessCmd)

{


	auto message = ref new ValueSet();

	//if (m_toggle)
	//	message->Insert(L"GPIO.Command", "GPO.5 = 1; GPO.7 = 1;GPO.18 = 1");
	//else
	//	message->Insert(L"GPIO.Command", "GPO.5 = 0; GPO.7 = 0;GPO.18 = 0");
	message->Insert(L"GPIO.Command", doProcessCmd);
	//m_toggle = !m_toggle;
	auto messageTask = Concurrency::create_task(m_serviceConnection->SendMessageAsync(message));
	messageTask.then([this](AppServiceResponse ^appServiceResponse)

	{

		if ((appServiceResponse->Status == AppServiceResponseStatus::Success) && (appServiceResponse->Message != nullptr))
		{
			if (appServiceResponse->Message->HasKey(L"Response")) {
				auto response = appServiceResponse->Message->Lookup(L"Response");

				String^ responseMessage = safe_cast<String^>(response);

				if (String::operator==(responseMessage, L"Success"))
				{

					if (appServiceResponse->Message->HasKey(L"GPIO.State")) {
						auto response = appServiceResponse->Message->Lookup(L"GPIO.State");

						String^ responseMessage = safe_cast<String^>(response);
						m_pGPIOClientInOut->DoProcessCommand(responseMessage);
						SendData(responseMessage);
					//	int value = m_pGPIOClientInOut->getGPIOPinValueByIdx(23);
					//	if (value>0) SendData("GPIOServiceClient.Movement=1"); // send data back
					//	else SendData("GPIOServiceClient.Movement=0"); // send data back
					}

				}
			}


		}

	});

}


void GPIOChunkReceiver::DoProcessChunk(DataReader^ reader)
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
				/*
				int64 time = cv::getTickCount();
				m_DeltaChunkReceived = time - m_StartChunkReceived;
				m_StartChunkReceived = time;
*/

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

void GPIOChunkReceiver::ReceiveChunkLoop(DataReader^ reader, StreamSocket^ socket)
{
	// Read first 4 bytes (length of the subsequent string).
	setSocket(socket);
	auto token = m_pSocketCancelTaskToken->get_token();

	unsigned int readBufferLen = m_ReadBufferLen;
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
		catch (Exception^ exception)
		{
			auto lock = m_lock.LockExclusive();
			this->getSocketEventSource()->fire(socket, NotificationEventReason::CanceledbyError);
			m_socket = nullptr;
			cancel_current_task();

		}


		catch (task_canceled&)
		{
			auto lock = m_lock.LockExclusive();
			this->getSocketEventSource()->fire(socket, NotificationEventReason::CanceledbyUser);
			m_socket = nullptr;
			delete socket;
		}

		catch (...)
		{
			auto lock = m_lock.LockExclusive();
			this->getSocketEventSource()->fire(socket, NotificationEventReason::CanceledbyUser);
			m_socket = nullptr;
			delete socket;

		}
	});



}
/*void GPIOChunkReceiver::ReceiveChunkLoop(DataReader^ reader, StreamSocket^ socket)
{
	setSocket(socket);
	// Read first 4 bytes (length of the subsequent string).
	auto token = m_pSocketCancelTaskToken->get_token();

	//	create_task(reader->LoadAsync(len)).then([this, reader, socket](task<unsigned int> size) {

	task_from_result().then([this, reader, socket]() {
		//	unsigned int len = CHUNK_LENGTH_FIELD_LEN;
		unsigned int readBufferLen = CHUNK_LENGTH_FIELD_LEN;
		readBufferLen = m_ReadBufferLen;
		//	readBufferLen = 4096;
		return reader->LoadAsync(readBufferLen);
	}, token).then([this, reader, socket](task<unsigned int> size) {

		try {

			unsigned int readlen = 0;
			readlen = size.get();
			if (readlen == 0) {

				cancel_current_task();
			}
			else {

				DoProcessChunk(reader);

				ReceiveChunkLoop(reader, socket);
			}

		}
		catch (Exception^ exception)
		{
			auto lock = m_lock.LockExclusive();
			this->getSocketEventSource()->fire(socket, NotificationEventReason::CanceledbyError);
			m_socket = nullptr;
			cancel_current_task();

		}


		catch (task_canceled&)
		{
			auto lock = m_lock.LockExclusive();
			this->getSocketEventSource()->fire(socket, NotificationEventReason::CanceledbyUser);
			m_socket = nullptr;
			delete socket;
		}

		catch (...)
		{
			auto lock = m_lock.LockExclusive();
			this->getSocketEventSource()->fire(socket, NotificationEventReason::CanceledbyUser);
			m_socket = nullptr;
			delete socket;

		}
	});



}
*/

void GPIOChunkReceiver::SendData(Platform::String^ info)
{


	if (info->Length() == 0) return;


	auto lock = m_lock.LockExclusive();

	if (m_socket == nullptr) return;

	Windows::Storage::Streams::IBuffer^ buffer = createBufferfromSendData(info);

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
			//this->CancelAsyncSocketOperation();
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
			this->getSocketEventSource()->fire(m_socket, NotificationEventReason::CanceledbyError);
		//	delete m_socket;
			m_socket = nullptr;
			return;
		}
		catch (Exception^ exception) {
			this->getSocketEventSource()->fire(m_socket, NotificationEventReason::CanceledbyError);
	//		delete m_socket;
			m_socket = nullptr;
			return;
		}

		catch (...)
		{
			this->getSocketEventSource()->fire(m_socket, NotificationEventReason::CanceledbyError);
	//		delete m_socket;
			m_socket = nullptr;
			return;
		}

	});
}

Windows::Storage::Streams::IBuffer^ GPIOChunkReceiver::createBufferfromSendData(Platform::String^ stringToSend) {


	DataWriter^ writer = ref new DataWriter();
	// Write first the length of the string a UINT32 value followed up by the string. The operation will just store 
	// the data locally.
//	Platform::String^ stringToSend = StringFromAscIIChars((char*)stringinfo.c_str());

	writer->WriteUInt32(writer->MeasureString(stringToSend));
	writer->WriteByte(0x55);	// CheckByte 1 for verification
	writer->WriteByte(0x55);	// CheckByte 2
	writer->WriteString(stringToSend);

	return writer->DetachBuffer();

}



