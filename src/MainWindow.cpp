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

#include "MainWindow.h"
#pragma warning(push, 1) 
#include "ui_MainWindow.h"

#include <version.h>

#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>
#include <QScreen>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "QProgressIndicator.h"
#pragma warning(pop) 

#include "NewRuleSuppression.h"
#include "NewFileFilter.h"
#include "Cleaner.h"
#include "LoadingSARIF.h"

#include <iostream>
#include <regex>

#if __cplusplus >= 202000L
#include <format>
#else
#include <sstream>
#endif

MainWindow::MainWindow(QWidget* parent) :
	QMainWindow(parent),
	ui(std::make_unique<Ui::MainWindow>()),
	_cleaner(std::make_unique<Cleaner>())
{
	ui->setupUi(this);
	setWindowIcon(QIcon(":/ui_icon.svg"));

	auto squeegie = QIcon(":/squeegie.svg");
	auto plus = QIcon(":/plus.svg");
	auto minus = QIcon(":/minus.svg");

	ui->cleanButton->setIcon(squeegie);
	ui->newFileFilterButton->setIcon(plus);
	ui->newRuleButton->setIcon(plus);
	ui->removeFileFilterButton->setIcon(minus);
	ui->removeRuleButton->setIcon(minus);
	
#if __cplusplus >= 202000L
	std::string version = std::format("v{}.{}.{}", CleanSARIF_VERSION_MAJOR, CleanSARIF_VERSION_MINOR, CleanSARIF_VERSION_PATCH);
#else
	std::ostringstream s;
	s << "v" << CleanSARIF_VERSION_MAJOR << "." << CleanSARIF_VERSION_MINOR << "." << CleanSARIF_VERSION_PATCH << " RC2";
	std::string version = s.str();
#endif
	ui->versionLabel->setText(QString::fromStdString(version));
	ui->fileFiltersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ui->suppressedRulesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

	connect(ui->fileFiltersTable, &QTableWidget::itemSelectionChanged, this, &MainWindow::fileFilterSelectionChanged);
	connect(ui->suppressedRulesTable, &QTableWidget::itemSelectionChanged, this, &MainWindow::ruleSuppressionSelectionChanged);
	connect(_cleaner.get(), &Cleaner::errorOccurred, this, &MainWindow::loadFailed);

	QSettings settings; 

	settings.beginGroup("Options");
	_lastOpenedDirectory = settings.value("lastOpenedDirectory", QDir::homePath()).toString();
	_lastSavedDirectory = settings.value("lastSavedDirectory", "").toString();
	settings.endGroup();

	if (ui->inputFileLineEdit->text().isEmpty())
		disableForNoInput();
	else
		enableForInput();

	// Calculate a default position:
	QSize defaultSize(800, 700);
	auto screenSize = qApp->primaryScreen()->size();
	QPoint upperLeft(screenSize.width() / 2 - defaultSize.width() / 2, screenSize.height() / 2 - defaultSize.height() / 2);

	settings.beginGroup("MainWindow");
	resize(settings.value("size", defaultSize).toSize());
	move(settings.value("pos", upperLeft).toPoint());
	settings.endGroup();
}

MainWindow::~MainWindow()
{
}

void MainWindow::loadSARIF(const QString& filename)
{
	_loadingDialog = std::make_unique<LoadingSARIF>(this);
	_loadingDialog->show();
	_cleaner->SetInfile(filename);
	connect(_loadingDialog.get(), &LoadingSARIF::rejected, _cleaner.get(), &Cleaner::requestInterruption);
	connect(_loadingDialog.get(), &LoadingSARIF::accepted, _cleaner.get(), &Cleaner::requestInterruption);
	connect(_cleaner.get(), &Cleaner::fileLoaded, this, &MainWindow::loadComplete);
	_cleaner->start();
}

void MainWindow::on_browseInputFileButton_clicked()
{
	QString startingDirectory = ui->inputFileLineEdit->text();
	if (startingDirectory.isEmpty()) {
		startingDirectory = _lastOpenedDirectory;
	}
	auto filename = QFileDialog::getOpenFileName(this, tr("Select SARIF file to clean"), startingDirectory,tr("SARIF files (*.sarif)"));
	if (!filename.isEmpty()) {
		loadSARIF(filename);
	}
}

