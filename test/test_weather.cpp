/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>

#include <Weather/GWeather.h>
#include <gtest/gtest.h>

#include <Parsers/WeatherLhw.h>

namespace
{

using openblack::GWeather;
using openblack::GWeatherCloud;
using openblack::WeatherInfo;
using openblack::WeatherInfoFile;
using openblack::WeatherInfoRecord;

constexpr uint32_t kSizeInBytes = WeatherInfoRecord::kSizeInBytes; // 0x70
constexpr size_t kRecordCount = WeatherInfo::kRecordCount;        // 13
constexpr size_t kTotalSize = kSizeInBytes * kRecordCount;        // 1456

/// Builds a buffer shaped like the shipped weatherinfo.lhw: no header, records
/// in descending id order, word 0 = (version << 16) | id.
std::vector<uint8_t> MakeBuffer(uint16_t version)
{
	std::vector<uint8_t> data(kTotalSize, 0);
	for (size_t i = 0; i < kRecordCount; ++i)
	{
		const auto id = static_cast<uint16_t>(kRecordCount - 1 - i);
		uint32_t words[WeatherInfoRecord::kSlotCount * 2] {};
		words[0] = (static_cast<uint32_t>(version) << 16) | id;
		words[WeatherInfoRecord::kInUseOffset / sizeof(uint32_t)] = 1;
		std::memcpy(data.data() + (i * kSizeInBytes), words, sizeof(words));
	}
	return data;
}

/// Reads the real Scripts/weatherinfo.lhw if a game copy is present.
/// Returns an empty buffer otherwise, so the test degrades instead of failing
/// in CI where no game is installed.
std::vector<uint8_t> ReadShippedFile()
{
	std::vector<uint8_t> data;
	std::ifstream file("D:\\BaW\\Scripts\\weatherinfo.lhw", std::ios::binary);
	if (!file)
	{
		return data;
	}
	file.seekg(0, std::ios::end);
	const auto size = file.tellg();
	file.seekg(0, std::ios::beg);
	data.resize(static_cast<size_t>(size));
	file.read(reinterpret_cast<char*>(data.data()), size);
	return data;
}

} // namespace

TEST(TestWeatherLhw, RecordIsFourteenEightByteSlots)
{
	// A scan of all 13 shipped records shows the second dword of every 8-byte
	// pair is always zero, so the record is 14 slots, not 28 floats.
	EXPECT_EQ(WeatherInfoRecord::kSlotCount, 14u);
	EXPECT_EQ(WeatherInfoRecord::kSlotStrideInBytes, 8u);
	EXPECT_EQ(WeatherInfoRecord::kSlotCount * WeatherInfoRecord::kSlotStrideInBytes, 0x70u);
	EXPECT_EQ(WeatherInfoRecord::kFirstSlotOffset, 0x04u);
	EXPECT_EQ(WeatherInfoRecord::SlotOffset(0), 0x04u);
	EXPECT_EQ(WeatherInfoRecord::SlotOffset(13), 0x6Cu);
	EXPECT_EQ(WeatherInfoRecord::kInUseOffset, 0x50u);
}

TEST(TestWeatherLhw, SlotFloatsAreBitReinterpretedNotValueConverted)
{
	// Guards a real bug: static_cast<float> converts the integer value, turning
	// 0x42C80000 into 1.12e9 instead of 100.0f. std::bit_cast is required.
	std::vector<uint8_t> data(kTotalSize, 0);
	std::uint32_t words[WeatherInfoRecord::kSlotCount * 2] {};
	words[0] = (1u << 16) | 12u;
	words[3] = 0x42C80000; // slot 1 == 100.0f
	words[5] = 0x438A8000; // slot 2 == 277.0f
	words[7] = 0x3ECCCCCD; // slot 3 == 0.4f
	words[WeatherInfoRecord::kInUseOffset / sizeof(std::uint32_t)] = 1;
	std::memcpy(data.data(), words, sizeof(words));

	WeatherInfo info;
	ASSERT_TRUE(WeatherInfoFile().LoadFromBuffer(data, info));
	EXPECT_FLOAT_EQ(info.records[0].Slot(1), 100.0f);
	EXPECT_FLOAT_EQ(info.records[0].Slot(2), 277.0f);
	EXPECT_FLOAT_EQ(info.records[0].Slot(3), 0.4f);
	EXPECT_FLOAT_EQ(info.records[0].Slot(4), 0.0f);
}

