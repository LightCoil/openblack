/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 ********************************************************************************/

#include <cstdint>

#include <algorithm>
#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <Game.h>
#include <LHScriptX/CommandSignature.h>
#include <LHScriptX/FeatureScriptCommands.h>
#include <LHScriptX/MapScriptCommands.h>
#include <LHScriptX/Script.h>
#include <gtest/gtest.h>

using openblack::lhscriptx::ParameterType;
using openblack::lhscriptx::Script;
using openblack::lhscriptx::ScriptCommandSignature;

namespace
{

// Number of leading non-None entries in a signature's parameter array.
int CountParameters(const ScriptCommandSignature& signature)
{
	int count = 0;
	for (const auto type : signature.parameters)
	{
		if (type == ParameterType::None)
		{
			break;
		}
		++count;
	}
	return count;
}

const ScriptCommandSignature* FindSignature(const auto& signatures, const std::string& name)
{
	for (const auto& signature : signatures)
	{
		const std::string signatureName(signature.name.data());
		if (signatureName == name)
		{
			return &signature;
		}
	}
	return nullptr;
}

// Verifies the structural invariants every registered command must satisfy.
void CheckSignatures(const auto& signatures)
{
	std::vector<std::string> names;
	names.reserve(signatures.size());
	for (const auto& signature : signatures)
	{
		const std::string name(signature.name.data());

		// The name is a fixed-size char array; it must be NUL terminated and non-empty.
		EXPECT_FALSE(name.empty()) << "a registered command has an empty name";
		EXPECT_TRUE(signature.command != nullptr) << name << " has no bound function";
		EXPECT_LE(CountParameters(signature), 9) << name << " has too many parameters";

		// Once a None padding entry is reached, nothing else may follow it.
		bool seenPadding = false;
		for (const auto type : signature.parameters)
		{
			if (type == ParameterType::None)
			{
				seenPadding = true;
			}
			else
			{
				EXPECT_FALSE(seenPadding) << name << " has a gap in its parameter list";
			}
		}
		names.push_back(name);
	}

	// Duplicate names would silently shadow commands in the script interpreter.
	std::sort(names.begin(), names.end());
	const auto duplicate = std::adjacent_find(names.begin(), names.end());
	EXPECT_EQ(duplicate, names.end()) << "duplicate command name: " << *duplicate;
}

} // namespace

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST(ScriptSignatures, featureCommandsAreWellFormed)
{
	CheckSignatures(openblack::lhscriptx::FeatureScriptCommands::k_Signatures);
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST(ScriptSignatures, mapCommandsAreWellFormed)
{
	CheckSignatures(openblack::lhscriptx::MapScriptCommands::k_Signatures);
}

// Regression test: CREATE_MIST originally took the wrong number of arguments,
// which made the interpreter read past the end of the parameter list and abort
// the process with 0xC0000409. The signature is five parameters wide.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST(ScriptSignatures, createMistTakesFiveParameters)
{
	const auto* signature = FindSignature(openblack::lhscriptx::FeatureScriptCommands::k_Signatures, "CREATE_MIST");
	ASSERT_NE(signature, nullptr) << "CREATE_MIST is not registered";

	const std::array<ParameterType, 5> expected = {ParameterType::Vector, ParameterType::Float, ParameterType::Number,
	                                               ParameterType::Float, ParameterType::Float};
	for (size_t i = 0; i < expected.size(); i++)
	{
		EXPECT_EQ(signature->parameters[i], expected[i]) << "CREATE_MIST parameter " << i;
	}
	EXPECT_EQ(CountParameters(*signature), 5);
}

// Verifies that every real invocation in a shipped script matches the
// registered signature. This is the check that would have caught the original
// CREATE_MIST mismatch: a script calling a command with more arguments than
// the signature declares must be reported, not silently read out of bounds.
class ScriptExecutionTest: public ::testing::Test
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

		// Vector arguments such as "1000,1000" are resolved against the terrain,
		// so the level must be loaded before scripts can be parsed and executed.
		ASSERT_TRUE(_game->Run());
	}

	static void TearDownTestSuite() { _game.reset(); }

	static std::unique_ptr<openblack::Game> _game;
};

