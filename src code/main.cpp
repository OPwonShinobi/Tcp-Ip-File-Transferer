#include "MainWindowController.h"
#include <QtWidgets/QApplication>

/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE: main.cpp - Program entry point
--
-- PROGRAM: TcpUdpFileTransfer/PacketLogger
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- NOTES:
-- This program is a multithreaded data transfer application which uses the Qt framework and the Windows Socket 2 API. 
-- A user can input the host info(name or ip address) and port number of a computer on the same network.
-- When run with another instance of this program on that computer, the user can then choose to send or receive 
-- a text file using TCP or UDP.
--  
-- Critical statistics such as transmission time, packet count, packet size, and protocol used, are displayed
-- upon completion of file. As well, any time a packet was sent/failed to be sent, a status will to logged
-- to a console.
-- 
-- If a user did not enter necessary information correctly, a alert message box will popup. Or if the send/receive
-- was not successful, the result printed becomes an error message instead. 		
----------------------------------------------------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	MainWindowController w;
	w.show();
	return a.exec();
}
