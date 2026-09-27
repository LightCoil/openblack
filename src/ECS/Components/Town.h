/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <set>
#include <string>
#include <unordered_map>

#include <glm/vec3.hpp>

namespace openblack::ecs::components
{

struct Town
{
	uint32_t id;
	std::unordered_map<std::string, float> beliefs;
	/// Upper bound applied to a belief, set by SET_TOWN_BELIEF_CAP.
	std::unordered_map<std::string, float> beliefCaps;
	/// Spells granted to the town, set by CREATE_TOWN_SPELL.
	std::set<std::string> spells;
	/// Multiplier applied to the town influence, set by SET_TOWN_INFLUENCE_MULTIPLIER.
	float influenceMultiplier = 1.0f;
	/// Scale applied to the belief drift, set by SET_TOWN_BALANCE_BELIEF_SCALE.
	float beliefScale = 1.0f;
	/// Where villagers gather, set by SET_TOWN_CONGREGATION_POS.
	glm::vec3 congregationPos = {0.0f, 0.0f, 0.0f};
	/// Where the town still needs services, set by TOWN_NEEDS_POS.
	glm::vec3 needsPos = {0.0f, 0.0f, 0.0f};
	/// Bonuses applied to the town desire, set by TOWN_DESIRE_BOOST.
	std::unordered_map<std::string, float> desireBoosts;
	bool uninhabitable = false;
	std::set<entt::entity> homelessVillagers;
};

} // namespace openblack::ecs::components
