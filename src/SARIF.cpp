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

#include "SARIF.h"

#include <exception>
#include <regex>

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

SARIF::SARIF(const std::string& file)
{
	QFile infile(QString::fromStdString(file));
	if (!infile.open(QIODevice::ReadOnly | QIODevice::Text))
		throw std::exception("Unable to open specified file");
	auto fileContents = infile.readAll();
	_json = QJsonDocument::fromJson(fileContents);
	if (_json.isNull())
		throw std::exception("File does not contain valid JSON data");

	// Make sure this is really SARIF data:
	auto o = _json.object();
	if (o.contains("$schema") && o["$schema"].isString()) {
		auto schema = o["$schema"].toString();
		if (schema.contains("sarif")) {
			std::string base;
			if (o.contains("runs") && o["runs"].isArray()) {
				auto runs = o["runs"].toArray().first().toObject();
				if (runs.contains("results") && runs["results"].isArray()) {
					auto resultArray = runs["results"].toArray();
					for (auto result = resultArray.begin(); result != resultArray.end(); ++result) {
						auto uri = SARIF::GetArtifactUri(result->toObject());
						if (base.empty())
							base = uri;
						else
							base = SARIF::MaxMatch(base, uri);
					}
				}
			}
			_originalBasePath = base;
		}
		else {
			throw std::exception("File read and JSON parsed, but schema is not SARIF");
		}
	}
	else {
		throw std::exception("File read, but no $schema found");
	}
}

void SARIF::Export(const std::string& file) const
{
	QJsonDocument filteredJSONDoc(_json); // Makes a copy

	// Make our changes to the new copy
	auto o = filteredJSONDoc.object();
	if (o.contains("runs") && o["runs"].isArray()) {
		auto runs = o["runs"].toArray().first().toObject();
		if (runs.contains("results") && runs["results"].isArray()) {
			auto resultArray = runs["results"].toArray();
			for (auto result = resultArray.begin(); result != resultArray.end(); ++result) {
			}
		}
	}

	// Write out the copy
	QFile newFile(QString::fromStdString(file));
	newFile.open(QIODevice::OpenModeFlag::Truncate | QIODevice::OpenModeFlag::WriteOnly);
	if (newFile.isOpen())
		newFile.write(filteredJSONDoc.toBinaryData());
	else
		throw std::exception("Could not open requested file for writing");
}

std::vector<std::tuple<std::string, std::string>> SARIF::Rules() const
{
	std::vector<std::tuple<std::string, std::string>> ruleTuples;
	auto o = _json.object();
	if (o.contains("runs") && o["runs"].isArray()) {
		auto runs = o["runs"].toArray().first().toObject();
		if (runs.contains("tool") && runs["tool"].isObject()) {
			auto tool = runs["tool"].toObject();
			if (tool.contains("driver") && tool["driver"].isObject()) {
				auto driver = tool["driver"].toObject();
				if (driver.contains("rules") && driver["rules"].isArray()) {
					auto rules = driver["rules"].toArray();
					for (auto rule = rules.begin(); rule != rules.end(); ++rule) {
						if (rule->isObject()) {
							auto ruleObject = rule->toObject();
							std::string id;
							if (ruleObject.contains("id")) 
								id = ruleObject["id"].toString().toStdString();
							std::string text;
							if (ruleObject.contains("shortDescription") && ruleObject["shortDescription"].toObject().contains("text"))
								text = ruleObject["shortDescription"].toObject()["text"].toString().toStdString();
							else if (ruleObject.contains("fullDescription") && ruleObject["fullDescription"].toObject().contains("text"))
								text = ruleObject["fullDescription"].toObject()["text"].toString().toStdString();
							else if (ruleObject.contains("help") && ruleObject["fullDescription"].toObject().contains("text"))
								text = ruleObject["help"].toObject()["text"].toString().toStdString();
							if (!id.empty())
								ruleTuples.emplace_back(id, text);
						}
					}
				}
			}
		}
	}
	return ruleTuples;
}

std::string SARIF::GetBase() const
{
	if (_overrideBase)
		return _overrideBaseWith;
	else
		return _originalBasePath;
}

void SARIF::SetBase(const std::string& newBase)
{
	_overrideBase = true;
	_overrideBaseWith = newBase;
}

int SARIF::SuppressRule(const std::string& ruleID)
{
	_suppressedRules.push_back(ruleID);

	int counter = 0;
	auto o = _json.object();
	if (o.contains("runs") && o["runs"].isArray()) {
		auto runs = o["runs"].toArray().first().toObject();
		if (runs.contains("results") && runs["results"].isArray()) {
			auto resultArray = runs["results"].toArray();
			for (auto result = resultArray.begin(); result != resultArray.end(); ++result) {
				if (SARIF::GetRule(result->toObject()) == ruleID)
					++counter;
			}
		}
	}
	return counter;
}

void SARIF::UnsuppressRule(const std::string& ruleID)
{
	_suppressedRules.erase(std::remove(_suppressedRules.begin(), _suppressedRules.end(), ruleID), _suppressedRules.end());
}

std::vector<std::string> SARIF::SuppressedRules() const
{
	return _suppressedRules;
}

int SARIF::AddLocationFilter(const std::string& regex)
{
	_locationFilters.push_back(regex);
	std::regex compiledRegex(regex);
	int counter = 0;
	auto o = _json.object();
	if (o.contains("runs") && o["runs"].isArray()) {
		auto runs = o["runs"].toArray().first().toObject();
		if (runs.contains("results") && runs["results"].isArray()) {
			auto resultArray = runs["results"].toArray();
			for (auto result = resultArray.begin(); result != resultArray.end(); ++result) {
				if (std::regex_search(SARIF::GetArtifactUri(result->toObject()), compiledRegex)) {
					++counter;
				}
			}
		}
	}
	return counter;
}

void SARIF::RemoveLocationFilter(const std::string& regex)
{
	_locationFilters.erase(std::remove(_locationFilters.begin(), _locationFilters.end(), regex), _locationFilters.end());
}

std::vector<std::string> SARIF::LocationFilters() const
{
	return _locationFilters;
}

std::string SARIF::GetArtifactUri(const QJsonObject& result)
{
	std::string uri;
	if (result.contains("locations") && result["locations"].isArray()) {
		auto location = result["locations"].toArray().first().toObject();
		if (location.contains("physicalLocation") && location["physicalLocation"].isObject()) {
			auto physicalLocation = location["physicalLocation"].toObject();
			if (physicalLocation.contains("artifactLocation") && physicalLocation["artifactLocation"].isObject()) {
				auto artifactLocation = physicalLocation["artifactLocation"].toObject();
				if (artifactLocation.contains("uri") && artifactLocation["uri"].isString()) {
					uri = artifactLocation["uri"].toString().toStdString();
				}
			}
		}
	}

	return uri;
}

std::string SARIF::MaxMatch(const std::string& a, const std::string& b)
{
	int location = 0;
	while (location < a.size() && location < b.size() && a[location] == b[location])
		++location;
	return a.substr(0,location);
}

std::string SARIF::GetRule(const QJsonObject& result)
{
	if (result.contains("ruleId") && result["ruleId"].isString())
		return result["ruleId"].toString().toStdString();
	else
		return std::string();
}
