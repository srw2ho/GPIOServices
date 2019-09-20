"# GPIOServices"  subdiveded intto:
- BME280Service
- GPIOService

GPIOService

GPIOService is a Background Application for handling GPIO pins for the Raspberry Pi 3B(+).The service runs in the background and is started when the Pi is started using a schtasks command and runs for the entire runtime. Communication with foreground apps or other background apps takes place via a socket connection (StreamSocket), which runs permanently.
The data interface is implemented using simple text commands.
For example:
// "GPO.1 = 1;" sets output 1 to value 1
// "GPI.3 = 0;" watch for input changings on input-Pin 3  
// "GOO.5 = 0, pulse time=1000;" sets output 5 to value 1 for 1000 ms
// "PWM9685.5 = 0.1, Freq=50;" sets Hardware PWM output 5 to value 0.1  with Frequency = 50, area area is 0 ...1.0 (0...100%)
// "PWM.5 = 0.1, Freq=50;" sets Software PWM output 5 to value 0.1  with Frequency = 50, area area is 0 ...1.0 (0...100%)
// "GPO.1 = 1;" sets output 1 to value 1
// "HC_SR04.5, TrigPin = 20;" HCR-SR04-Senosre for reading input signal on echon-Pin 5 and send Trigger out to pin 20
// "BME280.0x76 = 0;" BME280 sensor command for reading BME280-Sensor for ID 0x76

for using on client-side:
	GPIOHCSR04* pGPIOHCSR04 = new GPIOHCSR04(m_pGPIOEventPackageQueue, 12, 23, 0); // Echo-input = 12, Trigger = 20
	pGPIOHCSR04->setActivateOutputProcessing(true); // set activate output processing
	m_GPIOClientInOut->addGPIOPin(pGPIOHCSR04);

	
  pOutput = new GPIOOutputPin(m_pGPIOEventPackageQueue, 27, 1); // waiting for changing input on Pin 27
	pOutput->setActivateOutputProcessing(true);
	m_GPIOClientInOut->addGPIOPin(pOutput);
  
  pOutput = new GPIOOutputPin(m_pGPIOEventPackageQueue, 21, 1); // 
	pOutput->setActivateOutputProcessing(true);
	m_GPIOClientInOut->addGPIOPin(pOutput);
  
	BME280Sensor* pBME280Sensor = new BME280Sensor(m_pGPIOEventPackageQueue, 0x76,0); // BME280_I2C_ADDR_PRIM
	pBME280Sensor->setActivateOutputProcessing(false); //
	m_GPIOClientInOut->addGPIOPin(pBME280Sensor);
  
  // Send all commands to Server-Side for processing commands
  Platform::String^ state = m_GPIOClientInOut->GetGPIClientSendState(); // Status of all outpus

	Windows::Storage::Streams::IBuffer^ buf = SocketHelpers::createPayloadBufferfromSendData(state);
	m_pServiceListener->SendDataToClients(buf); // sends command-data like "GPO.1 = 1;HC_SR04.5, TrigPin = 20;PWM.5 = 0.1, Freq=50;" to GPIO-Service (Server side)
  
  Arrive information from GPIOService can be received and process as followd
  Concurrency::task<void> StartupTask::doProcessReadValues()
{
	auto token = m_pCancelTaskToken->get_token();
	auto tsk = create_task([this, token]() -> void
	{
		bool dowhile = true;
		DWORD waitTime = 500; // 500 msec waiting
c:\PSSCRIPTS\ on devide		while (dowhile) {
			try {
				GPIOEventPackage* pPacket = nullptr;
				m_pGPIOEventPackageQueue->waitForPacket(&pPacket, waitTime); // incommming GPIOService data will inserted into Processing Queue
				if (pPacket != nullptr)
				{
					GPIOPin* pGPIOPin = (GPIOPin*)  pPacket->getAdditional(); // Pin-Object for processing data
					if (pGPIOPin != nullptr)
					{
  	        if (pGPIOPin->getPinNumber() == 24) {
		          double		value = pGPIOPin->getPinValue() ? 1 : 0; // Relais low aktiv
              // ... doing
           }

					}

					delete pPacket;
				}

				if (token.is_canceled()) {
					cancel_current_task();
				}

			}
			catch (task_canceled&)c:\PSSCRIPTS\ on devide
			{
				dowhile = false;

			}
			catch (Exception^ exception)
			{
				dowhile = false;
			}

		}

	}, token);

	return tsk;
}

LoopBackExempt-Handling for running GPIOService and any GPIOClient on same machine
Loopback for GPIOService (Server-side)
schtasks /create /tn "GPIOServiceLoopBack" /f /ru system /xml "c:\PSSCRIPTS\GPIOServiceLoopBack.xml" 

file "GPIOServiceLoopBack.xml" is also checked in. This file must be copied into folder "c:\PSSCRIPTS\" on devide

// LoopBack for Service for GPIPService (GPIOService-uwp_cctb7d6fhpv4a as family-Package-id)

CheckNetIsolation.exe LoopbackExempt -is -n=GPIOService-uwp_cctb7d6fhpv4a

Please see code GPIOservice and for Client-Using "SharedSources/GPIOServiceConnector" or better "GPIODashboard"
