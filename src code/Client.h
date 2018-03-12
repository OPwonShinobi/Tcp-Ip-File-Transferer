#pragma once
#pragma comment(lib, "ws2_32.lib")

#include <QObject>
#include <QThread>
#include <iostream>
#include <fstream>
#include <WinSock2.h>

class Client : public QObject
{
	Q_OBJECT

public:
	virtual ~Client() = default;
	void SendTcpPackets(SOCKET, const QString&, const size_t, const size_t);
	void SendUdpPackets(SOCKET, const QString&, const size_t, const size_t, struct sockaddr_in);

signals:
	void ClientAlertableErrorOccured(const QString&);
	void ClientPrintableStatusReady(const QString&);

};
