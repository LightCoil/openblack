/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "WeatherLhw.h"

#include <bit>
#include <cstring>

#include <spdlog/spdlog.h>

#include "FileSystem/FileSystemInterface.h"
#include "Locator.h"

namespace openblack
{

namespace
{
constexpr size_t kInUseWordIndex = 0x50 / sizeof(uint32_t);
} // namespace

bool WeatherInfoFile::LoadFromBuffer(const std::vector<uint8_t>& data, WeatherInfo& out) const noexcept
{
	if (data.empty() || (data.size() % WeatherInfoRecord::kSizeInBytes) != 0)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "weatherinfo.lhw has size {}, which is not a multiple of {}",
		                    data.size(), WeatherInfoRecord::kSizeInBytes);
		return false;
	}

	const size_t count = data.size() / WeatherInfoRecord::kSizeInBytes;
	if (count > WeatherInfo::kRecordCount)
	{
		SPDLOG_LOGGER_WARN(spdlog::get("game"), "weatherinfo.lhw holds {} records, more than the {} expected; extra records are dropped",
		                   count, WeatherInfo::kRecordCount);
	}

	WeatherInfo parsed;
	for (size_t i = 0; i < parsed.records.size(); ++i)
	{
		auto& record = parsed.records[i];
		if (i >= count)
		{
			break;
		}

		std::uint32_t words[WeatherInfoRecord::kSlotCount * 2];
		std::memcpy(words, data.data() + (i * WeatherInfoRecord::kSizeInBytes), sizeof(words));

		record.versionAndId = words[0];
		record.version = static_cast<uint16_t>((words[0] >> 16) & 0xFFFF);
		record.id = static_cast<uint16_t>(words[0] & 0xFFFF);
		record.inUse = words[kInUseWordIndex];

		// Only the first dword of each 8-byte slot ever carries a value in the
		// shipped file; the second one is read and discarded.
		// std::bit_cast is required: static_cast<float> would convert the
		// integer *value* and turn 0x42C80000 into 1.12e9 instead of 100.0f.
		for (size_t s = 0; s < WeatherInfoRecord::kSlotCount; ++s)
		{
			record.slots[s] = std::bit_cast<float>(words[s * 2 + 1]);
		}
	}

	out = std::move(parsed);
	return true;
}

bool WeatherInfoFile::LoadFromFile(const std::filesystem::path& path, WeatherInfo& out) const noexcept
{
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading weather info from file: {}", path.generic_string());

	const auto data = Locator::filesystem::value().ReadAll(path);
	return LoadFromBuffer(data, out);
}

} // namespace openblack
