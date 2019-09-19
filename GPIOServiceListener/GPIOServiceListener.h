#pragma once

#include <vector>
#include <list>
#include <memory>
#include <sstream>

#include <collection.h>
#include <ppltasks.h>
#include <wrl.h>
#include <robuffer.h>

#include "GPIOChunkReceiver.h"

namespace GPIOServiceListener
{
    public ref class GPIOListener sealed
    {
		concurrency::cancellation_token_source* m_pSocketCancelTaskToken;
		Windows::Networking::Sockets::StreamSocketListener^ m_listener;
		int m_port;
		GPIOServiceListener::GPIOChunkReceiver* m_pGPIOChunkReceiver;
		Windows::Foundation::EventRegistrationToken m_OnConnectionReceivedToken;
		Windows::Foundation::EventRegistrationToken m_GPIOChunkReceiverToken;
		Windows::ApplicationModel::AppService::AppServiceConnection ^m_serviceConnection;
		GPIODriver::GPIOClientInOut^ m_pGPIOClientInOut;

	private:
		void OnConnectionReceived(
			_In_ Windows::Networking::Sockets::StreamSocketListener^ sender,
			_In_ Windows::Networking::Sockets::StreamSocketListenerConnectionReceivedEventArgs^ e
		);
		void StartListenerAsync();

		void OnStopStreaming(Windows::Networking::Sockets::StreamSocket^ socket, int status);
    public:

		GPIOListener(GPIODriver::GPIOClientInOut^pGPIOClientInOut,Windows::ApplicationModel::AppService::AppServiceConnection  ^serviceConnection);

		Windows::Foundation::IAsyncAction ^ StartServerAsync(int port);
		void CloseHTTPServerAsync();
		event Windows::Foundation::TypedEventHandler<Platform::Object^, Windows::Networking::Sockets::StreamSocket^  >^ startStreaming;
		event Windows::Foundation::TypedEventHandler<Platform::Object^, int>^ stopStreaming;



    };
}
