#include "MainWindowController.h"

/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE: MainWindowController.cpp - An class for handling Qt UI functions
--
-- PROGRAM: TcpUdpFileTransfer/PacketLogger
--
-- FUNCTIONS:
	void InitializeSignals();
	void InitializeGuiElements();
	void PickFile();
	void ToggleClientServerGuiElements();
	void Connect();
	void ClientSend(const int);
	void ServerReceive(const int);
	void PrintStatusToConsole(const QString&);
	void DisplayServerResults(const int, const int, const QString&, const QString&);
	void ToggleDisconnect(const bool);
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- NOTES:
-- This class implements various Qt Widgets to control the main UI window. It handles any user   
-- interaction with the UI, including button clicks and checking the input from the text fields.
-- Once the connect button on the UI is clicked, the necessary text fields ( host ip address, port, etc)
-- is checked for content, and a call is made to the WSASocketManager. 
-- The WSASocketManager handles all WinSock API calls, and send string statuses and alerts to this class 
-- when appropriate (a packet is sent for example). Then this MainWindowController will pass that string 
-- alert/status (could be an error message if nothing found) to either console or a popup.
-- The WSASocketManager also sends server results to display fields here to show, once a file is received.  
----------------------------------------------------------------------------------------------------------------------*/
MainWindowController::MainWindowController(QWidget *parent)
	: QMainWindow(parent)
{
	socketManager = new WSASocketManager(this);
	ui.setupUi(this);
	InitializeGuiElements();
	InitializeSignals();
	ToggleClientServerGuiElements();
}

