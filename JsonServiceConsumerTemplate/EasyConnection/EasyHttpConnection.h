/*
 * Copyright (c) 2011 MoSync AB
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*
 * File: EasyHttpConnection.h
 * Author: Mikael Kindborg
 */

#ifndef EASYHTTPCONNECTION_H
#define EASYHTTPCONNECTION_H

#include <ma.h>
#include <malloc.h>
#include <MAUtil/String.h>
#include <MAUtil/Connection.h>

namespace EasyConnection
{
// Constants for error codes.
#define SUCCESS 1
#define ERROR -1

/**
 * Utility function that copies string data referenced by a
 * handle to a String.
 */
bool HandleToString(MAHandle data, MAUtil::String& str);

// Forward declaration.
class EasyHttpConnection;

/**
 * \brief Base class for helper classes that handle the download.
 * We have only one such class right now, EasyReaderThatReadsChunks.
 */
class EasyReader
{
public:
	/**
	 * Constructor.
	 */
	EasyReader(EasyHttpConnection* connection);

	/**
	 * Start downloading data.
	 */
	virtual void startRecvToData() = 0;

	/**
	 * Called when the new data is available.
	 */
	virtual void connRecvFinished(int result) = 0;

	int getContentLength();

protected:
	/**
	 * The connection object served by this reader.
	 */
	EasyHttpConnection* mConnection;

	/**
	 * Total length of downloaded data (accumulated value).
	 */
	int mContentLength;
};

/**
 * \brief Class that handles download when content-length is NOT known.
 * Here we read in chunks until we get result CONNERR_CLOSED in
 * connRecvFinished.
 */
class EasyReaderThatReadsChunks : public EasyReader
{
public:
	/**
	 * Constructor.
	 */
	EasyReaderThatReadsChunks(EasyHttpConnection* downloader);

	/**
	 * Destructor.
	 */
	virtual ~EasyReaderThatReadsChunks();

	/**
	 * Start downloading data.
	 */
	virtual void startRecvToData();

	/**
	 * Called when the new data is available.
	 */
	virtual void connRecvFinished(int result);

protected:
	bool readNextChunk();
	void finishedDownloadingChunkedData();

protected:
	/**
	 * Vector with chunks used while downloading data.
	 */
	MAUtil::Vector<MAHandle> mDataChunks;

	/**
	 * Size of a chunk of data.
	 */
	int mDataChunkSize;

	/**
	 * Current location (write offset) in the current chunk.
	 */
	int mDataChunkOffset;
};

/**
 * A high-level HTTP connection object that is a bit easier to use
 * that HttpConnection. Has an integrated listener.
 * This class coes not the the "content-length" HTTP header and thus
 * works when this header is not set.
 */
class EasyHttpConnection :
	public MAUtil::HttpConnection,
	public MAUtil::HttpConnectionListener
{
public:
	EasyHttpConnection();
	virtual ~EasyHttpConnection();

	/**
	 * This is the starting point of the JSON request.
	 * \return SUCCESS if successful, ERROR on error.
	 */
	int postJsonRequest(const char* url, const char* jsonData);

	/**
	 * This is the starting point of a GET request.
	 */
	int get(const char* url);

	/**
	 * Called by an EasyReader when download is successfully finished.
	 */
	void downloadSuccess(MAHandle handle);

	/**
	 * Called by an EasyReader when there is a download error.
	 */
	void downloadError(int result);

protected:
	/**
	 * Implement this method in a subclass of this class.
	 * Called when the HTTP connection has finished downloading data.
	 * \param data Handle to the data, will be 0 on error, > 0 on success.
	 * \param result Result code, RES_OK on success, otherwise an HTTP error code.
	 * The subclass takes ownership of this data and has the responsibility
	 * of deallocating the data.
	 */
	virtual void dataDownloaded(MAHandle data, int result) = 0;

	/**
	 * This method is called when the HTTP request is complete.
	 * Now all data is sent to the server and we can start reading
	 * the reply and download data.
	 */
	void httpFinished(MAUtil::HttpConnection* connection, int result);

	void connWriteFinished(MAUtil::Connection* connection, int result);

	void connRecvFinished(MAUtil::Connection* connection, int result);

	void connReadFinished(MAUtil::Connection* connection, int result);

	void deallocateData();

	/**
	 * Delete the reader object (this is the object that
	 * performs the download).
	 */
	void deleteReader();

private:
	/**
	 * Object that performs the actual download.
	 */
	EasyReader* mReader;
};

} // namespace

#endif
