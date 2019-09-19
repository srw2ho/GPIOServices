#pragma once
#ifndef BME280IODRIVERWRAPPER_H_
#define BME280IODRIVERWRAPPER_H_

#include <BME280Driver.h>



namespace GPIOService
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
		bool m_I2CError;
		ProcessingReadingModes m_ProcessReadingMode;
	public:
		BME280IoTDriverWrapper(uint8_t dev_id);
		virtual ~BME280IoTDriverWrapper();

		bool IsInitialized() { return m_bInitialized; };
		void SetInitialized(bool isInit) { m_bInitialized = isInit; };
		bool IsI2CError() {	return m_I2CError;};
		void setI2CError (bool Error) { m_I2CError = Error;	};

		int InitProcessingMode();
		concurrency::task<bool>  Initi2cDevice();
		concurrency::task<bool>  Initi2cDeviceWithRecovery();
		Windows::Devices::I2c::I2cDevice^  geti2cDevice() { return m_i2cDevice; };
		void setReadValuesProcessingMode(ProcessingReadingModes mode) { m_ProcessReadingMode = mode; };
		ProcessingReadingModes getReadValuesProcessingMode() { return m_ProcessReadingMode; };

		bool IsDeviceConnected();
	//	bme280_com_fptr_t m_readFkt;	// callback funktion fo winIoT-Driver
	//	bme280_com_fptr_t m_writeFkt;	// callback funktion fo winIoT-Driver

		//bool InitDevice(Windows::Devices::I2c::I2cController ^ cController);


	protected:

	};
}

#endif /**/