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
#include <filesystem>
#include <vector>

namespace openblack
{

/// One record of Scripts/weatherinfo.lhw.
///
/// Layout was recovered by static inspection of BW1W120 (SHA-1
/// BCCCCBD3A08FE9A5D6CF59CD114E916401ABFB51) and by dumping the shipped file.
///
/// The file is 1456 bytes, which is exactly 13 * 0x70, so it carries no header
/// and the first record starts at offset 0. Records are stored in descending id
/// order, and dword 0 packs (version << 16) | id.
///
/// The record is **14 slots of 8 bytes**, not 28 independent floats: a scan of
/// all 13 records shows the second dword of every pair is always zero, and only
/// the offsets below ever carry a value. Each slot therefore holds a float in
/// its first dword and leaves the second dword unused. Slot 6 also carries the
/// in-use flag at +0x50.
///
/// The meaning of the 14 floats is NOT known. They are read and preserved
/// verbatim so that later reverse engineering can map them, but no name is
/// invented here.
struct WeatherInfoRecord
{
	/// 14 slots at 8-byte stride: +0x04, +0x0C, ... +0x6C.
	static constexpr size_t kSlotCount = 14;
	static constexpr size_t kSlotStrideInBytes = 8;
	static constexpr size_t kSizeInBytes = kSlotCount * kSlotStrideInBytes; // 0x70

	/// Offset of the first slot's float within the record.
	static constexpr size_t kFirstSlotOffset = 0x04;
	/// Offset of the in-use flag, which sits in slot 6.
	static constexpr size_t kInUseOffset = 0x50;

	/// (version << 16) | id, as stored in dword 0.
	uint32_t versionAndId = 0;

	uint16_t version = 0;
	uint16_t id = 0;

	/// 1 in every record of the shipped file except the two empty ones.
	uint32_t inUse = 0;

	/// One float per slot, in slot order.
	std::array<float, kSlotCount> slots {};

	/// Reads slot @a index (0..13). Slots that the file leaves empty are 0.
	[[nodiscard]] float Slot(size_t index) const noexcept
	{
		return index < slots.size() ? slots[index] : 0.0f;
	}

	[[nodiscard]] static constexpr size_t SlotOffset(size_t index) noexcept
	{
		return kFirstSlotOffset + (index * kSlotStrideInBytes);
	}
};

/// Contents of Scripts/weatherinfo.lhw.
///
/// 13 records are read; the two lowest ids (0 and 1) are empty in the shipped
/// file apart from inUse, so 11 records carry data.
struct WeatherInfo
{
	static constexpr size_t kRecordCount = 13;
	static constexpr size_t kVersion = 1;

	std::array<WeatherInfoRecord, kRecordCount> records {};

	[[nodiscard]] const WeatherInfoRecord* FindById(uint16_t id) const noexcept
	{
		for (const auto& record : records)
		{
			if (record.id == id)
			{
				return &record;
			}
		}
		return nullptr;
	}

	/// Number of records that are not the two empty lowest ones.
	[[nodiscard]] size_t ActiveCount() const noexcept
	{
		size_t count = 0;
		for (const auto& record : records)
		{
			if (record.id >= 2)
			{
				++count;
			}
		}
		return count;
	}
};

class WeatherInfoFile
{
public:
	/// Parses a weatherinfo.lhw image. Returns false and leaves @a out
	/// untouched if the size is not an exact multiple of the record size.
	[[nodiscard]] bool LoadFromBuffer(const std::vector<uint8_t>& data, WeatherInfo& out) const noexcept;

	/// Reads @a path through Locator::filesystem and parses it.
	[[nodiscard]] bool LoadFromFile(const std::filesystem::path& path, WeatherInfo& out) const noexcept;
};

} // namespace openblack
