#pragma once
#pragma comment(lib, "ws2_32.lib")

#include <QObject>
#include <QThread>
#include <iostream>
#include <fstream>
#include <WinSock2.h>

class Server : public QObject
{
	Q_OBJECT

public:
	virtual ~Server();
	void ReceiveUdpPackets(SOCKET, const QString&);
	void ReceiveTcpPackets(SOCKET, const QString&, const size_t);
	void StopPolling();

signals:
	void PacketReceived(const size_t, const size_t);
	
private:	
	bool keepPolling;
};
