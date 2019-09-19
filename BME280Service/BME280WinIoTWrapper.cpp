#include "pch.h"
//#include "bme280.h"

#include "BME280WinIoTWrapper.h"



using namespace Platform;
using namespace Windows::ApplicationModel::AppService;
using namespace Microsoft::IoT::Lightning::Providers;

using namespace Windows::Devices;


using namespace Windows::Devices::Pwm;
using namespace Windows::Devices::Gpio;
using namespace Windows::Devices::I2c;

using namespace Windows::Foundation;
using namespace concurrency;
using namespace Windows::System::Threading;
using namespace Windows::Foundation::Collections;

using namespace Windows::Devices::Enumeration;


using namespace Windows::Devices::I2c;

using namespace Windows::Foundation;

using namespace Windows::Foundation::Collections;

using namespace Windows::UI::Core;

using namespace Windows::UI::Xaml;

using namespace Windows::UI::Xaml::Controls;

using namespace Windows::UI::Xaml::Controls::Primitives;

using namespace Windows::UI::Xaml::Data;

using namespace Windows::UI::Xaml::Input;

using namespace Windows::UI::Xaml::Media;

using namespace Windows::UI::Xaml::Navigation;

using namespace Windows::Storage::Streams;

namespace BME280Service
{
	BME280WinIoTWrapper*pGlobBME280WinIoTWrapper = nullptr;



	int8_t BME280WinIoTWrapper::user_i2c_write(uint8_t id, uint8_t reg_addr, uint8_t *data, uint16_t len) {
		/*
		write(fd, &reg_addr, 1);
		read(fd, data, len);
		return 0;
		*/
		int8_t ret = -1;
		if (pGlobBME280WinIoTWrapper != nullptr) {
			BME280Service::BME280IoTDriverWrapper* pi2TDevice = pGlobBME280WinIoTWrapper->getDeviceById(id);
			if (pi2TDevice != nullptr) {
				Windows::Devices::I2c::I2cDevice^ i2cDevice = pi2TDevice->geti2cDevice();
				if (i2cDevice != nullptr) {

					auto recdata = ref new Array<byte>(len);
					try {
						i2cDevice->Read(recdata);



						for (size_t i = 0; i < recdata->Length; i++)
						{
							data[i] = recdata[i];
						}

						ret = 0;
					}
					catch (Exception^ exception)
					{
						ret = -1;
					}
				}
			}

		}

		return ret;
	}

	int8_t BME280WinIoTWrapper::user_i2c_read(uint8_t id, uint8_t reg_addr, uint8_t *data, uint16_t len)
	{
		/*
		int8_t *buf;
		buf = malloc(len + 1);
		buf[0] = reg_addr;
		memcpy(buf + 1, data, len);
		write(fd, buf, len + 1);
		free(buf);
		return 0;
		*/
		int8_t ret = -1;
		if (pGlobBME280WinIoTWrapper != nullptr) {
			BME280Service::BME280IoTDriverWrapper* pi2TDevice = pGlobBME280WinIoTWrapper->getDeviceById(id);
			if (pi2TDevice != nullptr) {
				Windows::Devices::I2c::I2cDevice^ i2cDevice = pi2TDevice->geti2cDevice();
				if (i2cDevice != nullptr) {
					/*
					std::vector<unsigned char> writedata(len);
					int i = 0;
					while (i < len)
					{
						writedata[i] = *(data + i);
						i++;
					}
					*/
					auto writedata = ref new Array<byte>(len);

					int i = 0;
					while (i < len)
					{
						writedata[i] = *(data + i);
						i++;
					}

					try {

						i2cDevice->Write(writedata);

						//				i2cDevice->Write(::Platform::ArrayReference<unsigned char>(&writedata[0], (unsigned int)writedata.size()));
						ret = 0;
					}
					catch (Exception^ exception)
					{
						ret = -1;
					}
					// i2

				}
			}

		}

		return ret;
	}
	void BME280WinIoTWrapper::user_delay_ms(uint32_t period)
	{
		//usleep(period * 1000);
		Sleep(period);// Sleep in ms
	}



	
	BME280WinIoTWrapper::BME280WinIoTWrapper(BME280Service::BME280Listener^ listener)
	{
		//m_packetQueue = new std::vector<OpenCVPackage*>();
		m_pBMEDataQueue = new BME280Service::BME280DataQueue();
		InitializeCriticalSection(&m_CritLock);
		m_pBME280IoTDrivers = new std::vector<BME280Service::BME280IoTDriverWrapper*>();
		pGlobBME280WinIoTWrapper = this;
		m_pCancelTaskToken = new concurrency::cancellation_token_source();
		m_bProcessingReadValuesStarted = false;
		m_ProcessReadingMode = ProcessingReadingModes::Normal;
		m_pBME280Listener = listener;
	}