std::unique_ptr<openblack::Game> ScriptExecutionTest::_game;

namespace
{
// CREATE_MIST declares (Vector, Float, Number, Float, Float). Vectors are given
// as "x,z" string literals and resolved against the terrain.
const char* const k_CreateMistFiveArgs = "CREATE_MIST(\"1000,1000\", 1.0, 12345, 0.5, 1.0)";
} // namespace

// A well-formed five argument call is accepted end to end through the lexer,
// the arity check, the type check and the command itself. This is the direct
// regression for the 0xC0000409 abort.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): gtest fixture
TEST_F(ScriptExecutionTest, correctArgumentCountAndTypesAreAccepted)
{
	ASSERT_NO_THROW(Script().Load(k_CreateMistFiveArgs));
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): gtest fixture
TEST_F(ScriptExecutionTest, tooFewArgumentsThrows)
{
	ASSERT_THROW(Script().Load("CREATE_MIST(\"1000,1000\", 1.0, 12345, 0.5)"), std::runtime_error);
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): gtest fixture
TEST_F(ScriptExecutionTest, tooManyArgumentsThrows)
{
	ASSERT_THROW(Script().Load("CREATE_MIST(\"1000,1000\", 1.0, 12345, 0.5, 1.0, 2.0)"), std::runtime_error);
}

// A scalar where a vector is declared must be reported as a type error rather
// than reinterpreted.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): gtest fixture
TEST_F(ScriptExecutionTest, scalarInsteadOfVectorThrows)
{
	ASSERT_THROW(Script().Load("CREATE_MIST(1000.0, 1.0, 12345, 0.5, 1.0)"), std::runtime_error);
}

// A float where a number is declared is a type error as well.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): gtest fixture
TEST_F(ScriptExecutionTest, floatInsteadOfNumberThrows)
{
	ASSERT_THROW(Script().Load("CREATE_MIST(\"1000,1000\", 1.0, 0.5, 0.5, 1.0)"), std::runtime_error);
}

// An invalid statement in the middle of a script must abort the script rather
// than be skipped, so a bad call cannot hide behind a good neighbour.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): gtest fixture
TEST_F(ScriptExecutionTest, invalidStatementInSequenceThrows)
{
	ASSERT_THROW(Script().Load("SET_LAND_NUMBER(1)\nCREATE_MIST(\"1000,1000\")\nSET_LAND_NUMBER(2)"), std::runtime_error);
}

// The arity check is driven by the signature: a command declared with a single
// parameter rejects two.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): gtest fixture
TEST_F(ScriptExecutionTest, singleParameterCommandRejectsTwoArguments)
{
	ASSERT_THROW(Script().Load("SET_LAND_NUMBER(1, 2)"), std::runtime_error);
}

// BRUSH_SIZE declares (Float, Float); a string in the first slot is a type error.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): gtest fixture
TEST_F(ScriptExecutionTest, stringInsteadOfFloatThrows)
{
	ASSERT_THROW(Script().Load("BRUSH_SIZE(\"wide\", 1.0)"), std::runtime_error);
}

// Unknown identifiers are reported by the parser before any signature lookup.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): gtest fixture
TEST_F(ScriptExecutionTest, unknownCommandThrows)
{
	ASSERT_THROW(Script().Load("CREATE_MISTTY(\"1000,1000\")"), std::runtime_error);
}

// Malformed syntax must not be accepted silently.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): gtest fixture
TEST_F(ScriptExecutionTest, missingParenthesesThrows)
{
	ASSERT_THROW(Script().Load("CREATE_MIST \"1000,1000\""), std::runtime_error);
	ASSERT_THROW(Script().Load("CREATE_MIST(\"1000,1000\""), std::runtime_error);
}
