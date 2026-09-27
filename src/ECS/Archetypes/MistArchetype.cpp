#include "MistArchetype.h"

#include <glm/vec3.hpp>

#include "ECS/Components/Mist.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

entt::entity MistArchetype::Create(const glm::vec3& position, float scale, const glm::vec3& colour, float transparency,
                                   float heightRatio)
{
	auto& registry = Locator::entitiesRegistry::value();

	const auto entity = registry.Create();
	registry.Assign<Transform>(entity, position, glm::mat3(1.0f), glm::vec3(scale));
	registry.Assign<Mist>(entity);
	auto& mist = registry.Get<Mist>(entity);
	mist.colour = colour;
	mist.transparency = transparency;
	mist.heightRatio = heightRatio;

	return entity;
}
