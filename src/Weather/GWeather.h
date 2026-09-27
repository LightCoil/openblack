/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <array>
#include <cstddef>

namespace openblack
{

/// A cloud slot inside GWeather.
///
/// GWeather::DrawClouds (BW1W120 0x0083FC90) indexes with
/// `lea ecx,[eax+eax*2]; shl ecx,4`, i.e. a stride of 48 (0x30) bytes, over an
/// embedded array that starts at `this + 0xBC`, and clamps the count to 16.
///
/// Only the five fields that the disassembly actually writes are modelled here.
/// The original slot occupies 48 bytes and the remaining 28 were not identified,
/// so this struct is deliberately a partial view and does NOT claim the original
/// stride; openblack keeps its own layout.
struct GWeatherCloud
{
	static constexpr size_t kMaxClouds = 16;
	/// Stride used by the original, recorded for reference only.
	static constexpr size_t kOriginalStrideInBytes = 0x30;

	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	float scaleX = 0.0f;
	float scaleY = 0.0f;
};

/// Atmosphere state, reconstructed from the BW1W120 binary.
///
/// Field offsets below were read from the constructor at 0x0083F3F0 and from
/// GWeather::CalcAtmos at 0x008400E0. The *names* are offsets, not recovered
/// identifiers: the original symbol names come from a PDB that is not
/// available, and the meaning of most fields is still unknown. Nothing here is
/// guessed at beyond what the disassembly states outright.
class GWeather
{
public:
	GWeather() noexcept;

	/// One clock tick. BW1W120 0x0083F900:
	///     m_0x58 += deltaTime;
	///     if (m_0x58 > m_0x20) return 0;
	///     if (m_0x58 != 0.0f) m_0x94 = 1;
	///     return 0;
	/// Note the comparison is `<=`, taken from the FPU zero flag.
	void Update(float deltaTime) noexcept;

	/// Interpolates the six atmosphere bytes at @a out (which must hold at
	/// least 6 bytes) for the point (@a x, @a y). BW1W120 0x008400E0.
	///
	/// The function returns false and leaves @a out untouched when the point
	/// fails one of the four boundary tests, or when the derived scale is 0.
	[[nodiscard]] bool CalcAtmos(float x, float y, int8_t* out) const noexcept;

	[[nodiscard]] size_t CloudCount() const noexcept { return _cloudCount; }
	void SetCloudCount(size_t count) noexcept;

	[[nodiscard]] GWeatherCloud& Cloud(size_t index) noexcept { return _clouds[index]; }
	[[nodiscard]] const GWeatherCloud& Cloud(size_t index) const noexcept { return _clouds[index]; }

	// --- Offsets confirmed by the constructor at 0x0083F3F0 ---
	float m_0x00 = 0.0f; ///< 0
	float m_0x04 = 0.0f; ///< 0
	float m_0x08 = 0.0f; ///< 0
	float m_0x0C = 100.0f;
	float m_0x10 = 300.0f;
	float m_0x14 = 10.0f;
	float m_0x18 = 100.0f;
	float m_0x1C = 1.0f;
	uint32_t m_0x20 = 8; ///< integer, not a float
	float m_0x24 = 0.5f;
	float m_0x28 = 160.0f;
	float m_0x2C = 1.0f;
	float m_0x30 = 0.0f;
	float m_0x34 = 0.0f;
	float m_0x38 = 0.0f;
	float m_0x3C = 0.0f;
	float m_0x40 = 1.2f;
	int8_t m_0x48 = 10;
	int8_t m_0x49 = 100;
	int8_t m_0x4A = 0;
	int8_t m_0x4B = 100;
	int8_t m_0x4C = 10;
	int8_t m_0x4D = 0;
	int8_t m_0x4E = 0;
	int8_t m_0x4F = 0;

	/// Six signed bytes at +0x50..+0x55 that CalcAtmos perturbs. The original
	/// copies them with six consecutive `mov byte ptr` instructions.
	std::array<int8_t, 6> m_0x50 {};

	float m_0x58 = 0.0f; ///< accumulator advanced by Update
	bool m_0x94 = false; ///< raised once by the update phase

	/// Rectangle and thresholds used by CalcAtmos / DrawClouds.
	float m_0xA0 = 0.0f;
	float m_0xA4 = 0.0f;
	float m_0xA8 = 0.0f;
	float m_0xAC = 0.0f;
	float m_0xB0 = 0.0f;
	float m_0xB4 = 0.0f;

	/// Multiplier applied before the float->int conversion in CalcAtmos.
	/// The original calls 0x007A1400, which converts the value left on the FPU
	/// stack by the caller; it is NOT a random number generator. Truncation
	/// toward zero is assumed, matching the MXCSR RC=0x8000 the original sets.
	float ScaleFromThreshold() const noexcept;

	/// Constants read from .rdata. The raw floats were read from the binary:
	///
	///  - kThreshold  [0x8AA390] = 1.0f     (used as the 1.0 fallback in CalcAtmos)
	///  - kScale256   [0x8D45CC] = 256.0f   (this is why the byte mix shifts by 8)
	///  - kCloudScaleX[0x8AC404] = 0.1f
	///  - kCloudScaleY[0x8AC408] = 65536.0f
	///  - kCloudPos   [0x8AA3B4] = 0.5f
	///  - kSunScale   [0x8AB270] = 255.0f
	///
	/// Their *purpose* is still not proven, only their values and the role they
	/// play in the arithmetic.
	static constexpr float kThreshold = 1.0f;
	static constexpr float kScale256 = 256.0f;
	static constexpr float kCloudScaleX = 0.1f;
	static constexpr float kCloudScaleY = 65536.0f;
	static constexpr float kCloudPos = 0.5f;
	static constexpr float kSunScale = 255.0f;

private:
	size_t _cloudCount = 0;
	std::array<GWeatherCloud, GWeatherCloud::kMaxClouds> _clouds {};
};

} // namespace openblack
