#include "pch.h"
#include "GameWindowFocuser.h"

#include "Windows.h"

struct EnumWindowsCallbackArgs
{
	EnumWindowsCallbackArgs(DWORD p) : pid(p)
	{
	}

	const DWORD pid;
	std::vector<HWND> handles;
};

static BOOL CALLBACK EnumWindowsCallback(HWND hnd, LPARAM lParam)
{
	const auto args = (EnumWindowsCallbackArgs*)lParam;

	DWORD windowPID;
	(void)::GetWindowThreadProcessId(hnd, &windowPID);
	if (windowPID == args->pid)
	{
		args->handles.push_back(hnd);
	}

	return TRUE;
}

std::vector<HWND> getToplevelWindows()
{
	EnumWindowsCallbackArgs args(::GetCurrentProcessId());
	if (::EnumWindows(&EnumWindowsCallback, (LPARAM)&args) == FALSE)
	{
		// XXX Log error here
		return {};
	}
	return args.handles;
}

void GameWindowFocuser::MoveGameToFront()
{
	const auto handles = getToplevelWindows();
	//LOG("handles: {}", handles.size());
	for (auto* h : handles)
	{
		int const bufferSize = 1 + GetWindowTextLength(h);
		std::wstring title(bufferSize, L'\0');

		if (title.find(L"Rocket League (64-bit, DX11, Cooked)") != std::wstring::npos)
		{
			::SetForegroundWindow(h);
			::ShowWindow(h, SW_RESTORE);
			break;
		}
	}
}
