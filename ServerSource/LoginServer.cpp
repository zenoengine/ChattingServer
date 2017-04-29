#include "stdafx.h"
#include "LoginServer.h"

LoginServer::LoginServer()
{
}

LoginServer::~LoginServer()
{
}

bool LoginServer::Init()
{
	mMockedLoginDB.insert(make_pair("abc", "123"));
	mMockedLoginDB.insert(make_pair("def", "123"));
	mMockedLoginDB.insert(make_pair("123", "123"));
	return true;
}

void LoginServer::Clear()
{
	mMockedLoginDB.clear();
}

bool LoginServer::TryLogin(string id, string password)
{
	auto foundInfo = mMockedLoginDB.find(id);
	if (foundInfo == mMockedLoginDB.end())
	{
		return false;
	}

	
	if (password.compare(foundInfo->second) != 0)
	{
		return false;
	}

	return true;
}