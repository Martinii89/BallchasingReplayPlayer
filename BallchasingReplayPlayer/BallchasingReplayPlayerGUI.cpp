#include "pch.h"
#include "BallchasingReplayPlayer.h"

/*
// Do ImGui rendering here
void BallchasingReplayPlayer::Render()
{
	if (!ImGui::Begin(menuTitle_.c_str(), &isWindowOpen_, ImGuiWindowFlags_None))
	{
		// Early out if the window is collapsed, as an optimization.
		ImGui::End();
		return;
	}

	ImGui::End();

	if (!isWindowOpen_)
	{
		cvarManager->executeCommand("togglemenu " + GetMenuName());
	}
}

// Name of the menu that is used to toggle the window.
std::string BallchasingReplayPlayer::GetMenuName()
{
	return "BallchasingReplayPlayer";
}

// Title to give the menu
std::string BallchasingReplayPlayer::GetMenuTitle()
{
	return menuTitle_;
}

// Don't call this yourself, BM will call this function with a pointer to the current ImGui context
void BallchasingReplayPlayer::SetImGuiContext(uintptr_t ctx)
{
	ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}

// Should events such as mouse clicks/key inputs be blocked so they won't reach the game
bool BallchasingReplayPlayer::ShouldBlockInput()
{
	return ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
}

// Return true if window should be interactive
bool BallchasingReplayPlayer::IsActiveOverlay()
{
	return true;
}

// Called when window is opened
void BallchasingReplayPlayer::OnOpen()
{
	isWindowOpen_ = true;
}

// Called when window is closed
void BallchasingReplayPlayer::OnClose()
{
	isWindowOpen_ = false;
}
*/
