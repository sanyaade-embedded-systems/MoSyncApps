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
 * Application that displays a slide show. Touch right half of the
 * screen to go to the next slide, touch left half to go to the previous
 * slide.
 *
 * To keep this example simple, no scaling of the images is currently done.
 *
 * The actual slides used in the example was done for a GTUG presentation
 * in Stockholm, in February 10, 2011, and are now a bit out-dated. The
 * SlideShow app developed for that presentation was programmed in Lua. :)
 *
 * This example shows how to use a simple caching mechanism to load
 * and unload images, in order to prevent out-of-memory problems.
 *
 * On Android, the bitmap memory that can be used by an application
 * is limited. If you have too many/too big bitmaps you get the error:
 *
 *   java.lang.OutOfMemoryError: bitmap size exceeds VM budget
 *
 * Note that there are 24 images in the slide show, but only 6 unique
 * images. The images will be shown repeatedly. The reuse of the images
 * is just a way of stressing the memory limit (24 unloaded binary
 * resources will be allocated).
 *
 * If you inspect the log (adb logcat on Android), you can see how
 * images get loaded and unloaded as you move through the slides.
 *
 * The .ubin resource type does not load the resource into memory when
 * the application starts, but keeps them "on disk" in an unloaded state.
 * Whenever there is a syscall that takes a handle to a ubin resource
 * as a parameter, the data is read from disk and an object is created
 * in memory using that data (like an image). You then call maDestoryObject
 * to free the allocated object (for example an image). The resource is
 * still on disk, and therefore if can again be used to create a new object.
 * This is the way the caching mechanism used in this program works.
 *
 * Resource ids (handle values) start at 1, and in this program we
 * never refer to resource by name, but by the sequence number (index).
 * If you want to add other resources to the program, you can add them at
 * the end of the resource file, and refer to them by name. In that case
 * you need to include the resource definitions like this:
 *
 *   #include "MAHeaders.h"
 *
 * Happy hacking! :)
 */

#include <MAUtil/Moblet.h>
#include <conprint.h>

using namespace MAUtil;

// Hard coded value for the number of images.
#define NUMBER_OF_IMAGES 24

/**
 * Moblet that displays images (slides).
 */
class SlideShowMoblet : public Moblet
{
public:
	/**
	 * Initialize the application in the constructor.
	 */
	SlideShowMoblet() :
		mNumberOfImages(NUMBER_OF_IMAGES),
		mCurrentImageIndex(0)
	{
		// Set all image handles to NULL.
		for (int i = 0; i < mNumberOfImages; ++i)
		{
			mImageCache[i] = NULL;
		}

		// Show first slide.
		showCurrentSlide();
	}

	/**
	 * Exit the application when the back or zero key is pressed.
	 */
	void keyPressEvent(int keyCode, int nativeCode)
	{
		if (MAK_BACK == keyCode || MAK_0 == keyCode)
		{
			// Call close to exit the application.
			close();
		}
	}

	/**
	 * Called when the screen is touched. Here we flip
	 * to the next or previous slide, depending on which
	 * half of the screen we have touched.
	 */
	void pointerPressEvent(MAPoint2d point)
	{
		// Check which side of the screen we have touched.
		int midX = EXTENT_X(maGetScrSize());
		if (point.x < midX)
		{
			// Touched left half, go back.
			--mCurrentImageIndex;
			if (mCurrentImageIndex < 0)
			{
				// Wrap around.
				mCurrentImageIndex = mNumberOfImages - 1;
			}
		}
		else
		{
			// Touched right half, go forward.
			++mCurrentImageIndex;
			if (mCurrentImageIndex >= mNumberOfImages)
			{
				// Wrap around.
				mCurrentImageIndex = 0;
			}
		}

		// Display the slide.
		showCurrentSlide();
	}

	void showCurrentSlide()
	{
		MAHandle image = getImage(mCurrentImageIndex);
		if (NULL != image)
		{
			// TODO: Add scaling.
			maDrawImage(image, 0, 0);
			maUpdateScreen();
		}
		else
		{
			// Display a red screen if there is no image.
			// (Might happen in low memory conditions.)
			maSetColor(0xFF0000);
			maFillRect(0, 0, 10000, 10000);
			maUpdateScreen();
		}
	}

	MAHandle getImage(int index)
	{
		// Is the image loaded?
		if (NULL != mImageCache[index])
		{
			// Return it.
			return mImageCache[index];
		}

		// Try to load the image until successful.
		for (int i = 0; i < mNumberOfImages; ++i)
		{
			// Resource indexes start at 1 (not zero).
			MAHandle image = loadImageFromResource(index + 1);
			if (NULL != image)
			{
				// Success. Save image cache.
				mImageCache[index] = image;

				// Return the image.
				return image;
			}

			// Image was not loaded. Free an image.
			if (NULL != mImageCache[i])
			{
				// Print log message to track cache use.
				lprintfln("SlideShow: Deleting cached image.");

				// Free the image.
				maDestroyObject(mImageCache[i]);
				mImageCache[i] = NULL;
			}
		}

		// Image could not be loaded.
		return NULL;
	}

	MAHandle loadImageFromResource(int resourceId)
	{
		lprintfln(
			"SlideShow: loadImageFromResource: "
			"resourceId: %d "
			"size: %d B",
			resourceId,
			maGetDataSize(resourceId));

		MAHandle image = maCreatePlaceholder();
		int result = maCreateImageFromData(
			image,
			resourceId,
			0,
			maGetDataSize(resourceId));

		if (RES_OK == result)
		{
			// Print log message.
			lprintfln("SlideShow: Loading image.");

			return image;
		}
		else
		{
			return NULL;
		}
	}

private:
	/**
	 * The total number of images.
	 */
	int mNumberOfImages;

	/**
	 * The index of the image shown (current slide).
	 */
	int mCurrentImageIndex;

	/**
	 * Array that hold handles to images.
	 *
	 * TODO: It would be generally more useful to have
	 * a vector for this, since the list can then easily
	 * grow, if you download images and add to the slideshow,
	 * for example.
	 */
	MAHandle mImageCache[NUMBER_OF_IMAGES];
};

/**
 * Entry point of the program.
 */
extern "C" int MAMain()
{
	Moblet::run(new SlideShowMoblet());
	return 0;
}
