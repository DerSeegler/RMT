/* Multitouch Finger Detection Framework v4.0
* for VS2013 & OpenCV 2.4.10
*
* Bjoern Froemmer, January 2010 - March 2015
*/

#include "opencv2/opencv.hpp"
#include <time.h>
#include <math.h>
#include <windows.h>
#include <algorithm>

struct Blob
{
	int id;
	cv::Point2f position;
	bool found;
	int notFoundCount;
};

std::vector<Blob> blobs;


double getDistance(cv::Point2f source, cv::Point2f target) {
	return cv::norm(source - target);
}

int findNearestBlob(cv::Point2f currentBlobCenter) {
	for (int i = 0; i < blobs.size(); i++)
	{
		Blob blob = blobs.at(i);
		if (getDistance(blob.position, currentBlobCenter) < 25) {
			blobs.at(i).found = true;
			return i;
		}
	}

	return -1;
}

void deleteNotFoundBlobs() {
	for (int i = 0; i < blobs.size(); i++)
	{
		if (!blobs.at(i).found) {
			if (blobs.at(i).notFoundCount > 2) {
				blobs.erase(blobs.begin() + i);
			}
			else {
				blobs.at(i).notFoundCount++;
			}
		}
		else {
			blobs.at(i).found = false;
			blobs.at(i).notFoundCount = 0;
		}
	}
}

int main(void)
{
	//VideoCapture cap(0); // use the first camera found on the system
	cv::VideoCapture cap("../mt_camera_raw.AVI");

	if (!cap.isOpened())
	{
		std::cout << "ERROR: Could not open camera / video stream.\n";
		return -1;
	}

	// get the frame size of the videocapture object
	double videoWidth = cap.get(CV_CAP_PROP_FRAME_WIDTH);
	double videoHeight = cap.get(CV_CAP_PROP_FRAME_HEIGHT);

	cv::Mat frame, original, grey, result, blurredCopyOfFirstDiff;

	int currentFrame = 0; // frame counter
	clock_t ms_start, ms_end, ms_time; // time

	char buffer[10]; // buffer for int to ascii conversion -> itoa(...)

	for (;;)
	{
		Sleep(30);

		ms_start = clock(); // time start

		cap >> frame; // get a new frame from the videostream

		if (frame.data == NULL) // terminate the program if there are no more frames
		{
			std::cout << "TERMINATION: Camerastream stopped or last frame of video reached.\n";
			break;
		}

		if (currentFrame == 0) {
			original = frame.clone(); // copy frame to original
			cvtColor(original, original, CV_BGR2GRAY); // convert frame to greyscale image (copies the image in the process!)
		}

		cvtColor(frame, frame, CV_BGR2GRAY);

		cv::absdiff(original, frame, result);

		cv::blur(result, blurredCopyOfFirstDiff, cv::Size(40, 40));

		cv::absdiff(result, blurredCopyOfFirstDiff, result);

		cv::threshold(result, result, 16.0, 255.0, cv::THRESH_BINARY);

		std::vector<std::vector<cv::Point>> contours;
		std::vector<cv::Vec4i> hierarchy;

		findContours(result, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);
		// iterate through all the top-level contours -> "hierarchy" may not be empty!)
		if (hierarchy.size() > 0)
		{
			for (int idx = 0; idx >= 0; idx = hierarchy[idx][0])
			{
				// check contour size (number of points) and area ("blob" size)
				if (cv::contourArea(cv::Mat(contours.at(idx))) > 30 && contours.at(idx).size() > 4)
				{
					cv::RotatedRect rec = cv::fitEllipse(cv::Mat(contours.at(idx)));
					ellipse(result, rec, cv::Scalar(0, 0, 255), 1, 8);

					Blob currentBlob;
					int foundIndex = findNearestBlob(rec.center);
					if (foundIndex >= 0) {
						currentBlob = blobs.at(foundIndex);
						blobs.at(foundIndex).position = rec.center;
					}
					else {
						if (blobs.size() > 0) {
							currentBlob = { blobs.at(blobs.size() - 1).id + 1, rec.center, false, 0 };
						}
						else {
							currentBlob = { 1, rec.center, false, 0 };
						}

						blobs.push_back(currentBlob);
					}

					cv::putText(result, (std::string)_itoa(currentBlob.id, buffer, 10), cv::Point2i(rec.center.x + rec.size.width / 2 + 3, rec.center.y + rec.size.height / 2 + 3), cv::FONT_HERSHEY_PLAIN, 1, CV_RGB(255, 255, 255), 1, 8);

					// fit & draw ellipse to contour at index
					drawContours(result, contours, idx, cv::Scalar(255, 0, 0), 1, 8,
						hierarchy);
					// draw contour at index
				}
			}

			deleteNotFoundBlobs();
		}

		if (cv::waitKey(1) == 27) // wait for user input
		{
			std::cout << "TERMINATION: User pressed ESC\n";
			break;
		}

		currentFrame++;

		// time end
		ms_end = clock();
		ms_time = ms_end - ms_start;

		putText(result, "frame #" + (std::string)_itoa(currentFrame, buffer, 10), cvPoint(0, 15), cv::FONT_HERSHEY_PLAIN, 1, CV_RGB(255, 255, 255), 1, 8); // write framecounter to the image (useful for debugging)
		putText(result, "time per frame: " + (std::string)_itoa(ms_time, buffer, 10) + "ms", cvPoint(0, 30), cv::FONT_HERSHEY_PLAIN, 1, CV_RGB(255, 255, 255), 1, 8); // write calculation time per frame to the image

		imshow("window", result); // render the frame to a window	
	}

	std::cout << "SUCCESS: Program terminated like expected.\n";
	return 1;
}