#pragma once
#include <unordered_map>
#include <string>
using namespace std;

namespace Packet
{
	enum PacketType
	{
		BEGIN = 0,
		PT_COMMAND,
		PT_LOGIN,
		PT_LOGIN_RESULT,
		PT_MESSAGE,
		PT_REQUEST_EXIT,
		PT_NONE,
		END,
	};

	struct PacketBase
	{
		PacketBase()
		{
			type = PT_NONE;
		}

		PacketType type;
	};

	struct LoginPacket : public PacketBase
	{
		LoginPacket()
		{
			type = PT_LOGIN;
		};

		char id[32];
		char password[32];
	};

	struct LoginResult : public PacketBase
	{
		LoginResult()
		{
			type = PT_LOGIN_RESULT;
		};

		bool bSuccessd;
		char id[32];
	};

	struct MessagePacket : public PacketBase
	{
		MessagePacket()
		{
			type = PT_MESSAGE;
		};

		char senderID[32];
		char message[256];
	};

	struct RequestExitPakcet : public PacketBase
	{
		RequestExitPakcet()
		{
			type = PT_REQUEST_EXIT;
		}
	};
}

//TODO : Return dest size, use new operator
static bool MakePacket(char* dest, const size_t destSize, const Packet::PacketBase* body, const size_t bodySize)
{
	if (dest == nullptr || body == nullptr)
	{
		return false;
	}

	if (destSize < bodySize)
	{
		return false;
	}

	size_t contentLength = bodySize;
	string header;
	header.append("Content-Length: ");
	header.append(to_string(contentLength));
	header.append("\r\n");
	header.append("\r\n");

	memset(dest, 0, destSize);
	strcpy(dest, header.c_str());
	memcpy(dest + header.length(), body, contentLength);
	return true;
}

static bool ParseHeader(const string& headerString, unordered_map<string, string>* headers)
{
	if (headers == nullptr)
	{
		return false;
	}

	headers->clear();

	auto foundEndOfHeader = headerString.find_last_of("\r\n\r\n");
	if (foundEndOfHeader != string::npos)
	{
		size_t lastPos = 0;
		size_t lastBreak = headerString.find_last_of("\r\n");
		while (lastPos != lastBreak)
		{
			size_t colonPos = headerString.find(": ", lastPos);
			string leftValue = headerString.substr(lastPos, colonPos - lastPos);
			lastPos = colonPos + 2;

			size_t endOfLine = headerString.find("\r\n", lastPos);
			string rightValue = headerString.substr(lastPos, endOfLine - lastPos);
			lastPos = endOfLine + 3;

			headers->insert(make_pair(leftValue, rightValue));
		}
	}

	if (headers->empty())
	{
		return false;
	}

	return true;
}