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

	class  BME280IoTDriverWrapper //: public BME280Driver::BME280IoTDriver
	{
		Windows::Devices::I2c::I2cController^ m_i2cController;
		Windows::Devices::I2c::I2cDevice^  m_i2cDevice;
		bool m_bInitialized;
		bool m_I2CError;
		ProcessingReadingModes m_ProcessReadingMode;

		BME280Driver::BME280IoTDriver* m_pBME280Driver;
	public:
		BME280IoTDriverWrapper(uint8_t dev_id);
		virtual ~BME280IoTDriverWrapper();
		BME280Driver::BME280IoTDriver* getBME280IoTDriver();
		bool IsInitialized();
		void SetInitialized(bool isInit);
		bool IsI2CError();
		void setI2CError(bool Error);

		int InitProcessingMode();
		concurrency::task<bool>  Initi2cDevice();
		concurrency::task<bool>  Initi2cDeviceWithRecovery();
		Windows::Devices::I2c::I2cDevice^ geti2cDevice();
		void setReadValuesProcessingMode(ProcessingReadingModes mode);
		ProcessingReadingModes getReadValuesProcessingMode();

		bool IsDeviceConnected();


	protected:

	};
}

#endif /**/