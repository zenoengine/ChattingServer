#pragma once
#include "IServer.h"

using namespace std;
class ServerManager;
class ChatServer :	public IServer
{
public:
	ChatServer(ServerManager* server);
	virtual ~ChatServer();

	bool Init() override;
	void Clear() override;
	void Run() override;

	void BroadCastMessage(const string& senderID, const string& message);
private:
	ServerManager* mServerManager = nullptr;
	Logger mChatLogger;
};

