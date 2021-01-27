// TODO: Freeze players until host moves? Or just add a countdown? Still need to figure this out for team training plugin...
// TODO: Dpad to cycle spawns when car is still in spawn
// TODO: Add a bot for buddy kickoff

#include "pch.h"
#include "HoopsKickoffPractice.h"

#include <sstream>
#include <unordered_set>

BAKKESMOD_PLUGIN(HoopsKickoffPractice, "Hoops Kickoff Practice", plugin_version, PLUGINTYPE_FREEPLAY | PERMISSION_CUSTOM_TRAINING);

std::string GetUniqueID(PlayerReplicationInfoWrapper pri) {
	if (pri.IsNull()) {
		return "";
	}

	std::string uid = pri.GetUniqueIdWrapper().str();
	if (uid == "0") {
		auto name = pri.GetPlayerName();
		if (name.IsNull()) {
			return "";
		}
		return name.ToString();
	}

	return uid;
}

void HoopsKickoffPractice::onLoad()
{
	gameWrapper->HookEventPost(freeplay_started_event,
		std::bind(&HoopsKickoffPractice::onFreeplayStarted, this, std::placeholders::_1));

	gameWrapper->HookEventPost(freeplay_ended_event,
		std::bind(&HoopsKickoffPractice::onFreeplayEnded, this, std::placeholders::_1));
}

void HoopsKickoffPractice::onUnload()
{
}

void HoopsKickoffPractice::onFreeplayStarted(std::string eventName)
{
	if (!gameWrapper->IsInFreeplay()) {
		return;
	}

	auto server = gameWrapper->GetGameEventAsServer();
	if (server.IsNull()) {
		cvarManager->log("Server is null");
		return;
	}

	if (!isHoops(server)) {
		return;
	}

	gameWrapper->UnhookEventPost(freeplay_started_event);

	gameWrapper->HookEventPost(ball_added_event,
		std::bind(&HoopsKickoffPractice::onBallAdded, this, std::placeholders::_1));
	gameWrapper->HookEventPost(ball_lift_added_event,
		std::bind(&HoopsKickoffPractice::onBallLiftAdded, this, std::placeholders::_1));
}

void HoopsKickoffPractice::onFreeplayEnded(std::string eventName)
{
	gameWrapper->HookEventPost(freeplay_started_event,
		std::bind(&HoopsKickoffPractice::onFreeplayStarted, this, std::placeholders::_1));

	gameWrapper->UnhookEventPost(ball_added_event);
	gameWrapper->UnhookEventPost(movement_check_event);
	gameWrapper->UnhookEventPost(ball_lift_added_event);
}

void HoopsKickoffPractice::onStop()
{
	gameWrapper->UnhookEventPost(movement_check_event);
	gameWrapper->UnhookEventPost(ball_added_event);
}

void HoopsKickoffPractice::onBallAdded(std::string eventName)
{
	ballLiftAdded = false;

	if (!gameWrapper->IsInFreeplay()) {
		return;
	}

	auto server = gameWrapper->GetGameEventAsServer();
	if (server.IsNull()) {
		return;
	}

	if (!isHoops(server)) {
		return;
	}

	auto ball = server.GetBall();
	if (ball.IsNull()) {
		return;
	}

	auto cars = server.GetCars();
	if (cars.IsNull()) {
		return;
	}
	if (cars.Count() == 0) {
		return;
	}

	assignSpawnLocations(cars);

	for (int i = 0; i < cars.Count(); i++) {
		auto car = cars.Get(i);
		if (!car.IsNull()) {
			auto pri = car.GetPRI();
			if (!pri.IsNull()) {
				SpawnName spawn = playerToSpawn[GetUniqueID(pri)];
				SpawnPoint point = spawnNameToPoint[spawn];
				car.Teleport(point.location, point.rotation, 0, 0, 0);
			}
		}
	}

	// If ball was set by the game event, the game will provide the velocity if the player is currently providing input
	if (eventName.size() == 0 || !carHasInput(cars)) {
		delaySet = false;
		gameWrapper->HookEventPost("Function GameEvent_Soccar_TA.Active.Tick",
			std::bind(&HoopsKickoffPractice::checkCarMoved, this, std::placeholders::_1));
	}
}

void HoopsKickoffPractice::onBallLiftAdded(std::string eventName)
{
	ballLiftAdded = true;
}

void HoopsKickoffPractice::checkCarMoved(std::string eventName)
{
	if (!gameWrapper->IsInFreeplay()) {
		gameWrapper->UnhookEventPost("Function GameEvent_Soccar_TA.Active.Tick");
		return;
	}

	auto server = gameWrapper->GetGameEventAsServer();
	if (server.IsNull()) {
		return;
	}

	auto ball = server.GetBall();
	if (ball.IsNull()) {
		return;
	}

	auto cars = server.GetCars();
	if (cars.IsNull()) {
		return;
	}
	if (cars.Count() == 0) {
		return;
	}

	ball.SetLocation(Vector{ 0, 0, 103.13 });
	ball.SetVelocity(Vector{ 0, 0, 0 });

	if (carHasInput(cars)) {
		if (!delaySet) {
			ball.SetLocation(Vector{ 0, 0, 103.13 });
			ball.SetVelocity(Vector{ 0, 0, 0 });

			gameWrapper->UnhookEventPost("Function GameEvent_Soccar_TA.Active.Tick");
			delaySet = true;
			gameWrapper->SetTimeout([&](GameWrapper* gw) {
				if (!delaySet || !ballLiftAdded) {
					return;
				}
				delaySet = false;

				if (!gameWrapper->IsInFreeplay()) {
					return;
				}

				auto server = gw->GetGameEventAsServer();
				if (server.IsNull()) {
					return;
				}

				if (!isHoops(server)) {
					return;
				}

				auto ball = server.GetBall();
				if (ball.IsNull()) {
					return;
				}
				ball.SetLocation(Vector{ 0, 0, 103.13 });
				ball.SetVelocity(Vector{ 0, 0, hoopsVelocity });

				gameWrapper->UnhookEventPost("Function GameEvent_Soccar_TA.Active.Tick");
				}, kickoff_delay);
		}
	}
}

