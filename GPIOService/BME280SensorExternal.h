#pragma once
#ifndef BME280SENSOREXTERNAL_H_
#define BME280SENSOREXTERNAL_H_

#include <map> 
#include "GPIOPin.h"
#include "BME280IoTDriverWrapper.h"

using namespace GPIODriver;




namespace GPIOService
{

	class BME280SensorExternal  : public BME280Sensor
	{

		BME280IoTDriverWrapper* m_pBME280IoTDriverWrapper;

		//std::map<int, GPIOService::BME280SensorExternal*>* m_pMapBME280SensorExternal;

	public:
		BME280SensorExternal(GPIOEventPackageQueue* pGPIOEventPackageQueue, int PinNo, double InitPinValue);


		virtual bool Init(Windows::Devices::Gpio::GpioController ^ GPIOController);

		concurrency::task<bool> InitDriver(unsigned char adress);

		virtual bool doProcessing();


		virtual ~BME280SensorExternal();
		BME280IoTDriverWrapper* getBME280IoTDriverWrapper();

	protected:
		int8_t  static user_i2c_write(uint8_t id, uint8_t reg_addr, uint8_t *data, uint16_t len);
		int8_t  static user_i2c_read(uint8_t id, uint8_t reg_addr, uint8_t *data, uint16_t len);
		void static user_delay_ms(uint32_t period);

		int InitDevice(GPIOService::BME280IoTDriverWrapper*pDriver);
	};



}
#endif /**/