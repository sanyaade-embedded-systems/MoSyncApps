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

/**
 * @file main.cpp
 * @author Mikael Kindborg
 *
 * Template for an application that downloads and parses Json data
 * from a service.
 *
 * To make this example work, you must:
 * 1) Enter the SERVICE_URL below.
 * 2) Update the Json parser code to access the fields you want to use.
 */

#include <conprint.h>
#include <YAJLDom/YAJLDom.h>
#include "MyMoblet.h"

using namespace MAUtil;
using namespace MAUtil::YAJLDom;
using namespace EasyConnection;

/**
 * Connection class for downloading data. Used for downloding Json data.
 * The downloaded data is passed on to the moblet. 
 */
class JsonServiceConnection : public EasyHttpConnection
{
public:
	JsonServiceConnection(MyMoblet* moblet)
		: EasyHttpConnection()
	{
		mMoblet = moblet;
	}

	/**
	 * Called when the HTTP connection has finished downloading data.
	 * \param data Handle to the data, will be 0 on error, > 0 on success.
	 * \param result Result code, RES_OK on success, otherwise an HTTP 
	 * error code.
	 */
	void dataDownloaded(MAHandle data, int result)
	{
		mMoblet->dataDownloaded(data, result);
	}

private:
	/**
	 * Pointer to the moblet.
	  */
	MyMoblet* mMoblet;
};


/**
 * Initialize the application in the constructor.
 */
MyMoblet::MyMoblet()
{
	LOG("Application started\n");
	LOG("Touch screen to start download\n");
	if (NULL == SERVICE_URL)
	{
		maPanic(0, "You must edit MyMoblet.h and add a service url");
	}
}

/**
 * Destructor.
 */
MyMoblet::~MyMoblet()
{
	// Delete the connection.
	deleteConnection();
}

/**
 * Delete and close the connection if it exists.
 */
void MyMoblet::deleteConnection()
{
	if (NULL != mConnection)
	{
		mConnection->close();
		delete mConnection;
		mConnection = NULL;
	}
}

/**
 * Exit the application when the back or zero key is pressed.
 */
void MyMoblet::keyPressEvent(int keyCode, int nativeCode)
{
	if (MAK_BACK == keyCode || MAK_0 == keyCode)
	{
		// Call close to exit the application.
		close();
	}
}

/**
 * Called when the screen is touched. Start download
 * Json data.
 */
void MyMoblet::pointerPressEvent(MAPoint2d point)
{
	startDownloadJsonData();
}

/**
 * Start the download of the Json data from the SERVICE_URL.
 * \return SUCCESS if successful, ERROR on error.
 */
int MyMoblet::startDownloadJsonData()
{
	// Check that there is no ongoing connection.
	if (NULL != mConnection)
	{
		return ERROR;
	}

	// Set the download url.
	String url = SERVICE_URL;

	LOG("startDownloadJsonData url: %s\n", url.c_str());

	mConnection = new JsonServiceConnection(this);
	int result = mConnection->get(url.c_str());

	LOG("startDownloadJsonData result: %i\n", result);

	// Make sure connection is deleted in case download
	// failed to start.
	if (SUCCESS != result)
	{
		deleteConnection();
	}

	// Returns SUCCESS or ERROR.
	return result;
}

/**
 * Called when download of Json data is complete.
 */
void MyMoblet::dataDownloaded(MAHandle data, int result)
{
	// Delete the connection.
	deleteConnection();

	// Check that we have a valid data handle.
	if (data <= 0)
	{
		LOG("Failed to download data - result: %d\n", result);
		return;
	}

	// Get the Json data as a string.
	String jsonData;
	bool success = HandleToString(data, jsonData);
	if (!success)
	{
		LOG("Failed to get string data\n");
		return;
	}

	LOG("Data downloaded size: %d\n", jsonData.size());

	// Parse Json data.
	Value* root = YAJLDom::parse(
		(const unsigned char*)jsonData.c_str(),
		jsonData.size());

	// Traverse the Json tree and print data.
	traverseJsonTree(root);

	// Delete Json tree.
	deleteValue(root);
}

/**
 * Traverse and print Json data.
 * TODO: Adapt this function to do whatever you wish to do
 * with your own data.
 */
int MyMoblet::traverseJsonTree(Value* root)
{
	// Check that the root is valid.
	// The root type must be MAP for our data.
	if (NULL == root
		|| Value::NUL == root->getType()
		|| Value::MAP != root->getType())
	{
		return ERROR;
	}

	// Traverse the Json tree and print values.
	// The example data assumed by this code is in file:
	// https://raw.github.com/divineprog/MoSyncApps/master/JsonServiceConsumerTemplate/sample.json
	// TODO: Modify this code to do whatever you want to do with your own data.

	// Get the people array.
	Value* people = root->getValueForKey("people");
	if (Value::NUL == people->getType()
		|| Value::ARRAY != people->getType())
	{
		return ERROR;
	}

	// Iterate over the people array and print data.
	for (int i = 0; i < people->getNumChildValues(); ++i)
	{
		Value* person = people->getValueByIndex(i);
		Value* name = person->getValueForKey("name");
		Value* company = person->getValueForKey("company");
		LOG("name: %s company: %s\n",
			name->toString().c_str(),
			company->toString().c_str());
	}
}

