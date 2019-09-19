#pragma once

namespace BME280Service
{

	enum  NotificationEventReason {
		EndRequest = 0,
		CanceledbyUser,
		CanceledbyError
	};



	delegate void NotificationEventhandler(Windows::Networking::Sockets::StreamSocket^ socket, int status);

	// interface that has an event and a function to invoke the event  
	interface struct EventInterface {
	public:
		event NotificationEventhandler ^ eventhandler;
		void fire(Windows::Networking::Sockets::StreamSocket^ socket, int status);
	};


	// class that implements the interface event and function  
	ref class SocketEventSource : public EventInterface {
	public:

		virtual event NotificationEventhandler^ eventhandler;

		virtual void fire(Windows::Networking::Sockets::StreamSocket^ socket, int status)
		{
			eventhandler(socket, status);

		}
	};

}