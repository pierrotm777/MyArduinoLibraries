////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//     This software is supplied under the terms of a license agreement or    //
//     nondisclosure agreement with Mitov Software and may not be copied      //
//     or disclosed except in accordance with the terms of that agreement.    //
//         Copyright(c) 2002-2015 Mitov Software. All Rights Reserved.        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _MITOV_ETHERNET_SHIELD_h
#define _MITOV_ETHERNET_SHIELD_h

#include <Mitov.h>
#include <Mitov_BasicEthernet.h>

namespace Mitov
{
	class BasicEthernetSocket;
//---------------------------------------------------------------------------
	class EthernetShield : public OpenWire::Component
	{
		typedef OpenWire::Component inherited;

	public:
		Mitov::SimpleList<BasicEthernetSocket*>	Sockets;

	public:
		class TMACAddress
		{
		public:
			byte FMacAddress[6];

		public:
			TMACAddress(
				 uint8_t mac_0, 
				 uint8_t mac_1, 
				 uint8_t mac_2, 
				 uint8_t mac_3, 
				 uint8_t mac_4, 
				 uint8_t mac_5 )
			{
				FMacAddress[ 0 ] = mac_0;
				FMacAddress[ 1 ] = mac_1;
				FMacAddress[ 2 ] = mac_2;
				FMacAddress[ 3 ] = mac_3;
				FMacAddress[ 4 ] = mac_4;
				FMacAddress[ 5 ] = mac_5;
			}

		};

	protected:
		class BasicShieldIPAddress
		{
		public:
			bool		Enabled;
			IPAddress	IP;

		public:
			BasicShieldIPAddress() : Enabled( false ) {}

		};

		class ShieldGatewayAddress : public BasicShieldIPAddress
		{
		public:
			BasicShieldIPAddress	Subnet;
		};

		class ShieldDNSAddress : public BasicShieldIPAddress
		{
		public:
			ShieldGatewayAddress	Gateway;
		};

		class ShieldIPAddress : public BasicShieldIPAddress
		{
		public:
			ShieldDNSAddress	DNS;
		};

	protected:
		TMACAddress FMacAddress;

	public:
		ShieldIPAddress	IPAddress;
		bool	UseDHCP;
		bool	Enabled;

	public:
		void SetEnabled( bool AValue )
		{
            if( Enabled == AValue )
                return;

            Enabled = AValue;
			if( Enabled )
				StartEthernet();

			else
				StopEthernet();

		}

	protected:
		virtual void SystemInit()
		{
			if( Enabled )
				StartEthernet();

			inherited::SystemInit();
		}

		void StopEthernet();

		void StartEthernet()
		{
//			int AIndex = ((int)Parity) * 2 * 4 + ( StopBits - 1 ) + ( DataBits - 5);
//			T_SERIAL->begin( Speed );

//			Serial.println( "StartEthernet" );

			if( ! IPAddress.Enabled )
				Ethernet.begin( FMacAddress.FMacAddress );

			else
			{
				if( UseDHCP )
					if( Ethernet.begin( FMacAddress.FMacAddress ))
						return;

				if( ! IPAddress.DNS.Enabled )
				{
/*
					Serial.println( "StartEthernet IP" );
					Serial.print( FMacAddress.FMacAddress[ 0 ] );
					Serial.print( FMacAddress.FMacAddress[ 1 ] );
					Serial.print( FMacAddress.FMacAddress[ 2 ] );
					Serial.print( FMacAddress.FMacAddress[ 3 ] );
					Serial.println();
					IPAddress.IP.printTo( Serial );
					Serial.println();
*/
					Ethernet.begin( FMacAddress.FMacAddress, IPAddress.IP );
				}

				else
				{
					if( ! IPAddress.DNS.Gateway.Enabled )
						Ethernet.begin( FMacAddress.FMacAddress, IPAddress.IP, IPAddress.DNS.IP );

					else
					{
					if( ! IPAddress.DNS.Gateway.Subnet.Enabled )
						Ethernet.begin( FMacAddress.FMacAddress, IPAddress.IP, IPAddress.DNS.IP, IPAddress.DNS.Gateway.IP );

					else
						Ethernet.begin( FMacAddress.FMacAddress, IPAddress.IP, IPAddress.DNS.IP, IPAddress.DNS.Gateway.IP, IPAddress.DNS.Gateway.Subnet.IP );
					}
				}
			}
		}