MainWindowController::~MainWindowController()
{
	delete socketManager;
	delete intInputEnforcer;
}
/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION InitializeGuiElements
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: void InitializeGuiElements(void)
--
-- RETURNS: void.
--
-- NOTES:
-- Call this function to link instance references to GUI elements from the ui QMainWindow class.
-- Also sets some fields to numeric only to save the trouble of validating them later.
----------------------------------------------------------------------------------------------------------------------*/
void MainWindowController::InitializeGuiElements()
{
	intInputEnforcer = new QIntValidator;
	hostNameField = ui.HostNameLineEdit;
	ipAddrField = ui.IpAddressLineEdit;

	portNumField = ui.PortLineEdit;
	portNumField->setValidator(intInputEnforcer);
	
	packetSizeField = ui.PacketSizeLineEdit;
	packetSizeField->setValidator(intInputEnforcer);
	
	packetCountField = ui.TransmissionTimeLineEdit;
	packetCountField->setValidator(intInputEnforcer);
	
	filePathField = ui.FilePathLineEdit;

	clientServerToggler = ui.ClientServerDropDown;
	tcpUdpToggler = ui.TcpUdpDropDown;
	serverResultFields = ui.ServerSideResults;

	packetSizeDisplayField = ui.PacketSizeLabel;
	packetCountDisplayField = ui.PacketCountLabel;
	transmitTimeDisplayField = ui.TransmissionTimeLabel;
	protocolDisplayField = ui.ProtocolLabel;
	console = ui.ConsoleTextEdit;
	ui.DisconnectButton->setEnabled(false);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION InitializeSignals
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: void InitializeSignals (void)
--
-- RETURNS: void.
--
-- NOTES:
-- Call this function to set up signal handlers between the UI and SocketManager class. 
-- Only need to be called once per execution. Uses the Qt signals and slots system.
----------------------------------------------------------------------------------------------------------------------*/
void MainWindowController::InitializeSignals()
{
	connect(ui.FilePickerButton, &QPushButton::pressed, this, &MainWindowController::PickFile);
	connect(ui.ConnectButton, &QPushButton::pressed, this, &MainWindowController::Connect);
	connect(ui.DisconnectButton, &QPushButton::pressed, socketManager, &WSASocketManager::FinishReceivePackets);

	connect(clientServerToggler, &QComboBox::currentTextChanged, this, &MainWindowController::ToggleClientServerGuiElements);
	connect(tcpUdpToggler, &QComboBox::currentTextChanged, this, &MainWindowController::ToggleClientServerGuiElements);
	connect(socketManager, &WSASocketManager::AlertableErrorOccured, this, &MainWindowController::DisplayAlertMessage);
	connect(socketManager, &WSASocketManager::PrintableStatusReady, this, &MainWindowController::PrintStatusToConsole);
	connect(socketManager, &WSASocketManager::ServerResultsReady, this, &MainWindowController::DisplayServerResults);
	connect(socketManager, &WSASocketManager::DisconnectAllowed, this, &MainWindowController::ToggleDisconnect);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION ToggleClientServerGuiElements
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: void ToggleClientServerGuiElements(void)
--
-- RETURNS: void.
--
-- NOTES:
-- No need to directly call this function. It should be linked to the protocol & client/server
-- dropdowns and is called on change of their values.
--
-- Switches GUI from client to server mode. There are various GUI elements that are unneeded 
-- for running as a server, and various GUI elements that are unneeded for client.
-- This function turns them on and off, depending on what is currently selected on the 
-- client/server dropdown. 
--
*/ 
void MainWindowController::ToggleClientServerGuiElements()
{
	//server mode
	bool inServerMode = (clientServerToggler->currentText() == "Server");
	//server only display fields
	serverResultFields->setEnabled(inServerMode);
	//client only entry fields
	packetSizeField->setEnabled(!inServerMode);
	packetCountField->setEnabled(!inServerMode);
	hostNameField->setEnabled(!inServerMode);
	ipAddrField->setEnabled(!inServerMode);
	//set sending packet info to N/A if in server mode, or default value of 1
	//in client mode
	QString sentPacketInfo = inServerMode ? "N/A" : "1";
	packetSizeField->setText(sentPacketInfo);
	packetCountField->setText(sentPacketInfo);

	// change settings for TCP server mode only
	if (inServerMode && tcpUdpToggler->currentText() == "TCP")
	{
		//tcp needs packet size, must enter a size otherwise just streams of bytes
		packetSizeField->setEnabled(true);
		packetSizeField->setText("1");
	}
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION PickFile
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: void PickFile(void)
--
-- RETURNS: void.
--
-- NOTES:
-- Opens up a new file explorer-link dialog. Lets user picks a .txt file, and saves
-- the absolute filePath to a string instance variable.
-- */
void MainWindowController::PickFile()
{
	QString filePath = QFileDialog::getOpenFileName(this, "Choose File for Packets", "./",
		"Text File (*.txt)");
	filePathField->setText(filePath);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION Connect
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: void Connect(void)
--
-- RETURNS: void.
--
-- NOTES:
-- Called when user clicks Connect button. 
-- Different behaviour depending on if in client or server mode:
-- */
void MainWindowController::Connect()
{
	QString portNumStr = portNumField->text().trimmed();
	// portNumStr restricted to numeric / blank input
	// need not check for numeric
	if (portNumStr == "")
	{
		DisplayAlertMessage("Please enter a port number");
		return;
	}
	bool inClientMode = (clientServerToggler->currentText() == "Client");
	if (inClientMode)
	{
		ClientSend(portNumStr.toInt());
	}
	else
	{
		ServerReceive(portNumStr.toInt());
	}
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION ServerReceive
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: void ServerReceive(const int port)
--			- port : integer port number to connect to
--
-- RETURNS: void.
--
-- NOTES:
-- Called when user clicks Connect button and is in Server mode. 
-- Starts process of setting up to receive packets depending on the
-- protocol selected.
-- */
void MainWindowController::ServerReceive(const int port)
{
	QString protocol = tcpUdpToggler->currentText();
	//pass protocol to func, and let it handle it
	size_t packetSize = packetSizeField->text().toUInt();
	if (packetSize <= 0 && protocol == "TCP")
	{
		DisplayAlertMessage("Packet size must be 1 or greater.");
		return;
	}

	QString filePath = filePathField->text().trimmed();
	if (socketManager->SetupReceiving(protocol, port, filePath, packetSize))
	{
		console->clear();
		socketManager->ReceivePackets();
	}	
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION ClientSend
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: void ClientSend(const int port)
--			- port : integer port number to connect to
--
-- RETURNS: void.
--
-- NOTES:
-- Called when user clicks Connect button and is in Client mode. 
-- Starts process of setting up to send packets depending on the
-- protocol selected.
-- */
void MainWindowController::ClientSend(const int port)
{
	QString protocol = tcpUdpToggler->currentText();

	size_t packetSize = packetSizeField->text().toUInt();
	if (packetSize <= 0)
	{
		DisplayAlertMessage("Packet size must be 1 or greater.");
		return;
	}
	if (protocol == "UDP" && packetSize >= 65508)
	{
		DisplayAlertMessage("UDP Packet size must be less than ~64KB or 65508 Bytes.");
		return;
	}
	size_t packetCount = packetCountField->text().toUInt();
	if (packetCount <= 0)
	{
		DisplayAlertMessage("Times to transmit must be 1 or greater.");
		return;
	}
	QString hostName = hostNameField->text().trimmed();
	QString filePath = filePathField->text().trimmed();
	if (hostName != "")
	{
		if (socketManager->SetupSendingByName(hostName, protocol, port, filePath))
		{
			console->clear();
			socketManager->SendPackets(packetSize, packetCount);
			return;
		}
	}
	QString ipAddrStr = ipAddrField->text().trimmed();
	bool ipValid = socketManager->CheckIPFormat(ipAddrStr);
	if (ipValid)
	{
		if (socketManager->SetupSendingByIp(ipAddrStr, protocol, port, filePath))
		{
			console->clear();
			socketManager->SendPackets(packetSize, packetCount);
		}
		return;
	}
	this->DisplayAlertMessage("Please enter either a host name, or alternatively a \n valid numeric IP address, in \'X.X.X.X\' format");
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION ToggleDisconnect
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: void ToggleDisconnect(const bool turnOn) 
--			- turnOn : boolean switch, whether turns on disconnect or not
--
-- RETURNS: void.
--
-- NOTES:
-- Called to enable disconnect button (and subsequently, its on click handler)
-- Should only be called once a Server has connected, and thus able
-- to be disconnected.
-- Logically, also turns ConnectButton off.
-- */
void MainWindowController::ToggleDisconnect(const bool turnOn) 
{
	ui.DisconnectButton->setEnabled(turnOn);
	ui.ConnectButton->setEnabled(!turnOn);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION DisplayAlertMessage
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: void DisplayAlertMessage(const QString& alertMsg)
--			- alertMsg : QString msg to be displayed in a popup window
--
-- RETURNS: void.
--
-- NOTES:
-- Called to create a new popup to alert user with msg.
-- Should only be called for important connection-breaking messages.
-- No one likes too many popups. For less important alerts, use PrintStatusToConsole.
-- */
void MainWindowController::DisplayAlertMessage(const QString& alertMsg)
{
	QMessageBox msgBox;
	msgBox.setText(alertMsg);
	msgBox.setDefaultButton(QMessageBox::Ok);
	msgBox.exec();
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION PrintStatusToConsole
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: void PrintStatusToConsole(const QString& status)
--			- status : QString msg to be printed in console
--
-- RETURNS: void.
--
-- NOTES:
-- Called to insert a status/msg in the console
-- Use this to print all frequently reoccuring messages, eg. -sent packet 
-- User may be less alerted by this. For very important alerts, use DisplayAlertMessage
-- */
void MainWindowController::PrintStatusToConsole(const QString& status)
{
	console->insertPlainText(status + "\n");
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION DisplayServerResults
--
-- DATE: Feb 10, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: void DisplayServerResults(const size_t packetSize, const size_t packetCount, const QString& time, const QString& protocol)
			- packetSize : unsigned int, size of each packet received for UDP, or if TCP used, expected packet size
			- packetCount : unsigned int, number of packets received
			- time : QString, time it took for all packets that arrived to do so, converted to string so its easier to handle
			- protocol : QString, TCP or UDP
--
-- RETURNS: void.
--
-- NOTES:
-- This function is either called when server has received updated packet information. 
-- Or to clear server reults fields by passing in 0s
-- */
void MainWindowController::DisplayServerResults(const size_t packetSize, const size_t packetCount, const QString& time, const QString& protocol)
{
	packetSizeDisplayField->setText(QString::number(packetSize));
	packetCountDisplayField->setText(QString::number(packetCount));
	transmitTimeDisplayField->setText(time);
	protocolDisplayField->setText(protocol);
}