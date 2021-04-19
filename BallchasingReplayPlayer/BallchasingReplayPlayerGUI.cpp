#include "pch.h"
#include "BallchasingReplayPlayer.h"

static void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

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
	if (ImGui::Button("Fix command whitelist"))
	{
		RegisterRCONWhitelist();
	}
	ImGui::SameLine();
	HelpMarker("Check the F6 Console for the result");
}