#include "Server.h"
/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE: Server.cpp - An class for handling receiving related function calls in the WinSock2 API
--
-- PROGRAM: TcpUdpFileTransfer/PacketLogger
--
-- FUNCTIONS:
	void ReceiveUdpPackets(SOCKET, const QString&);
	void ReceiveTcpPackets(SOCKET, const QString&, const size_t);
	void StopPolling();
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- NOTES:
-- This class implements the same WinSock capabilities as WSASocketManager, but
-- is specific to receiving packets, and lives in its own thread instead of main. 
-- It handles most calls(except for bind) to Receiving-related functions of WinSock2 API
----------------------------------------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION Server Destructor
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: ~Server(void)
--
-- RETURNS: N/A
--
-- NOTES:
-- Server lives and dies with ServerThread
-- Sets polling to false as extra precaution.
----------------------------------------------------------------------------------------------------------------------*/
Server::~Server()
{
	keepPolling = false;
}


/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION ReceiveUdpPackets
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: void ReceiveUdpPackets(SOCKET serverSocket, const QString& filePath)
		- serverSocket : SOCKET, socket to receive packets from 
		- filePath : QString, absolute path to file to write data to

-- RETURNS: void.
--
-- NOTES:
-- This function assumes all parameters are pre-validated in WSASocketManager.
-- This function lies on a different thread than main: should be signaled, not directly called.
--
-- Enters a loop which repeatedly gets any available datagrams from a socket, and prints it to a file.
----------------------------------------------------------------------------------------------------------------------*/
void Server::ReceiveUdpPackets(SOCKET serverSocket, const QString& filePath)
{
	keepPolling = true;
	size_t packetsReceived = 0;
	// actual max size of a datagram is 65508 bytes, next higher power of 2 up -> 64x2^10 = 64KB = 65536B
	size_t MAX_BUFFER_SIZE = 65536; 
	char* packetBuffer = (char*)malloc(MAX_BUFFER_SIZE * sizeof(char));

	while(keepPolling)
	{
		int bytesRead = 0;
		struct sockaddr_in client;
		int client_len = sizeof(client); 
		memset(packetBuffer, 0, MAX_BUFFER_SIZE * sizeof(char));

		bytesRead = recvfrom(serverSocket, packetBuffer, MAX_BUFFER_SIZE, 0, (struct sockaddr *)&client, &client_len);
		if( bytesRead < 0)
		{
			QThread* curr = QThread::currentThread();
			QString currStr = curr->objectName();
			QThread::msleep(100); //smallest interrupt possible without taking too much cpu resources
			continue;
		}
		if (bytesRead == 0)
		{
			continue; //dont wait, transmission started
		}
		++packetsReceived;
		emit PacketReceived(bytesRead, packetsReceived);
		// open & print to file & close
		std::ofstream outputFile(filePath.toStdString(), std::ofstream::app);
		outputFile << packetBuffer;
		outputFile.close(); //actually optional in c++
	}
	free(packetBuffer);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION ReceiveTcpPackets
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: void ReceiveTcpPackets(SOCKET serverSocket, const QString& filePath, const size_t expectedPacketSize)
		- serverSocket : SOCKET, socket to listen for connections on 
		- filePath : QString, absolute path to file to write data to
		- expectedPacketSize : unsigned int, used to calculate packetCount  

-- RETURNS: void.
--
-- NOTES:
-- This function assumes all parameters are pre-validated in WSASocketManager.
-- This function lies on a different thread than main: should be signaled, not directly called.
--
-- Enters a loop which repeatedly listens, then accepts any connections using a new client socket.
-- If any bytes are read from that socket, prints it to a file.
----------------------------------------------------------------------------------------------------------------------*/
void Server::ReceiveTcpPackets(SOCKET serverSocket, const QString& filePath, const size_t expectedPacketSize)
{
	keepPolling = true;
	size_t packetsReceived = 0;
	size_t bytesReadTotal = 0;
	size_t MAX_BUFFER_SIZE = 2147483648; //2GB, 2 x 2^30 B  for now
	QThread* curr = QThread::currentThread(); //debugging
	QString currStr = curr->objectName(); //debugging
	char* packetBuffer = (char*)malloc(MAX_BUFFER_SIZE * sizeof(char));

	while(keepPolling)
	{
		if (listen(serverSocket, 5) < 0)
		{
			QThread::msleep(100);
			continue;
		}

		SOCKET clientSocket;
		struct sockaddr_in client;
		int client_len = sizeof(client); 
		memset(packetBuffer, 0, MAX_BUFFER_SIZE * sizeof(char));

		if ((clientSocket = accept (serverSocket, (struct sockaddr *)&client, &client_len)) == -1)
		{
			closesocket (clientSocket);
			QThread::msleep(100);
			continue;
		}
		//connection accepted, start timing
		emit PacketReceived(-1, -1);

		//this might block this thread, new socket did not set non_blocking 
		size_t bytesRead = 0;
		std::ofstream outputFile(filePath.toStdString(), std::ofstream::app);
		while( (bytesRead = recv(clientSocket, packetBuffer, MAX_BUFFER_SIZE, 0)) >= 0)
		{
			bytesReadTotal += bytesRead;
			if (bytesRead == 0)
				break;
			outputFile << packetBuffer;
			//outputBinFile.write(packetBuffer, bytesRead); 
			// only way to write \0 to file is binary mode
			// but all chars printed becomes binary too
		}
		packetsReceived += bytesReadTotal / expectedPacketSize;
		emit PacketReceived(expectedPacketSize, packetsReceived);
		closesocket (clientSocket);
	} 
	free(packetBuffer);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION StopPolling
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: void StopPolling()

-- RETURNS: void.
--
-- NOTES:
-- Stops loop.
-- When user disconnects on MainWindow, a signal is set to WSASocketManager, which in turn calls this. 
----------------------------------------------------------------------------------------------------------------------*/
void Server::StopPolling()
{
	keepPolling = false;
}
