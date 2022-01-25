#include "pch.h"
#include "BallchasingReplayPlayer.h"

std::string BallchasingReplayPlayer::GetPluginName()
{
	return "Ballchasing Replay Viewer";
}

void BallchasingReplayPlayer::SetImGuiContext(uintptr_t ctx)
{
	ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}

void BallchasingReplayPlayer::RenderSettings()
{
    ImGui::Text("Open the ballchasing website while the plugin is running.\nThe website will show new buttons for downloading and playing the replays");
}