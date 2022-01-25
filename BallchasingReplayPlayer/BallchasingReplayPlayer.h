#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"

#include "version.h"
#include "WsServer.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "."
	stringify(VERSION_BUILD);


class BallchasingReplayPlayer : public BakkesMod::Plugin::BakkesModPlugin,
                                public BakkesMod::Plugin::PluginSettingsWindow
{
public:
	//Boilerplate
	void onLoad() override;
	void onUnload() override;

	void RenderSettings() override;
	std::string GetPluginName() override;
	void SetImGuiContext(uintptr_t ctx) override;

private:
	const std::string notifer_name_ = "ballchasing_viewer";
	WsServer ws_server_;
	const uint16_t ws_port_ = 9420;

	float download_progress = 0;
	std::unique_ptr<ImageWrapper> progress_texture;

	void InitWsServer();

	void DownloadAndPlayReplay(std::string replay_id, std::string token = "");
	void OnWebSocketMessage(WsServer::Server* s, websocketpp::connection_hdl const& hdl,
	                        WsServer::MessagePtr const& message);


	void DrawDownloadProgress(CanvasWrapper canvas) const;
	void DrawProgressBar(CanvasWrapper canvas, float percent_factor) const;
};