void MainWindow::on_browseOutputFileButton_clicked()
{
	QString startingDirectory = ui->outputFileLineEdit->text();
	if (startingDirectory.isEmpty()) {
		startingDirectory = _lastSavedDirectory;
	}
	auto filename = QFileDialog::getSaveFileName(this, tr("Save new file as..."), _lastSavedDirectory);
	if (!filename.isEmpty())
		ui->outputFileLineEdit->setText(filename);
}

void MainWindow::on_browseBasePathButton_clicked()
{
	auto filename = QFileDialog::getExistingDirectory(this, tr("Set analyzed source code base path to"), ui->basePathLineEdit->text());
	if (!filename.isEmpty())
		ui->basePathLineEdit->setText(filename);
}

void MainWindow::on_removeFileFilterButton_clicked()
{
	auto ranges = ui->fileFiltersTable->selectedRanges();
	std::vector<int> rowsToRemove;
	for (const auto & range : ranges) {
		int start = range.topRow();
		int end = range.bottomRow();
		for (int row = start; row <= end; ++row) {
			rowsToRemove.push_back(row);
		}
	}
	std::sort(rowsToRemove.begin(), rowsToRemove.end(), std::greater<int>());
	for (auto row : rowsToRemove) {
		_cleaner->RemoveLocationFilter(ui->fileFiltersTable->item(row, 0)->text());
		ui->fileFiltersTable->removeRow(row);
	}
}

void MainWindow::on_newFileFilterButton_clicked()
{
	auto filesToFilter = NewFileFilter::GetNewFileFilter(this, _cleaner->GetFiles());
	
	if (std::get<0>(filesToFilter).isEmpty())
		return;

	const int row = ui->fileFiltersTable->rowCount();
	ui->fileFiltersTable->setRowCount(row + 1);

	QTableWidgetItem* filter = new QTableWidgetItem(std::get<0>(filesToFilter));
	QTableWidgetItem* note = new QTableWidgetItem(std::get<1>(filesToFilter));
	QTableWidgetItem* count = new QTableWidgetItem();
	count->setData(Qt::EditRole, std::get<2>(filesToFilter)); // Retain as integer for sorting
	ui->fileFiltersTable->setItem(row, 0, filter);
	ui->fileFiltersTable->setItem(row, 1, count);
	ui->fileFiltersTable->setItem(row, 2, note);
	_cleaner->AddLocationFilter(std::get<0>(filesToFilter));
}

void MainWindow::on_removeRuleButton_clicked()
{
	auto ranges = ui->suppressedRulesTable->selectedRanges();
	std::vector<int> rowsToRemove;
	for (const auto& range : ranges) {
		int start = range.topRow();
		int end = range.bottomRow();
		for (int row = start; row <= end; ++row) {
			rowsToRemove.push_back(row);
		}
	}
	std::sort(rowsToRemove.begin(), rowsToRemove.end(), std::greater<int>());
	for (auto row : rowsToRemove) {
		_cleaner->UnsuppressRule(ui->suppressedRulesTable->item(row,0)->text());
		ui->suppressedRulesTable->removeRow(row);
	}
}

