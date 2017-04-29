#include "stdafx.h"
#include "Packet.h"
#include "ServerManager.h"

#include "ChatServer.h"
#include "LoginServer.h"

ServerManager::ServerManager()
{
}

bool ServerManager::Init()
{
	mSystemLogger.Init("SystemLog.txt");

	unsigned int serverPort = 9000;
	if (WSAStartup(MAKEWORD(2, 2), &mWSAData) != 0)
	{
		return false;
	}

	mListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (mListenSocket == INVALID_SOCKET)
	{
		return false;
	}

	mServerPort = serverPort;
	mServerAdress.sin_family = AF_INET;

	mServerAdress.sin_addr.s_addr = inet_addr("127.0.0.1");
	mServerAdress.sin_port = htons(mServerPort);
	int retval = bind(mListenSocket, (SOCKADDR *)&mServerAdress, sizeof(mServerAdress));
	if (retval == SOCKET_ERROR)
	{
		DWORD word = GetLastError();
		return false;
	}

	retval = listen(mListenSocket, SOMAXCONN);
	if (retval == SOCKET_ERROR)
	{
		return false;
	}

	InitServers();

	InitializeCriticalSection(&mCriticalSection);

	return true;
}

void ServerManager::Clear()
{	
	if (mListenSocket != INVALID_SOCKET)
	{
		closesocket(mListenSocket);
	}
	DeleteCriticalSection(&mCriticalSection);
	WSACleanup();

	mSystemLogger.Clear();
}

void ServerManager::Run()
{
	while (1)
	{
		SOCKET clientSock = 0;
		sockaddr clientaddr = { 0, };
		int addrlen = sizeof(clientaddr);
		HANDLE hThread;

		clientSock = accept(mListenSocket, &clientaddr, &addrlen);
		assert(clientSock != SOCKET_ERROR);

		ProcessClientArg argument;
		argument.cleintSocket = clientSock;
		argument.tcpServer = this;
		hThread = CreateThread(NULL, 0, ServerManager::ProcessClient, (LPVOID)&argument, 0, NULL);

		if (hThread == NULL)
		{
			closesocket(clientSock);
		}
		else
		{
			CloseHandle(hThread);
		}
	}
}

DWORD ServerManager::ProcessClient(LPVOID arg)
{
	using namespace Packet;
	ProcessClientArg* processClientArg = (ProcessClientArg*)arg;
	SOCKET clientSock = processClientArg->cleintSocket;
	ServerManager* pServerManager = processClientArg->tcpServer;

	int retval = 0;
	char recvBuffer[BUFFSIZE + 1] = { 0, };
	bool running = true;

	while (running)
	{
		retval = recv(clientSock, recvBuffer, BUFFSIZE, 0);

		if (retval == SOCKET_ERROR) {
			break;
		}

		pServerManager->Lock();

		string headerString = recvBuffer;
		unordered_map<string, string> headers;

		if (ParseHeader(headerString, &headers))
		{
			auto foundEndOfHeader = headerString.find_last_of("\r\n\r\n");
			auto contentLengthHeader = headers.find("Content-Length");
			size_t contentLength = 0;
			if (contentLengthHeader != headers.end())
			{
				contentLength = atoi(contentLengthHeader->second.c_str());
				char* body = new char[contentLength];
				memcpy(body, (recvBuffer + foundEndOfHeader + 1), contentLength);

				PacketBase* packet = (PacketBase*)body;

				switch (packet->type)
				{
				case PT_LOGIN:
				{
					LoginPacket* loginPacket = static_cast<LoginPacket*>(packet);
					LoginServer* pLoginServer = nullptr;

					if (pServerManager->GetServer(Server::ST_LOGIN, (IServer**)&pLoginServer))
					{
						LoginResult loginResultPacket;
						
						if (pLoginServer->TryLogin(loginPacket->id, loginPacket->password))
						{
							loginResultPacket.bSuccessd = true;
							strcpy(loginResultPacket.id, loginPacket->id);
							
							ChatServer* chatServer = nullptr;
							if (pServerManager->GetServer(Server::ST_CHAT, (IServer**)&chatServer))
							{
								string message;
								message = loginPacket->id;
								message.append(", connected");
								chatServer->BroadCastMessage("Notice", message);
							}
							time_t timeT = time(NULL);
							tm* timeStructure = localtime(&timeT);
							string timeStamp = to_string(timeStructure->tm_year) + "년" +
							to_string(timeStructure->tm_mon) + "월" +
							to_string(timeStructure->tm_mday) + "일" +
							to_string(timeStructure->tm_hour) + "시" +
							to_string(timeStructure->tm_min) + "분" +
							to_string(timeStructure->tm_sec) + "초";

							pServerManager->WriteLog(timeStamp.c_str());

							ClientInformation information;
							information.socket = clientSock;
							information.userID = loginPacket->id;
							information.firstConnectTime = timeGetTime();
							
							pServerManager->PushClient(information);
						}
						else
						{
							loginResultPacket.bSuccessd = false;
						}

						char packet[BUFFSIZE];
						MakePacket(packet, BUFFSIZE, &loginResultPacket, sizeof(loginResultPacket));
						send(clientSock, packet, BUFFSIZE, 0);
					}

				}
				break;

				case PT_MESSAGE:
				{
					MessagePacket* messagePacket = static_cast<MessagePacket*>(packet);
					ChatServer* chatServer = nullptr;

					if (pServerManager->GetServer(Server::ST_CHAT, (IServer**)&chatServer))
					{
						chatServer->BroadCastMessage(messagePacket->senderID, messagePacket->message);
					}
				}
				break;

				case PT_REQUEST_EXIT:
				{
					pServerManager->RemoveClient(clientSock);
					running = false;
				}
				break;
				}

				delete[] body;
			}
		}
		pServerManager->Unlock();
	}
	pServerManager->RemoveClient(clientSock);
	
	closesocket(clientSock);

	return 0;
}

