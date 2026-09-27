/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// Phase 3: the script commands that were previously stubs must have a real,
// observable effect. A test that only checked that the call does not throw
// would also pass against the old no-op bodies, so each test asserts on the
// stored state.
#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>

#include <ECS/Archetypes/TownArchetype.h>
#include <ECS/Components/Town.h>
#include <ECS/Registry.h>
#include <Enums.h>
#include <Game.h>
#include <LHScriptX/FeatureScriptCommands.h>
#include <LHScriptX/Script.h>
#include <Locator.h>
#include <gtest/gtest.h>

using openblack::lhscriptx::FeatureScriptCommands;
using openblack::lhscriptx::Script;
using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

namespace
{
constexpr int32_t k_TownId = 4242;
constexpr int32_t k_UnknownTownId = 999999;

void CreateTestTown()
{
	TownArchetype::Create(k_TownId, {1000.0f, 0.0f, 1000.0f}, PlayerNames::PLAYER_ONE, Tribe::CELTIC);
}

const Town& GetTestTown()
{
	auto& registry = openblack::Locator::entitiesRegistry::value();
	return registry.Get<Town>(registry.Context().towns.at(k_TownId));
}
} // namespace

// Runs scripts against a loaded level, the same way the interpreter does.
// The fixture must live at namespace scope for TEST_F to derive from it.
class ScriptCommandTest: public ::testing::Test
{
protected:
	static void SetUpTestSuite()
	{
		static const auto mockGamePath = std::filesystem::path(TEST_BINARY_DIR) / "mock";
		auto args = openblack::Arguments {
		    .graphicsBackend = openblack::GraphicsBackend::Noop,
		    .gamePath = mockGamePath.string(),
		    .numFramesToSimulate = 0,
		    .logFile = "stdout",
		    .startLevel = "Land1.txt",
		};
		std::fill_n(args.logLevels.begin(), args.logLevels.size(), spdlog::level::err);
		_game = std::make_unique<openblack::Game>(std::move(args));
		ASSERT_TRUE(_game->Initialize());
		ASSERT_TRUE(_game->Run());
	}

	static void TearDownTestSuite() { _game.reset(); }

	static std::unique_ptr<openblack::Game> _game;
};

std::unique_ptr<openblack::Game> ScriptCommandTest::_game;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): gtest fixture
TEST_F(ScriptCommandTest, setTownBeliefCapStoresTheCap)
{
	CreateTestTown();

	ASSERT_NO_THROW(Script().Load("SET_TOWN_BELIEF_CAP(4242, \"PLAYER_ONE\", 0.75)"));
	EXPECT_FLOAT_EQ(GetTestTown().beliefCaps.at("PLAYER_ONE"), 0.75f);
}

// CREATE_TOWN_SPELL must record the spell on the town.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): gtest fixture
TEST_F(ScriptCommandTest, createTownSpellRecordsTheSpell)
{
	CreateTestTown();

	ASSERT_NO_THROW(Script().Load("CREATE_TOWN_SPELL(4242, \"MAGIC_SHOT\")"));
	EXPECT_EQ(GetTestTown().spells.count("MAGIC_SHOT"), 1U);
}

// CREATE_NEW_TOWN_SPELL shares the implementation of CREATE_TOWN_SPELL.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): gtest fixture
TEST_F(ScriptCommandTest, createNewTownSpellRecordsTheSpell)
{
	CreateTestTown();

	ASSERT_NO_THROW(Script().Load("CREATE_NEW_TOWN_SPELL(4242, \"HARMONY\")"));
	EXPECT_EQ(GetTestTown().spells.count("HARMONY"), 1U);
}

// SET_TOWN_CONGREGATION_POS and TOWN_NEEDS_POS must store their positions.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): gtest fixture
TEST_F(ScriptCommandTest, townPositionsAreStored)
{
	CreateTestTown();

	ASSERT_NO_THROW(Script().Load("SET_TOWN_CONGREGATION_POS(4242, \"10,20\")"));
	ASSERT_NO_THROW(Script().Load("TOWN_NEEDS_POS(4242, \"30,40\")"));

	EXPECT_FLOAT_EQ(GetTestTown().congregationPos.x, 10.0f);
	EXPECT_FLOAT_EQ(GetTestTown().congregationPos.z, 20.0f);
	EXPECT_FLOAT_EQ(GetTestTown().needsPos.x, 30.0f);
	EXPECT_FLOAT_EQ(GetTestTown().needsPos.z, 40.0f);
}

// Commands addressing an unknown town must not create or touch any town.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): gtest fixture
TEST_F(ScriptCommandTest, unknownTownIsIgnored)
{
	ASSERT_NO_THROW(Script().Load("SET_TOWN_BELIEF_CAP(999999, \"PLAYER_ONE\", 0.5)"));
	ASSERT_NO_THROW(Script().Load("CREATE_TOWN_SPELL(999999, \"MAGIC_SHOT\")"));
	EXPECT_FALSE(openblack::Locator::entitiesRegistry::value().Context().towns.contains(k_UnknownTownId));
}

