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
#include "QProgressIndicator.h"
#pragma warning(pop) 

#include "NewRuleSuppression.h"
#include "NewFileFilter.h"
#include "Cleaner.h"
#include "LoadingSARIF.h"

#include <iostream>

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
	setWindowIcon(QIcon("ui_icon.svg"));
	
#if __cplusplus >= 202000L
	std::string version = std::format("v{}.{}.{}", CleanSARIF_VERSION_MAJOR, CleanSARIF_VERSION_MINOR, CleanSARIF_VERSION_PATCH);
#else
	std::ostringstream s;
	s << "v" << CleanSARIF_VERSION_MAJOR << "." << CleanSARIF_VERSION_MINOR << "." << CleanSARIF_VERSION_PATCH << " pre-alpha";
	std::string version = s.str();
#endif
	ui->versionLabel->setText(QString::fromStdString(version));
	ui->fileFiltersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ui->suppressedRulesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

	connect(ui->fileFiltersTable, &QTableWidget::itemSelectionChanged, this, &MainWindow::fileFilterSelectionChanged);
	connect(ui->suppressedRulesTable, &QTableWidget::itemSelectionChanged, this, &MainWindow::ruleSuppressionSelectionChanged);

	QSettings settings; 

	settings.beginGroup("Options");
	ui->inputFileLineEdit->setText(settings.value("inputFile", "").toString());
	ui->outputFileLineEdit->setText(settings.value("outputFile", "").toString());
	settings.endGroup();

	if (ui->inputFileLineEdit->text().isEmpty())
		disableForNoInput();
	else
		enableForInput();

	// Calculate a default position:
	QSize defaultSize(600, 500);
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
	connect(_cleaner.get(), &Cleaner::fileLoaded, this, &MainWindow::loadComplete);
	connect(_cleaner.get(), &Cleaner::errorOccurred, this, &MainWindow::loadFailed);
	_cleaner->start();
}

void MainWindow::on_browseInputFileButton_clicked()
{
	auto filename = QFileDialog::getOpenFileName(this, tr("Select SARIF file to clean"), ui->inputFileLineEdit->text(),tr("SARIF files (*.sarif)"));
	if (!filename.isEmpty()) {
		loadSARIF(filename);
	}
}

void MainWindow::on_browseOutputFileButton_clicked()
{
	auto filename = QFileDialog::getSaveFileName(this, tr("Save new file as..."), ui->outputFileLineEdit->text());
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
		ui->suppressedRulesTable->removeRow(row);
	}
}

void MainWindow::on_newRuleButton_clicked()
{
	auto rulesToSuppress = NewRuleSuppression::GetNewRuleSuppression(this, _cleaner->GetRules());
	const int start = ui->suppressedRulesTable->rowCount();
	ui->suppressedRulesTable->setRowCount(start + std::get<0>(rulesToSuppress).count());
	int row = start;
	for (auto rule = std::get<0>(rulesToSuppress).begin(); rule != std::get<0>(rulesToSuppress).end(); ++rule) {
		QTableWidgetItem* name = new QTableWidgetItem(*rule);
		QTableWidgetItem* note = new QTableWidgetItem(std::get<1>(rulesToSuppress));
		ui->suppressedRulesTable->setItem(row, 0, name);
		ui->suppressedRulesTable->setItem(row, 1, note);
		++row;
	}
}

void MainWindow::on_saveFiltersButton_clicked()
{
}

void MainWindow::on_loadFiltersButton_clicked()
{
}

void MainWindow::on_cleanButton_clicked()
{
	ui->cleanButton->setDisabled(true);
	
	// Store off the last thing we ran as our default settings for next time...
	QSettings settings;	
	settings.beginGroup("Options");

	settings.setValue("inputFile", ui->inputFileLineEdit->text());
	settings.setValue("outputFile", ui->outputFileLineEdit->text());
	
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

void MainWindow::handleSuccess()
{
	QMessageBox::information(this, tr("Processing complete"), tr("The SARIF file was processed successfully."), QMessageBox::Close);
	ui->cleanButton->setEnabled(true);
}

void MainWindow::handleError(const std::string &message)
{
	QMessageBox::warning(this, tr("Processing failed"), QString::fromStdString(message), QMessageBox::Close);
	ui->cleanButton->setEnabled(true);
}

void MainWindow::loadComplete(const QString& filename)
{
	_loadingDialog.reset();
	ui->inputFileLineEdit->setText(filename);
	enableForInput();
	createDefaultOutfileName();
}

void MainWindow::loadFailed(const QString& message)
{
	_loadingDialog.reset();
	QMessageBox::critical(this, tr("Loading failed"), message, QMessageBox::Close);
}

void MainWindow::disableForNoInput()
{
	ui->cleanButton->setDefault(false);
	ui->browseInputFileButton->setDefault(true);	
	
	ui->basePathLabel->setDisabled(true);
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

	ui->basePathLabel->setEnabled(true);
	ui->browseBasePathButton->setEnabled(true);
	ui->basePathLineEdit->setEnabled(true);
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
	auto newDefault = path + basename + tr("_cleaned", "Append to cleaned filename") + ".sarif";
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
	ui->outputFileLineEdit->setText(event->mimeData()->urls().first().toLocalFile());

	event->acceptProposedAction();
}