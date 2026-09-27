#pragma once

#include <entt/fwd.hpp>
#include <glm/fwd.hpp>

namespace openblack::ecs::archetypes
{
class MistArchetype
{
public:
	static entt::entity Create(const glm::vec3& position, float scale, const glm::vec3& colour, float transparency,
	                           float heightRatio);
	MistArchetype() = delete;
};
} // namespace openblack::ecs::archetypes