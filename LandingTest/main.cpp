// ==============================================================================================
// This file is part of the VRmagic VRmUsbCam2 C API Demo Application
// ==============================================================================================
// Main Function
// ----------------------------------------------------------------------------------------------

#include "main.h"
#include "MavlinkBridge.h"

using namespace std;
using namespace d3;

int main(int argc, char** argv) {
	time_t starttime;
	time(&starttime);
	char datetime[80];
	struct tm tstruct = *localtime(&starttime);
	strftime(datetime, sizeof(datetime), "%Y%m%d%H%M%S", &tstruct);
	string dateTimeString = datetime;

	ofstream logfile;
	logfile.open(dateTimeString + ".log", ios_base::app);

	MavlinkBridge mavlink;
	mavlink.start();

	VrmStatus* vrmStatus = new VrmStatus();
	vector<Rect> objs;

	initCamera(vrmStatus);
	startGrabber(vrmStatus);
	logfile << "Reading from camera..." << endl;

#ifdef LINUX
	cvNamedWindow("result");
	startWindowThread();
#endif

	CascadeClassifier* face_cascade = new CascadeClassifier();
	face_cascade->load("cascade.xml");

	getNetxImage(vrmStatus);

	int width = vrmStatus->p_source_img->m_image_format.m_width;
	int height = vrmStatus->p_source_img->m_image_format.m_height;

	Scalar color = CV_RGB(0, 0, 255);
	VideoWriter videoOut(dateTimeString + ".avi", CV_FOURCC('M', 'J', 'P', 'G'), 10, Size(width, height), false);
	if (!videoOut.isOpened()) {
		cout << "VideoWriter could not be opened" << endl;
	}

	logfile << "frameCounter;detectTime;midx;midy;x;y;width;height;roll;pitch;yaw;correctureX;correctureY;localX;localY;localXSet;localYSet" << endl;
	do {
		getNetxImage(vrmStatus);
		getMostRecentImage(vrmStatus);

		// Note: p_source_img may be null in case a recoverable error
		// (like a trigger timeout) occured.
		if (vrmStatus->p_source_img) {
			getFrameCounter(vrmStatus);
			//logfile << "frame grabbed clock():" << clock() << "  m_time_stamp: " << vrmStatus->p_source_img->m_time_stamp << "  frame_counter: " << vrmStatus->frame_counter << endl;
			// see, if we had to drop some frames due to data transfer stalls. if so,
			// output a message
			//if (vrmStatus->frames_dropped)
			//	logfile << "- " << vrmStatus->frames_dropped << " frames dropped-" << endl;

			Mat mat = getMat(vrmStatus->p_source_img);
			objs.clear();
			timespec detectTimer = timer_start();
			//int detecteSize = 0;
			for (int i = 512; i > 32; i /= 2) {
				//detecteSize = i;
				face_cascade->detectMultiScale(mat, objs, 1.1, 2, 0, Size(i, i));
				if (!objs.empty()) {
					break;
				}
			}
			long int detectTime = timer_end(detectTimer);

			int height = vrmStatus->p_source_img->m_image_format.m_height;
			int width = vrmStatus->p_source_img->m_image_format.m_width;
			//logfile << "detectTime" << detectTime << "ms detectSize: " << detecteSize << endl;
			if (objs.size() > 0) {
				int maxX = 0;
				int maxY = 0;
				vector<Rect>::const_iterator max;
				for (vector<Rect>::const_iterator r = objs.begin(); r != objs.end(); r++) {
					if (r->width >= maxX && r->height >= maxY) {
						maxX = r->width;
						maxY = r->height;
						max = r;
					}
				}
				for (vector<Rect>::const_iterator r = objs.begin(); r != objs.end(); r++) {
					if (r != max) {
						rectangle(mat, cvPoint(cvRound(r->x), cvRound(r->y)), cvPoint(cvRound((r->x + r->width - 1)), cvRound((r->y + r->height - 1))), CV_RGB(0, 0, 128), 1, 8, 0);
					}
				}
				int midX = max->x + max->width / 2 - width / 2;
				int midY = max->y + max->height / 2 - height / 2;
				mavlink_attitude_t attitude = mavlink.getAttitude();
				float corrX = midX * 0.0005f + attitude.roll;
				float corrY = midY * 0.0005f + attitude.pitch;
				logfile << vrmStatus->frame_counter << ";" << detectTime << ";" << midX << ";" << midY << ";" << max->x << ";" << max->y;
				logfile << ";" << max->width << ";" << max->height;
				logfile << ";" << attitude.roll << ";" << attitude.pitch << ";" << attitude.yaw;
				logfile << ";" << corrX << ";" << corrY;
				mavlink_local_position_ned_t lp = mavlink.getLocalPostition();
				logfile << ";" << lp.x << ";" << lp.y;
				mavlink_position_target_local_ned_t lpt = mavlink.getLocalPositionTarget();
				logfile << ";" << lpt.x << ";" << lpt.y;
				logfile << endl;
				mavlink.sendCorrection(corrY * -1, corrX * 1, 0.0f);
				rectangle(mat, cvPoint(cvRound(max->x), cvRound(max->y)), cvPoint(cvRound((max->x + max->width - 1)), cvRound((max->y + max->height - 1))), color, 3, 8, 0);
			} else {
				mavlink.sendCorrection(0.0f, 0.0f, 0.0f);
			}
			putText(mat, to_string(vrmStatus->frame_counter), Point(0, 30), FONT_HERSHEY_SCRIPT_SIMPLEX, 1, color);
			videoOut.write(mat);
#ifdef LINUX
			imshow("result", mat);
			waitKey(1);
#endif
			unlockImage(vrmStatus);
			//mavlink_attitude_t at = mavlink.getAttitude();
			//printf("attitude: time=%f roll=%f pitch=%f yaw=%f rollspeed=%f pitchspeed=%f yawspeed=%f\n", //
			//(float) at.time_boot_ms, at.roll, at.pitch, at.yaw, at.rollspeed, at.pitchspeed, at.yawspeed);	//
		}
	} while (1);
#ifdef LINUX
	cvDestroyWindow("result");
#endif
	closeDevice(vrmStatus);
	logfile << "exit." << endl;

	mavlink.stop();
	return 0;
}
