#pragma comment(lib, "ws2_32.lib")
#include "stdafx.h"
#include "ServerManager.h"

int main()
{
	ServerManager serverManager;
	
	if (serverManager.Init())
	{
		serverManager.Run();
	}

	serverManager.Clear();

	return 0;
}