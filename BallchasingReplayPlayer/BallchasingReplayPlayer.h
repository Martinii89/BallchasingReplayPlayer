#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"

#include "version.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);


class BallchasingReplayPlayer: public BakkesMod::Plugin::BakkesModPlugin, public BakkesMod::Plugin::PluginSettingsWindow
{
public:
	
	static void MoveGameToFront();
	const std::string notifer_name_ = "ballchasing_viewer";
	
	//Boilerplate
	virtual void onLoad();
	virtual void onUnload();
	void RegisterRCONWhitelist() const;
	void RegisterURIHandler() const;
	void DownloadAndPlayReplay(std::string replay_id, std::string token = "");

	float download_progress = 0;
	std::unique_ptr<ImageWrapper> progress_texture;
	
	void DrawDownloadProgress(CanvasWrapper canvas) const;
	void DrawProgressBar(CanvasWrapper canvas, float percent_factor) const;
	

	bool pipe_server_running = true;
	std::thread pipe_server_thread_;
	void StartPipeServer();
	
	void RenderSettings() override;
	std::string GetPluginName() override;
	void SetImGuiContext(uintptr_t ctx) override;

};