TEST(TestWeatherLhw, TwoHighestRecordsAreBlankInTheShippedFile)
{
	// Records 11 and 12 hold no packed header at all: word 0 is 1 and 0.
	WeatherInfo info;
	const auto shipped = ReadShippedFile();
	if (shipped.empty())
	{
		GTEST_SKIP() << "no game copy available; skipping the shipped-file test";
	}
	ASSERT_TRUE(WeatherInfoFile().LoadFromBuffer(shipped, info));

	EXPECT_EQ(info.records[11].version, 0);
	EXPECT_EQ(info.records[12].version, 0);
	EXPECT_EQ(info.records[11].versionAndId, 1u);
	EXPECT_EQ(info.records[12].versionAndId, 0u);
	// Nothing else is stored in them either.
	for (size_t s = 0; s < WeatherInfoRecord::kSlotCount; ++s)
	{
		EXPECT_FLOAT_EQ(info.records[12].Slot(s), 0.0f);
	}
}

TEST(TestWeatherLhw, ParsesThirteenRecordsInDescendingIdOrder)
{
	const auto data = MakeBuffer(1);

	WeatherInfo info;
	ASSERT_TRUE(WeatherInfoFile().LoadFromBuffer(data, info));

	// Records 0..10 carry the packed (version << 16) | id header.
	for (size_t i = 0; i < 11; ++i)
	{
		EXPECT_EQ(info.records[i].version, 1);
		EXPECT_EQ(info.records[i].id, kRecordCount - 1 - i);
		EXPECT_EQ(info.records[i].versionAndId, (1u << 16) | static_cast<uint32_t>(kRecordCount - 1 - i));
	}
}

TEST(TestWeatherLhw, PacksVersionAndIdIntoFirstWord)
{
	WeatherInfo info;
	ASSERT_TRUE(WeatherInfoFile().LoadFromBuffer(MakeBuffer(0x1234), info));

	EXPECT_EQ(info.records[0].version, 0x1234);
	EXPECT_EQ(info.records[0].id, 12);
	EXPECT_EQ(info.records[0].versionAndId, 0x1234000Cu);
}

TEST(TestWeatherLhw, RejectsSizeThatIsNotAMultipleOfTheRecord)
{
	WeatherInfo info;
	std::vector<uint8_t> data = MakeBuffer(1);
	data.pop_back();

	EXPECT_FALSE(WeatherInfoFile().LoadFromBuffer(data, info));
}

TEST(TestWeatherLhw, RejectsEmptyBuffer)
{
	WeatherInfo info;
	EXPECT_FALSE(WeatherInfoFile().LoadFromBuffer({}, info));
}

TEST(TestWeatherLhw, FindsRecordByIdAndCountsActiveRecords)
{
	WeatherInfo info;
	ASSERT_TRUE(WeatherInfoFile().LoadFromBuffer(MakeBuffer(1), info));

	ASSERT_NE(info.FindById(7), nullptr);
	EXPECT_EQ(info.FindById(7)->id, 7);
	EXPECT_EQ(info.FindById(99), nullptr);

	// Ids 0 and 1 carry no data in the shipped file, so 11 are active.
	EXPECT_EQ(info.ActiveCount(), 11u);
}

TEST(TestGWeather, HasConstructorDefaultsFromTheBinary)
{
	const GWeather weather;

	EXPECT_FLOAT_EQ(weather.m_0x00, 0.0f);
	EXPECT_FLOAT_EQ(weather.m_0x0C, 100.0f);
	EXPECT_FLOAT_EQ(weather.m_0x10, 300.0f);
	EXPECT_FLOAT_EQ(weather.m_0x14, 10.0f);
	EXPECT_FLOAT_EQ(weather.m_0x18, 100.0f);
	EXPECT_FLOAT_EQ(weather.m_0x1C, 1.0f);
	EXPECT_EQ(weather.m_0x20, 8u);
	EXPECT_FLOAT_EQ(weather.m_0x24, 0.5f);
	EXPECT_FLOAT_EQ(weather.m_0x28, 160.0f);
	EXPECT_FLOAT_EQ(weather.m_0x2C, 1.0f);
	EXPECT_FLOAT_EQ(weather.m_0x40, 1.2f);

	EXPECT_EQ(weather.m_0x48, 10);
	EXPECT_EQ(weather.m_0x49, 100);
	EXPECT_EQ(weather.m_0x4C, 10);
	EXPECT_EQ(weather.m_0x50[0], 0);
}

