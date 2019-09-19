#pragma once
#include <BME280Driver.h>

namespace BME280Service
{

	enum ProcessingReadingModes {
		Normal = 1,
		Force = 1,
	};

	class  BME280IoTDriverWrapper : public BME280Driver::BME280IoTDriver
	{
		Windows::Devices::I2c::I2cController^ m_i2cController;
		Windows::Devices::I2c::I2cDevice^  m_i2cDevice;
		bool m_bInitialized;
		ProcessingReadingModes m_ProcessReadingMode;
	public:
		BME280IoTDriverWrapper(uint8_t dev_id);
		virtual ~BME280IoTDriverWrapper();
		int InitProcessingMode();
		concurrency::task<bool>  Initi2cDevice();
		concurrency::task<bool>  Initi2cDeviceWithRecovery();
		Windows::Devices::I2c::I2cDevice^  geti2cDevice() { return m_i2cDevice; };
		void setReadValuesProcessingMode(ProcessingReadingModes mode) { m_ProcessReadingMode = mode; };

	//	bme280_com_fptr_t m_readFkt;	// callback funktion fo winIoT-Driver
	//	bme280_com_fptr_t m_writeFkt;	// callback funktion fo winIoT-Driver



	protected:

	};
}