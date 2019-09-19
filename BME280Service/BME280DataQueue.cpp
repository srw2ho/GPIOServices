#include "pch.h"
#include "BME280DataQueue.h"

#include <windows.h>


//Windows::Foundation::DateTime Time;

namespace BME280Service
{


	BME280DataPackage::BME280DataPackage()
	{
		m_DevId = 0;
		m_pressure = 0;
		/*! Compensated temperature */
		m_temperature = 0;
		/*! Compensated humidity */
		m_humidity = 0;
		m_UnixTime = getActualUnixTime();
	}

	BME280DataPackage::BME280DataPackage(BYTE devID, double pressure, double temperature, double humidity)
	{
		m_DevId = devID;
		m_pressure = pressure;
		/*! Compensated temperature */
		m_temperature = temperature;
		/*! Compensated humidity */
		m_humidity = humidity;
		m_UnixTime = getActualUnixTime();

	}

	BME280DataPackage::~BME280DataPackage()
	{
	}


	BME280DataQueue::BME280DataQueue()
	{
		m_packetQueue = new std::vector<BME280DataPackage*>();
		InitializeCriticalSection(&m_CritLock);
		m_hWriteEvent = CreateEvent(
			NULL,               // default security attributes
			TRUE,               // manual-reset event
			FALSE,              // initial state is nonsignaled
			nullptr
			//TEXT("WriteEvent")  // object name
		);
	}


	BME280DataQueue::~BME280DataQueue()
	{
		this->Flush();
		delete m_packetQueue;
		DeleteCriticalSection(&m_CritLock);
		CloseHandle(m_hWriteEvent);
	}


	void BME280DataQueue::cancelwaitForPacket() {
		::SetEvent(m_hWriteEvent);
	}

	size_t BME280DataQueue::waitForPacket(BME280DataPackage** Packet, DWORD waitTime) {

		this->Lock();
		*Packet = nullptr;
		size_t size = m_packetQueue->size();
		if (size == 0) {//no packet then wait for
			this->UnLock();
			DWORD dwWaitResult = WaitForSingleObject(m_hWriteEvent, // event handle
				waitTime);    // indefinite wait
			if (dwWaitResult == WAIT_OBJECT_0) {}
			this->Lock();
			size = m_packetQueue->size();
			if (size > 0) {
				//	*Packet = m_packetQueue->at(size-1);
				//	DeleteFirstPackets(size - 2);
				*Packet = m_packetQueue->front();
				m_packetQueue->erase(m_packetQueue->begin());
			}
			::ResetEvent(m_hWriteEvent);
			this->UnLock();


		}
		else {
			*Packet = m_packetQueue->front();
			m_packetQueue->erase(m_packetQueue->begin());


			this->UnLock();
		}

		return size;
	};


	void BME280DataQueue::Flush()
	{
		this->Lock();
		while (!m_packetQueue->empty())
		{
			BME280DataPackage* Packet = PopPacket();
			delete Packet;
		}
		this->UnLock();
	};


	void BME280DataQueue::PushPacket(BME280DataPackage* ppacket) {


		this->Lock();
		m_packetQueue->push_back(ppacket);
		::SetEvent(m_hWriteEvent);

		if (m_packetQueue->size() > 1000) { // ersten  löschen, wenn überfüllt
			BME280DataPackage* Packet = PopPacket();
			delete Packet;
		//	unrefPacket((BME280DataPackage*)Packet);
		}

		this->UnLock();

	};

	void BME280DataQueue::DeleteFirstPackets(size_t size)
	{
		this->Lock();
		while (m_packetQueue->size() > size)
		{
			BME280DataPackage* Packet = PopPacket();
			delete Packet;
	
		}
		this->UnLock();
	}

	size_t BME280DataQueue::getPacketSize() {
		size_t size = 0;

		this->Lock();
		size = m_packetQueue->size();
		this->UnLock();
		return size;

	};

	BME280DataPackage* BME280DataQueue::PopPacket() {
		BME280DataPackage*pPacketRet = nullptr;
		this->Lock();
		if (!m_packetQueue->empty())
		{
			pPacketRet = m_packetQueue->front();
			//	avPacket = m_packetQueue.front();
			m_packetQueue->erase(m_packetQueue->begin());

			//		m_packetQueue->pop();
			::ResetEvent(m_hWriteEvent);
		}
		this->UnLock();
		return pPacketRet;
	};

	void BME280DataQueue::Lock() {
		EnterCriticalSection(&m_CritLock);
	}

	void BME280DataQueue::UnLock() {
		LeaveCriticalSection(&m_CritLock);
	}
}