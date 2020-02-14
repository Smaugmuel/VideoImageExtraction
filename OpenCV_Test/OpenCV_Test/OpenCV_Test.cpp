#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <crtdbg.h>
//#include "ppmIO.hpp"

//#define OUTPUT_VIDEO

int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);



	/*****************************************
	** Independent Constants *****************
	*****************************************/
	constexpr size_t FRAME_JUMP = 5;			// Number of frames to jump before sampling the next one
	constexpr size_t VIDEO_INDEX = 1;			// Video identifier for image file names
	constexpr size_t IMAGES_TO_PRINT = 20'000;	// Minimum number of images to print before stopping, assuming there are enough frames
	//const cv::Size ROI_SIZE(64, 64);			// Sizes of regions of interest and images
	const cv::Size ROI_SIZE(64, 64);			// Sizes of regions of interest and images
	constexpr int INVERSE_SCALE_WIDTH = 8;		// Inverse scale of width, from ROI_SIZE, of the final image
	constexpr int INVERSE_SCALE_HEIGHT = 8;		// Inverse scale of height, from ROI_SIZE, of the final image

	const std::string VID_DIR_PATH = "C:/Users/Samuel Asp/Documents/BTH/Master_Thesis_Project/Data_sets/";	// Video input directory
	const std::string VID_PATH = VID_DIR_PATH + "VID_20200129_150436.mp4";									// Full single video path
	//const std::string IMG_OUT_DIR_PATH = "D:/BTH Saved Files/Examensarbete/Images64x64_roiScale4/";			// Image output directory
	const std::string IMG_OUT_DIR_PATH =
		"D:/BTH Saved Files/Examensarbete/Images_dims" +
		std::to_string(ROI_SIZE.width) + "x" + std::to_string(ROI_SIZE.height) + "_scale" +
		std::to_string(INVERSE_SCALE_WIDTH) + "x" + std::to_string(INVERSE_SCALE_HEIGHT) + "/";			// Image output directory
#ifdef OUTPUT_VIDEO
	const std::string VID_OUT_PATH = "D:/BTH Saved Files/Examensarbete/Videos/OutputVideo.avi";
#endif

	/*****************************************
	** Read input video file *****************
	*****************************************/
	cv::VideoCapture inputVideo(VID_PATH);
	if (!inputVideo.isOpened())
	{
		std::cout << "ERROR: Could not open video file!\n";
		return 0;
	}
	
	/*****************************************
	** Video-dependent constants *************
	*****************************************/
	const int INPUT_WIDTH = (int)inputVideo.get(cv::CAP_PROP_FRAME_WIDTH);						// Frame width
	const int INPUT_HEIGHT = (int)inputVideo.get(cv::CAP_PROP_FRAME_HEIGHT);					// Frame height
	const int OUTPUT_WIDTH = INPUT_WIDTH / INVERSE_SCALE_WIDTH;									// Frame width after scale
	const int OUTPUT_HEIGHT = INPUT_HEIGHT / INVERSE_SCALE_HEIGHT;								// Frame height after scale
	const cv::Size ROI_COUNT(OUTPUT_WIDTH / ROI_SIZE.width, OUTPUT_HEIGHT / ROI_SIZE.height);	// Number of regions and images per frame
	const int IMAGES_PER_FRAME = ROI_COUNT.width * ROI_COUNT.height;							// Total number of images per frame

	const std::string FILE_NAME_PREFIX =
		"Clouds_vid" + std::to_string(VIDEO_INDEX) +
		
		//"_dims_" + std::to_string(ROI_SIZE.width) + "x" + std::to_string(ROI_SIZE.height) +
		//"_scale_" + std::to_string(INVERSE_SCALE_WIDTH) + "x" + std::to_string(INVERSE_SCALE_HEIGHT) +
		
		"_frame";

	/*****************************************
	** Variables *****************************
	*****************************************/
	cv::Mat frame;											// Frame retrieved from video
	cv::Mat scaledFrame;									// Frame after scaling
	cv::Mat region;											// Image created from region of frame
	cv::Rect roi(0, 0, ROI_SIZE.width, ROI_SIZE.height);	// Current region area
	size_t frameIndex = FRAME_JUMP;							// Current frame index. Does not start at 0 to avoid invalid initial frames
	size_t imagesPrinted = 0;								// Number of images generated


#ifdef OUTPUT_VIDEO
	/*****************************************
	** Create output video file **************
	*****************************************/
	cv::VideoWriter outputVideo;
	outputVideo.open(
		VID_OUT_PATH,
		(int)inputVideo.get(cv::CAP_PROP_FOURCC),
		inputVideo.get(cv::CAP_PROP_FPS),
		cv::Size(WIDTH, HEIGHT),
		true
		);

	if (!outputVideo.isOpened())
	{
		std::cout << "ERROR: Could not create video file!\n";
		return 0;
	}