void MainWindow::on_newRuleButton_clicked()
{
	auto rulesToSuppress = NewRuleSuppression::GetNewRuleSuppression(this, _cleaner->GetRules());
	const int start = ui->suppressedRulesTable->rowCount();
	ui->suppressedRulesTable->setRowCount(start + std::get<0>(rulesToSuppress).count());
	int row = start;
	for (const auto &rule : std::get<0>(rulesToSuppress)) {
		_cleaner->SuppressRule(rule);
		QTableWidgetItem* name = new QTableWidgetItem(rule);
		QTableWidgetItem* note = new QTableWidgetItem(std::get<1>(rulesToSuppress));
		ui->suppressedRulesTable->setItem(row, 0, name);
		ui->suppressedRulesTable->setItem(row, 1, note);
		++row;
	}
}
void MainWindow::on_cleanButton_clicked()
{
	ui->cleanButton->setDisabled(true);

	if (ui->replaceURICheckbox->isChecked()) {
		_cleaner->SetBase(ui->basePathLineEdit->text());
	}
	_cleaner->SetOutfile(ui->outputFileLineEdit->text());

	_loadingDialog = std::make_unique<LoadingSARIF>(this);
	_loadingDialog->show();
	connect(_loadingDialog.get(), &LoadingSARIF::rejected, _cleaner.get(), &Cleaner::requestInterruption);
	connect(_loadingDialog.get(), &LoadingSARIF::accepted, _cleaner.get(), &Cleaner::requestInterruption);
	connect(_cleaner.get(), &Cleaner::fileWritten, this, &MainWindow::cleanComplete);
	_cleaner->start();
	
	// Store off the last thing we ran as our default settings for next time...
	QSettings settings;	
	settings.beginGroup("Options");
	
	settings.endGroup();
	settings.sync();
}

void MainWindow::on_closeButton_clicked()
{
	QSettings settings;
	settings.beginGroup("MainWindow");
	settings.setValue("size", size());
	settings.setValue("pos", pos());
	settings.endGroup();
	settings.sync();
	QApplication::quit();
}

void MainWindow::on_replaceURICheckbox_stateChanged(int state)
{
	if (state == Qt::CheckState::Checked) {
		ui->browseBasePathButton->setEnabled(true);
		ui->basePathLineEdit->setEnabled(true);
	}
	else {
		ui->browseBasePathButton->setEnabled(false);
		ui->basePathLineEdit->setEnabled(false);
	}
}

void MainWindow::fileFilterSelectionChanged()
{
	auto count = ui->fileFiltersTable->selectedRanges().count();
	if (count > 0) {
		ui->removeFileFilterButton->setEnabled(true);
	}
	else {
		ui->removeFileFilterButton->setEnabled(false);
	}
}

void MainWindow::ruleSuppressionSelectionChanged()
{
	auto count = ui->suppressedRulesTable->selectedRanges().count();
	if (count > 0) {
		ui->removeRuleButton->setEnabled(true);
	}
	else {
		ui->removeRuleButton->setEnabled(false);
	}
}

void MainWindow::loadComplete(const QString& filename)
{
	_loadingDialog.reset();
	ui->inputFileLineEdit->setText(filename);
	enableForInput();
	createDefaultOutfileName();
	disconnect(_cleaner.get(), &Cleaner::fileLoaded, this, &MainWindow::loadComplete);

	QFileInfo fi(filename);
	_lastOpenedDirectory = fi.path();

	ui->basePathLineEdit->setText(_cleaner->GetBase());

	QSettings settings;
	settings.beginGroup("Options");
	settings.setValue("lastOpenedDirectory", _lastOpenedDirectory);
	settings.endGroup();
}

void MainWindow::loadFailed(const QString& message)
{
	_loadingDialog.reset();
	QMessageBox::critical(this, tr("Processing failed"), message, QMessageBox::Close);
}

void MainWindow::cleanComplete(const QString& filename)
{
	_loadingDialog.reset();
	disconnect(_cleaner.get(), &Cleaner::fileWritten, this, &MainWindow::cleanComplete);
	QMessageBox::information(this, tr("Processing complete"), tr("Cleaning complete. Output file in:\n") + filename, QMessageBox::Close);

	QFileInfo fi(filename);
	_lastSavedDirectory = fi.path();

	QSettings settings;
	settings.beginGroup("Options");
	settings.setValue("lastSavedDirectory", _lastSavedDirectory);
	settings.endGroup();

}

