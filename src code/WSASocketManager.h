#pragma once
#pragma comment(lib, "ws2_32.lib")

#include <cstdio>
#include <WinSock2.h>
#include <iostream>
#include <fstream>
#include <QObject>
#include <QThread>
#include <QEvent>
#include <chrono>
#include "Server.h"
#include "Client.h"

bool WinApiConnectToSocket(SOCKET&, struct sockaddr_in&);

class WSASocketManager : public QObject
{
	Q_OBJECT

public:
	WSASocketManager(QObject *parent);
	virtual ~WSASocketManager();
	bool CheckIPFormat(const QString&);
	bool SetupSendingByName(const QString&, const QString&, const int, const QString&);
	bool SetupSendingByIp(const QString&, const QString&, const int, const QString&);
	bool SetupReceiving(const QString&, const int, const QString&, size_t = 0);
	void SendPackets(const size_t, const size_t);
	void ReceivePackets();
	//slot function, dont call directly
	void FinishReceivePackets();
	void PrintClientStatus(const QString&);
	void DisplayClientAlert(const QString&);

signals:
	void AlertableErrorOccured(const QString&);
	void PrintableStatusReady(const QString&);
	void ServerResultsReady(const size_t, const size_t, const QString&, const QString&);

	void UdpPacketRecvSelected(SOCKET, const QString&);
	void TcpPacketRecvSelected(SOCKET, const QString&, const size_t);

	void UdpPacketSendSelected(SOCKET, const QString&, const size_t, const size_t, struct sockaddr_in);
	void TcpPacketSendSelected(SOCKET, const QString&, const size_t, const size_t);

	void Disconnected();
	void DisconnectAllowed(const bool);

private:
	SOCKET transmit_socket;
	struct hostent* host = NULL;
	struct sockaddr_in server_socketaddr;
	struct sockaddr_in client_socketaddr;
	
	QString protocol;
	QString filePath;
	size_t resultPacketSize; //to print as result
	size_t expectedPacketSize; //get from mainwindow user input
	size_t resultPacketsReceived;
	QThread* serverThread;
	Server* server;
	QThread* clientThread;
	Client* client;
	std::chrono::steady_clock::time_point startTime;
	std::chrono::steady_clock::time_point endTime;

	QString GetErrorString();
	bool SetupSocket(const int);
	bool SetupPacketFile(const QString&);
	bool CreateSocket(const QString&);
	bool ConnectToSocket(const int);
	void UpdateTimer(const size_t, const size_t);
};
