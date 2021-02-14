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

#include "NewFileFilter.h"

#pragma warning(push, 1) 
#include "ui_NewFileFilter.h"
#include <QMessageBox>
#include <QSettings>
#include <QApplication>
#include <QScreen>
#pragma warning(pop)

#include <regex>

NewFileFilter::NewFileFilter(QWidget* parent) :
	QDialog(parent),
	ui(new Ui::NewFileFilter)
{
	ui->setupUi(this);

	QSize defaultSize(600, 500);
	auto screenSize = qApp->primaryScreen()->size();
	QPoint upperLeft(screenSize.width() / 2 - defaultSize.width() / 2, screenSize.height() / 2 - defaultSize.height() / 2);
	QSettings settings;
	settings.beginGroup("NewFileFilter");
	resize(settings.value("size", defaultSize).toSize());
	move(settings.value("pos", upperLeft).toPoint());
	settings.endGroup();
}

void NewFileFilter::SetFiles(const QStringList& allFiles)
{
	_allFiles = allFiles;
	ui->regexLineEdit->setText(".*");
	on_testButton_clicked();
}

QString NewFileFilter::GetFilter() const
{
	// Make sure it compiles before sending it along:
	try {
		std::regex re(ui->regexLineEdit->text().toStdString());
	}
	catch (const std::regex_error& e) {
		return QString::fromStdString(e.what());
	}
	return ui->regexLineEdit->text();
}

QString NewFileFilter::GetNote() const
{
	return ui->noteLineEdit->text();
}

int NewFileFilter::GetNumberOfMatches() const
{
	auto regex = ui->regexLineEdit->text().toStdString();
	try {
		auto re = std::regex(regex);
		int matches = 0;
		for (auto file = _allFiles.begin(); file != _allFiles.end(); ++file) {
			if (std::regex_search(file->toStdString(), re)) {
				++matches;
			}
		}
		return matches;
	}
	catch (const std::regex_error&) {
		return 0;
	}
}

void NewFileFilter::on_testButton_clicked()
{
	ui->resultsList->clear();
	auto regex = ui->regexLineEdit->text().toStdString();
	try {
		auto re = std::regex(regex);
		for (auto file = _allFiles.begin(); file != _allFiles.end(); ++file) {
			if (std::regex_search(file->toStdString(), re)) {
				ui->resultsList->addItem(*file);
			}
		}
		ui->numberOfMatchesLabel->setText(QString::number(ui->resultsList->count()));
	}
	catch (const std::regex_error& e) {
		QMessageBox::critical(this, tr("Invalid Regular Expression"), e.what());
	}
}

std::tuple<QString, QString, int> NewFileFilter::GetNewFileFilter(QWidget* parent, const QStringList& allFiles)
{
	NewFileFilter dialog(parent);
	dialog.SetFiles(allFiles);
	auto result = dialog.exec();
	if (result == QDialog::Accepted) {
		return std::make_tuple(dialog.GetFilter(), dialog.GetNote(), dialog.GetNumberOfMatches());
	}
	else {
		return std::make_tuple(QString(), QString(), 0);
	}
}


void NewFileFilter::done(int r)
{
	QSettings settings;
	settings.beginGroup("NewFileFilter");
	settings.setValue("size", size());
	settings.setValue("pos", pos());
	settings.endGroup();
	settings.sync();

	QDialog::done(r);
}