void MainWindow::disableForNoInput()
{
	ui->cleanButton->setDefault(false);
	ui->browseInputFileButton->setDefault(true);	
	
	ui->outputFileLabel->setDisabled(true);
	ui->outputFileLineEdit->setDisabled(true);
	ui->browseOutputFileButton->setDisabled(true);
	ui->replaceURICheckbox->setDisabled(true);
	ui->browseBasePathButton->setDisabled(true);
	ui->basePathLineEdit->setDisabled(true);
	ui->fileFiltersLabel->setDisabled(true);
	ui->fileFiltersTable->setDisabled(true);
	ui->suppressedRulesLabel->setDisabled(true);
	ui->suppressedRulesTable->setDisabled(true);
	ui->removeRuleButton->setDisabled(true);
	ui->newRuleButton->setDisabled(true);
	ui->removeFileFilterButton->setDisabled(true);
	ui->newFileFilterButton->setDisabled(true);
	ui->saveFiltersButton->setDisabled(true);
	ui->loadFiltersButton->setDisabled(true);
	ui->cleanButton->setDisabled(true);
}

void MainWindow::enableForInput()
{
	ui->cleanButton->setDefault(true);
	ui->browseInputFileButton->setDefault(false);

	ui->outputFileLabel->setEnabled(true);
	ui->outputFileLineEdit->setEnabled(true);
	ui->browseOutputFileButton->setEnabled(true);
	ui->replaceURICheckbox->setEnabled(true);
	//ui->browseBasePathButton->setEnabled(true); // Enabled on check of replaceURICheckbox
	//ui->basePathLineEdit->setEnabled(true); // Enabled on check of replaceURICheckbox
	ui->fileFiltersLabel->setEnabled(true);
	ui->fileFiltersTable->setEnabled(true);
	ui->suppressedRulesLabel->setEnabled(true);
	ui->suppressedRulesTable->setEnabled(true);
	//ui->removeRuleButton->setEnabled(true); // Enabled on selection
	ui->newRuleButton->setEnabled(true);
	//ui->removeFileFilterButton->setEnabled(true); // Enabled on selection
	ui->newFileFilterButton->setEnabled(true);
	ui->saveFiltersButton->setEnabled(true);
	ui->loadFiltersButton->setEnabled(true);
	ui->cleanButton->setEnabled(true);
}

void MainWindow::createDefaultOutfileName()
{
	auto infile = ui->inputFileLineEdit->text();
	QFileInfo fi(infile);
	auto path = fi.path();
	auto basename = fi.completeBaseName();
	auto newDefault = path + "/" + basename + tr("_filtered", "Appended to default output filename") + ".sarif";
	ui->outputFileLineEdit->setText(newDefault);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
	QSettings settings;
	settings.beginGroup("MainWindow");
	settings.setValue("size", size());
	settings.setValue("pos", pos());
	settings.endGroup();
	settings.sync();
	event->accept();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
	if (event->mimeData()->hasUrls())
		event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event)
{
	ui->inputFileLineEdit->setText(event->mimeData()->urls().first().toLocalFile());
	createDefaultOutfileName();
	event->acceptProposedAction();
}



// ---------------------- Loading and Saving Filter Data ---------------------- //

void MainWindow::on_saveFiltersButton_clicked()
{
	auto filename = QFileDialog::getSaveFileName(this, tr("Save filters as..."), _lastSavedDirectory, "JSON (*.json)");
	if (!filename.isEmpty()) {
		QJsonObject jsonBase;
		jsonBase.insert("application", QApplication::applicationName());
		jsonBase.insert("applicationVersion", ui->versionLabel->text());
		jsonBase.insert("fileFormatMajorVersion", "1");
		jsonBase.insert("fileFormatMinorVersion", "0");
		QJsonObject data;

		// Base path
		if (ui->replaceURICheckbox->isChecked()) {
			data.insert("basePath", ui->basePathLineEdit->text());
		}

		// Rule filters
		QJsonArray ruleFilters;
		for (int row = 0; row < ui->suppressedRulesTable->rowCount(); ++row) {
			QJsonObject filter;
			filter.insert("rule", ui->suppressedRulesTable->item(row, 0)->text());
			filter.insert("note", ui->suppressedRulesTable->item(row, 1)->text());
			ruleFilters.append(filter);
		}
		data.insert("ruleFilters", ruleFilters);

		// File filters
		QJsonArray fileFilters;
		for (int row = 0; row < ui->fileFiltersTable->rowCount(); ++row) {
			QJsonObject filter;
			filter.insert("regex", ui->fileFiltersTable->item(row, 0)->text());
			// Omit the match count, it's not part of the save data
			filter.insert("note", ui->fileFiltersTable->item(row, 2)->text());
			fileFilters.append(filter);
		}
		data.insert("fileFilters", fileFilters);

		jsonBase.insert("xdata", data);

		QJsonDocument doc(jsonBase);

		QFile outfile(filename);
		if (!outfile.open(QIODevice::WriteOnly | QIODevice::Text)) {
			QMessageBox::critical(this, tr("Save failed"), tr("Failed to create the file:\n") + filename, QMessageBox::Close);
			return;
		}
		outfile.write(doc.toJson());
		outfile.close();
	}
}

