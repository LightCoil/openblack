/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "GWeather.h"

#include <algorithm>
#include <cmath>

namespace openblack
{

namespace
{
/// Truncation toward zero, matching the MXCSR RC=0x8000 that the original
/// installs before it uses this conversion (0x007ACA80 and neighbours).
int32_t TruncToZero(float value) noexcept
{
	return static_cast<int32_t>(std::trunc(value));
}

int8_t ClampToByte(int32_t value) noexcept
{
	return static_cast<int8_t>(std::clamp(value, -128, 127));
}
} // namespace

GWeather::GWeather() noexcept = default;

void GWeather::Update(float deltaTime) noexcept
{
	m_0x58 += deltaTime;

	// The original compares with fcomp and tests the FPU zero flag, so the
	// processing phase runs while m_0x58 <= m_0x20.
	if (m_0x58 <= static_cast<float>(m_0x20))
	{
		if (m_0x58 != 0.0f)
		{
			m_0x94 = true;
		}
	}
}

float GWeather::ScaleFromThreshold() const noexcept
{
	// CalcAtmos: fmul [this+0xB4]; fmul 256.0f; call 0x7A1400
	// 0x7A1400 pops the FPU stack and converts to int, so the caller supplies
	// the value. The result is the truncated product.
	return static_cast<float>(TruncToZero(m_0xB4 * kScale256));
}

void GWeather::SetCloudCount(size_t count) noexcept
{
	_cloudCount = std::min(count, GWeatherCloud::kMaxClouds);
}

bool GWeather::CalcAtmos(float x, float y, int8_t* out) const noexcept
{
	if (out == nullptr)
	{
		return false;
	}

	// Four boundary tests. In the original each is an fcomp against a value
	// built from the rectangle corners, and any mismatch sends control to the
	// common exit, which leaves the output untouched.
	const float loX = m_0xA0 - m_0xB0;
	if (loX == x)
	{
		return false;
	}
	const float hiX = m_0xB0 + m_0xA0;
	if (hiX == x)
	{
		return false;
	}
	const float loY = m_0xA8 - m_0xB0;
	if (loY == y)
	{
		return false;
	}
	const float hiY = m_0xA8 + m_0xB0;
	if (hiY == y)
	{
		return false;
	}

	// Weight. The original computes the two products off the same pair of
	// differences and adds them, i.e. twice the product; it does NOT normalise
	// by the rectangle size.
	const float dx = x - m_0xA0;
	const float dy = y - m_0xA8;
	const float weight = (dy * dx) + (dx * dy);

	const float bSquared = m_0xB0 * m_0xB0;
	if (weight == bSquared)
	{
		return false;
	}

	// Threshold branch. If weight * m_0xAC == weight the constant 1.0 is used,
	// otherwise sqrt(weight) is folded against the corners.
	float magnitude;
	const float acSquared = m_0xAC * m_0xAC;
	if (acSquared == weight)
	{
		magnitude = kThreshold;
	}
	else
	{
		magnitude = kThreshold - ((std::sqrt(weight) - m_0xAC) / (m_0xB0 - m_0xAC));
	}

	const auto scale = static_cast<int32_t>(TruncToZero(magnitude * m_0xB4 * kScale256));
	if (scale == 0)
	{
		return false;
	}

	// The six bytes. Byte 0 is the odd one out: the original subtracts the old
	// value before scaling, adds the old value back afterwards, and does not
	// clamp the result.
	int8_t result[6];
	{
		const int32_t old = static_cast<int8_t>(out[0]);
		const int32_t mixed = ((static_cast<int32_t>(m_0x50[0]) - old) * scale) >> 8;
		result[0] = static_cast<int8_t>(static_cast<uint8_t>(mixed + old));
	}
	for (size_t i = 1; i < 6; ++i)
	{
		const int32_t mixed = (static_cast<int32_t>(m_0x50[i]) * scale) >> 8;
		result[i] = ClampToByte(mixed + static_cast<int32_t>(out[i]));
	}

	for (size_t i = 0; i < 6; ++i)
	{
		out[i] = result[i];
	}
	return true;
}

} // namespace openblack
