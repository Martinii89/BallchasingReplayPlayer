#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"

#include "version.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);


class BallchasingReplayPlayer: public BakkesMod::Plugin::BakkesModPlugin/*, public BakkesMod::Plugin::PluginWindow*/
{
	static void MoveGameToFront();
	//std::shared_ptr<bool> enabled;
	//Boilerplate
	virtual void onLoad();
	virtual void onUnload();
	void RegisterURIHandler() const;
	void DownloadAndPlayReplay(std::string replay_id);

	float download_progress = 0;
	std::unique_ptr<ImageWrapper> progress_texture;
	
	void DrawDownloadProgress(CanvasWrapper canvas) const;
	void DrawProgressBar(CanvasWrapper canvas, float percent_factor) const;
	

	bool pipe_server_running = true;
	std::thread pipe_server_thread_;
	void StartPipeServer();

	// Inherited via PluginWindow
	/*

	bool isWindowOpen_ = false;
	bool isMinimized_ = false;
	std::string menuTitle_ = "BallchasingReplayPlayer";

	virtual void Render() override;
	virtual std::string GetMenuName() override;
	virtual std::string GetMenuTitle() override;
	virtual void SetImGuiContext(uintptr_t ctx) override;
	virtual bool ShouldBlockInput() override;
	virtual bool IsActiveOverlay() override;
	virtual void OnOpen() override;
	virtual void OnClose() override;
	
	*/
};

