#include "pch.h"
#include "BME280SensorExternal.h"


using namespace GPIODriver;
using namespace GPIOService;


using namespace Platform;
using namespace Windows::ApplicationModel::AppService;
using namespace Microsoft::IoT::Lightning::Providers;
using namespace concurrency;
using namespace Windows::Devices;
using namespace Windows::Foundation;
using namespace Windows::System::Threading;




namespace GPIOService
{
	typedef std::map<int, GPIOService::BME280SensorExternal*> MapBME280SensorExternal;

	MapBME280SensorExternal globMapBME280SensorExternal;


	BME280SensorExternal::BME280SensorExternal(GPIOEventPackageQueue* pGPIOEventPackageQueue, int PinNo, double InitPinValue) : BME280Sensor(pGPIOEventPackageQueue,PinNo,InitPinValue) {

		m_pBME280IoTDriverWrapper = nullptr;

	}

	BME280SensorExternal::~BME280SensorExternal()
	{
		if (m_pBME280IoTDriverWrapper != nullptr) {
			globMapBME280SensorExternal.erase(m_pBME280IoTDriverWrapper->getBME280IoTDriver()->getDeviceId());
			delete m_pBME280IoTDriverWrapper;
		}

	}

	int BME280SensorExternal::InitDevice(GPIOService::BME280IoTDriverWrapper*pDriver) {
		// only one device has Access to I2C-Bus
		Lock();

		auto devId = pDriver->getBME280IoTDriver()->getDeviceId();
		globMapBME280SensorExternal[devId] = this;
		m_pBME280IoTDriverWrapper = pDriver;

		int binit = !BME280_OK;
		bool bIsConnected = pDriver->IsDeviceConnected();
		if (bIsConnected)
		{
		
			pDriver->getBME280IoTDriver()->setReadFkt(GPIOService::BME280SensorExternal::user_i2c_read);		// Read-Callback-Funktion
			pDriver->getBME280IoTDriver()->setWriteFkt(GPIOService::BME280SensorExternal::user_i2c_write);		// Write-Callback-Funktion
			pDriver->getBME280IoTDriver()->setDelayFkt(GPIOService::BME280SensorExternal::user_delay_ms);		// Delay- Callback-Funktion
			binit = pDriver->getBME280IoTDriver()->Initialization();

			if (binit == BME280_OK)
			{
				binit = pDriver->InitProcessingMode();
			}
		}

		if (binit != BME280_OK)
		{
			m_pBME280IoTDriverWrapper = nullptr;
			globMapBME280SensorExternal.erase(pDriver->getBME280IoTDriver()->getDeviceId());
	
		}
		else
		{
			pDriver->SetInitialized(true);
		}

		UnLock();

		return binit;

	}

	bool BME280SensorExternal::Init(Windows::Devices::Gpio::GpioController ^ GPIOController)
	{
		return false;
	}

	BME280IoTDriverWrapper* BME280SensorExternal::getBME280IoTDriverWrapper() { return m_pBME280IoTDriverWrapper; };


	concurrency::task<bool> BME280SensorExternal::InitDriver(unsigned char adress) {

		GPIOService::BME280IoTDriverWrapper*pDriver = new GPIOService::BME280IoTDriverWrapper(adress);
		Concurrency::task<bool> tsk;
		if (Microsoft::IoT::Lightning::Providers::LightningProvider::IsLightningEnabled)
		{
			Windows::Devices::LowLevelDevicesController::DefaultProvider = LightningProvider::GetAggregateProvider();
			tsk = pDriver->Initi2cDevice();
		}
		else
		{
			tsk = pDriver->Initi2cDeviceWithRecovery();
		}

		create_task(tsk).then([this, pDriver](task<bool> value)->bool {

			try {
				bool ok = value.get();
				if (ok)
				{
					int init = InitDevice(pDriver);
					if (init != BME280_OK) {
						delete pDriver;
					}

					return (init == BME280_OK);
				}
				else
				{
					return false;

				}

			}

			catch (...)
			{
				delete pDriver;
				return false;
			}
		});

		return tsk;
	}


