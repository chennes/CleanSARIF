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

#include <string>
#include <vector>

#include <QJsonDocument>

class SARIF
{
public:
	
	/**
	 * \brief Construct a SARIF object from a SARIF-formatted input file
	 * \throws std::runtime_exception if the file cannot be loaded
	 * \param file The full path to the input file
	 */
	explicit SARIF(const std::string &file);

	/**
	 * \brief Export to a new SARIF file
	 * \param file The file to export to. Overwritten if pre-existing.
	 * \throws If export fails for any reason, a std::exception is thrown.
	 * The exported file reflects the application of the filters set in the various
	 * Set* functions in this class. The new file is a correctly-formatted SARIF file that
	 * has been filtered and modified according to those rules.
	 */
	void Export(const std::string& file) const;

	/**
	 * \brief List the rules present in this SARIF object
	 * \returns a tuple containing the ID of the rule, and its help text
	 */
	std::vector<std::tuple<std::string, std::string>> Rules() const;

	/**
	 * \brief Get the part of the artifactLocation that all results have in common
	 */
	std::string GetBase() const;

	/**
	 * \brief Modify the artifactLocation in the SARIF results to be "rebased" on a new location
	 * \param newBase the new location
	 * \see GetBase()
	 */
	void SetBase(const std::string &newBase);

	/**
	 * \brief When outputting this object, don't include rule \s ruleID
	 * \param ruleID The ID of the rule to suppress
	 * \returns The number of results that this filter will remove (independent for each rule)
	 */
	int SuppressRule(const std::string &ruleID);

	/**
	 * \brief Remove suppression of a rule
	 * \param ruleID The ID of the rule to unsuppress
	 * \see SuppressRule()
	 */
	void UnsuppressRule(const std::string &ruleID);

	/**
	 * \brief Get a list of the currently-suppressed rules 
	 */
	std::vector<std::string> SuppressedRules() const;

	/**
	 * \brief Suppress the output of results whose artifactLocation matches a regular expression
	 * \param regex The regular expression to apply to the artifactLocation
	 * \returns The number of results this filter will remove (independent of any other filter)
	 */
	int AddLocationFilter(const std::string &regex);

	/**
	 * \brief Stop suppression of results matching a given regex
	 * \param regex The regular expression to remove from the suppression list
	 * \see AddLocationFilter()
	 */
	void RemoveLocationFilter(const std::string& regex);

	/**
	 * \brief Get the list of currently-active location filters 
	 */
	std::vector<std::string> LocationFilters() const;

private:
	QJsonDocument _json;

	bool _overrideBase = false;
	std::string _overrideBaseWith;
	std::string _originalBasePath;

	std::vector<std::string> _suppressedRules;

	std::vector<std::string> _locationFilters;

	/**
	 * \brief A utility function to extract the artifact URI from a single SARIF-formatted JSON result object
	 * \param result - A JSON-formatted object that conforms to the SARIF schema for a single item in the result array.
	 * In particular, it is expected to have a \a locations array with a single \a physicalLocation object, containing an
	 * \a artifactLocation object, which contains a \a uri string. Any additional \a physicalLocation objects in the 
	 * \a locations array are ignored.
	 */
	static std::string GetArtifactUri(const QJsonObject& result);

	/**
	 * \brief Get the largest shared substring between \a a and \a b, starting from the front.
	 */
	static std::string MaxMatch(const std::string &a, const std::string &b);

	/**
	 * \brief Given a single result, return what rule it represents
	 * \param result - A JSON-formatted object that conforms to the SARIF schema for a single item in the result array.
	 */
	static std::string GetRule(const QJsonObject& result);
};