#include "pch.h"
#include "StartupTask.h"

using namespace BME280Service;


using namespace Platform;
using namespace Windows::ApplicationModel::Background;





// The Background Application template is documented at http://go.microsoft.com/fwlink/?LinkID=533884&clcid=0x409


StartupTask::StartupTask()
{
	m_pBME280Listener = ref new BME280Service::BME280Listener();
	m_pBME280WinIoTWrapper = new BME280Service::BME280WinIoTWrapper(m_pBME280Listener);



}




void StartupTask::Run(IBackgroundTaskInstance^ taskInstance)
{
    // 
    // TODO: Insert code to perform background work
    //
    // If you start any asynchronous methods here, prevent the task
    // from closing prematurely by using BackgroundTaskDeferral as
    // described in http://aka.ms/backgroundtaskdeferral
    //

	m_serviceDeferral = taskInstance->GetDeferral();

	taskInstance->Canceled += ref new BackgroundTaskCanceledEventHandler(this, &StartupTask::OnTaskCanceled);

	m_pBME280WinIoTWrapper->setReadValuesProcessingMode(ProcessingReadingModes::Normal);
	// Add i2CDriver with recovery searchinf for device-Adress
	m_pBME280WinIoTWrapper->AddDriverwithRecoverDevice(BME280_I2C_ADDR_SEC);
	// Add i2CDriver without recovery searchinf for device-Adress
	m_pBME280WinIoTWrapper->AddDriver(BME280_I2C_ADDR_PRIM);
	
	m_pBME280Listener->CreateListenerServerAsync(3010);
	m_pBME280WinIoTWrapper->startProcessingReadValues(); // Start Processing Read Values

	
}

void StartupTask::StartupTask::OnTaskCanceled(IBackgroundTaskInstance^ sender, BackgroundTaskCancellationReason reason)
{

	if (m_pBME280WinIoTWrapper != nullptr) {

		m_pBME280WinIoTWrapper->stopProcessingReadValues();// wait for finished processing

		m_pBME280WinIoTWrapper->FlushDrivers();

		delete m_pBME280WinIoTWrapper;

	}
	if (m_serviceDeferral != nullptr)
	{
		// Complete the service deferral
		m_serviceDeferral->Complete();
		m_serviceDeferral = nullptr;
	}


}