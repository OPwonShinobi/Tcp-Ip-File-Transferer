#include "Client.h"

/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE: Client.cpp - An class for handling sending related function calls in the WinSock2 API
--
-- PROGRAM: TcpUdpFileTransfer/PacketLogger
--
-- FUNCTIONS:
	void SendTcpPackets(SOCKET, const QString&, const size_t, const size_t);
	void SendUdpPackets(SOCKET, const QString&, const size_t, const size_t, struct sockaddr_in);
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- NOTES:
-- This class implements the same WinSock capabilities as WSASocketManager, but
-- is specific to sending packets, and lives in its own thread instead of main. 
-- It handles all calls(except for connect) to Sending-related functions of WinSock2 API.
----------------------------------------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION SendTcpPackets
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: void SendTcpPackets(SOCKET clientSocket, const QString& filePath, const size_t packetSize, 
	const size_t packetCount)
		- clientSocket : SOCKET, socket to send packets to, already connected
		- filePath : QString, absolute path to file to write data to
		- packetSize : unsigned int, size to make packets at
		- packetCount : unsigned int, number of times to send packet

-- RETURNS: void.
--
-- NOTES:
-- This function assumes all parameters are pre-validated in WSASocketManager.
-- This function lies on a different thread than main: should be signaled, not directly called.
--
-- Makes a packet from a file, then repeated sends it to a socket to a server(with up to 3 retransmits if fail).  
----------------------------------------------------------------------------------------------------------------------*/
void Client::SendTcpPackets(SOCKET clientSocket, const QString& filePath, const size_t packetSize, const size_t packetCount)
{
	//this buffer holds all of packet data
	std::string* packetDataBuffer = new std::string();
	//~9GB, this number exceeeds upper bounds of int 
	size_t MAX_BUFFER_SIZE = packetDataBuffer->max_size();
	if (packetSize > MAX_BUFFER_SIZE)
	{
		emit ClientAlertableErrorOccured(QString("PacketSize wayyy too big. Use a number\n smaller than %1").arg(MAX_BUFFER_SIZE));
		return;
	}
	//make packets
	std::ifstream packetDataFile(filePath.toStdString());
	std::streambuf &optmzedBuf = *packetDataFile.rdbuf();
	while(packetDataBuffer->size() < packetSize )
	{
		char c = optmzedBuf.sbumpc();
		// pad packet with 0s after reaching eof in file
		if(c == EOF)
			c = 0;
		packetDataBuffer->push_back(c);
	}
	int retrans_count = 0;
	//send packets (at least) specified times
	for (int i = 0; i < packetCount; ++i)
	{
		if (send(clientSocket, packetDataBuffer->c_str(), packetDataBuffer->size(), 0) == -1)
		{
			int error_code = WSAGetLastError();
			if (error_code = WSAEWOULDBLOCK) //resource busy, try again
			{
				if (retrans_count < 3)
				{
					--i;
					++retrans_count;
					emit ClientPrintableStatusReady(QString("-send failed %1 times, retransmiting...").arg(retrans_count));				
					QThread::msleep(100);
					continue;
				}
				emit ClientPrintableStatusReady("-send failed, retransmit too many times");				
				continue;
			} 
			emit ClientPrintableStatusReady(QString("-send failed, unexpected error code: %1").arg(WSAGetLastError()));					
		}
		else
		{
			retrans_count = 0;
			emit ClientPrintableStatusReady("-sent packet.");				
		}
	}
	delete packetDataBuffer;
	//emit signal print sht to console
	emit ClientPrintableStatusReady("-Finished sending all packets.");
	packetDataFile.close();	
	closesocket(clientSocket);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION SendUdpPackets
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: void Client::SendUdpPackets(SOCKET clientSocket, const QString& filePath, const size_t packetSize,
	 const size_t packetCount, struct sockaddr_in server_socketaddr)
		- clientSocket : SOCKET, socket to send packets to
		- filePath : QString, absolute path to file to write data to
		- packetSize : unsigned int, size to make packets at
		- packetCount : unsigned int, number of times to send packet
		- server_socketaddr : struct sockaddr_in, struct holding address & port of server

-- RETURNS: void.
--
-- NOTES:
-- This function assumes all parameters are pre-validated in WSASocketManager.
-- This function lies on a different thread than main: should be signaled, not directly called.
--
-- Makes a UDP packet from a file, then repeated sends it to a socket to a server(with no retransmits).  
----------------------------------------------------------------------------------------------------------------------*/
void Client::SendUdpPackets(SOCKET clientSocket, const QString& filePath, const size_t packetSize, const size_t packetCount, struct sockaddr_in server_socketaddr)
{
	//this buffer holds all of packet data
	std::string* packetDataBuffer = new std::string();
	//~9GB, this number exceeeds upper bounds of int 
	size_t MAX_BUFFER_SIZE = packetDataBuffer->max_size();
	if (packetSize > MAX_BUFFER_SIZE)
	{
		ClientAlertableErrorOccured(QString("PacketSize wayyy too big. Use a number\n smaller than %1").arg(MAX_BUFFER_SIZE));
		return;
	}
	//make packets
	std::ifstream packetDataFile(filePath.toStdString());
	while(packetDataBuffer->size() < packetSize )
	{
		char c;
		packetDataFile.get(c);
		//pad packet after reaching eof in file
		if(packetDataFile.fail()) {
			// break;
			packetDataBuffer->push_back(0);
			continue;
		}
		packetDataBuffer->push_back(c);
	}
	
	//server_socketaddr
	size_t server_len = sizeof(server_socketaddr);
	for (int i = 0; i < packetCount; ++i)
	{
		if (sendto(clientSocket, packetDataBuffer->c_str(), packetDataBuffer->size(), 0,
			(struct sockaddr*)& server_socketaddr, server_len) == -1)
		{
			emit ClientPrintableStatusReady(QString("-sendto'd failed, unexpected error code: %1").arg(WSAGetLastError()));					
		} 
		else
		{
			emit ClientPrintableStatusReady("-sendto'd packet.");				
		}
	}
	delete packetDataBuffer;
	//emit signal print sht to console
	emit ClientPrintableStatusReady("-Finished sending all packets.");
	packetDataFile.close();	
	closesocket(clientSocket);
}