// SET_TOWN_BALANCE_BELIEF_SCALE stores the scale on the town.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): gtest fixture
TEST_F(ScriptCommandTest, setTownBalanceBeliefScaleStoresTheScale)
{
	CreateTestTown();

	ASSERT_NO_THROW(Script().Load("SET_TOWN_BALANCE_BELIEF_SCALE(4242, 2.5)"));
	EXPECT_FLOAT_EQ(GetTestTown().beliefScale, 2.5f);
}

// SET_TOWN_INFLUENCE_MULTIPLIER applies to every existing town.

// The global balance commands must reach the registry context.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): gtest fixture
TEST_F(ScriptCommandTest, globalBalanceCommandsAreStored)
{
	auto& context = openblack::Locator::entitiesRegistry::value().Context();

	ASSERT_NO_THROW(Script().Load("SET_PLAYER_INFLUENCE_MULTIPLIER(4.0)"));
	EXPECT_FLOAT_EQ(context.playerInfluenceMultiplier, 4.0f);

	ASSERT_NO_THROW(Script().Load("SET_GLOBAL_LAND_BALANCE(0, 1.5)"));
	EXPECT_FLOAT_EQ(context.globalLandBalance, 1.5f);

	ASSERT_NO_THROW(Script().Load("SET_LAND_BALANCE(\"CELTIC\", 0, -0.5)"));
	EXPECT_FLOAT_EQ(context.landBalance.at("CELTIC"), -0.5f);

	ASSERT_NO_THROW(Script().Load("SET_LOST_TOWN_SCALE(0.25)"));
	EXPECT_FLOAT_EQ(context.lostTownScale, 0.25f);
}

// SET_NIGHTTIME and SET_INTERACT_DESIRE must reach the registry context.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): gtest fixture
TEST_F(ScriptCommandTest, worldScalarsAreStored)
{
	auto& context = openblack::Locator::entitiesRegistry::value().Context();

	ASSERT_NO_THROW(Script().Load("SET_NIGHTTIME(20.0, 6.0, 1.0)"));
	EXPECT_FLOAT_EQ(context.nighttime.x, 20.0f);
	EXPECT_FLOAT_EQ(context.nighttime.y, 6.0f);

	ASSERT_NO_THROW(Script().Load("SET_INTERACT_DESIRE(0.8)"));
	EXPECT_FLOAT_EQ(context.interactDesire, 0.8f);
}

// CREATE_FOREST must register the forest so later trees can attach to it.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): gtest fixture
TEST_F(ScriptCommandTest, createForestRegistersTheForest)
{
	ASSERT_NO_THROW(Script().Load("CREATE_FOREST(77, \"1000,1000\")"));
	EXPECT_TRUE(openblack::Locator::entitiesRegistry::value().Context().forests.contains(77));
}

// Game messages accumulate: START_GAME_MESSAGE resets, ADD_GAME_MESSAGE_LINE appends.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): gtest fixture
TEST_F(ScriptCommandTest, gameMessagesAreCollected)
{
	auto& context = openblack::Locator::entitiesRegistry::value().Context();

	ASSERT_NO_THROW(Script().Load("START_GAME_MESSAGE(\"First line\", 0)"));
	ASSERT_EQ(context.gameMessages.size(), 1U);

	ASSERT_NO_THROW(Script().Load("ADD_GAME_MESSAGE_LINE(\"Second line\", 0)"));
	ASSERT_EQ(context.gameMessages.size(), 2U);
	EXPECT_EQ(context.gameMessages.back(), "Second line");

	ASSERT_NO_THROW(Script().Load("START_GAME_MESSAGE(\"Reset\", 0)"));
	EXPECT_EQ(context.gameMessages.size(), 1U);
	EXPECT_EQ(context.gameMessages.back(), "Reset");
}

// Computer player and firefly settings must reach the registry context.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): gtest fixture
TEST_F(ScriptCommandTest, computerPlayerAndFireFlySettingsAreStored)
{
	auto& context = openblack::Locator::entitiesRegistry::value().Context();

	ASSERT_NO_THROW(Script().Load("SET_COMPUTER_PLAYER_CREATURE_LIKE(\"PLAYER_TWO\", \"A_Creature\")"));
	EXPECT_EQ(context.computerPlayerCreatureLike.at("PLAYER_TWO"), "A_Creature");

	ASSERT_NO_THROW(Script().Load("FIRE_FLY_SPELL_REWARD_PROB(\"MAGIC_SHOT\", 0.3)"));
	EXPECT_FLOAT_EQ(context.fireFlySpellRewardProb.at("MAGIC_SHOT"), 0.3f);
}

// SET_TOWN_INFLUENCE_MULTIPLIER applies to every existing town.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): gtest fixture
TEST_F(ScriptCommandTest, setTownInfluenceMultiplierAppliesToEveryTown)
{
	CreateTestTown();

	ASSERT_NO_THROW(Script().Load("SET_TOWN_INFLUENCE_MULTIPLIER(3.0)"));
	EXPECT_FLOAT_EQ(GetTestTown().influenceMultiplier, 3.0f);
}
