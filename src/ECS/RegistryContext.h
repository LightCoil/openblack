/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <entt/entity/fwd.hpp>
#include <glm/vec3.hpp>

#include "Components/Footpath.h"
#include "Components/Stream.h"
#include "Components/Town.h"

namespace openblack::ecs
{
struct RegistryContext
{
	std::unordered_map<components::Footpath::Id, entt::entity> footpaths;
	std::unordered_map<components::Stream::Id, entt::entity> streams;
	std::unordered_map<uint32_t, entt::entity> towns;
	/// Index of the currently loaded land, set by the SET_LAND_NUMBER script command.
	uint32_t landNumber = 0;
	/// Multiplier applied to every player influence, set by SET_PLAYER_INFLUENCE_MULTIPLIER.
	float playerInfluenceMultiplier = 1.0f;
	/// Global land balance, set by SET_GLOBAL_LAND_BALANCE.
	float globalLandBalance = 0.0f;
	/// Per-tribe land balance, set by SET_LAND_BALANCE.
	std::unordered_map<std::string, float> landBalance;
	/// Scale applied to towns lost to the player, set by SET_LOST_TOWN_SCALE.
	float lostTownScale = 1.0f;
	/// Names of the towns whose citadel was lost, used by MAKE_LAST_OBJECT_ARTIFACT.
	std::string lastObjectArtifactTown;
	/// Forests keyed by their script id, set by CREATE_FOREST.
	std::unordered_map<uint32_t, entt::entity> forests;
	/// Desired villager interaction level, set by SET_INTERACT_DESIRE.
	float interactDesire = 0.0f;
	/// Night window as (start hour, end hour, transition), set by SET_NIGHTTIME.
	glm::vec3 nighttime = {0.0f, 0.0f, 0.0f};
	/// Creature each computer player is modelled on, set by SET_COMPUTER_PLAYER_CREATURE_LIKE.
	std::unordered_map<std::string, std::string> computerPlayerCreatureLike;
	/// Personality assigned to a computer player, set by SET_COMPUTER_PLAYER_PERSONALITY.
	std::unordered_map<std::string, std::string> computerPlayerPersonality;
	/// Chance that a spell is granted by a firefly, set by FIRE_FLY_SPELL_REWARD_PROB.
	std::unordered_map<std::string, float> fireFlySpellRewardProb;
	/// Game messages shown to the player, written by START_GAME_MESSAGE and ADD_GAME_MESSAGE_LINE.
	std::vector<std::string> gameMessages;
};
} // namespace openblack::ecs
