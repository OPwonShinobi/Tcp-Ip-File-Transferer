#include "WSASocketManager.h"
/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE: WSASocketManager.cpp - An class for handling WinSock2 API calls
--
-- PROGRAM: TcpUdpFileTransfer/PacketLogger
--
-- FUNCTIONS:
		bool CheckIPFormat(const QString&);
		bool SetupSendingByName(const QString&, const QString&, const int, const QString&);
		bool SetupSendingByIp(const QString&, const QString&, const int, const QString&);
		bool SetupReceiving(const QString&, const int, const QString&, size_t = 0);
		void SendPackets(const size_t, const size_t);
		void ReceivePackets();
		void FinishReceivePackets();
		void PrintClientStatus(const QString&);
		void DisplayClientAlert(const QString&);
		QString GetErrorString();		
		bool SetupSocket(const int);
		bool SetupPacketFile(const QString&);
		bool CreateSocket(const QString&);
		bool ConnectToSocket(const int);
		void UpdateTimer(const size_t, const size_t);
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- NOTES:
-- This class uses various functions in the Windows Sockets 2 API to perform setup and validation
-- work with sockets and socket addresses (socket_addr structs). This class acts like a controller
-- between the UI view, and the WinSock2 API, so that the UI class never directly calls any API calls.
-- It also acts as the controller between the main thread, and the client and server threads;
----------------------------------------------------------------------------------------------------------------------*/
#include "WSASocketManager.h"

WSASocketManager::WSASocketManager(QObject *parent)
	: QObject(parent)
{
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	WSAStartup(wVersionRequested, &wsaData);
	qRegisterMetaType<SOCKET>("SOCKET");
	qRegisterMetaType<struct sockaddr_in>("struct sockaddr_in");
	qRegisterMetaType<size_t>("size_t"); //wtf qt? u dont know size_t???

	QThread::currentThread()->setObjectName("mainThread");
    
    serverThread = new QThread; //this does not start the thread 
	serverThread->setObjectName("serverThread");
	
	clientThread = new QThread;
	serverThread->setObjectName("clientThread");

	client = new Client;
	client->moveToThread(clientThread);
	connect(clientThread, &QThread::finished, client, &QObject::deleteLater);
    connect(this, &WSASocketManager::UdpPacketSendSelected, client, &Client::SendUdpPackets);
    connect(this, &WSASocketManager::TcpPacketSendSelected, client, &Client::SendTcpPackets);
	connect(client, &Client::ClientAlertableErrorOccured, this, &WSASocketManager::DisplayClientAlert);
	connect(client, &Client::ClientPrintableStatusReady, this, &WSASocketManager::PrintClientStatus);
	clientThread->start();

	server = new Server;
	server->moveToThread(serverThread);
	connect(serverThread, &QThread::finished, server, &QObject::deleteLater);
	connect(server, &Server::PacketReceived, this, &WSASocketManager::UpdateTimer);
    connect(this, &WSASocketManager::UdpPacketRecvSelected, server, &Server::ReceiveUdpPackets);
    connect(this, &WSASocketManager::TcpPacketRecvSelected, server, &Server::ReceiveTcpPackets);
    connect(this, &WSASocketManager::Disconnected, server, &Server::StopPolling);
	serverThread->start();

}

