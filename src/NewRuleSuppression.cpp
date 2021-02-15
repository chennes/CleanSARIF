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

#include "NewRuleSuppression.h"

#pragma warning(push, 1) 
#include "ui_NewRuleSuppression.h"
#include <QSettings>
#include <QApplication>
#include <QScreen>
#pragma warning(pop)

NewRuleSuppression::NewRuleSuppression(QWidget* parent) : 
	QDialog(parent), 
	ui(new Ui::NewRuleSuppression)
{
	ui->setupUi(this);

	QSize defaultSize(600, 500);
	auto screenSize = qApp->primaryScreen()->size();
	QPoint upperLeft(screenSize.width() / 2 - defaultSize.width() / 2, screenSize.height() / 2 - defaultSize.height() / 2);
	QSettings settings;
	settings.beginGroup("NewRuleSuppression");
	resize(settings.value("size", defaultSize).toSize());
	move(settings.value("pos", upperLeft).toPoint());
	settings.endGroup();

}

NewRuleSuppression::~NewRuleSuppression()
{
}

void NewRuleSuppression::SetRulesList(const std::vector<std::tuple<QString, int>>& ruleList)
{
	ui->tableWidget->setRowCount(static_cast<int>(ruleList.size()));

	int row = 0;
	for (const auto &rule : ruleList) {
		QTableWidgetItem* name = new QTableWidgetItem(std::get<0>(rule));
		QTableWidgetItem* count = new QTableWidgetItem();
		count->setData(Qt::EditRole, std::get<1>(rule)); // Make sure to add as an integer so it sorts correctly
		ui->tableWidget->setItem(row, 0, name);
		ui->tableWidget->setItem(row, 1, count);
		++row;
	}
}

QStringList NewRuleSuppression::GetSelectedRules() const
{
	QStringList rules;
	auto selection = ui->tableWidget->selectedRanges();
	for (auto range = selection.begin(); range != selection.end(); ++range) {
		int firstRow = range->topRow();
		int lastRow = range->bottomRow();
		for (int row = firstRow; row <= lastRow; ++row) {
			rules.append(ui->tableWidget->item(row, 0)->text());
		}
	}
	return rules;
}

QString NewRuleSuppression::GetNote() const
{
	return ui->noteLineEdit->text();
}

std::tuple<QStringList, QString> NewRuleSuppression::GetNewRuleSuppression(QWidget* parent, const std::vector<std::tuple<QString, int>>& ruleList)
{
	NewRuleSuppression dialog (parent);
	dialog.SetRulesList(ruleList);
	auto result = dialog.exec();
	if (result == QDialog::Accepted) {
		return std::make_tuple(dialog.GetSelectedRules(), dialog.GetNote());
	}
	else {
		return std::tuple<QStringList, QString>();
	}
}

void NewRuleSuppression::done(int r)
{
	QSettings settings;
	settings.beginGroup("NewRuleSuppression");
	settings.setValue("size", size());
	settings.setValue("pos", pos());
	settings.endGroup();
	settings.sync();

	QDialog::done(r);
}