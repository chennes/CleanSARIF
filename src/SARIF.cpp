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
			// This does look like a SARIF file
		}
		else {
			throw std::exception("File read and JSON parsed, but schema is not SARIF");
		}
	}
	else {
		throw std::exception("File read, but no $schema found");
	}
}

std::vector<std::tuple<std::string, std::string>> SARIF::Rules() const
{
	return std::vector<std::tuple<std::string, std::string>>();
}

std::string SARIF::GetBase() const
{
	return std::string();
}

void SARIF::SetBase(const std::string& newBase)
{
}

int SARIF::SuppressRule(const std::string& ruleID)
{
	return 0;
}

void SARIF::UnsuppressRule(const std::string& ruleID)
{
}

std::vector<std::string> SARIF::SuppressedRules() const
{
	return std::vector<std::string>();
}

int SARIF::AddLocationFilter(const std::string& regex)
{
	return 0;
}

void SARIF::RemoveLocationFilter(const std::string& regex)
{
}

std::vector<std::string> SARIF::LocationFilters() const
{
	return std::vector<std::string>();
}
