#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <vector>
#include <map>

#include "IServer.h"
#include "Logger.h"

using namespace std;

class ServerManager : IServer
{
private:
	struct ProcessClientArg
	{
		SOCKET cleintSocket;
		ServerManager* tcpServer;
	};

	struct ClientInformation
	{
		SOCKET socket;
		string userID;
		DWORD firstConnectTime;
	};

public:
	ServerManager();
	bool Init() override;
	void Clear();
	void Run();

	static DWORD WINAPI ProcessClient(LPVOID arg);

	bool GetServer(Server::ServerType type, Server::IServer** service);

	const vector<ClientInformation>& GetClients() const { return mClients; };
private:
	void InitServers();
	void ClearServers();

	void PushClient(ClientInformation socket);
	void RemoveClient(SOCKET socket);
	
	void Lock();
	void Unlock();

	void WriteLog(const char* text);
private:
	WSADATA mWSAData = { 0, };
	SOCKET mListenSocket = 0;
	SOCKADDR_IN mServerAdress = { 0, };
	CRITICAL_SECTION mCriticalSection;

	unsigned int mServerPort = 0;
	static const int BUFFSIZE = 1024;

	map< Server::ServerType, Server::IServer* > mServers;
	vector<ClientInformation> mClients;
	Logger mSystemLogger;

};

