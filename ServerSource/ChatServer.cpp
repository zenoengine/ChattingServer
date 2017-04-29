#include "stdafx.h"
#include "ServerManager.h"
#include "ChatServer.h"

ChatServer::ChatServer(ServerManager * pServerManager)
{
	mServerManager = pServerManager;
}

ChatServer::~ChatServer()
{
}

bool ChatServer::Init()
{
	mChatLogger.Init("ChatLog.txt");

	return true;
}

void ChatServer::Clear()
{
	mChatLogger.Clear();

}

void ChatServer::Run()
{	
}

void ChatServer::BroadCastMessage(const string& senderID, const string& message)
{
	auto clients = mServerManager->GetClients();

	Packet::MessagePacket messagePacket;
	strcpy(messagePacket.senderID, senderID.c_str());
	strcpy(messagePacket.message, message.c_str());
	
	string chatLog = senderID + " \t " + message;
	mChatLogger.WriteLog(chatLog.c_str());

	char packet[BUFSIZ];
	MakePacket(packet, BUFSIZ, &messagePacket, sizeof(messagePacket));
	for (auto clientSock : clients)	
	{
		send(clientSock.socket, packet, BUFSIZ, 0);
	}
}
