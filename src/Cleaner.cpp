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

#pragma warning(push, 1) 
#include <QFile>
#pragma warning(pop)

#include <sstream>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <stack>
#include <vector>
#include <set>
#include <algorithm>
#include <filesystem>

using namespace std::placeholders;


Cleaner::Cleaner()
{
}

void Cleaner::SetInfile(const QString& infile)
{
	_infile = infile;
}

void Cleaner::SetOutfile(const QString& outfile)
{
	_outfile = outfile;
}

std::vector<std::tuple<QString, int>> Cleaner::GetRules() const
{
	auto internalRules = _sarif.GetRules();
	std::vector<std::tuple<QString, int>> rules;
	for (const auto& r : internalRules) {
		rules.emplace_back(std::make_tuple(QString::fromStdString(r.first), r.second));
	}
	return rules;
}

QStringList Cleaner::GetFiles() const
{
	QStringList files;
	auto internalFiles = _sarif.Files();
	for (const auto& file : internalFiles) {
		files.append(QString::fromStdString(file));
	}
	return files;
}

QString Cleaner::GetBase() const
{
	return QString::fromStdString(_sarif.GetBase());
}

void Cleaner::SetBase(const QString& newBase)
{
	// If the old base ended with a slash or backslash, make sure the new one does too
	auto oldBase = QString::fromStdString(_sarif.GetBase());
	QString adjustedBase = newBase;
	if (*oldBase.rbegin() == '/' && *adjustedBase.rbegin() != '/') {
		adjustedBase += "/";
	}
	else if (*oldBase.rbegin() == '\\' && *adjustedBase.rbegin() != '\\') {
		adjustedBase += "\\";
	}
	_newBase = adjustedBase;
}

int Cleaner::SuppressRule(const QString& ruleID)
{
	if (!_suppressedRules.contains(ruleID)) {
		_suppressedRules.append(ruleID);
	}
	return _sarif.SuppressRule(ruleID.toStdString());
}

void Cleaner::UnsuppressRule(const QString& ruleID)
{
	_suppressedRules.removeOne(ruleID);
	_sarif.UnsuppressRule(ruleID.toStdString());
}

QStringList Cleaner::SuppressedRules() const
{
	return _suppressedRules;
}

int Cleaner::AddLocationFilter(const QString& regex)
{
	if (!_fileFilters.contains(regex)) {
		_fileFilters.append(regex);
	}
	return _sarif.AddLocationFilter(regex.toStdString());
}

void Cleaner::RemoveLocationFilter(const QString& regex)
{
	_fileFilters.removeOne(regex);
	_sarif.RemoveLocationFilter(regex.toStdString());
}

QStringList Cleaner::LocationFilters() const
{
	return _fileFilters;
}

void Cleaner::run()
{
	if (_infile.isEmpty()) {
		emit errorOccurred(tr("No input file set, aborting run"));
		exit(-1);
		return;
	}

	// Reset the SARIF object:
	_sarif = SARIF();

	try {
		_sarif.Load(_infile.toStdString(), std::bind(&Cleaner::isInterruptionRequested, QThread::currentThread()));
	}
	catch (std::runtime_error& e) {
		emit errorOccurred(e.what());
		exit(-1);
		return;
	}
	emit fileLoaded(_infile);

	if (_outfile.isEmpty()) {
		exit(0);
		return;
	}

	_sarif.SetBase(_newBase.toStdString());

	for (const auto& fileFilter : _fileFilters) {
		_sarif.AddLocationFilter(fileFilter.toStdString());
		if (QThread::currentThread()->isInterruptionRequested()) {
			emit errorOccurred(tr("Operation cancelled"));
			exit(-1);
			return;
		}
	}

	for (const auto& rule : _suppressedRules) {
		_sarif.SuppressRule(rule.toStdString());
		if (QThread::currentThread()->isInterruptionRequested()) {
			emit errorOccurred(tr("Operation cancelled"));
			exit(-1);
			return;
		}
	}

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
	catch (const std::runtime_error& e) {
		emit errorOccurred(e.what());
		exit(-1);
		return;
	}
	catch (...) {
		emit errorOccurred(QString::fromLatin1("Failed to export to ") + _outfile);
		exit(-1);
		return;
	}

	emit fileWritten(_outfile);
}