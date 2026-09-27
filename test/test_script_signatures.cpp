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
#include <gtest/gtest.h>

using openblack::lhscriptx::ParameterType;
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

// Invokes the bound command through the signature, which exercises the same
// parameter extraction path the script interpreter uses. A Game must be
// initialized first because the command writes to the entity registry.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST(ScriptSignatures, createMistIsInvocableWithFiveParameters)
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
	auto game = std::make_unique<openblack::Game>(std::move(args));
	ASSERT_TRUE(game->Initialize());

	const auto* signature = FindSignature(openblack::lhscriptx::FeatureScriptCommands::k_Signatures, "CREATE_MIST");
	ASSERT_NE(signature, nullptr) << "CREATE_MIST is not registered";
	ASSERT_NO_THROW(signature->command(
	    {openblack::lhscriptx::ScriptCommandParameter(0.0f, 0.0f, 0.0f), openblack::lhscriptx::ScriptCommandParameter(1.0f),
	     openblack::lhscriptx::ScriptCommandParameter(static_cast<int32_t>(0x80FF8800)),
	     openblack::lhscriptx::ScriptCommandParameter(0.5f), openblack::lhscriptx::ScriptCommandParameter(1.0f)}));
	game.reset();
}