WSASocketManager::~WSASocketManager()
{
	// recommended way to clean up qt thread
	if (serverThread->isRunning())
	{
		server->StopPolling();
		serverThread->quit(); 
		serverThread->wait();
	}
	if (clientThread->isRunning())
	{
		clientThread->quit(); 
		clientThread->wait();
	}
	//thread objects auto deleted once QThread finished
	delete serverThread;
	delete clientThread;
	WSACleanup();
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION CheckIPFormat
--
-- DATE: Jan 18, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: bool CheckIPFormat (const QString& IPAddrStr)
			const QString& IPAddrStr : string of IPv4 address in dotted decimal format
--
-- RETURNS: bool : whether the passed in argument is a valid IPv4 address string or not
--
-- NOTES:
-- From assignment 1:
--
-- This function attempts to verify if the string passed in is an IP address in dotted decimal format.
-- It converts it to an IPv4 address long usable for the in_addr structure of the WinSock2 API. Then 
-- it returns whether the converted address is equal to the constant INADDR_NONE; all invalid addresses are
-- converted into this constant.
----------------------------------------------------------------------------------------------------------------------*/
bool WSASocketManager::CheckIPFormat(const QString& IPAddrStr)
{
	return inet_addr(IPAddrStr.toStdString().c_str()) != INADDR_NONE;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION ReceivePackets
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: void ReceivePackets()
--
-- NOTES:
-- Called after input from MainWindowController is validated, and in serverMode.
-- This is the starting point of the server thread; 
----------------------------------------------------------------------------------------------------------------------*/
void WSASocketManager::ReceivePackets()
{
	resultPacketSize = 0;
	resultPacketsReceived = 0;
	startTime = std::chrono::steady_clock::now();
	endTime = startTime;
	emit ServerResultsReady(0, 0, "0", protocol); //clear results fields
	if (protocol == "UDP")
	{
		emit UdpPacketRecvSelected(transmit_socket, filePath);
	}	
	if (protocol == "TCP")
	{
		emit TcpPacketRecvSelected(transmit_socket, filePath, expectedPacketSize);
	}
	emit PrintableStatusReady("-Server receiving in background");
	emit DisconnectAllowed(true);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION SendPackets
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: void SendPackets(const size_t packetSize, const size_t packetCount)
--			- packetSize : unsigned int, size of packet to be created 
--			- packetCount : unsigned int, number of times to send packet
--
-- NOTES:
-- Called after input from MainWindowController is validated, and in clientMode.
-- This is the starting point of the client thread; 
----------------------------------------------------------------------------------------------------------------------*/
void WSASocketManager::SendPackets(const size_t packetSize, const size_t packetCount)
{
	if (protocol == "TCP")
	{
		emit TcpPacketSendSelected(transmit_socket, filePath, packetSize, packetCount);
		//(transmit_socket, filePath, packetSize, packetCount); 
	}
	if (protocol == "UDP")
	{
		emit UdpPacketSendSelected(transmit_socket, filePath, packetSize, packetCount, server_socketaddr); 
	}
	emit PrintableStatusReady("-Client sending in background");
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION UpdateTimer
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: void WSASocketManager::UpdateTimer(const size_t updatedPacketSize, const size_t updatedPacketsReceived)
--			- updatedPacketSize : unsigned int, size of packet just received
--			- updatedPacketsReceived : unsigned int, total number of packets received so far during connection
--
-- NOTES:
-- Signaled when Server receives packet and needs to start/update timer.
-- When called with no prior packets, starts the timer, otherwise updates endpoint of timer.
----------------------------------------------------------------------------------------------------------------------*/
void WSASocketManager::UpdateTimer(const size_t updatedPacketSize, const size_t updatedPacketsReceived)
{
	if (resultPacketsReceived == 0)
	{
		startTime = std::chrono::steady_clock::now();
		endTime = startTime;
	}
	//already got some packets
	else
	{
		endTime = std::chrono::steady_clock::now();
	}
	resultPacketSize = updatedPacketSize;
	resultPacketsReceived = updatedPacketsReceived;
	if ( updatedPacketSize == -1 && updatedPacketsReceived == -1) //tcp start timer calls this with -1, an otherwise impossible number
		return;
	long long deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
	emit ServerResultsReady(resultPacketSize, resultPacketsReceived, QString::number(deltaTime), protocol);
	emit PrintableStatusReady("-received packet(s)");
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION FinishReceivePackets
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: void FinishReceivePackets()
--
-- NOTES:
-- Signaled when user clicks Disconnect on MainWindowController. 
-- Stops the loop in server thread, and passes out final packets info to MainWindowController to display.
----------------------------------------------------------------------------------------------------------------------*/
void WSASocketManager::FinishReceivePackets()
{
	server->StopPolling();
	long long deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
	emit PrintableStatusReady("-Receive finished");
	emit ServerResultsReady(resultPacketSize, resultPacketsReceived, QString::number(deltaTime), protocol);
	closesocket(transmit_socket);
	emit DisconnectAllowed(false);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION SetupReceiving(
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: bool SetupReceiving(const QString& protocolStr, const int port, const QString& filePathStr,
			 size_t packetSize)
			 - protocolStr : QString
			 - port : int
			 - filePath : QString
			 - packetSize : unsigned int
--
-- RETURNS: bool : whether all arguments are valid and usable 
--
-- NOTES:
-- Passes arguments off to validate if suitable for server.
-- Setsup socket & filePath;
----------------------------------------------------------------------------------------------------------------------*/
bool WSASocketManager::SetupReceiving(const QString& protocolStr, const int port, const QString& filePathStr, size_t packetSize)
{
	//func only exist to provide public interface to MainWindowController
	//didnt want it to be able
	protocol = protocolStr;
	filePath = filePathStr;
	expectedPacketSize = packetSize;
	return SetupSocket(port) && SetupPacketFile(filePath);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION SetupSendingByName
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: bool SetupSendingByName(const QString& hostName, const QString& protocolStr, const int port, 
		const QString& filePathStr)
			 - hostName : QString
			 - protocolStr : QString
			 - port : int
			 - filePathStr : QString
--
-- RETURNS: bool : whether all arguments are valid and usable 
--
-- NOTES:
-- Call by MainWindowController to check if arguments can be used for client.
-- Checks host as well as Setsup socket & filePath.
----------------------------------------------------------------------------------------------------------------------*/
bool WSASocketManager::SetupSendingByName(const QString& hostName, const QString& protocolStr, const int port, const QString& filePathStr)
{
	host = gethostbyname(hostName.toStdString().c_str());
	protocol = protocolStr;
	filePath = filePathStr;
	if (host == NULL)
	{
		emit AlertableErrorOccured(GetErrorString());
		return false;
	}
	return SetupSocket(port) && SetupPacketFile(filePath);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION SetupSendingByIp
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: bool SetupSendingByIp(const QString& ipAddr,, const QString& protocolStr, const int port, 
		const QString& filePathStr)
			 - ipAddr : QString
			 - protocolStr : QString
			 - port : int
			 - filePathStr : QString
--
-- RETURNS: bool : whether all arguments are valid and usable 
--
-- NOTES:
-- Call by MainWindowController after SetupSendingByName has failed, to check if ip can be used to connect.
-- Checks host as well as Setsup socket & filePath.
----------------------------------------------------------------------------------------------------------------------*/
bool WSASocketManager::SetupSendingByIp(const QString& ipAddr, const QString& protocolStr, const int port, const QString& filePathStr)
{
	struct in_addr *ipv4_addr = (struct in_addr*)malloc(sizeof(struct in_addr));
	ipv4_addr->s_addr = inet_addr(ipAddr.toStdString().c_str());
	host = gethostbyaddr((char*)ipv4_addr, PF_INET, sizeof(struct in_addr));
	free(ipv4_addr); //this is not used anymore, ever
	protocol = protocolStr;
	filePath = filePathStr;
	if (host == NULL)
	{
		emit AlertableErrorOccured(GetErrorString());
		return false;
	}
	return SetupSocket(port) && SetupPacketFile(filePath);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION SetupPacketFile
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: bool SetupPacketFile(const QString& filePath)
			 - filePath : QString, absolute path to file
--
-- RETURNS: bool : whether file can be opened 
--
-- NOTES:
-- Tries to open a file given an absolute system file path.
-- If file opened, close file and return true.
-- If file cannot be opened, return false. Later functions need guaranteed openable file.
----------------------------------------------------------------------------------------------------------------------*/
bool WSASocketManager::SetupPacketFile(const QString& filePath)
{
	//this also sets packetDataFile to a new filestream, no need to assign
	std::fstream packetDataFile(filePath.toStdString());
	if (!packetDataFile.is_open())
	{
		emit AlertableErrorOccured(QString("Can't open file at:\n") + filePath);
		return false;
	}
	packetDataFile.close();	
	return true;
}

bool WSASocketManager::SetupSocket(const int port)
{
	if ( CreateSocket(protocol) == false)
	{
		emit AlertableErrorOccured("Can't create socket");
		return false;
	}
	if ( ConnectToSocket(port) == false)
	{
		emit AlertableErrorOccured("Can't connect to socket");
		return false;
	}
	return true;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION CreateSocket
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: bool CreateSocket(const QString& protocol)
			 - protocol : QString, either TCP or UDP
--
-- RETURNS: bool : whether socket is created 
--
-- NOTES:
-- Calls WinSock socket function depeneding on passed in selected protocol, to get available socket
----------------------------------------------------------------------------------------------------------------------*/
bool WSASocketManager::CreateSocket(const QString& protocol)
{
	int protocCode = (protocol == "UDP") ? SOCK_DGRAM : SOCK_STREAM ; 
	// if ((transmit_socket = socket(PF_INET, protocCode|O_NONBLOCK, 0)) == -1)
	transmit_socket = socket(PF_INET, protocCode, 0);
	bool socketCreated = (transmit_socket != -1);
	
	if (socketCreated)
	{
		unsigned long on = 1;
		//winsock equivalent of fcnctl O_NODELAY
		if (ioctlsocket(transmit_socket, FIONBIO, &on) != 0)
		{
			return false;
		}
		return true;
	}
	return false;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION ConnectToSocket
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: bool ConnectToSocket(const int port)
			 - port : int, port number to append to socket
--
-- RETURNS: bool : whether can connect to socket or not  
--
-- NOTES:
-- Binds socket depending on if client or server.
-- Connect is also called here if is TCP client.
----------------------------------------------------------------------------------------------------------------------*/
bool WSASocketManager::ConnectToSocket(const int port)
{
	memset((char* )&server_socketaddr, 0, sizeof(struct sockaddr_in));
	server_socketaddr.sin_family = AF_INET;
	server_socketaddr.sin_port = htons(port);
	// server side
	if (host == NULL)
	{
		server_socketaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		int bindStatus = bind(transmit_socket, (struct sockaddr *)&server_socketaddr, sizeof(server_socketaddr));
		return (bindStatus == -1) ? false : true ;	
	}
	// client side
	else 
	{
		memcpy((char* )&server_socketaddr.sin_addr, host->h_addr, host->h_length);
		char* t = (char*)(&server_socketaddr.sin_addr); //used for debugging
		if( protocol == "TCP" )
		{
			return WinApiConnectToSocket(transmit_socket, server_socketaddr);
		}
		//dont need to call connect (or bind) for just udp send
		memset((char *)&client_socketaddr, 0, sizeof(client_socketaddr));
		client_socketaddr.sin_family = AF_INET;
		client_socketaddr.sin_port = htons(0);  // bind to any available port
		client_socketaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		int bindStatusClient = bind(transmit_socket, (struct sockaddr *)&client_socketaddr, sizeof(client_socketaddr));
		return (bindStatusClient == -1) ? false : true;
	}
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION WinApiConnectToSocket
--
-- DATE: Feb 12, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: bool WinApiConnectToSocket(SOCKET& socket, struct sockaddr_in& socket_address)
			 - socket : SOCKET, socket to connect
--			 - socket_address : struct sockaddr_in, socket address to connect to
-- RETURNS: bool : whether socket is connected to server not
--
-- NOTES:
-- This function is a global function called by client.
-- Connect is a WinSock function, but Qt also has a function named connect, which overrides the WinSock one. 
-- This function is a workaround to call the WinSock version of connect.
----------------------------------------------------------------------------------------------------------------------*/
bool WinApiConnectToSocket(SOCKET& socket, struct sockaddr_in& socket_address)
{
	connect(socket, (struct sockaddr*) &socket_address, sizeof(socket_address));
	struct timeval tv;
	fd_set socketOptions;
	FD_ZERO(&socketOptions);
	FD_SET(socket, &socketOptions);
	tv.tv_sec = 2;             /* 2 second timeout */
	tv.tv_usec = 0;
	int connect_status = select(socket, NULL, &socketOptions, NULL, &tv);
	
	return (connect_status == -1) ? false : true;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION PrintClientStatus
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: PrintClientStatus(const QString& status)
			- status : QString, status to print
--
-- NOTES:
-- This function calls MainWindowController and passes a string status to print to console.
-- To promote isolation, WSASocketManager doesnt store a reference to its parent, MainWindowController.
-- So it must use a signal to call its gui print functions. 
----------------------------------------------------------------------------------------------------------------------*/
void WSASocketManager::PrintClientStatus(const QString& status)
{
	emit PrintableStatusReady(status);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION DisplayClientAlert
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: DisplayClientAlert(const QString& alertMsg)
			- alertMsg : QString, alert to display in popup
--
-- NOTES:
-- This function calls MainWindowController and passes a string status to print to console.
-- To promote isolation, WSASocketManager doesnt store a reference to its parent, MainWindowController.
-- So it must use a signal to call its gui print functions. 
----------------------------------------------------------------------------------------------------------------------*/
void WSASocketManager::DisplayClientAlert(const QString& alertMsg)
{
	emit AlertableErrorOccured(alertMsg);
}


/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION GetErrorString
--
-- DATE: Jan 18, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: QString GetErrorString (void)
--
-- RETURNS: QString : a error message string
--
-- NOTES:
-- - From assignment 1:

-- Call this function when a DNS lookup(gethostbyname/byip) is unsuccessful. The WinSock2 api will store the cause of the last 
-- unsuccessful query in memory as an integer error code. This decodes some of those error codes, and either returns
-- a specific message(most of the cases), of a generic "unexpected error occured" message. 
----------------------------------------------------------------------------------------------------------------------*/
QString WSASocketManager::GetErrorString()
{
	int errorCode = WSAGetLastError();
	switch (errorCode)
	{
		//same as WSANO_DATA
		case WSANO_ADDRESS : 
			return QString("API Error: Ip address not found.");
		case WSATRY_AGAIN :
			return QString("API Error: Server failed. Try again.");
		case WSAHOST_NOT_FOUND :
			return QString("API Error: Host not found.");
		case WSAEINPROGRESS : 
			return QString("API Error: A callback function is blocking WinSocket calls.");
		case WSAENETDOWN :
			return QString("API Error: Network subsystem failed.");
		default : 
			return QString("API Error: Unexpected error.");
	}
}