bool ServerManager::GetServer(Server::ServerType type, Server::IServer ** ppServer)
{
	auto serverPair = mServers.find(type);

	if (serverPair == mServers.end())
	{
		return false;
	}

	*ppServer = serverPair->second;

	return true;
}

void ServerManager::PushClient(ClientInformation information)
{
	for (auto clientInfo : mClients)
	{
		if (clientInfo.socket == information.socket)
		{
			return;
		}
	}
	
	mClients.push_back(information);
}

void ServerManager::RemoveClient(SOCKET socket)
{
	string userID;
	for (auto clientInfoIter = mClients.begin(); clientInfoIter != mClients.end(); clientInfoIter++)
	{
		if (clientInfoIter->socket == socket)
		{
			SOCKADDR_IN clientaddr;
			int addrlen = sizeof(clientaddr);
			getpeername(socket, (SOCKADDR*)&clientaddr, &addrlen);

			DWORD timeDiff = timeGetTime() - clientInfoIter->firstConnectTime;
			size_t sec = timeDiff / 1000;
			
			string removeClientLog;
			removeClientLog.append("[TCP 서버] 클라이언트 종료 : IP 주소 = ");
			removeClientLog.append(inet_ntoa(clientaddr.sin_addr));
			removeClientLog.append("포트 번호 =");
			removeClientLog.append(to_string(ntohs(clientaddr.sin_port)));
			removeClientLog.append("접속 시간 :");
			removeClientLog.append(to_string(sec));
			removeClientLog.append("초");
			WriteLog(removeClientLog.c_str());

			userID = clientInfoIter->userID;
			mClients.erase(clientInfoIter);

			ChatServer* chatServer;
			if (GetServer(ST_CHAT, (IServer**)&chatServer))
			{
				chatServer->BroadCastMessage("Notice", userID + " , disconnected");
			}

			break;
		}
	}
}

void ServerManager::Lock()
{
	EnterCriticalSection(&mCriticalSection);
}

void ServerManager::Unlock()
{
	LeaveCriticalSection(&mCriticalSection);
}

void ServerManager::WriteLog(const char * text)
{
	mSystemLogger.WriteLog(text);
}

void ServerManager::InitServers()
{
	mServers.insert(make_pair(Server::ST_CHAT, new ChatServer(this)));
	mServers.insert(make_pair(Server::ST_LOGIN, new LoginServer()));

	for (auto serverPair : mServers)
	{
		IServer* pServer = serverPair.second;
		pServer->Init();
	}
}

void ServerManager::ClearServers()
{
	for (auto serverPair : mServers)
	{
		IServer* pServer = serverPair.second;
		pServer->Clear();
		delete pServer;
	}

	mServers.clear();
}
