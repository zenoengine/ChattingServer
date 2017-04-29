// ImGui - standalone example application for DirectX 9
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <atlstr.h>

#include <imgui.h>
#include "imgui_impl_dx9.h"
#include <d3d9.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <queue>

#include "Packet.h"

const int BUFFSIZE = 1024;
#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE    1024

using namespace std;
using namespace Packet;

CRITICAL_SECTION gCS;


vector<string> gChatLog;
queue<PacketBase*> gPacketQueue;

enum ClientState
{
	CS_LOGIN = 0,
	CS_LOGIN_WAITING,
	CS_CHAT,
	CS_EXIT
};


ClientState state = CS_LOGIN;
string userID;

DWORD WINAPI ProcessRecv(LPVOID arg)
{
	SOCKET sock = (SOCKET)arg;

	char recvBuffer[BUFFSIZE + 1];
	while (1)
	{
		if (state == CS_EXIT)
		{
			return 0;
		}

		int retval = recv(sock, recvBuffer, BUFFSIZE, 0);
		if (retval == SOCKET_ERROR) {
			state = CS_EXIT;
			break;
		}
		else if (retval == 0)
			break;

		EnterCriticalSection(&gCS);
		std::unordered_map<string, string> headers;
		string headerString = recvBuffer;
		if (ParseHeader(recvBuffer, &headers))
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
				string message;
				gPacketQueue.push(packet);
			}
		}
		LeaveCriticalSection(&gCS);
	}

	return 0;
}

void ProcessMessageQueue()
{
	EnterCriticalSection(&gCS);
	while (!gPacketQueue.empty())
	{
		PacketBase* packet = gPacketQueue.front();
		switch (packet->type)
		{
		case PT_LOGIN_RESULT:
		{
			LoginResult* loginResult = static_cast<LoginResult*>(packet);

			if (loginResult->bSuccessd)
			{
				state = CS_CHAT;
				userID = loginResult->id;
			}
			else
			{
				state = CS_LOGIN;
			}
		}
		break;

		case PT_MESSAGE:
		{
			MessagePacket* messagePacket = static_cast<MessagePacket*>(packet);
			string chatMessage;
			chatMessage.append(messagePacket->senderID);
			chatMessage.append(":");
			chatMessage.append(messagePacket->message);
			if (gChatLog.size() > 100)
			{
				for (size_t idx = 0; idx < 50; idx++)
				{
					gChatLog.erase(gChatLog.begin());
				}
			}
			gChatLog.push_back(chatMessage);
		}
		break;
		}

		delete[] packet;

		gPacketQueue.pop();
	}
	LeaveCriticalSection(&gCS);
}

// Data
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp;

extern LRESULT ImGui_ImplDX9_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplDX9_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
		{
			ImGui_ImplDX9_InvalidateDeviceObjects();
			g_d3dpp.BackBufferWidth = LOWORD(lParam);
			g_d3dpp.BackBufferHeight = HIWORD(lParam);
			HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
			if (hr == D3DERR_INVALIDCALL)
				IM_ASSERT(0);
			ImGui_ImplDX9_CreateDeviceObjects();
		}
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		state = CS_EXIT;
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

const size_t STRING_MAX = 256;
void WideToUTF8(wchar_t* strUni, char* strUtf8)
{
	int nLen = WideCharToMultiByte(CP_UTF8, 0, strUni, lstrlenW(strUni), NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_UTF8, 0, strUni, lstrlenW(strUni), strUtf8, nLen, NULL, NULL);
}
void UTF8ToWide(string& strMulti, wstring& outString)
{
	int nLen = MultiByteToWideChar(CP_UTF8, 0, &strMulti[0], strMulti.size(), NULL, NULL);
	outString.resize(nLen);
	MultiByteToWideChar(CP_UTF8, 0, &strMulti[0], strMulti.size(), &outString[0], nLen);
}

