#pragma once
#include "Packet.h"

namespace Server
{
	enum ServerType
	{
		ST_BEGIN = 0,
		ST_COMMAND,
		ST_CHAT,
		ST_LOGIN,
		ST_END,
	};

	class IServer
	{
	public:
		virtual bool Init() = 0;
		virtual void Run() = 0;
		virtual void Clear() = 0;
	};
}


using namespace Server;