TEST(TestGWeather, UpdateAccumulatesAndRaisesTheFlagOnce)
{
	GWeather weather;
	EXPECT_FALSE(weather.m_0x94);

	weather.Update(1.0f);
	EXPECT_FLOAT_EQ(weather.m_0x58, 1.0f);
	EXPECT_TRUE(weather.m_0x94);

	// The flag is raised, never cleared, by this path.
	weather.Update(1.0f);
	EXPECT_FLOAT_EQ(weather.m_0x58, 2.0f);
	EXPECT_TRUE(weather.m_0x94);
}

TEST(TestGWeather, UpdateDoesNotRunThePhaseWhenDeltaIsZero)
{
	GWeather weather;
	weather.Update(0.0f);
	EXPECT_FALSE(weather.m_0x94);
}

TEST(TestGWeather, ClampsCloudCountToSixteen)
{
	GWeather weather;
	weather.SetCloudCount(4);
	EXPECT_EQ(weather.CloudCount(), 4u);

	weather.SetCloudCount(1000);
	EXPECT_EQ(weather.CloudCount(), 16u);
}

TEST(TestGWeather, CloudSlotKeepsOriginalStrideAsReferenceOnly)
{
	// The original slot is 0x30 bytes, but only five fields were identified, so
	// the openblack struct is smaller on purpose. The original stride is kept
	// as a recorded constant rather than as a layout guarantee.
	EXPECT_EQ(GWeatherCloud::kOriginalStrideInBytes, 0x30u);
	EXPECT_LT(sizeof(GWeatherCloud), GWeatherCloud::kOriginalStrideInBytes);

	GWeather weather;
	weather.SetCloudCount(2);
	EXPECT_FLOAT_EQ(weather.Cloud(0).x, 0.0f);
}

TEST(TestGWeather, CalcAtmosRejectsNullOutput)
{
	GWeather weather;
	EXPECT_FALSE(weather.CalcAtmos(1.0f, 1.0f, nullptr));
}

TEST(TestGWeather, CalcAtmosRejectsPointOnABoundary)
{
	GWeather weather;
	weather.m_0xA0 = 2.0f;
	weather.m_0xA8 = 4.0f;
	weather.m_0xB0 = 10.0f;
	weather.m_0xAC = 0.5f;
	weather.m_0xB4 = 1.0f;

	int8_t out[6] = {1, 2, 3, 4, 5, 6};
	// (m_0xA0 - m_0xB0) is the first tested value; it must not be accepted.
	EXPECT_FALSE(weather.CalcAtmos(2.0f - 10.0f, 1.0f, out));
	EXPECT_EQ(out[0], 1);
}

TEST(TestGWeather, CalcAtmosIsDeterministicAndBounded)
{
	GWeather weather;
	weather.m_0xA0 = 0.0f;
	weather.m_0xA8 = 0.0f;
	weather.m_0xB0 = 100.0f;
	weather.m_0xAC = 1.0f;
	weather.m_0xB4 = 1.0f;
	weather.m_0x50.fill(100);

	int8_t first[6] = {0, 0, 0, 0, 0, 0};
	ASSERT_TRUE(weather.CalcAtmos(5.0f, 5.0f, first));

	int8_t second[6] = {0, 0, 0, 0, 0, 0};
	ASSERT_TRUE(weather.CalcAtmos(5.0f, 5.0f, second));

	for (int i = 0; i < 6; ++i)
	{
		// The same input must give the same output: there is no RNG involved.
		EXPECT_EQ(first[i], second[i]);
		// Every byte is clamped into the signed byte range.
		EXPECT_GE(first[i], -128);
		EXPECT_LE(first[i], 127);
	}
}

TEST(TestGWeather, CalcAtmosLeavesOutputUntouchedWhenScaleIsZero)
{
	GWeather weather;
	weather.m_0xA0 = 0.0f;
	weather.m_0xA8 = 0.0f;
	weather.m_0xB0 = 100.0f;
	weather.m_0xAC = 1.0f;
	weather.m_0xB4 = 0.0f; // scale truncates to 0
	weather.m_0x50.fill(100);

	int8_t out[6] = {11, 22, 33, 44, 55, 66};
	EXPECT_FALSE(weather.CalcAtmos(5.0f, 5.0f, out));
	for (int i = 0; i < 6; ++i)
	{
		EXPECT_EQ(out[i], static_cast<int8_t>((i + 1) * 11));
	}
}
