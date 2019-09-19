#pragma once
#include <BME280Driver.h>
#include "BME280DataQueue.h"
#include "BME280IoTDriverWrapper.h"
#include "BME280Listener.h"

namespace BME280Service
{



class BME280WinIoTWrapper
{

	BME280Service::BME280Listener^ m_pBME280Listener;
	std::vector<BME280Service::BME280IoTDriverWrapper*>* m_pBME280IoTDrivers;
	CRITICAL_SECTION m_CritLock;
	concurrency::cancellation_token_source* m_pCancelTaskToken;

	concurrency::task<void> m_ProcessingReadValuesTsk;
	bool m_bProcessingReadValuesStarted;
	ProcessingReadingModes m_ProcessReadingMode;
	
	BME280Service::BME280DataQueue* m_pBMEDataQueue;
public:
	BME280WinIoTWrapper(BME280Service::BME280Listener^);
	virtual ~BME280WinIoTWrapper();

	void setReadValuesProcessingMode(ProcessingReadingModes mode) { m_ProcessReadingMode = mode; };
	ProcessingReadingModes getReadValuesProcessingMode() { return m_ProcessReadingMode; };
	void AddDriver(unsigned char adress);
	void AddDriverwithRecoverDevice(unsigned char adress);
	void FlushDrivers();
	BME280Service::BME280IoTDriverWrapper* BME280WinIoTWrapper::getDeviceById(unsigned char adress);
	int8_t stream_sensor_data_normal_mode();


	void startProcessingReadValues();
	void stopProcessingReadValues();

protected:


	int8_t  static user_i2c_write(uint8_t id, uint8_t reg_addr, uint8_t *data, uint16_t len);
	int8_t  static user_i2c_read(uint8_t id, uint8_t reg_addr, uint8_t *data, uint16_t len);
	void static user_delay_ms(uint32_t period);

	Concurrency::task<void> BME280WinIoTWrapper::doProcessReadValues();
	int InitDevice(BME280Service::BME280IoTDriverWrapper*pDriver);

	Windows::Storage::Streams::IBuffer^ createBufferforSendData();
	void Lock();

	void UnLock();



};

}