void MainWindow::on_loadFiltersButton_clicked()
{
	auto filename = QFileDialog::getOpenFileName(this, tr("Choose filter file..."), _lastOpenedDirectory, "JSON (*.json)");
	if (!filename.isEmpty()) {
		QFile infile(filename);
		if (!infile.open(QIODevice::ReadOnly | QIODevice::Text)) {
			QMessageBox::critical(this, tr("Load failed"), tr("Failed to open the file:\n") + filename, QMessageBox::Close);
			return;
		}
		auto doc = QJsonDocument::fromJson(infile.readAll());;

		if (!doc.object().contains("fileFormatMajorVersion")) {
			QMessageBox::critical(this, tr("Load failed"), tr("Unrecognized file format:\n") + filename, QMessageBox::Close);
			return;
		}
		auto version = doc.object()["fileFormatMajorVersion"].toString();
		if (version == "1") {
			loadVersion1(doc);
		}
		else {
			QMessageBox::critical(this, tr("Load failed"), tr("The current software cannot read file format version ") + version, QMessageBox::Close);
			return;
		}

	}
}

void MainWindow::loadVersion1(const QJsonDocument& doc)
{
	QJsonObject data = doc.object()["xdata"].toObject();

	if (data.contains("basePath") && data["basePath"].isString()) {
		QString basePath = data["basePath"].toString();
		ui->basePathLineEdit->setText(basePath);
		ui->replaceURICheckbox->setChecked(true);
	}

	if (data.contains("ruleFilters") && data["ruleFilters"].isArray()) {
		QJsonArray ruleFilters = data["ruleFilters"].toArray();
		int row = ui->suppressedRulesTable->rowCount();
		ui->suppressedRulesTable->setRowCount(row + ruleFilters.count());
		for (const auto& rule : ruleFilters) {
			QString ruleText = rule.toObject()["rule"].toString();
			QString ruleNote = rule.toObject()["note"].toString();

			QTableWidgetItem* name = new QTableWidgetItem(ruleText);
			QTableWidgetItem* note = new QTableWidgetItem(ruleNote);
			ui->suppressedRulesTable->setItem(row, 0, name);
			ui->suppressedRulesTable->setItem(row, 1, note);
			_cleaner->SuppressRule(ruleText);
			++row;
		}
	}

	if (data.contains("fileFilters") && data["fileFilters"].isArray()) {
		QJsonArray fileFilters = data["fileFilters"].toArray();
		int row = ui->fileFiltersTable->rowCount();
		ui->fileFiltersTable->setRowCount(row + fileFilters.count());
		auto currentFileList = _cleaner->GetFiles();
		for (const auto& rule : fileFilters) {
			QString regexText = rule.toObject()["regex"].toString();
			QString regexNote = rule.toObject()["note"].toString();
			auto matches = _cleaner->AddLocationFilter(regexText);

			QTableWidgetItem* regex = new QTableWidgetItem(regexText);
			QTableWidgetItem* note = new QTableWidgetItem(regexNote);
			QTableWidgetItem* count = new QTableWidgetItem();
			count->setData(Qt::EditRole, matches); // Retain as integer for sorting
			ui->fileFiltersTable->setItem(row, 0, regex);
			ui->fileFiltersTable->setItem(row, 1, count);
			ui->fileFiltersTable->setItem(row, 2, note);
			++row;
		}
	}
}