int main(int, char**)
{
	DWORD customPort = SERVERPORT;
	string ipAddress = SERVERIP;

	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// socket()
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	assert(sock != INVALID_SOCKET);

	// connect()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(ipAddress.c_str());
	serveraddr.sin_port = htons(customPort);
	retval = connect(sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	assert(retval != SOCKET_ERROR);


	// Create application window
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, LoadCursor(NULL, IDC_ARROW), NULL, NULL, _T("ImGui Example"), NULL };
	RegisterClassEx(&wc);
	HWND hwnd = CreateWindow(_T("ImGui Example"), _T("멀티 채팅 클라이언트"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

	InitializeCriticalSection(&gCS);
	CreateThread(0, 0, &ProcessRecv, (LPVOID)sock, 0, 0);

	// Initialize Direct3D
	LPDIRECT3D9 pD3D;
	if ((pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
	{
		UnregisterClass(_T("ImGui Example"), wc.hInstance);
		return 0;
	}
	ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
	g_d3dpp.Windowed = TRUE;
	g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	g_d3dpp.EnableAutoDepthStencil = TRUE;
	g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	// Create the D3DDevice
	if (pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
	{
		pD3D->Release();
		UnregisterClass(_T("ImGui Example"), wc.hInstance);
		return 0;
	}

	// Setup ImGui binding
	ImGui_ImplDX9_Init(hwnd, g_pd3dDevice);

	// Load Fonts
	// (there is a default font, this is only if you want to change it. see extra_fonts/README.txt for more details)
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF("../../extra_fonts/NanumGothic.ttf", 14.0f, NULL, io.Fonts->GetGlyphRangesKorean());

	bool show_test_window = true;
	bool show_another_window = false;
	ImVec4 clear_col = ImColor(114, 144, 154);

	// Main loop
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	ShowWindow(hwnd, SW_SHOWDEFAULT);
	UpdateWindow(hwnd);

	char idBuf[STRING_MAX] = { 0, };
	char pwBuf[STRING_MAX] = { 0, };
	char labelUtf8[STRING_MAX] = { 0, };

	WideToUTF8(L"로그인", labelUtf8);

	while (msg.message != WM_QUIT)
	{
		ProcessMessageQueue();

		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			continue;
		}

		ImGui_ImplDX9_NewFrame();

		if (state == CS_LOGIN || state == CS_LOGIN_WAITING)
		{
			ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiSetCond_FirstUseEver);
			ImGui::Begin("Login", &show_another_window);
			ImGui::InputText("id", idBuf, STRING_MAX);
			ImGui::InputText("pw", pwBuf, STRING_MAX, ImGuiInputTextFlags_Password);
			if (ImGui::Button(labelUtf8, ImVec2(100, 30)))
			{
				wstring wStringID;
				UTF8ToWide(string(idBuf), wStringID);
				string id = CW2A(wStringID.c_str());

				wstring wStringPW;
				UTF8ToWide(string(pwBuf), wStringPW);
				string pw = CW2A(wStringPW.c_str());

				Packet::LoginPacket loginPacket;

				strcpy(loginPacket.id, id.c_str());
				strcpy(loginPacket.password, pw.c_str());

				PacketBase* pBase = &loginPacket;
				char packet[BUFSIZE];
				if (MakePacket(packet, BUFSIZE, &loginPacket, sizeof(loginPacket)))
				{
					state = CS_LOGIN_WAITING;
					retval = send(sock, packet, BUFSIZE, 0);
					if (retval == SOCKET_ERROR)
					{
						break;
					}
				}

			}

			ImGui::End();
		}

		if (state == CS_CHAT)
		{
			ImGui::SetNextWindowSize(ImVec2(800, 512), ImGuiSetCond_FirstUseEver | ImGuiWindowFlags_NoResize);
			ImGui::SetNextWindowPos(ImVec2(0, 0));
			ImGui::Begin("ChatBox", &show_another_window);

			for (auto text : gChatLog)
			{
				ImGui::Text(text.c_str());
			}
			ImGui::SetScrollPosHere();

			ImGui::End();
			ImGui::SetNextWindowSize(ImVec2(800, 100));
			ImGui::SetNextWindowPos(ImVec2(0, 512));
			ImGui::Begin("InputBox", &show_another_window, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
			char buf[STRING_MAX] = { 0, };


			bool modified = ImGui::InputText("input", buf, 256, ImGuiInputTextFlags_EnterReturnsTrue);
			ImGui::SetKeyboardFocusHere();

			string message = buf;
			if (modified && !message.empty())
			{
				char packet[BUFFSIZE];
				if (message.find("/bye") != string::npos)
				{
					Packet::RequestExitPakcet exitPacket;
					MakePacket(packet, BUFFSIZE, &exitPacket, sizeof(exitPacket));
					state = CS_EXIT;
				}
				else
				{
					Packet::MessagePacket messagePacket;
					strcpy(messagePacket.senderID, userID.c_str());
					messagePacket.type = Packet::PacketType::PT_MESSAGE;
					strcpy(messagePacket.message, message.c_str());
					MakePacket(packet, BUFFSIZE, &messagePacket, sizeof(messagePacket));
				}

				retval = send(sock, packet, BUFSIZE, 0);

				if (retval == SOCKET_ERROR) {
					break;
				}
			}
			ImGui::End();

		}

		if (state == CS_EXIT)
		{
			break;
		}

		// Rendering
		g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, false);
		g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
		g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, false);
		D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_col.x*255.0f), (int)(clear_col.y*255.0f), (int)(clear_col.z*255.0f), (int)(clear_col.w*255.0f));
		g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
		if (g_pd3dDevice->BeginScene() >= 0)
		{
			ImGui::Render();
			g_pd3dDevice->EndScene();
		}
		g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
	}

	ImGui_ImplDX9_Shutdown();
	if (g_pd3dDevice) g_pd3dDevice->Release();
	if (pD3D) pD3D->Release();
	UnregisterClass(_T("ImGui Example"), wc.hInstance);

	DeleteCriticalSection(&gCS);

	return 0;
}
