#include "pch.h"
#include "HoopsKickoffPractice.h"

#include <sstream>

using namespace std;

std::vector<SpawnName> SpawnNameOrder = { blue_left, blue_mid_left, blue_mid, blue_mid_right, blue_right,
										  orange_left, orange_mid_left, orange_mid, orange_mid_right, orange_right };

void displayError(std::string err)
{
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(255, 0, 0, 255));
	ImGui::TextWrapped(err.c_str());
	ImGui::PopStyleColor();
}

void HoopsKickoffPractice::Render()
{
	if (!this->isWindowOpen) {
		cvarManager->executeCommand("togglemenu " + GetMenuName());
		return;
	}

	ImGuiWindowFlags windowFlags = 0; //| ImGuiWindowFlags_MenuBar;

	ImGui::SetNextWindowSizeConstraints(ImVec2(450, 200), ImVec2(FLT_MAX, FLT_MAX));
	if (!ImGui::Begin(GetMenuTitle().c_str(), &this->isWindowOpen, windowFlags)) {
		// Early out if the window is collapsed, as an optimization
		this->shouldBlockInput = ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
		ImGui::End();
		return;
	}

	if (!gameWrapper->IsInFreeplay()) {
		displayError("Must be in freeplay");
	}
	else {
		auto server = gameWrapper->GetGameEventAsServer();
		if (server.IsNull()) {
			displayError("Failed to get server data");
		}
		else {
			auto cars = server.GetCars();
			if (cars.Count() == 0) {
				displayError("Failed to find cars");
			}
			else {
				ImGui::TextWrapped("Drag a player's name into another location to change their spawn location");
				ImGui::TextWrapped("Choose Hoops as the game mode when creating a hoops freeplay session in Rocket Plugin");

				// Players should already be assigned to positions
				ImGuiDragDropFlags src_drag_flags = 0;// | ImGuiDragDropFlags_SourceNoDisableHover | ImGuiDragDropFlags_SourceNoHoldToOpenOthers | ImGuiDragDropFlags_SourceNoPreviewTooltip;
				ImGuiDragDropFlags tgt_drag_flags = 0;// | ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect;

				ImGui::Columns(2);
				for (enum SpawnName spawn : SpawnNameOrder) {
					if (spawn == 0) {
						ImGui::Separator();
					} else if (spawn == num_spawns / 2) {
						ImGui::NextColumn();
					}

					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(255, 255, 255, 255));
					if (spawn < num_spawns / 2) {
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 1.0f, 1.0f));
					}
					else {
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.5f, 0, 1.0f));
					}

					std::string assigned = unassigned_loc_btn_txt;
					auto it = spawnToPlayerData.find(spawn);
					if (it != spawnToPlayerData.end()) {
						assigned = it->second.playerName;
					}

					float y = spawnNameToPoint[spawn].location.Y;
					int offset = roundf((3200 - abs(y)) / 120.0f) * 10.0f;
					cvarManager->log("y: " + std::to_string(y) + ", indent: " + std::to_string(offset));
					
					if (spawn < num_spawns / 2) { // left aligned, use indent for offset
						if (offset > 1) {
							ImGui::Indent(offset);
						}

						ImGui::Button(assigned.c_str());

						if (offset > 1) {
							ImGui::Unindent(offset);
						}
					}
					else { // right aligned, use SetCursorPosX for offset
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(assigned.c_str()).x - ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x - 5 - offset);
						ImGui::Button(assigned.c_str());
					}

					if (ImGui::BeginDragDropSource(src_drag_flags)) {
						ImGui::SetDragDropPayload("PLAYER_SPAWN_LOCATION", &spawn, sizeof(SpawnName), ImGuiCond_Once);
						ImGui::Button(unassigned_loc_btn_txt);
						ImGui::EndDragDropSource();
					}

					if (ImGui::BeginDragDropTarget()) {
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PLAYER_SPAWN_LOCATION", tgt_drag_flags)) {						
							swapBySpawns(spawn, *(SpawnName *) payload->Data);
							ImGui::EndDragDropTarget();
						}
					}

					ImGui::PopStyleColor();
					ImGui::PopStyleColor();
				}
			}
		}
	}

	ImGui::End();

	this->shouldBlockInput = ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
}

std::string HoopsKickoffPractice::GetMenuName()
{
	return "hoopskickoffpractice";
}

std::string HoopsKickoffPractice::GetMenuTitle()
{
	return menuTitle;
}

void HoopsKickoffPractice::SetImGuiContext(uintptr_t ctx)
{
	ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}

bool HoopsKickoffPractice::ShouldBlockInput()
{
	return this->shouldBlockInput;
}

bool HoopsKickoffPractice::IsActiveOverlay()
{
	return true;
}

void HoopsKickoffPractice::OnOpen()
{
	this->isWindowOpen = true;
}

void HoopsKickoffPractice::OnClose()
{
	this->isWindowOpen = false;
}