	bool BME280SensorExternal::doProcessing()
	{
		int state = !BME280_OK;
		Lock();
		if (m_pBME280IoTDriverWrapper != nullptr) {
		
			if (m_pBME280IoTDriverWrapper->IsInitialized()) {

				ProcessingReadingModes mode =  m_pBME280IoTDriverWrapper->getReadValuesProcessingMode();
				if (mode = ProcessingReadingModes::Normal) {
					state = m_pBME280IoTDriverWrapper->getBME280IoTDriver()->ReadSensorDataIntoNormalMode();
				}
				else
				{
					state = m_pBME280IoTDriverWrapper->getBME280IoTDriver()->ReadSensorDataIntoForcedMode();
				}

				if (state == BME280_OK) {
					this->m_pressure = m_pBME280IoTDriverWrapper->getBME280IoTDriver()->getPressure();
					this->m_temperature = m_pBME280IoTDriverWrapper->getBME280IoTDriver()->getTemperature();
					this->m_humidity = m_pBME280IoTDriverWrapper->getBME280IoTDriver()->getHumidity();
					this->m_PinValue = 1; // Sensor reading O.k.
				}
				else
				{
					this->m_pressure = -1;
					this->m_temperature = -1;
					this->m_humidity =  - 1;
					this->m_PinValue = -1; // Error
				}
			}

		}
		else this->m_PinValue = -1; // Error


		UnLock();
		return (state == BME280_OK);
	}
	

	int8_t BME280SensorExternal::user_i2c_read(uint8_t id, uint8_t reg_addr, uint8_t* data, uint16_t len) {
		int8_t ret = -1;
		//	if (m_pGlobMapBME280SensorExternal != nullptr) 
		{

			MapBME280SensorExternal::const_iterator it;

			it = globMapBME280SensorExternal.find(id); // Object zum zugehörigen InputPin suchen

			if (it != globMapBME280SensorExternal.end())
			{
				GPIOService::BME280SensorExternal* pBMEExternal = it->second;

				GPIOService::BME280IoTDriverWrapper* pi2TDevice = pBMEExternal->getBME280IoTDriverWrapper();
				if (pi2TDevice != nullptr) {
					if (pi2TDevice->IsI2CError()) return -1;
					Windows::Devices::I2c::I2cDevice^ i2cDevice = pi2TDevice->geti2cDevice();
					if (i2cDevice != nullptr) {

						try {

							auto writedata = ref new Array<byte>(1);
							writedata[0] = reg_addr;
							auto recdata = ref new Array<byte>(len);
							i2cDevice->Write(writedata);
							i2cDevice->Read(recdata);
							for (size_t i = 0; i < len; i++)
							{
								data[i] = recdata[i];
							}

							ret = 0;
						}
						catch (Exception^ exception)
						{
							ret = -1;
							pi2TDevice->setI2CError(true);
						}
						catch (...)
						{
							ret = -1;
							pi2TDevice->setI2CError(true);
						}
					}
				}
			}




		}

		return ret;
	}

	int8_t BME280SensorExternal::user_i2c_write(uint8_t id, uint8_t reg_addr, uint8_t* data, uint16_t len)
	{

		int8_t ret = -1;

		//if (m_pGlobMapBME280SensorExternal != nullptr)
		{


			MapBME280SensorExternal::const_iterator it;

			it = globMapBME280SensorExternal.find(id); // Object zum zugehörigen InputPin suchen

			if (it != globMapBME280SensorExternal.end())
			{
				GPIOService::BME280SensorExternal* pBMEExternal = it->second;

				GPIOService::BME280IoTDriverWrapper* pi2TDevice = pBMEExternal->getBME280IoTDriverWrapper();

				if (pi2TDevice != nullptr) {
					Windows::Devices::I2c::I2cDevice^ i2cDevice = pi2TDevice->geti2cDevice();
					if (i2cDevice != nullptr) {

						try {

							if (pi2TDevice->IsI2CError()) return -1;

							auto writedata = ref new Array<byte>(len + 1);
							writedata[0] = reg_addr;
							int i = 0;
							while (i < len)
							{
								writedata[i + 1] = *(data + i);
								i++;
							}


							i2cDevice->Write(writedata);
							ret = 0;

						}
						catch (Exception^ exception)
						{
							ret = -1;
							pi2TDevice->setI2CError(true);
						}
						catch (...)
						{
							ret = -1;
							pi2TDevice->setI2CError(true);
						}
						// i2

					}
				}

			}








		}

		return ret;
	}
	void BME280SensorExternal::user_delay_ms(uint32_t period)
	{
		//usleep(period * 1000);
		Sleep(period);// Sleep in ms
	}







}