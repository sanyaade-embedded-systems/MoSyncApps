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
 * @file MyMoblet.h
 * @author Mikael Kindborg
 */

#ifndef MYMOBLET_H
#define MYMOBLET_H

#include <MAUtil/Moblet.h>
#include <EasyConnection/EasyHttpConnection.h>
#include <YAJLDom/YAJLDom.h>

// TODO: Enter the url that points to your service here.
//#define SERVICE_URL NULL
#define SERVICE_URL "https://raw.github.com/divineprog/MoSyncApps/master/JsonServiceConsumerTemplate/sample.json"
//#define SERVICE_URL "http://myserver.com/MyService"

// Shorthand for printing/logging.
#define LOG printf
//#define LOG lprintfln
//#define LOG

/**
 * Template for a moblet that consumes (reads) Json data from a service.
 */
class MyMoblet : public MAUtil::Moblet
{
public:
	/**
	 * Initialize the application in the constructor.
	 */
	MyMoblet();

	/**
	 * Destructor.
	 */
	virtual ~MyMoblet();

	/**
	 * Exit the application when the back or zero key is pressed.
	 */
	void keyPressEvent(int keyCode, int nativeCode);

	/**
	 * Called when the screen is touched. Start download
	 * Json data.
	 */
	void pointerPressEvent(MAPoint2d point);

	/**
	 * Called when download of Json data is complete.
	 */
	void dataDownloaded(MAHandle data, int result);

private:
	/**
	 * Delete and close the connection if it exists.
	 */
	void deleteConnection();

	/**
	 * Start the download of the Json data from the SERVICE_URL.
	 * \return SUCCESS if successful, ERROR on error.
	 */

	int startDownloadJsonData();

	/**
	 * Traverse and print Json data.
	 * TODO: Adapt this function to do whatever you wish to do
	 * with your own data.
	 */
	int traverseJsonTree(MAUtil::YAJLDom::Value* root);

private:
	/**
	 * The currently active connection. Only one connection 
	 * can be active at a time. If needed this can be changed.
	 */
	EasyConnection::EasyHttpConnection* mConnection;
};

#endif
