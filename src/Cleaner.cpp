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

#include "Cleaner.h"

#include <QFile>

#include <sstream>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <stack>
#include <vector>
#include <set>
#include <algorithm>
#include <filesystem>


Cleaner::Cleaner(const QString& infile, const QString& outfile) :
	_infile(infile),
	_outfile(outfile),
	_sarif(infile.toStdString())
{
}

std::vector<std::tuple<QString, QString>> Cleaner::Rules() const
{
	auto internalRules = _sarif.Rules();
	std::vector<std::tuple<QString, QString>> rules;
	for (const auto& r : internalRules) {
		rules.emplace_back(std::make_tuple(QString::fromStdString(std::get<0>(r)), QString::fromStdString(std::get<1>(r))));
	}
	return rules;
}

QString Cleaner::GetBase() const
{
	return QString::fromStdString(_sarif.GetBase());
}

void Cleaner::SetBase(const QString& newBase)
{
	_sarif.SetBase(newBase.toStdString());
}

int Cleaner::SuppressRule(const QString& ruleID)
{
	return _sarif.SuppressRule(ruleID.toStdString());
}

void Cleaner::UnsuppressRule(const QString& ruleID)
{
	_sarif.UnsuppressRule(ruleID.toStdString());
}

QStringList Cleaner::SuppressedRules() const
{
	QStringList rules;
	auto internalRules = _sarif.SuppressedRules();
	for (const auto& rule : internalRules) {
		rules.append(QString::fromStdString(rule));
	}
	return rules;
}

int Cleaner::AddLocationFilter(const QString& regex)
{
	return _sarif.AddLocationFilter(regex.toStdString());
}

void Cleaner::RemoveLocationFilter(const QString& regex)
{
	_sarif.RemoveLocationFilter(regex.toStdString());
}

QStringList Cleaner::LocationFilters() const
{
	QStringList filters;
	auto internalFilters = _sarif.LocationFilters();
	for (const auto& filter : internalFilters) {
		filters.append(QString::fromStdString(filter));
	}
	return filters;
}

void Cleaner::run()
{

	if (_infile == _outfile) {
		// Make a backup:
		try {
			std::filesystem::copy(_infile.toStdString(), _infile.toStdString() + ".backup");
		}
		catch (...) {
			emit errorOccurred(QString::fromLatin1("Could not make a backup of ") + _infile);
			exit(-1);
			return;
		}
	}


	try {
		_sarif.Export(_outfile.toStdString());
	}
	catch (const std::exception& e) {
		emit errorOccurred(e.what());
		exit(-1);
		return;
	}
	catch (...) {
		emit errorOccurred(QString::fromLatin1("Failed to export to ") + _outfile);
		exit(-1);
		return;
	}


	emit processComplete();
}
