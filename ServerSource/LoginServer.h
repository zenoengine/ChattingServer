#pragma once
#include "IServer.h"


class LoginServer : public IServer
{
public:
	LoginServer();
	virtual ~LoginServer();

	void Run() override {};
	bool Init() override;
	void Clear() override;

	bool TryLogin(string id, string password);
private:
	map<string, string> mMockedLoginDB;

};