#endif


	/*****************************************
	** Generate and print images *************
	*****************************************/
	while (true)
	{
		// Retrieve every n:th frame
		inputVideo.set(cv::CAP_PROP_POS_FRAMES, (double)frameIndex);
		inputVideo >> frame;

		// Quit when reaching the end of the video
		if (frame.empty()) break;

		// Resize unless scale is equal to 1x1
		if constexpr (!(INVERSE_SCALE_WIDTH == 1 && INVERSE_SCALE_HEIGHT == 1))
			cv::resize(frame, scaledFrame, cv::Size(), 1.0 / INVERSE_SCALE_WIDTH, 1.0 / INVERSE_SCALE_HEIGHT);
		else
			scaledFrame = frame;

#ifdef OUTPUT_VIDEO
		// Append frame to output video
		outputVideo << frame;
#endif

		// Frame dependent file name prefix
		const std::string FILE_NAME_FRAME_PREFIX = FILE_NAME_PREFIX +
			std::to_string(frameIndex) + "_area";

		// Print images for every region
		for (int x = 0; x < ROI_COUNT.width; x++)
		{
			for (int y = 0; y < ROI_COUNT.height; y++)
			{
				// Move region around frame
				roi.x = ROI_SIZE.width * x;
				roi.y = ROI_SIZE.height * y;

				// Retrieve region of image
				region = scaledFrame(roi);

				// Final full file path
				const std::string fullPath =
					IMG_OUT_DIR_PATH + 
					FILE_NAME_FRAME_PREFIX +
					std::to_string(x) + "x" + std::to_string(y) + ".png";

				// Write image to file
				cv::imwrite(fullPath, region);
			}
		}

		// Skip forward a number of frames
		frameIndex += FRAME_JUMP;

		// Stop iterating if the number of images exceeds the threshold
		imagesPrinted += IMAGES_PER_FRAME;
		if (imagesPrinted >= IMAGES_TO_PRINT) break;
	}

	/*****************************************
	** Release OpenCV resources **************
	*****************************************/
	region.release();
	scaledFrame.release();
	frame.release();
	inputVideo.release();
#ifdef OUTPUT_VIDEO
	outputVideo.release();
#endif

	return 0;
}


/*	NOTES

	Check out "restrict" in parameters

	#if false

	// Load first frame
	vidCap >> frame;

	// Convert channel order from BGR to RGB
	cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);

	// Write the image to a .ppm-file
	std::cout << (PPM::WriteBinary(frame.data, W, H, 255, "image.ppm")
		? "Succeeded\n" : "Failed\n");


	// Convert channel order from RGB to BGR
	cv::cvtColor(frame, frame, cv::COLOR_RGB2BGR);

	// Display the image for five seconds
	cv::imshow("Flipped twice", frame);
	cv::waitKey(5000);
#elif false

	// Set tenth frame
	vidCap.set(cv::CAP_PROP_POS_FRAMES, 10);

	// Load first frame
	vidCap >> frame;

	// Convert channel order from BGR to RGB
	//cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
	cv::cvtColor(frame, frame, cv::COLOR_BGR2GRAY);
	//cv::Mat newFrame = cv::Mat(W, H, CV_8UC3, frame.data);

	// Convert channel order from RGB to BGR
	//cv::cvtColor(frame, frame, cv::COLOR_RGB2BGR);

	// Display the image for five seconds
	cv::imshow("Flipped twice", frame);
	cv::waitKey(5000);

#elif false
	//vidCap >> frame;

	cv::Rect topLeftROI(0, 0, 1024, 1024);
	cv::Rect topRightROI(1024, 0, 1024, 1024);

	//cv::Mat topLeftFrame = frame(topLeft);
	//cv::Mat topRightFrame = frame(topRight);

	//cv::imshow("Original", frame);
	//cv::imshow("Top Left", topLeftFrame);
	//cv::imshow("Top Right", topRightFrame);

	//cv::waitKey(20000);

	constexpr bool isColored = false;

	cv::VideoWriter vidWrite;
	vidWrite.open(
		"TestVideoTopRightGrayscale.avi",
		static_cast<int>(vidCap.get(cv::CAP_PROP_FOURCC)),
		vidCap.get(cv::CAP_PROP_FPS),
		cv::Size(1024, 1024),
		isColored
	);

	if (vidWrite.isOpened())
	{
		while (true)
		{
			vidCap >> frame;
			if (frame.empty()) break;

			cv::Mat topRightFrame = frame(topRightROI);

			//cv::cvtColor(topRightFrame, topRightFrame, cv::COLOR_BGR2GRAY);

			vidWrite << topRightFrame;
		}
	}

	//if (vidWrite.isOpened())
	//{
	//	vidWrite << frame;
	//	vidCap >> frame;
	//	vidWrite << frame;
	//	//vidWrite << topLeftFrame;
	//	//vidWrite << topRightFrame;
	//}

	//vidWrite.release();
	//topLeftFrame.release();
	//topRightFrame.release();

#elif true

#else
	while (vidCap.read(frame))
	{
		//cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
		cv::imshow("RGB", frame);
		cv::waitKey(1);
	}
	cv::destroyAllWindows();
#endif

*/
