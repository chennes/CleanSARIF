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
		* \brief Construct a Cleaner that reads from \a infile and writes to \a outfile. 
		* 
		* \param infile Must be a readable file, and is expected to be a SARIF JSON-formatted file.
		* \param outfile Must be writable, and can be the same as \a infile. If it is the same, a
		* backup file is automatically created. \a outfile will only be created in the event of complete
		* success. Any failures in processing prevent it from being created, or from overwriting
		* \a infile if that was what was requested.
		*/
	Cleaner(const std::string& infile, const std::string& outfile);
		
	~Cleaner() = default;
		
	

	/**
		* \brief This function is generally not called directly, but is run on its own thread by calling
		* the `start()` function on the Cleaner object.
		*/
	void run() override;

signals:

	/** 
		* \brief Signals successful completion of the run. 
		*/
	void processComplete();

	/**
		* \brief Signals failure of the run.
		* \param message A text message describing the failure.
		*/
	void errorOccurred(const std::string &message);

private:


	std::string _infile;
	std::string _outfile;
};

#endif // _CLEANSARIF_CLEANER_H_