	BME280WinIoTWrapper::~BME280WinIoTWrapper()
	{
		this->FlushDrivers();
		delete m_pBME280IoTDrivers;
		pGlobBME280WinIoTWrapper = nullptr;
		
		delete m_pCancelTaskToken;
	//	m_pBMEDataQueue->
		m_pBMEDataQueue->Flush();
		delete m_pBMEDataQueue;
		DeleteCriticalSection(&m_CritLock);
	}

	int BME280WinIoTWrapper::InitDevice(BME280Service::BME280IoTDriverWrapper*pDriver) {

		Lock();
		pDriver->setReadValuesProcessingMode(getReadValuesProcessingMode());// taking from father-collection
		pDriver->setReadFkt(BME280WinIoTWrapper::user_i2c_read);		// Read-Callback-Funktion
		pDriver->setWriteFkt(BME280WinIoTWrapper::user_i2c_write);		// Write-Callback-Funktion
		pDriver->setDelayFkt(BME280WinIoTWrapper::user_delay_ms);		// Delay- Callback-Funktion

		int binit = pDriver->Initialization();
		if (binit == BME280_OK)
		{
			binit = pDriver->InitProcessingMode();
		}
		if (binit == BME280_OK) {
			this->m_pBME280IoTDrivers->push_back(pDriver);
		}
		else
		{
			delete pDriver;
		}
		UnLock();

		return binit;

	}



	void BME280WinIoTWrapper::AddDriverwithRecoverDevice(unsigned char adress) {
	
		BME280Service::BME280IoTDriverWrapper*pDriver = new BME280Service::BME280IoTDriverWrapper(adress);
		
	

		create_task(pDriver->Initi2cDeviceWithRecovery()).then([this, pDriver](task<bool> value) {

			try {
				bool ok = value.get();
				if (ok)
				{
					InitDevice(pDriver);
				}
			}
			catch (...)
			{

			}
		});
		
	}

	void BME280WinIoTWrapper::AddDriver(unsigned char adress) {

		BME280Service::BME280IoTDriverWrapper*pDriver = new BME280Service::BME280IoTDriverWrapper(adress);


		create_task(pDriver->Initi2cDevice()).then([this, pDriver](task<bool> value) {

			try {
				bool ok = value.get();
				if (ok)
				{
					InitDevice(pDriver);
				}
			}
			catch (...)
			{

			}
		});


	}


	int8_t BME280WinIoTWrapper::stream_sensor_data_normal_mode() {
		BME280Driver::BME280IoTDriver* pBME280IoTDriver = getDeviceById(BME280_I2C_ADDR_PRIM);
		if (pBME280IoTDriver != nullptr)
		{
			pBME280IoTDriver->stream_sensor_data_normal_mode();
		}
		return 0;
	}

	BME280Service::BME280IoTDriverWrapper* BME280WinIoTWrapper::getDeviceById(unsigned char adress) {

		BME280Service::BME280IoTDriverWrapper* ret = nullptr;

		this->Lock();
		for (size_t i = 0; i < m_pBME280IoTDrivers->size(); i++)
		{
			BME280Service::BME280IoTDriverWrapper*pDriver = m_pBME280IoTDrivers->at(i);
			if (pDriver->getDeviceId() == adress)
			{
				ret = pDriver;
				break;
			}
		}
		this->UnLock();

		return ret;

	}

