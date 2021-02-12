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

#include "../SARIF.h"
#include <QFile>
#include <QTemporaryFile>

bool bytesAreEqual(const QByteArray& a, const QByteArray& b) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
	return a.compare(b) == 0;
#else
	if (a.size() == b.size()) {
		for (int byte = 0; byte < a.size(); ++byte) {
			if (a[byte] != b[byte])
				return false;
		}
		return true;
	}
	return false;
#endif
}


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

TEST_CASE("Base location is correct", "[sarif]") {
	const std::string expectedBaseLocation("/home/jdoe/repo/");
	auto sarif = SARIF("PVS-freecad-23754_210125.sarif");
	auto baseLocation = sarif.GetBase();
	REQUIRE(baseLocation == expectedBaseLocation);
}