        void RestartEthernet()
		{
			if( ! Enabled )
				return;

//			T_SERIAL->end();
//			Ethernet.end();
			StartEthernet();
		}

	public:
		EthernetShield( TMACAddress AMacAddress ) :
			Enabled( true ),
			FMacAddress( AMacAddress ),
			UseDHCP( false )
		{
		}

		EthernetShield( TMACAddress AMacAddress, bool UseDHCP, ::IPAddress local_ip) :
			Enabled( true ),
			FMacAddress( AMacAddress ),
			UseDHCP( UseDHCP )
		{
			IPAddress.Enabled = true;
			IPAddress.IP = local_ip;
		}

		EthernetShield( TMACAddress AMacAddress, bool UseDHCP, ::IPAddress local_ip, ::IPAddress dns_server) :
			Enabled( true ),
			FMacAddress( AMacAddress ),
			UseDHCP( UseDHCP )
		{
			IPAddress.Enabled = ! UseDHCP;
			IPAddress.IP = local_ip;
			IPAddress.DNS.Enabled = true;
			IPAddress.DNS.IP = dns_server;
		}

		EthernetShield( TMACAddress AMacAddress, bool UseDHCP, ::IPAddress local_ip, ::IPAddress dns_server, ::IPAddress gateway) :
			Enabled( true ),
			FMacAddress( AMacAddress ),
			UseDHCP( UseDHCP )
		{
			IPAddress.Enabled = ! UseDHCP;
			IPAddress.IP = local_ip;
			IPAddress.DNS.Enabled = true;
			IPAddress.DNS.IP = dns_server;
			IPAddress.DNS.Gateway.Enabled = true;
			IPAddress.DNS.Gateway.IP = dns_server;
		}

		EthernetShield( TMACAddress AMacAddress, bool UseDHCP, ::IPAddress local_ip, ::IPAddress dns_server, ::IPAddress gateway, ::IPAddress subnet) :
			Enabled( true ),
			FMacAddress( AMacAddress ),
			UseDHCP( UseDHCP )
		{
			IPAddress.Enabled = ! UseDHCP;
			IPAddress.IP = local_ip;
			IPAddress.DNS.Enabled = true;
			IPAddress.DNS.IP = dns_server;
			IPAddress.DNS.Gateway.Enabled = true;
			IPAddress.DNS.Gateway.IP = dns_server;
			IPAddress.DNS.Gateway.Subnet.Enabled = true;
			IPAddress.DNS.Gateway.Subnet.IP = dns_server;
		}
	};
//---------------------------------------------------------------------------
	class BasicEthernetSocket : public Mitov::BasicSocket
	{
		typedef Mitov::BasicSocket inherited;

	protected:
		EthernetShield &FShield;

	public:
		virtual bool IsEnabled()
		{
			return Enabled && FShield.Enabled;
		}

		virtual Print *GetPrint() = 0;

	public:
		BasicEthernetSocket( EthernetShield &AShield ) :
			FShield( AShield )
		{
			AShield.Sockets.push_back( this );
		}

	};
//---------------------------------------------------------------------------
	class TCPClientSocket : public BasicEthernetSocket
	{
		typedef BasicEthernetSocket inherited;

	public:
		String		URL;
		::IPAddress	IPAddress;

	protected:
		EthernetClient FClient;

	protected:
		virtual void StartSocket()
		{
//			Serial.println( "StartSocket" );
			if( URL.length() )
				FClient.connect( URL.c_str(), Port );

			else
			{
//				IPAddress.printTo( Serial );
				FClient.connect( IPAddress, Port );
			}
		}

