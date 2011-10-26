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
 * File: EasyHttpConnection.cpp
 * Author: Mikael Kindborg
 */

#include <mastdlib.h>
#include <mastring.h>
#include <mavsprintf.h>
#include <MAUtil/PlaceholderPool.h>
#include "EasyHttpConnection.h"

using namespace MAUtil;

namespace EasyConnection
{

// *************** Global functions *************** //

/**
 * Utility function that copies string data referenced by a
 * handle to a String.
 */
bool HandleToString(MAHandle data, MAUtil::String& str)
{
	// Get size of data.
	int size = maGetDataSize(data);

	// Allocate temporary buffer for the string data.
	char* buf = (char*) malloc(sizeof(char) * (size + 1));
	if (NULL == buf)
	{
		return false;
	}

	// Copy data to buffer.
	maReadData(data, buf, 0, size);

	// Zero terminate string.
	buf[size] = '\0';

    // Copy data to String object.
    str = buf;

    // Deallocate temporary buffer.
	free(buf);

	return true;
}

// *************** Local functions *************** //

/**
 * Allocate a handle from the placeholder pool.
 */
static MAHandle AllocateHandle();

/**
 * Return handle to placehoder pool, which deallocates it
 * (PlaceholderPool will call maDestroyObject).
 */
static void DeallocateHandle(MAHandle handle);

static MAHandle AllocateHandle()
{
	return PlaceholderPool::alloc();
}

static void DeallocateHandle(MAHandle handle)
{
	PlaceholderPool::put(handle);
}

// *************** Class EasyHttpConnection *************** //

EasyHttpConnection::EasyHttpConnection() :
	HttpConnection(this),
	mReader(NULL)
{
}

EasyHttpConnection::~EasyHttpConnection()
{
	deallocateData();
}

/**
 * This is the starting point of a JSON request.
 * \return SUCCESS if successful, ERROR on error.
 */
int EasyHttpConnection::postJsonRequest(const char* url, const char* jsonData)
{
	int result = create(url, HTTP_POST);
	if (result < 0)
	{
		return ERROR;
	}

	char contentLength[16];
	sprintf(contentLength, "%i", (int) strlen(jsonData));

	setRequestHeader("Content-type", "application/json");
	setRequestHeader("Charset", "UTF-8");
	setRequestHeader("Content-Length", contentLength);

	// Write request data.
	write(jsonData, strlen(jsonData));

	// Next this that happens is that connWriteFinished is called.

	return SUCCESS;
}

/**
 * This is the starting point of a GET request.
 */
int EasyHttpConnection::get(const char* url)
{
	int result = create(url, HTTP_GET);
	if (result > 0)
	{
		finish();

		// Next this that happens is that httpFinished is called.
	}

	return result;
}

void EasyHttpConnection::connWriteFinished(
	MAUtil::Connection* connection,
	int result)
{
	// Call finish to execute the post request if
	// write was successful.
	if (result > 0)
	{
		finish();

		// Next this that happens is that httpFinished is called.
	}
	else
	{
		// Inform about the error.
		dataDownloaded(0, result);
	}
}

/**
 * This method is called when the HTTP request is complete.
 * Now all data is sent to the server and we can start reading
 * the reply and download data.
 */
void EasyHttpConnection::httpFinished(
	MAUtil::HttpConnection* connection,
	int result)
{
	// Success status codes are 200 and 201.
	// TODO: Add more success codes here if needed.
	if ( ! (200 == result || 201 == result) )
	{
		// There was an error.
		dataDownloaded(0, result);
		return;
	}

	// Start to read the result using a DownloadReader helper object.
	deleteReader();
	mReader = new EasyReaderThatReadsChunks(this);
	mReader->startRecvToData();

	// Next this that happens is that connReadFinished is called.
}

void EasyHttpConnection::connRecvFinished(
	MAUtil::Connection* connection,
	int result)
{
	// Delegate to reader.
	mReader->connRecvFinished(result);
}

void EasyHttpConnection::connReadFinished(
	MAUtil::Connection* connection,
	int result)
{
	maPanic(
		1,
		"EasyHttpConnection::connReadFinished: "
		"This was not supposed to happen.");
}

/**
 * Called by an EasyReader when there is a download error.
 */
void EasyHttpConnection::downloadError(int result)
{
	close();
	deleteReader();
	deallocateData();
	dataDownloaded(0, result);
}

/**
 * Called by an EasyReader when download is successfully finished.
 */
void EasyHttpConnection::downloadSuccess(MAHandle handle)
{
	close();
	deleteReader();
	deallocateData();
	dataDownloaded(handle, RES_OK);
}

void EasyHttpConnection::deallocateData()
{
}

void EasyHttpConnection::deleteReader()
{
	if (mReader)
	{
		delete mReader;
		mReader = NULL;
	}
}

// *************** Class EasyReader *************** //

/**
 * Constructor.
 */
EasyReader::EasyReader(EasyHttpConnection* connection)
: mConnection(connection),
  mContentLength(0)
{
}

int EasyReader::getContentLength()
{
	return mContentLength;
}

// *************** Class EasyReaderThatReadsChunks *************** //

/**
 * Constructor.
 */
EasyReaderThatReadsChunks::EasyReaderThatReadsChunks(
		EasyHttpConnection* connection)
: EasyReader(connection),
  mDataChunkSize(2048),
  mDataChunkOffset(0)
{
}

/**
 * Destructor.
 */
EasyReaderThatReadsChunks::~EasyReaderThatReadsChunks()
{
	// Deallocate chunks.
	while (0 < mDataChunks.size())
	{
		// Remove first remaining chunk.
		MAHandle chunk = mDataChunks[0];

		// Return chunk to pool.
		DeallocateHandle(chunk);

		// Remove chunk from list.
		mDataChunks.remove(0);
	}
}

/**
 * Start downloading data.
 */
void EasyReaderThatReadsChunks::startRecvToData()
{
	// Content length is unknown, read data in chunks until we get
	// CONNERR_CLOSED.
	bool success = readNextChunk();
	if (!success)
	{
		mConnection->downloadError(RES_OUT_OF_MEMORY);
	}
}

/**
 * Called when the new data is available.
 */
void EasyReaderThatReadsChunks::connRecvFinished(int result)
{
	// If the connection is closed we have completed reading the data.
	if (CONNERR_CLOSED == result)
	{
		finishedDownloadingChunkedData();
		return;
	}

	// Have we got an error?
	if (result <= 0)
	{
		mConnection->downloadError(result);
		return;
	}

	// We have new data.
	mDataChunkOffset += result;
	mContentLength += result;
	int leftToRead = mDataChunkSize - mDataChunkOffset;

	if (leftToRead > 0)
	{
		// Read more data into current chunk.
		int currentChunkIndex = mDataChunks.size() - 1;
		MAHandle chunk = mDataChunks[currentChunkIndex];
		mConnection->recvToData(chunk, mDataChunkOffset, leftToRead);
	}
	else
	{
		// Read next chunk.
		bool success = readNextChunk();
		if (!success)
		{
			mConnection->downloadError(RES_OUT_OF_MEMORY);
		}
	}
}

bool EasyReaderThatReadsChunks::readNextChunk()
{
	// Allocate new a chunk of data.
	MAHandle chunk = AllocateHandle();
	int result = maCreateData(chunk, mDataChunkSize);
	if (RES_OUT_OF_MEMORY == result)
	{
		return false;
	}
	else
	{
		// Start reading into the new chunk.
		mDataChunks.add(chunk);
		mDataChunkOffset = 0;
		mConnection->recvToData(chunk, mDataChunkOffset, mDataChunkSize);
		return true;
	}
}

void EasyReaderThatReadsChunks::finishedDownloadingChunkedData()
{
	// Allocate big handle and copy the chunks to it.
	// mContentLength holds the accumulated size of read data.
	// We create a new placeholder here, not using the pool.
	// TODO: Consider using PlaceholderPool also for this handle.
	MAHandle dataHandle = maCreatePlaceholder();
	int errorCode = maCreateData(dataHandle, mContentLength);
	if (RES_OUT_OF_MEMORY == errorCode)
	{
		mConnection->downloadError(RES_OUT_OF_MEMORY);
		return;
	}

	// Copy chunks to the data object.
	int offset = 0;
	char* buf = new char[mDataChunkSize];
	while (0 < mDataChunks.size())
	{
		// Last chunk should only be partially written.
		int dataLeftToWrite = mContentLength - offset;

		// Set size to min(dataLeftToWrite, mDataChunkSize)
		int size = (dataLeftToWrite < mDataChunkSize
			? dataLeftToWrite : mDataChunkSize);

		// Copy first remaining chunk.
		MAHandle chunk = mDataChunks[0];
		maReadData(chunk, buf, 0, size);
		maWriteData(dataHandle, buf, offset, size);

		// Return chunk to pool.
		DeallocateHandle(chunk);

		// Remove chunk from list.
		mDataChunks.remove(0);

		// Increment offset.
		offset += mDataChunkSize;
	}
	delete[] buf;

	// Download is finished! Tell the connection about this.
	mConnection->downloadSuccess(dataHandle);
}

} // namespace
