// MIT License
//
// Copyright(c) 2021 Chris Hennes <chennes@pioneerlibrarysystem.org>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <catch2/catch_test_macros.hpp>

#include <QFile>
#include <QTemporaryFile>
#include "../SARIF.h"
#include <memory>
#include <fstream>
#include <regex>


TEST_CASE("Fail on non-existent file", "[sarif]") {
	REQUIRE_THROWS(
		SARIF("Nonexistent.sarif")
	);
}

TEST_CASE("Fail on non-JSON file", "[sarif]") {
	REQUIRE_THROWS(
		SARIF("NotJSON.sarif")
	);
}

TEST_CASE("Fail on JSON file without schema", "[sarif]") {
	REQUIRE_THROWS(
		SARIF("NoSchema.sarif")
	);
}

TEST_CASE("Fail on non-SARIF schema", "[sarif]") {
	REQUIRE_THROWS(
		SARIF("NotSARIF.sarif")
	);
}

TEST_CASE("Read SARIF file", "[sarif]") {
	REQUIRE_NOTHROW(
		SARIF("PVS-freecad-23754_210125.sarif")
	);
}

TEST_CASE("Count of rules", "[sarif]") {
	const int expectedRuleCount = 113; // cat PVS-freecad-23754_210125.sarif | grep "\"id\":" --count

	auto sarif = SARIF("PVS-freecad-23754_210125.sarif");
	auto rules = sarif.Rules();
	REQUIRE(rules.size() == expectedRuleCount);
}

TEST_CASE("Base location is read correctly", "[sarif]") {
	const std::string expectedBaseLocation("/home/jdoe/repo/");
	auto sarif = SARIF("PVS-freecad-23754_210125.sarif");
	auto baseLocation = sarif.GetBase();
	REQUIRE(baseLocation == expectedBaseLocation);
}

TEST_CASE("Base location can be set in code", "[sarif]") {
	auto sarif = SARIF("PVS-freecad-23754_210125.sarif");
	auto baseLocation = sarif.GetBase();
	std::string expectedBase = baseLocation + "EXTRA_DATA/";
	sarif.SetBase(expectedBase);
	std::string newBase = sarif.GetBase();
	REQUIRE(newBase == expectedBase);
}

TEST_CASE("Rule suppression works in code", "[sarif]") {
	const std::string ruleToSuppress = "V008";
	const int expectedSuppressionCount = 6;
	auto sarif = SARIF("PVS-freecad-23754_210125.sarif");
	auto suppressionCount = sarif.SuppressRule(ruleToSuppress);
	REQUIRE(suppressionCount == expectedSuppressionCount);
}

TEST_CASE("Rules are returned", "[sarif]") {
	const std::string ruleToSuppress = "V008";
	auto sarif = SARIF("PVS-freecad-23754_210125.sarif");
	auto ruleList = sarif.SuppressedRules();
	REQUIRE(ruleList.size() == 0);
	auto suppressionCount = sarif.SuppressRule(ruleToSuppress);
	ruleList = sarif.SuppressedRules();
	REQUIRE(ruleList.size() == 1);
	REQUIRE(ruleList.front() == ruleToSuppress);
}

TEST_CASE("Rules can be erased", "[sarif]") {
	const std::string ruleToSuppress = "V008";
	auto sarif = SARIF("PVS-freecad-23754_210125.sarif");
	auto suppressionCount = sarif.SuppressRule(ruleToSuppress);
	sarif.UnsuppressRule(ruleToSuppress);
	auto ruleList = sarif.SuppressedRules();
	REQUIRE(ruleList.empty());
}

TEST_CASE("File suppression works in code", "[sarif]") {
	const std::string regexForSuppression("^.*Mod/Draft/.*\\.cpp$");
	const int expectedSuppressionCount = 9;
	auto sarif = SARIF("PVS-freecad-23754_210125.sarif");
	int suppressionCount = sarif.AddLocationFilter(regexForSuppression);
	REQUIRE(suppressionCount == expectedSuppressionCount);
}

TEST_CASE("Regexes are returned", "[sarif]") {
	const std::string regexForSuppression("^.*Mod/Draft/.*\\.cpp$");
	auto sarif = SARIF("PVS-freecad-23754_210125.sarif");
	auto ruleList = sarif.LocationFilters();
	REQUIRE(ruleList.size() == 0);
	int suppressionCount = sarif.AddLocationFilter(regexForSuppression);
	ruleList = sarif.LocationFilters();
	REQUIRE(ruleList.size() == 1);
	REQUIRE(ruleList.front() == regexForSuppression);
}

TEST_CASE("Regexes can be erased", "[sarif]") {
	const std::string regexForSuppression("^.*Mod/Draft/.*\\.cpp$");
	auto sarif = SARIF("PVS-freecad-23754_210125.sarif");
	int suppressionCount = sarif.AddLocationFilter(regexForSuppression);
	sarif.RemoveLocationFilter(regexForSuppression);
	auto ruleList = sarif.LocationFilters();
	REQUIRE(ruleList.empty());
}

TEST_CASE("Export creates file", "[sarif]") {
	// Cheat and use Qt to get a valid temp file name and location
	QTemporaryFile tempFile;
	tempFile.open();
	std::string filename = tempFile.fileName().toStdString() + ".sarif";
	tempFile.close();

	auto sarif = SARIF("PVS-freecad-23754_210125.sarif");
	sarif.Export(filename);

	QFile exportedFile(QString::fromStdString(filename));
	REQUIRE(exportedFile.exists());
	QFile::remove(QString::fromStdString(filename));
}

