#pragma once

#include <QtWidgets/QMainWindow>
#include <QMessagebox>
#include <QFiledialog>
#include "GeneratedFiles/ui_MainWindow.h"
#include "WSASocketManager.h"

class MainWindowController : public QMainWindow
{
	Q_OBJECT

public:
	MainWindowController(QWidget *parent = Q_NULLPTR);
	virtual ~MainWindowController();
	void DisplayAlertMessage(const QString&);

private:
	Ui::MainWindow ui;
	WSASocketManager* socketManager;
	QLineEdit* hostNameField;
	QLineEdit* ipAddrField;
	QLineEdit* portNumField;
	QLineEdit* packetSizeField;
	QLineEdit* packetCountField;
	QComboBox* clientServerToggler;
	QComboBox* tcpUdpToggler;
	QGroupBox* serverResultFields;
	QLabel* packetSizeDisplayField;
	QLabel* packetCountDisplayField;
	QLabel* transmitTimeDisplayField;
	QLabel* protocolDisplayField;
	QPlainTextEdit* console;

	QLineEdit* filePathField;
	QIntValidator* intInputEnforcer;

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
};
