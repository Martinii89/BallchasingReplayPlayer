#include "pch.h"
#include "BallchasingReplayPlayer.h"
#include "utils/parser.h"

#include "SettingsManager.h"

#include <fstream>


BAKKESMOD_PLUGIN(BallchasingReplayPlayer, "BallchasingProtocolHandler", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

struct EnumWindowsCallbackArgs
{
	EnumWindowsCallbackArgs(DWORD p) : pid(p) { }
	const DWORD pid;
	std::vector<HWND> handles;
};

static BOOL CALLBACK EnumWindowsCallback(HWND hnd, LPARAM lParam)
{
	EnumWindowsCallbackArgs* args = (EnumWindowsCallbackArgs*)lParam;

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
		return std::vector<HWND>();
	}
	return args.handles;
}

void BallchasingReplayPlayer::MoveGameToFront()
{
	auto handles = getToplevelWindows();
	//LOG("handles: {}", handles.size());
	for (auto* h : handles)
	{
		int const bufferSize = 1 + GetWindowTextLength(h);
		std::wstring title(bufferSize, L'\0');
		int const nChars = GetWindowText(h, &title[0], bufferSize);
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

LinearColor GetPercentageColor(float percent, float alpha)
{
	LinearColor color = {0, 0, 0, 255.f * alpha};
	if (percent <= .5f)
	{
		color.R = 255.f;
		color.G = 0.f + (255.f * (percent * 2.f));
	}
	else
	{
		color.R = 255.f - (255.f * ((percent - 0.5f) * 2));
		color.G = 255.f;
	}

	return color;
}

enum class BlendMode
{
	Opaque = 0,
	Masked = 1,
	Translucent = 2,
	Additive = 3,
	Modulate = 4,
	ModulateAndAdd = 5,
	SoftMasked = 6,
	AlphaComposite = 7,
	DitheredTranslucent = 8,
	MAX = 9
};

void BallchasingReplayPlayer::onLoad()
{
	_globalCvarManager = cvarManager;
	RegisterURIHandler();
	RegisterRCONWhitelist();
	auto progress_bar_path = gameWrapper->GetDataFolder() / "ballchasing" / "progress_bar.png";
	progress_texture = std::make_unique<ImageWrapper>(progress_bar_path);
	pipe_server_thread_ = std::thread{&BallchasingReplayPlayer::StartPipeServer, this};

	//gameWrapper->RegisterDrawable([this](CanvasWrapper canvas)
	//{
	//	static float percent = 0;
	//	static int blend_mode = EBlendMode_BLEND_Masked;
	//	percent += 0.01f;
	//	if (percent > 1)
	//	{
	//		percent = 0;
	//		//blend_mode++;
	//	}
	//	DrawProgressBar(canvas, percent);
	//});


	cvarManager->registerNotifier(notifer_name_, [this](std::vector<std::string> args)
	{
		if (args.size() < 2)
		{
			return;
		}
		auto& command_argument = args[1];
		if (command_argument == "available")
		{
			cvarManager->executeCommand("sendback \"true", false);
		}
		else
		{
			DownloadAndPlayReplay(command_argument);
			cvarManager->executeCommand(
				fmt::format("sendback \"Attempting to download and open the replay with id {}\"", command_argument));
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

void BallchasingReplayPlayer::RegisterRCONWhitelist() const
{
	try
	{
		const auto whitelist_path = gameWrapper->GetDataFolder() / "rcon_commands.cfg";
		auto whitelist = std::fstream{whitelist_path, std::ios::app | std::ios::in | std::ios::out};
		std::string line;
		while (std::getline(whitelist, line))
		{
			if (line == notifer_name_)
			{
				LOG("RCON command already whitelisted");
				return;
			}
		}

		LOG("Adding RCON command to whitelist ({})", notifer_name_);
		whitelist.clear();
		whitelist << std::endl << notifer_name_;
		whitelist.close();
		cvarManager->executeCommand("rcon_refresh_allowed");
	}
	catch (std::exception& e)
	{
		LOG("Exception in RegisterRCONWhitelist:\n{}", e.what());
	}
}

void BallchasingReplayPlayer::RegisterURIHandler() const
{
	RegisterySettingsManager::SaveSetting(L"", L"URL:ballchasing protocol", L"Software\\Classes\\ballchasing",
		HKEY_CURRENT_USER);
	RegisterySettingsManager::SaveSetting(L"URL Protocol", L"ballchasing", L"Software\\Classes\\ballchasing",
		HKEY_CURRENT_USER);
	const auto* const command = L"cmd /c echo  %1 > \\\\.\\pipe\\ballchasing";
	RegisterySettingsManager::SaveSetting(L"", command, L"Software\\Classes\\ballchasing\\shell\\open\\command",
		HKEY_CURRENT_USER);
}

void BallchasingReplayPlayer::DownloadAndPlayReplay(std::string replay_id)
{
	replay_id = trim(replay_id);
	MoveGameToFront();
	const auto download_url = fmt::format("https://ballchasing.com/dl/replay/{}", replay_id);
	const auto save_path = gameWrapper->GetDataFolder() / "ballchasing" / "dl";
	const auto file_path = save_path / (replay_id + ".replay");

	if (exists(file_path) && file_size(file_path) > 0)
	{
		LOG("Replay for this id already downloaded. Using cached file");
		gameWrapper->Execute([this, path = file_path.wstring()](...)
		{
			gameWrapper->PlayReplay(path);
		});
		return;
	}

	CurlRequest req;
	req.url = download_url;
	req.verb = "POST";
	req.progress_function = [this](auto max, auto current, ...)
	{
		// Really spammy 
		//LOG("Download progress: {}/{}", current, max);
		if (max == 0 && current != 0)
		{
			download_progress += 0.05f;
			if (download_progress > 1)
			{
				download_progress = 0;
			}
		}
		else
		{
			download_progress = (current / max);
		}
	};
	download_progress = 0;
	gameWrapper->RegisterDrawable([this](auto canvas) { DrawDownloadProgress(canvas); });


	create_directories(save_path);
	HttpWrapper::SendCurlRequest(req, file_path, [this, replay_id](int code, const std::wstring& path)
	{
		gameWrapper->UnregisterDrawables();
		//LOG("Download request returned");
		if (code != 200)
		{
			try
			{
				remove(std::filesystem::path(path));
			}
			catch (std::filesystem::filesystem_error& e)
			{
				LOG("Error while deleting bad file: {}", e.what());
			}
			if (code == 404)
			{
				LOG("404 response. Trying to download it from the api with a API key");
				DownloadAndPlayReplayWithAuthentication(replay_id);
				return;
			}
			LOG("Error: response code not 200: (was {})", code);
			return;
		}
		gameWrapper->Execute([this, path](...)
		{
			gameWrapper->PlayReplay(path);
		});
	});
}

void BallchasingReplayPlayer::DownloadAndPlayReplayWithAuthentication(std::string replay_id)
{
	const auto api_key = cvarManager->getCvar("cl_autoreplayupload_ballchasing_authkey").getStringValue();
	if (api_key.empty())
	{
		LOG("No api key set in the auto replay uploader. Aborting API download");
		return;
	}
	
	replay_id = trim(replay_id);
	MoveGameToFront();
	const auto download_url = fmt::format("https://ballchasing.com/api/replays/{}/file", replay_id);
	LOG("Requesting {}", download_url);
	const auto save_path = gameWrapper->GetDataFolder() / "ballchasing" / "dl";
	const auto file_path = save_path / (replay_id + ".replay");

	if (exists(file_path) && file_size(file_path) > 0)
	{
		LOG("Replay for this id already downloaded. Using cached file");
		gameWrapper->Execute([this, path = file_path.wstring()](...)
		{
			gameWrapper->PlayReplay(path);
		});
		return;
	}

	CurlRequest req;
	req.url = download_url;
	req.verb = "GET";
	req.headers["Authorization"] = api_key;
	req.progress_function = [this](auto max, auto current, ...)
	{
		// Really spammy
		//LOG("Download progress: {}/{}", current, max);
		
		if (max == 0 && current != 0)
		{
			download_progress += 0.05f;
			if (download_progress > 1)
			{
				download_progress = 0;
			}
		}
		else
		{
			download_progress = (current / max);
		}
	};
	download_progress = 0;
	gameWrapper->RegisterDrawable([this](auto canvas) { DrawDownloadProgress(canvas); });


	create_directories(save_path);
	HttpWrapper::SendCurlRequest(req, file_path, [this, replay_id](int code, const std::wstring& path)
	{
		gameWrapper->UnregisterDrawables();
		//LOG("Download request returned");
		if (code != 200)
		{
			try
			{
				remove(std::filesystem::path(path));
			}
			catch (std::filesystem::filesystem_error& e)
			{
				LOG("Error while deleting bad file: {}", e.what());
			}
			LOG("Error: response code not 200: (was {})", code);
			return;
		}
		gameWrapper->Execute([this, path](...)
		{
			gameWrapper->PlayReplay(path);
		});
	});
}

void BallchasingReplayPlayer::DrawDownloadProgress(CanvasWrapper canvas) const
{
	if (download_progress > 0 && download_progress < 100)
	{
		auto start = canvas.GetSize();
		start.Y *= 0.67; // NOLINT(bugprone-narrowing-conversions)
		start.X *= 0.5; // NOLINT(bugprone-narrowing-conversions)
		canvas.SetPosition(start);

		DrawProgressBar(canvas, download_progress);
	}
}

void BallchasingReplayPlayer::DrawProgressBar(CanvasWrapper canvas, float percent_factor) const
{
	const auto col = GetPercentageColor(percent_factor, 1) / 255.f;

	auto tex_size = progress_texture->GetSizeF();
	auto& width = tex_size.X;
	auto& height = tex_size.Y;
	const auto half_height = height / 2.f;
	const auto progress_width = width * percent_factor;

	const int blend_mode = static_cast<int>(BlendMode::Masked);
	auto start = canvas.GetPosition();
	const auto box_size_half = Vector2{(int)width, (int)half_height} / 2;
	start -= box_size_half;
	canvas.SetPosition(start);

	canvas.DrawTile(progress_texture.get(), width, height / 2.f, 0, half_height, width, half_height, col, false,
		blend_mode);
	canvas.SetPosition(start);
	canvas.DrawTile(progress_texture.get(), progress_width, half_height, 0, 0, progress_width, half_height, col, false,
		blend_mode);

	const auto progress_text = fmt::format("{:.1f}%", percent_factor * 100);
	const auto text_size = canvas.GetStringSize(progress_text, 1, 1);
	const auto center = Vector2F{(float)start.X, (float)start.Y} + Vector2F{width, half_height} / 2.f;
	canvas.SetPosition(center - text_size / 2);
	canvas.SetColor({255, 255, 255, 255});
	canvas.DrawString(progress_text, 1, 1);
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