TEST_CASE("Export reports failure to open file", "[sarif]") {
	auto sarif = SARIF("PVS-freecad-23754_210125.sarif");
	REQUIRE_THROWS(sarif.Export("/you/probably/cant/write/to/this/file.sarif"));
}

TEST_CASE("Export can be imported", "[sarif]") {
	auto sarif = SARIF("PVS-freecad-23754_210125.sarif");
	QTemporaryFile tempFile;
	tempFile.open();
	std::string filename = tempFile.fileName().toStdString() + ".sarif";
	tempFile.close();
	sarif.Export(filename);
	REQUIRE_NOTHROW(SARIF(filename));
	QFile::remove(QString::fromStdString(filename));
}

TEST_CASE("Test equality comparison operator", "[sarif]") {
	auto sarifA = SARIF("SmallValidA.sarif");
	auto sarifAPlusWhitespace = SARIF("SmallValidAPlusWhitespace.sarif");
	auto sarifB = SARIF("SmallValidB.sarif");

	REQUIRE(sarifA == sarifA);
	REQUIRE(sarifA == sarifAPlusWhitespace);
	REQUIRE(sarifA != sarifB);
}

TEST_CASE("Import-export-import yields the same result", "[sarif]") {
	auto sarif = SARIF("PVS-freecad-23754_210125.sarif");
	QTemporaryFile tempFile;
	tempFile.open();
	std::string filename = tempFile.fileName().toStdString() + ".sarif";
	tempFile.close();
	sarif.Export(filename);
	std::unique_ptr<SARIF> sarif2;
	REQUIRE_NOTHROW(sarif2 = std::make_unique<SARIF>(filename));
	QFile::remove(QString::fromStdString(filename));
	REQUIRE(sarif == *sarif2);
}

TEST_CASE("Exported file removes filtered rules", "[sarif]") {
	auto sarif = SARIF("PVS-freecad-23754_210125.sarif");
	const std::string ruleToSuppress = "V008";
	auto suppressionCount = sarif.SuppressRule(ruleToSuppress);
	QTemporaryFile tempFile;
	tempFile.open();
	std::string filename = tempFile.fileName().toStdString() + ".sarif";
	tempFile.close();
	sarif.Export(filename);
	auto sarif2 = SARIF(filename);
	QFile::remove(QString::fromStdString(filename));
	REQUIRE(sarif != sarif2);

	auto suppressionCount2 = sarif2.SuppressRule(ruleToSuppress);
	REQUIRE(suppressionCount2 == 0);
}

TEST_CASE("Exported file removes filtered files", "[sarif]") {
	const std::string regexForSuppression("^.*Mod/Draft/.*\\.cpp$");
	auto sarif = SARIF("PVS-freecad-23754_210125.sarif");
	int suppressionCount = sarif.AddLocationFilter(regexForSuppression);
	QTemporaryFile tempFile;
	tempFile.open();
	std::string filename = tempFile.fileName().toStdString() + ".sarif";
	tempFile.close();
	sarif.Export(filename);
	auto sarif2 = SARIF(filename);
	QFile::remove(QString::fromStdString(filename));
	REQUIRE(sarif != sarif2);

	auto suppressionCount2 = sarif2.AddLocationFilter(regexForSuppression);
	REQUIRE(suppressionCount2 == 0);
}

TEST_CASE("Exported file updates base", "[sarif]") {
	auto sarif = SARIF("PVS-freecad-23754_210125.sarif");
	auto oldBase = sarif.GetBase();
	auto newBase = oldBase + "changed/";
	sarif.SetBase(newBase);

	QTemporaryFile tempFile;
	tempFile.open();
	std::string filename = tempFile.fileName().toStdString() + ".sarif";
	tempFile.close();
	sarif.Export(filename);
	auto sarif2 = SARIF(filename);
	QFile::remove(QString::fromStdString(filename));
	REQUIRE(sarif != sarif2);

	auto importedBase = sarif.GetBase();
	REQUIRE(importedBase == newBase);
}

TEST_CASE("Rule counts are correct", "[sarif]") {
	auto sarif = SARIF("SeveralRules.sarif");
	auto rules = sarif.GetRules();
	REQUIRE(rules.size() == 2);
	REQUIRE(rules.find("rule1") != rules.end());
	REQUIRE(rules.find("rule2") != rules.end());
	REQUIRE(rules.find("rule3") == rules.end());
	REQUIRE(rules["rule1"] == 2);
	REQUIRE(rules["rule2"] == 1);;
	REQUIRE(rules["rule3"] == 0);
}

TEST_CASE("Exported file puts version at the top", "[sarif]") {
	// Although the JSON standard is nominally unordered, SARIF actually specifies that the
	// first element in the file should be a version string. So we can't actually just use
	// pure JSON parsing either to read or write this data, because the element order is
	// undefined (and will be alphabetical, in the case of Qt's parser).

	auto sarif = SARIF("SmallValidA.sarif");
	QTemporaryFile tempFile;
	tempFile.open();
	std::string filename = tempFile.fileName().toStdString() + ".sarif";
	tempFile.close();
	sarif.Export(filename);

	std::ifstream exportedFile(filename);

	std::regex version("\"version\"\\s*:\\s\".*\"");
	std::regex data("[A-za-z0-9]");
	while (exportedFile.good()) {
		std::string line;
		std::getline(exportedFile, line);
		
		bool lineHasData = std::regex_search(line, data);
		bool lineHasVersion = std::regex_search(line, version);

		if (lineHasData) {
			if (lineHasVersion) {
				return; // We passed the test
			}
			FAIL("Exported file does not start with the version element");
		}
	}
}