void HoopsKickoffPractice::setKickoff(std::vector<std::string> params)
{
	auto server = gameWrapper->GetGameEventAsServer();
	if (server.IsNull()) {
		cvarManager->log("Server is null");
		return;
	}

	auto trainingEditor = TrainingEditorWrapper(gameWrapper->GetGameEventAsServer().memory_address);
	auto trainingSaveData = trainingEditor.GetTrainingData();
	auto training = trainingSaveData.GetTrainingData();

	Vector ballLoc = Vector{ 0, 0, 103.13 };
	Vector ballVel = Vector{ 0, 0, -1690 };
	Vector carLoc = Vector{ 1536, -3071.83, 17.01 };

	auto ball = trainingEditor.GetBall();
	if (ball.IsNull()) {
		return;
	}

	ball.SetLocation(ballLoc);
}

bool HoopsKickoffPractice::carHasInput(ArrayWrapper<CarWrapper> cars)
{
	for (int i = 0; i < cars.Count(); i++) {
		auto input = cars.Get(i).GetInput();
		if (input.Throttle > 0 || input.ActivateBoost > 0 || input.DodgeForward > 0 || input.Handbrake > 0 || input.HoldingBoost > 0 || input.Jump > 0 || input.Jumped > 0 || input.Throttle < 0) {
			return true;
		}
	}
	return false;
}

bool HoopsKickoffPractice::isHoops(ServerWrapper server)
{
	// Reverting to old detection, for custom hoops maps
	auto spawns = server.GetSpawnPoints();
	if (spawns.IsNull()) {
		return false;
	}

	for (int i = 0; i < spawns.Count(); i++) {
		auto spawn = spawns.Get(i).GetLocation();
		if (abs(spawn.X + 1152.0f) < location_epsilon && abs(spawn.Y + 3072.0f) < location_epsilon) {
			return true;
		}
	}

	return false;
}

void HoopsKickoffPractice::assignSpawnLocations(ArrayWrapper<CarWrapper> cars)
{
	// Track any of the players have not been assigned, and what spawns are available
	std::vector<std::string> unassigned;
	std::unordered_set<std::string> assignedPlayers;
	std::unordered_set<SpawnName> unassignedSpawns = { blue_left, blue_mid_left, blue_mid, blue_mid_right, blue_right,
													   orange_left, orange_mid_left, orange_mid, orange_mid_right, orange_right };
	for (int i = 0; i < cars.Count(); i++) {
		auto pri = cars.Get(i).GetPRI();
		if (!pri.IsNull()) {
			auto playerID = GetUniqueID(pri);
			cvarManager->log(playerID);
			auto it = playerToSpawn.find(playerID);
			if (it == playerToSpawn.end()) {
				unassigned.push_back(playerID);
			}
			else {
				assignedPlayers.insert(playerID);
				unassignedSpawns.erase(it->second);
			}
		}
	}

	// If there are extra assignments (players left), remove them but track them for new players
	std::queue<SpawnName> extras;
	if (cars.Count() - unassigned.size() > playerToSpawn.size()) {
		for (auto it = playerToSpawn.begin(); it != playerToSpawn.end(); ) {
			if (assignedPlayers.find(it->first) != assignedPlayers.end()) {
				extras.push(it->second);
				unassignedSpawns.erase(it->second);
				playerToSpawn.erase(it++);
			}
			else {
				++it;
			}
		}
	}

	// Assign spawns for new players, starting with previously assigned spawns and finishing with unassigned spawns
	auto it = unassignedSpawns.begin();
	for (std::string player : unassigned) {
		if (!extras.empty()) {
			SpawnName spawn = extras.front();
			extras.pop();
			playerToSpawn[player] = spawn;
		}
		else {
			playerToSpawn[player] = *it;
			it++;
		}
	}

	// Clear spawnToPlayerName and reset
	spawnToPlayerData.clear();
	for (int i = 0; i < cars.Count(); i++) {
		auto pri = cars.Get(i).GetPRI();
		if (!pri.IsNull()) {
			auto it = playerToSpawn.find(GetUniqueID(pri));
			if (it != playerToSpawn.end()) {
				spawnToPlayerData[it->second] = PlayerData{ GetUniqueID(pri), pri.GetPlayerName().ToString() };
			}
		}
	}
}

void HoopsKickoffPractice::swapBySpawns(SpawnName x, SpawnName y)
{
	// There's probably a better way but my brain hurts rn
	auto it1 = spawnToPlayerData.find(x);
	auto it2 = spawnToPlayerData.find(y);

	if (it1 == spawnToPlayerData.end() && it2 == spawnToPlayerData.end()) {
		return;
	}
	else if (it1 == spawnToPlayerData.end()) {
		playerToSpawn[it2->second.playerID] = x;
		spawnToPlayerData[x] = it2->second;
		spawnToPlayerData.erase(y);
	}
	else if (it2 == spawnToPlayerData.end()) {
		playerToSpawn[it1->second.playerID] = y;
		spawnToPlayerData[y] = it1->second;
		spawnToPlayerData.erase(x);
	}
	else {
		playerToSpawn[it2->second.playerID] = x;
		playerToSpawn[it1->second.playerID] = y;

		PlayerData& item1 = it1->second;
		PlayerData& item2 = it2->second;
		std::swap(item1, item2);
	}
}