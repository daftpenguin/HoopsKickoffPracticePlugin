#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"

#include "IMGUI/imgui.h"

#include <unordered_map>

constexpr auto plugin_version = "0.3";
constexpr auto kickoff_delay = 0.15f;
constexpr auto location_epsilon = 0.01f;
constexpr auto num_spawns = 10;
constexpr auto unassigned_loc_btn_txt = "Unassigned";

constexpr auto freeplay_started_event = "Function GameEvent_Soccar_TA.Active.StartRound";
constexpr auto freeplay_ended_event = "Function TAGame.GameEvent_Soccar_TA.Destroyed";
constexpr auto ball_added_event = "Function TAGame.GameEvent_TA.StartEvent";
constexpr auto movement_check_event = "Function TAGame.Car_TA.SetVehicleInput";
constexpr auto ball_lift_added_event = "Function TAGame.Mutator_Ball_TA.ApplyBallLift";

enum SpawnName { blue_left, blue_mid_left, blue_mid, blue_mid_right, blue_right,
				 orange_left, orange_mid_left, orange_mid, orange_mid_right, orange_right };
static const char* SpawnNameStrs[] = {
				"blue_left", "blue_mid_left", "blue_mid", "blue_mid_right", "blue_right",
				"orange_left", "orange_mid_left", "orange_mid", "orange_mid_right", "orange_right" };

typedef struct PlayerData {
	std::string playerID;
	std::string playerName;
};


typedef struct SpawnPoint {
	Vector location;
	Rotator rotation;
};

class HoopsKickoffPractice : public BakkesMod::Plugin::BakkesModPlugin, public BakkesMod::Plugin::PluginWindow
{
	//Boilerplate
	virtual void onLoad();
	virtual void onUnload();

	void onStop();
	void onFreeplayStarted(std::string eventName);
	void onFreeplayEnded(std::string eventName);
	void onBallAdded(std::string eventName);
	void onBallLiftAdded(std::string eventName);
	void checkCarMoved(std::string eventName);
	void setKickoff(std::vector<std::string> params);
	void swapBySpawns(SpawnName x, SpawnName y);

	float hoopsVelocity = -1580;

private:
	bool carHasInput(ArrayWrapper<CarWrapper> cars);
	bool isHoops(ServerWrapper server);
	void assignSpawnLocations(ArrayWrapper<CarWrapper> cars);

	bool delaySet;
	bool ballLiftAdded;
	
	std::unordered_map<std::string, SpawnName> playerToSpawn;
	std::unordered_map<SpawnName, SpawnPoint> spawnNameToPoint = {
		{ blue_left, SpawnPoint{Vector{ 1536, -3072, 36 }, Rotator{ -100, 16384, 0 }} },
		{ blue_mid_left, SpawnPoint{Vector{ 255.999, -2815.99, 36 }, Rotator{ -100, 16384, 0 }} },
		{ blue_mid, SpawnPoint{Vector{ -0.000984, -3199.99, 36 }, Rotator{ -100, 16384, 0 }} },
		{ blue_mid_right, SpawnPoint{Vector{ -256.002, -2815.99, 36 }, Rotator{ -100, 16384, 0 }} },
		{ blue_right, SpawnPoint{Vector{ -1536, -3072, 36 }, Rotator{ -100, 16384, 0 }} },
		{ orange_left, SpawnPoint{Vector{ 1536, 3072, 36 }, Rotator{ -100, -16384, 0 }} },
		{ orange_mid_left, SpawnPoint{Vector{ 255.999, 2815.99, 36 }, Rotator{ -100, -16384, 0 }} },
		{ orange_mid, SpawnPoint{Vector{ -0.000984, 3199.99, 36 }, Rotator{ -100, -16384, 0 }} },
		{ orange_mid_right, SpawnPoint{Vector{ -256.002, 2815.99, 36 }, Rotator{ -100, -16384, 0 }} },
		{ orange_right, SpawnPoint{Vector{ -1536, 3072, 36 }, Rotator{ -100, -16384, 0 }} },
	};
	std::unordered_map<SpawnName, PlayerData> spawnToPlayerData;


/*
 * GUI Stuff
 */

public:
	void Render();
	std::string GetMenuName();
	std::string GetMenuTitle();
	void SetImGuiContext(uintptr_t ctx);
	bool ShouldBlockInput();
	bool IsActiveOverlay();
	void OnOpen();
	void OnClose();

private:
	bool isWindowOpen = false;
	bool shouldBlockInput = true;
	std::string menuTitle = "Hoops Kickoff Practice";
};