	void BME280WinIoTWrapper::Lock() {
		EnterCriticalSection(&m_CritLock);
	}

	void BME280WinIoTWrapper::UnLock() {
		LeaveCriticalSection(&m_CritLock);
	}

	void BME280WinIoTWrapper::FlushDrivers()
	{
		this->Lock();
		while (!m_pBME280IoTDrivers->empty())
		{
			BME280Service::BME280IoTDriverWrapper*pDriver = m_pBME280IoTDrivers->front();
			m_pBME280IoTDrivers->erase(m_pBME280IoTDrivers->begin() + 0);
			delete pDriver;
		}
		this->UnLock();
	};




	Windows::Storage::Streams::IBuffer^ BME280WinIoTWrapper::createBufferforSendData() {


		DataWriter^ writer = ref new DataWriter();
		// Write first the length of the string a UINT32 value followed up by the string. The operation will just store 
		// the data locally.
	//	Platform::String^ stringToSend = StringFromAscIIChars((char*)stringinfo.c_str());

		writer->WriteUInt32(10);
		writer->WriteByte(0x55);	// CheckByte 1 for verification
		writer->WriteByte(0x55);	// CheckByte 2
		writer->WriteString(L"");

		return writer->DetachBuffer();

	}

	Concurrency::task<void> BME280WinIoTWrapper::doProcessReadValues()
	{
		auto token = m_pCancelTaskToken->get_token();
		//	create_async([this, cvImage](concurrency::cancellation_token token) -> void


		auto tsk = create_task([this, token]() -> void

		{
			bool dowhile = true;

			while (dowhile) {
				try {

					Lock();
					for (size_t i = 0; i < this->m_pBME280IoTDrivers->size(); i++) {
						BME280Service::BME280IoTDriverWrapper*pDevice = this->m_pBME280IoTDrivers->at(i);
						int state;
						if (m_ProcessReadingMode == ProcessingReadingModes::Normal) {
							state = pDevice->ReadSensorDataIntoNormalMode();
						}
						else	if (m_ProcessReadingMode == ProcessingReadingModes::Force) {
							state = pDevice->ReadSensorDataIntoForcedMode();
						}
						if (state == BME280_OK) {
							// wrote Values to Queue
							BME280DataPackage* ppacket = new BME280DataPackage(pDevice->getDeviceId(), pDevice->getPressure(), pDevice->getTemperature(), pDevice->getHumidity());
							m_pBMEDataQueue->PushPacket(ppacket);
					
						}
					}
					UnLock();
					
					//createBufferforSendData();
					//m_pBME280Listener->SendDataToClients();

					if (token.is_canceled()) {
						cancel_current_task();
					}
					else Sleep(500);

				}
				catch (task_canceled&)
				{

					dowhile = false;

				}
				catch (Exception^ exception	)
				{
					dowhile = false;


				}

			}



		}, token);

		return tsk;
	}

	void BME280WinIoTWrapper::startProcessingReadValues()
	{


		if (m_pCancelTaskToken != nullptr)
		{
			delete m_pCancelTaskToken;

		}
		m_pCancelTaskToken = new concurrency::cancellation_token_source();


		m_bProcessingReadValuesStarted = false;
		m_ProcessingReadValuesTsk = create_task(doProcessReadValues()).then([this](task<void> previous)
		{
			m_bProcessingReadValuesStarted = false;
			try {
				previous.get();
			}
			catch (Exception^ exception)
			{

			}

		});


	}

	void BME280WinIoTWrapper::stopProcessingReadValues()
	{
		if (!m_bProcessingReadValuesStarted) return;
		try {

			if (m_pCancelTaskToken != nullptr) {
				m_pCancelTaskToken->cancel();
			}
			Sleep(100);
			m_ProcessingReadValuesTsk.wait();
		}
		catch (Exception^ exception)
		{
			bool b = true;

		}


	}




}