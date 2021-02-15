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

#ifndef _CLEANSARIF_NEWRULESUPPRESSION_H_
#define _CLEANSARIF_NEWRULESUPPRESSION_H_

#pragma warning(push, 1) 
#include <QDialog>
#pragma warning(pop) 

#include <memory>


namespace Ui {
	class NewRuleSuppression;
}

/**
 * \brief 
 */
class NewRuleSuppression : public QDialog
{
	Q_OBJECT

public:
	explicit NewRuleSuppression(QWidget* parent);
	~NewRuleSuppression();

	void SetRulesList(const std::vector<std::tuple<QString, int>>& ruleList);

	QStringList GetSelectedRules() const;
	QString GetNote() const;

	static std::tuple<QStringList, QString> GetNewRuleSuppression(QWidget* parent, const std::vector<std::tuple<QString, int>>& ruleList);

public slots:

	void done(int r) override;

private:
	std::unique_ptr<Ui::NewRuleSuppression> ui;
};


#endif