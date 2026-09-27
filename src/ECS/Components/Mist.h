#pragma once

#include <glm/vec3.hpp>

namespace openblack::ecs::components
{

/// A cloud of soft fog placed by the CREATE_MIST script command.
/// Colours are in the 0..1 range and match the r/g/b floats popped by the
/// original CHL CREATE_MIST opcode.
struct Mist
{
	glm::vec3 colour = {0.0f, 0.0f, 0.0f};
	/// 0 is fully opaque, 1 is fully transparent.
	float transparency = 0.0f;
	/// Vertical stretch of the cloud relative to its footprint.
	float heightRatio = 0.0f;
};

} // namespace openblack::ecs::components