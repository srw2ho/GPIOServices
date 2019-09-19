#pragma once

namespace BME280Service
{

	class BME280DataPackage {
		BYTE m_DevId;
		double m_pressure;
		/*! Compensated temperature */
		double m_temperature;
		/*! Compensated humidity */
		double m_humidity;
		UINT64 m_UnixTime;
	
	public:
		BME280DataPackage();
		BME280DataPackage(BYTE devID,double pressure, double temperature, double humidity);
		virtual ~BME280DataPackage();
		double getPressure() { return m_pressure; };
		double getTemperature() { return m_temperature; };
		double getHumidity() { return m_humidity; };
		UINT64 getUnixTime() { return m_UnixTime; };
	};



	class BME280DataQueue
	{
	protected:
		HANDLE m_hWriteEvent;

		std::vector<BME280DataPackage*>* m_packetQueue;
		CRITICAL_SECTION m_CritLock;

	public:
		BME280DataQueue();
		virtual ~BME280DataQueue();
	//	virtual void unrefPacket(BME280DataPackage* Packet);


		virtual void cancelwaitForPacket();

		virtual size_t waitForPacket(BME280DataPackage** Packet, DWORD waitTime = INFINITE);


		virtual void Flush();

		virtual void PushPacket(BME280DataPackage* ppacket);

		virtual void DeleteFirstPackets(size_t size);

		virtual size_t getPacketSize();

		virtual BME280DataPackage* PopPacket();
;
		void Lock();

		void UnLock();
	};

}