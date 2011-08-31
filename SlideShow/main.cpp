

// TODO: Add file header & comment.

#include <MAUtil/Moblet.h>
#include <conprint.h>

using namespace MAUtil;

// Hard coded value for the number of images.
#define NUMBER_OF_IMAGES 24

/**
 * Moblet that displays images (slides). To keep this
 * example simple, no scaling of the images is done.
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
	int mNumberOfImages;
	int mCurrentImageIndex;
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