		virtual void StopSocket()
		{
			FClient.stop();
		}

	public:
		virtual Print *GetPrint()
		{
			return &FClient;
		}

	public:
		virtual void SystemLoopBegin( unsigned long currentMicros ) 
		{
			if ( FClient.available() )
			{
				unsigned char AByte = FClient.read();
				OutputPin.Notify( &AByte );
			}

			if (! FClient.connected()) 
				FClient.stop(); // Do we need this?

			inherited::SystemLoopBegin( currentMicros );
		}

	public:
		TCPClientSocket( EthernetShield &AShield, ::IPAddress AIPAddress ) :
			inherited( AShield ),
			IPAddress( AIPAddress )
		{
		}
	};
//---------------------------------------------------------------------------
	class TCPServerSocket : public BasicEthernetSocket
	{
		typedef BasicEthernetSocket inherited;

	protected:
		EthernetServer	*FServer;
		EthernetClient	FClient;

	protected:
		virtual void StartSocket()
		{
//			Serial.println( "StartSockect" );
			if( FServer )
				return;

//			Serial.println( Port );
			FServer = new EthernetServer( Port );
			FServer->begin();
//			Serial.println( "Start Server Sockect" );
		}

		virtual void SystemLoopBegin( unsigned long currentMicros ) 
		{
			if( FServer )
			{
				if( ! FClient )
				{
					FClient = FServer->available();

//					if( FClient )
//						Serial.println( "CLIENT" );

				}

/*
				if( FClient )
 					if (!FClient.connected()) 
					{
						Serial.println( "STOP" );
						FClient.stop(); // Do we need this?
					}
*/
				if( FClient )
				{
//					Serial.println( "CLIENT" );
					if( FClient.available() )
					{
						unsigned char AByte = FClient.read();
						OutputPin.Notify( &AByte );
					}

 					if( ! FClient.connected() ) 
					{
//						Serial.println( "STOP" );
						FClient.stop(); // Do we need this?
					}
				}
			}

			inherited::SystemLoopBegin( currentMicros );
		}

	public:
		virtual void StopSocket()
		{
			if( FServer )
			{
				delete FServer;
				FServer = NULL;
			}

		}

		virtual Print *GetPrint()
		{
			return FServer;
		}

	public:
		TCPServerSocket( EthernetShield &AShield ) :
			inherited( AShield ),
			FServer( NULL )
		{
		}

		virtual ~TCPServerSocket()
		{
			if( FServer )
				delete FServer;
		}
	};
//---------------------------------------------------------------------------
	class UDPSocket : public BasicEthernetSocket
	{
		typedef BasicEthernetSocket inherited;

	public:
		unsigned int	RemotePort;
		::IPAddress		RemoteIPAddress;

	protected:
		EthernetUDP FSocket;

	protected:
		virtual void StartSocket()
		{
			FSocket.begin( Port );
		}

	public:
		virtual void BeginPacket()
		{
			FSocket.beginPacket( RemoteIPAddress, RemotePort );
		}

		virtual void EndPacket()
		{
			FSocket.endPacket();
		}

		virtual void StopSocket()
		{
			FSocket.stop();
		}

		virtual Print *GetPrint()
		{
			return &FSocket;
		}

	public:
		virtual void SystemLoopBegin( unsigned long currentMicros ) 
		{
			if ( FSocket.available() )
			{
				unsigned char AByte = FSocket.read();
				OutputPin.Notify( &AByte );
			}

			inherited::SystemLoopBegin( currentMicros );
		}

	public:
		UDPSocket( EthernetShield &AShield, ::IPAddress ARemoteIPAddress ) :
			inherited( AShield ),
			RemoteIPAddress( ARemoteIPAddress )
		{
		}
	};
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void EthernetShield::StopEthernet()
{
	for( int i = 0; i < Sockets.size(); ++i )
		Sockets[ i ]->StopSocket();
}
//---------------------------------------------------------------------------
}

#endif
