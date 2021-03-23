#include "pch.h"
#include "BallchasingReplayPlayer.h"
#include "utils/parser.h"

#include "SettingsManager.h"

#include <fstream>


BAKKESMOD_PLUGIN(BallchasingReplayPlayer, "BallchasingProtocolHandler", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

struct EnumWindowsCallbackArgs {
    EnumWindowsCallbackArgs( DWORD p ) : pid( p ) { }
    const DWORD pid;
    std::vector<HWND> handles;
};

static BOOL CALLBACK EnumWindowsCallback( HWND hnd, LPARAM lParam )
{
    EnumWindowsCallbackArgs *args = (EnumWindowsCallbackArgs *)lParam;

    DWORD windowPID;
    (void)::GetWindowThreadProcessId( hnd, &windowPID );
    if ( windowPID == args->pid ) {
        args->handles.push_back( hnd );
    }

    return TRUE;
}

std::vector<HWND> getToplevelWindows()
{
    EnumWindowsCallbackArgs args( ::GetCurrentProcessId() );
    if ( ::EnumWindows( &EnumWindowsCallback, (LPARAM) &args ) == FALSE ) {
      // XXX Log error here
      return std::vector<HWND>();
    }
    return args.handles;
}

void BallchasingReplayPlayer::MoveGameToFront() {
	auto handles = getToplevelWindows();
	LOG("handles: {}", handles.size());
	for(auto* h: handles)
	{
		int const   bufferSize  = 1 + GetWindowTextLength( h );
		std::wstring     title( bufferSize, L'\0' );
		int const   nChars  = GetWindowText( h, &title[0], bufferSize );
		if (title.find(L"Rocket League (64-bit, DX11, Cooked)") != std::wstring::npos)
		{
			::SetForegroundWindow(h);
			::ShowWindow(h, SW_RESTORE);
			//::SetWindowPos(h,       // handle to window
			//	HWND_TOPMOST,  // placement-order handle
			//	0,     // horizontal position
			//	0,      // vertical position
			//	0,  // width
			//	0, // height
			//	SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE// window-positioning options
			//);
			break;
		}
	}
}

void BallchasingReplayPlayer::onLoad()
{
	_globalCvarManager = cvarManager;
	RegisterURIHandler();
	pipe_server_thread_ = std::thread{&BallchasingReplayPlayer::StartPipeServer, this};

	cvarManager->registerNotifier("ballchasing_viewer", [this](std::vector<std::string> args)
	{
		if (args.size() < 2)
		{
			return;
		}
		auto& command_argument = args[1];
		if (command_argument == "available")
		{
			cvarManager->executeCommand("sendback \"true");
		}else
		{
			DownloadAndPlayReplay(command_argument);
			cvarManager->executeCommand(fmt::format("sendback \"Attempting to download and open the replay with id {}\"", command_argument));
		}
		
	}, "", 0);
}

void BallchasingReplayPlayer::onUnload()
{
	pipe_server_running = false;
	CancelSynchronousIo(pipe_server_thread_.native_handle());
	if (pipe_server_thread_.joinable())
	{
		//LOG("Joining pipe server thread");
		pipe_server_thread_.join();
	}
}

void BallchasingReplayPlayer::RegisterURIHandler() const
{
	RegisterySettingsManager::SaveSetting(L"", L"URL:ballchasing protocol", L"Software\\Classes\\ballchasing", HKEY_CURRENT_USER);
	RegisterySettingsManager::SaveSetting(L"URL Protocol", L"ballchasing", L"Software\\Classes\\ballchasing", HKEY_CURRENT_USER);
	const auto* const command = L"cmd /c echo  %1 > \\\\.\\pipe\\ballchasing";
	RegisterySettingsManager::SaveSetting(L"", command, L"Software\\Classes\\ballchasing\\shell\\open\\command", HKEY_CURRENT_USER);
}

void BallchasingReplayPlayer::DownloadAndPlayReplay(std::string replay_id)
{
	//std::vector<std::string> splitted;
	//split(replay_id, splitted, '/');
	//replay_id = splitted[splitted.size() - 1];
	replay_id = trim(replay_id);
	MoveGameToFront();
	const auto download_url = fmt::format("https://ballchasing.com/dl/replay/{}", replay_id);
	HttpWrapper::SendRequest(download_url, "POST", {}, [this, replay_id](HttpResponseWrapper res)
	{
		if (!res)
		{
			LOG("Invalid response");
			return;
		}
		if (res.GetResponseCode() != 200)
		{
			LOG("wrong status code for replay download: {}", res.GetResponseCode());
			//LOG("{}", res.GetContentAsString());
			return;
		}
		auto [data, size] = res.GetContent();
		//LOG("Request content size: {}", size);
		const auto save_path = gameWrapper->GetDataFolder() / "ballchasing" / "dl";
		const auto file_path = save_path / (replay_id + ".replay");
		create_directories(save_path);
		auto out = std::ofstream(file_path, std::ios_base::binary);
		if (out)
		{
			out.write(data, size);
			out.close();
		}
		gameWrapper->PlayReplay(file_path.wstring());
	});
}

void BallchasingReplayPlayer::StartPipeServer()
{
	// This is 99% copy paste from some stackoverflow post
	char buffer[1024];
	DWORD dwRead;


	HANDLE hPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\ballchasing"),
		PIPE_ACCESS_INBOUND,
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
		// FILE_FLAG_FIRST_PIPE_INSTANCE is not needed but forces CreateNamedPipe(..) to fail if the pipe already exists...
		PIPE_UNLIMITED_INSTANCES,
		1024 * 16,
		1024 * 16,
		NMPWAIT_USE_DEFAULT_WAIT,
		NULL);

	if (hPipe == INVALID_HANDLE_VALUE)
	{
		int errorno = GetLastError();
		LOG("Error creating pipe {}", errorno);
		return;
	}

	while (hPipe != INVALID_HANDLE_VALUE)
	{
		if (!pipe_server_running)
		{
			//LOG("Stopping server");
			break;
		}
		//LOG("Checking pipe");
		if (ConnectNamedPipe(hPipe, NULL) != FALSE) // wait for someone to connect to the pipe
		{
			while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &dwRead, NULL) != FALSE)
			{
				/* add terminating zero */
				buffer[dwRead] = '\0';
				gameWrapper->Execute([this, msg = std::string(buffer)](...) { DownloadAndPlayReplay(msg); });
			}
		}
		DisconnectNamedPipe(hPipe);
	}
	pipe_server_running = false;
	//LOG("Pipe server stopping");
}
