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
	MavlinkBridge mavlink;
	mavlink.start();

	VrmStatus* vrmStatus = new VrmStatus();
	vector<Rect> objs;

	initCamera(vrmStatus);
	startGrabber(vrmStatus);
	cout << "Reading from camera..." << endl;

#ifdef LINUX
	cvNamedWindow("result");
	startWindowThread();
	Scalar color = CV_RGB(0, 0, 255);
#endif

	CascadeClassifier* face_cascade = new CascadeClassifier();
	face_cascade->load("cascade.xml");

	cout << "detectTime;midx;midy;x;y;width;height;roll;pitch;yaw" << endl;
	do {
		getNetxImage(vrmStatus);
		getMostRecentImage(vrmStatus);

		// Note: p_source_img may be null in case a recoverable error
		// (like a trigger timeout) occured.
		if (vrmStatus->p_source_img) {
			getFrameCounter(vrmStatus);
			//cout << "frame grabbed clock():" << clock() << "  m_time_stamp: " << vrmStatus->p_source_img->m_time_stamp << "  frame_counter: " << vrmStatus->frame_counter << endl;
			// see, if we had to drop some frames due to data transfer stalls. if so,
			// output a message
			//if (vrmStatus->frames_dropped)
			//	cout << "- " << vrmStatus->frames_dropped << " frames dropped-" << endl;

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
			//cout << "detectTime" << detectTime << "ms detectSize: " << detecteSize << endl;
			if (objs.size() > 0) {
				for (vector<Rect>::const_iterator r = objs.begin(); r != objs.end(); r++) {
					int midX = r->x + r->width / 2 - width / 2;
					int midY = r->y + r->height / 2 - height / 2;
					mavlink_attitude_t attitude = mavlink.getAttitude();
					//cout << "detectTime;midx;midy;x;y;width;height;roll;pitch;yaw";
					cout << detectTime << ";" << midX << ";" << midY << ";" << r->x << ";" << r->y;
					cout << ";" << r->width << ";" << r->height;
					cout << ";" << attitude.roll << ";" << attitude.pitch << ";" << attitude.yaw;
					cout << endl;

					//cout << " x=" << r->x << " y=" << r->y << " widh=" << r->width << " height=" << r->height << endl;
#ifdef LINUX
					rectangle(mat, cvPoint(cvRound(r->x), cvRound(r->y)), cvPoint(cvRound((r->x + r->width - 1)), cvRound((r->y + r->height - 1))), color, 3, 8, 0);
#endif
				}
			}
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
	cout << "exit." << endl;

	mavlink.stop();
	return 0;
}
