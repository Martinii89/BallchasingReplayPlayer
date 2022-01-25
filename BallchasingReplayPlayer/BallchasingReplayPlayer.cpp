#include "pch.h"
#include "BallchasingReplayPlayer.h"
#include "utils/parser.h"

#include <fstream>

#include "GameWindowFocuser.h"
#include "ScopeExec.h"


BAKKESMOD_PLUGIN(BallchasingReplayPlayer, "BallchasingReplayPlayer", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

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

void BallchasingReplayPlayer::InitWsServer()
{
	ws_server_.StartWebSocketServerAsync(ws_port_);
	ws_server_.AddMessageHandler("UniqueKeyDoesntMatter", [this](auto&& server, auto&& hdl, auto&& msg)
	{
		OnWebSocketMessage(server, hdl, msg);
	});
}

void BallchasingReplayPlayer::onLoad()
{
	_globalCvarManager = cvarManager;

	auto progress_bar_path = gameWrapper->GetDataFolder() / "ballchasing" / "progress_bar.png";
	progress_texture = std::make_unique<ImageWrapper>(progress_bar_path);


	InitWsServer();
}

void BallchasingReplayPlayer::onUnload()
{
}


void BallchasingReplayPlayer::DownloadAndPlayReplay(std::string replay_id, std::string token)
{
	replay_id = trim(replay_id);
	GameWindowFocuser::MoveGameToFront();
	const auto download_url = fmt::format("https://ballchasing.com/dl/replay/{}?token={}", replay_id, token);
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
	LOG("Attempting to download the replay with id {}", replay_id);

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
		start.Y *= 0.67;
		start.X *= 0.5;
		canvas.SetPosition(start);

		DrawProgressBar(canvas, download_progress);
	}
}

void BallchasingReplayPlayer::DrawProgressBar(CanvasWrapper canvas, float percent_factor) const
{
	const auto col = GetPercentageColor(percent_factor, 1) / 255.f;

	const auto tex_size = progress_texture->GetSizeF();
	const auto& width = tex_size.X;
	const auto& height = tex_size.Y;
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


void BallchasingReplayPlayer::OnWebSocketMessage(WsServer::Server* s, websocketpp::connection_hdl const& hdl,
                                                 WsServer::MessagePtr const& message)
{
	const auto msg = message->get_payload();
	auto command_list = parseConsoleInput(msg);
	ScopeExec dont_forget{[command_list] { delete command_list; }};

	for (const auto& args : *command_list)
	{
		if (args.empty() || args.size() < 2 || args[0] != notifer_name_) continue;

		auto& command_argument = args[1];
		if (command_argument == "available")
		{
			ws_server_.Send(hdl, "true");
			return;
		}
		std::string token;
		if (args.size() >= 3)
		{
			token = args[2];
		}
		DownloadAndPlayReplay(command_argument, token);
		ws_server_.Send(hdl, fmt::format("Attempting to download and open the replay with id {}", command_argument));
	}
}
