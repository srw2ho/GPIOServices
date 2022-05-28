#include "pch.h"

#include "BME280IoTDriverWrapper.h"


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



namespace GPIOService
{

	//BME280WinIoTWrapper*pGlobBME280WinIoTWrapper = nullptr;


	BME280IoTDriverWrapper::BME280IoTDriverWrapper(uint8_t dev_id)// : BME280IoTDriver(dev_id)
	{

		m_bInitialized = false;
		m_i2cController = nullptr;
		m_i2cDevice = nullptr;
		m_I2CError = false;
		m_i2cController = nullptr;
		m_ProcessReadingMode = ProcessingReadingModes::Normal;
		m_pBME280Driver = new BME280Driver::BME280IoTDriver(dev_id);
		//	m_readFkt = user_i2c_write;
	}


	BME280IoTDriverWrapper::~BME280IoTDriverWrapper()
	{
		if (m_i2cDevice != nullptr) {

			delete m_i2cDevice;
		}

		if (m_i2cController != nullptr) {

			delete m_i2cController;
		}

		if (m_pBME280Driver) {
			delete m_pBME280Driver;
		}
	}

	BME280Driver::BME280IoTDriver* BME280IoTDriverWrapper::getBME280IoTDriver() {

		return m_pBME280Driver;
	};

	bool BME280IoTDriverWrapper::IsInitialized() {
		return m_bInitialized; 
	};
	void BME280IoTDriverWrapper::SetInitialized(bool isInit) {
		m_bInitialized = isInit; 
	};
	bool BME280IoTDriverWrapper::IsI2CError() { 
		return m_I2CError; 
	};
	void BME280IoTDriverWrapper::setI2CError(bool Error) {
		m_I2CError = Error; 
	};


	Windows::Devices::I2c::I2cDevice^ BME280IoTDriverWrapper::geti2cDevice() { return m_i2cDevice; };
	void BME280IoTDriverWrapper::setReadValuesProcessingMode(ProcessingReadingModes mode) { m_ProcessReadingMode = mode; };
	ProcessingReadingModes BME280IoTDriverWrapper::getReadValuesProcessingMode() { return m_ProcessReadingMode; };


	
	int BME280IoTDriverWrapper::InitProcessingMode()
	{
		int state;
		if (m_ProcessReadingMode == ProcessingReadingModes::Normal) {
			state = m_pBME280Driver->setNormalModeSettings();
		}
		else	if (m_ProcessReadingMode == ProcessingReadingModes::Force) {
			state = m_pBME280Driver->setForceModeSettings();
		}

		return state;
	}

	/* this function initialization does not work with LightningProvider */
	
	concurrency::task<bool>  BME280IoTDriverWrapper::Initi2cDeviceWithRecovery()
	{

		if (m_i2cDevice != nullptr) // bereits initialisiert
		{
			return create_task([]
			{
				// Define work here.
				return true;
			});
		}
		else
		{
			/*
			if (Microsoft::IoT::Lightning::Providers::LightningProvider::IsLightningEnabled)
			{
				Windows::Devices::LowLevelDevicesController::DefaultProvider = LightningProvider::GetAggregateProvider();
			}
			*/

			String^ i2cDeviceSelector = I2cDevice::GetDeviceSelector();
			//i2cDeviceSelector = "I2C1";

		//	I2cController controller = (await I2cController.GetControllersAsync(LightningI2cProvider.GetI2cProvider()))[0];
		//	I2cDevice sensor = controller.GetDevice(new I2cConnectionSettings(0x40));

			
			return create_task(DeviceInformation::FindAllAsync(i2cDeviceSelector)).then([this](DeviceInformationCollection^ devices) {


				// 0x40 was determined by looking at the datasheet for the HTU21D sensor

				auto BME280_settings = ref new I2cConnectionSettings(this->m_pBME280Driver->getDeviceId());



				// If this next line crashes with an OutOfBoundsException,

				// then the problem is that no I2C devices were found.

				//

				// If the next line crashes with Access Denied, then the problem is

				// that access to the I2C device (HTU21D) is denied.

				//

				// The call to FromIdAsync will also crash if the settings are invalid.
				if (devices->Size > 0) {
					return I2cDevice::FromIdAsync(devices->GetAt(0)->Id, BME280_settings);
				}
				else
				{// create empty Async-Operatiion
					IAsyncOperation<Windows::Devices::I2c::I2cDevice ^>^ asyncOperation = create_async([devices, BME280_settings]() -> Windows::Devices::I2c::I2cDevice ^
					{
						return nullptr;

					});
					return asyncOperation;
				}


			}).then([this](task<I2cDevice^> i2cDevicetask) {


				try {
					I2cDevice^ i2cDevice = i2cDevicetask.get();;
					if (i2cDevice != nullptr) {
						m_i2cDevice = i2cDevicetask.get();
						return true;
					}
					return false;

				}
				catch (Exception^ exception)
				{
					m_i2cDevice = nullptr;
					return false;
				}
				// i2cDevice will be nullptr if there is a sharing violation on the device.

				// This will result in an access violation in Timer_Tick below.


			});
		}




	}

	


	concurrency::task<bool> BME280IoTDriverWrapper::Initi2cDevice()
	{

		if (m_i2cDevice!=nullptr) // bereits initialisiert
		{
			return create_task([]
			{
				// Define work here.
				return true;
			});
		}
		else
		{

			auto taski2cController = I2cController::GetDefaultAsync();

			return create_task(taski2cController).then([this](task<Windows::Devices::I2c::I2cController^> ctaskController) {
				try {


					I2cController ^ cController = ctaskController.get();
					if (cController != nullptr) {
						m_i2cController = cController;
						auto devID = this->m_pBME280Driver->getDeviceId();
						I2cConnectionSettings^ settings = ref new I2cConnectionSettings(int (devID));
		//				settings->SharingMode = I2cSharingMode::Shared;
						m_i2cDevice = m_i2cController->GetDevice(settings);
	//					m_bInitialized = true;
						return (m_i2cDevice != nullptr);
					}
					return false;
				}
				catch (Exception^ exception)
				{
					return false;
				}
			});
		}




	}

	bool BME280IoTDriverWrapper::IsDeviceConnected()
	{
		/* Initialize the display */
		try
		{
			if (m_i2cDevice == nullptr) return false;

			auto writedata = ref new Array<byte>(1);
			writedata[0] = 0;

			I2cTransferResult result = m_i2cDevice->WritePartial(writedata);
			if (result.Status == I2cTransferStatus::SlaveAddressNotAcknowledged)
			{
				return false;
			}
			else
			if (result.Status == I2cTransferStatus::PartialTransfer)
			{
				return false;
			}
			else if (result.Status == I2cTransferStatus::ClockStretchTimeout)
			{
				return false;
			}

			return true;

		}
		catch (Exception^ exception)
		{
			return false;

		}
		catch (...)
		{
			return false;

		}
	}

}