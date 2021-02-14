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


#ifndef _CLEANSARIF_CLEANER_H_
#define _CLEANSARIF_CLEANER_H_

#include <QThread>
#include <QException>

#include "SARIF.h"

class QString;

/**
	* \brief A worker class to process the SARIF file
	*
	* Before running, it should be configured using the `Set*()` family of functions, otherwise
	* it won't do anything.
	* 
	* This is a QThread, so once configured it will run in its own thread. Connect to the
	* `processComplete()` and `errorOcurred()` signals to be alerted when it has finished.
	*/
class Cleaner : public QThread {

	Q_OBJECT

public:

	/**
	 * \brief Construct a Cleaner with no initial state 
	 */
	Cleaner();
		
	~Cleaner() = default;

	/**
	 * \brief Set the input file
	 * \param infile Must be a readable file, and is expected to be a SARIF JSON-formatted file.
	 * \note If this is not set prior to running the thread, the run will do nothing. Just setting
	 * it does not execute the thread, you must still call start() (or run(), for asynchronous
	 * operation).
	 */
	void SetInfile(const QString& infile);

	/**
	 * \brief Set the output filename
	 * \param outfile Must be writable, and can be the same as \a infile. If it is the same, a
	 * backup file is automatically created. \a outfile will only be created in the event of complete
	 * success. Any failures in processing prevent it from being created, or from overwriting
	 * \a infile if that was what was requested.
	 * \note If outfile is set to something other than "", then executing this thread runs both the
	 * load and the clean routines. If outfile is unset, or set to "", then only the load is run.
	 */
	void SetOutfile(const QString &outfile);
		

	/**
	 * \brief List the rules present in this SARIF object
	 * \returns a tuple containing the ID of the rule and its hits count
	 */
	std::vector<std::tuple<QString, int>> GetRules() const;

	/**
	 * \brief Get the part of the artifactLocation that all results have in common
	 */
	QString GetBase() const;

	/**
	 * \brief Modify the artifactLocation in the SARIF results to be "rebased" on a new location
	 * \param newBase the new location
	 * \see GetBase()
	 */
	void SetBase(const QString& newBase);

	/**
	 * \brief When outputting this object, don't include rule \s ruleID
	 * \param ruleID The ID of the rule to suppress
	 * \returns The number of results that this filter will remove (independent for each rule)
	 */
	int SuppressRule(const QString& ruleID);

	/**
	 * \brief Remove suppression of a rule
	 * \param ruleID The ID of the rule to unsuppress
	 * \see SuppressRule()
	 */
	void UnsuppressRule(const QString& ruleID);

	/**
	 * \brief Get a list of the currently-suppressed rules
	 */
	QStringList SuppressedRules() const;

	/**
	 * \brief Suppress the output of results whose artifactLocation matches a regular expression
	 * \param regex The regular expression to apply to the artifactLocation
	 * \returns The number of results this filter will remove (independent of any other filter)
	 */
	int AddLocationFilter(const QString& regex);

	/**
	 * \brief Stop suppression of results matching a given regex
	 * \param regex The regular expression to remove from the suppression list
	 * \see AddLocationFilter()
	 */
	void RemoveLocationFilter(const QString& regex);

	/**
	 * \brief Get the list of currently-active location filters
	 */
	QStringList LocationFilters() const;

	/**
		* \brief This function is generally not called directly, but is run on its own thread by calling
		* the `start()` function on the Cleaner object.
		*/
	void run() override;

signals:

	/** 
     * \brief The final cleaned SARIF file was written
	 */
	void fileWritten(const QString & filename);

	/**
	 * \brief The input SARIF file was loaded
	 */
	void fileLoaded(const QString & filename);

	/**
		* \brief Signals failure of the run.
		* \param message A text message describing the failure.
		*/
	void errorOccurred(const QString &message);

private:

	QString _infile;
	QString _outfile;

	std::unique_ptr<SARIF> _sarif;
};

#endif // _CLEANSARIF_CLEANER_H_