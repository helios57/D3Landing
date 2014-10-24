#include "main.h"
#include "mavlink_bridge.h"

using namespace std;
bool connect = false;
VRmDWORD key = 0;

int rc = 0;
char* bmp_file = "Cross.bmp";
OR_RES res_buf[1];
int nres = 0;
int nobj = 1;
int res_size = 1;
OR_HND or_hnd;
image pat_img;
image srh_img;
LEARN_PAR lpar; /* learn parameter structure    */
SEARCH_PAR spar; /* search parameter structure   */
image bmp_img = { 0, 0, 0, 0 };
int img_buf_size;
int scr_dx;
int scr_dy;
int scr_pitch;
int pat_x, pat_y; /* pattern coordinates          */
int pat_dx, pat_dy; /* pattern dimensionss          */
int srh_x, srh_y; /* search area coordinates      */
int srh_dx, srh_dy; /* search area dimensionss      */
image edg_img; /* edge-mask image              */

char *edg_buf = NULL; /* edge-mask image buffer       */
char *pat_buf = NULL; /* pattern image buffer         */
char *srh_buf = NULL; /* search image buffer          */

#define OR_ALLOC_ERROR  104
#define LCLR   4    /* learn drawing color  */
#define SCLR   8    /* search drawing color */

void img_set(image *img, int clr) /* [in] pixel value (color) to set */
{
	char *p = (char *) (int) img->st;
	int dx = img->dx;
	int dy = img->dy;

	if (!p || dx <= 0 || dy <= 0)
		return;

	while (dy--) {
		memset(p, clr, dx);
		p += img->pitch;
	}
	return;
}

void img_copy(image *img1, image *img2) /* output image */
{
	int dx = (img1->dx < img2->dx) ? img1->dx : img2->dx;
	int dy = (img1->dy < img2->dy) ? img1->dy : img2->dy;
	char *p1 = (char *) (int) img1->st;
	char *p2 = (char *) (int) img2->st;

	if (!p1 || !p2 || dx <= 0 || dy <= 0)
		return;

	while (dy--) {
		memcpy(p2, p1, dx);
		p1 += img1->pitch;
		p2 += img2->pitch;
	}
	return;
}

int main(int argc, char** argv) {
	if (connect) {
		initStreams();
	}

	VRmCreateVMLIBKey(&key);

	rc = vm_lib_open(key);
	if (rc) {
		printf("VM_LIB open error = %d\n", rc);
		exit(1);
	}
	or_init(&or_hnd);

	rc = read_bmp_file(bmp_file, &bmp_img);
	if (rc) {
		printf("read_bmp_file error = %d\n", rc);
		exit(1);
	} else {
		printf("BMP image size: (%d x %d)\n", bmp_img.dx, bmp_img.dy);
	}

	scr_dx = bmp_img.dx;
	scr_dy = bmp_img.dy;
	scr_pitch = scr_dx;

	pat_x = 0;
	pat_y = 0;
	pat_dx = 512;
	pat_dy = 512;

	/*----------------------------------------------------------------------*/
	/*                        Allocate image buffers                        */
	/*----------------------------------------------------------------------*/
	img_buf_size = scr_dy * scr_pitch;
	rc = OR_ALLOC_ERROR;

	/*............. Allocate edge-mask image buffer */
	edg_buf = (char *) mem_alloc(img_buf_size);
	if (!edg_buf)
		exit(1);

	pat_buf = (char *) mem_alloc(img_buf_size);
	if (!pat_buf)
		exit(1);

	srh_buf = (char *) mem_alloc(img_buf_size);
	if (!srh_buf)
		exit(1);

	rc = 0;

	/*............. Setup images */
	pat_img.st = (long) pat_buf;
	pat_img.dx = scr_dx;
	pat_img.dy = scr_dy;
	pat_img.pitch = scr_pitch;

	srh_img.st = (long) srh_buf;
	srh_img.dx = scr_dx;
	srh_img.dy = scr_dy;
	srh_img.pitch = scr_pitch;

	edg_img.st = (long) edg_buf;
	edg_img.dx = pat_dx;
	edg_img.dy = pat_dy;
	edg_img.pitch = scr_pitch;

	/*............. Learn control parameters */
	lpar.min_edge_h = 50; /* min edge height: 7,..,15,..,127              */
	lpar.min_edge_g = 12; /* min edge gradient: 6,..,15,..,min_edge_h     */
	lpar.min_chain = 8; /* min edge contour length: 4,..,24 pixels      */
	lpar.save_edge_image = 0; /* save edge image (show pattern contours)      */
	lpar.lrn_mode = 0; /* learn mode:                                  */
	/*  0: normal, no edge masking, edg_img ignored */
	/*  1: create edge-mask image in edg_img        */
	/*  2: use edg_img to mask unwanted learn       */
	/*     pattern edges                            */
	lpar.dr_clr = LCLR; /* drawing color [4,255]                        */

	/*............. Search control parameters */
	spar.min_edge_h = 50; /* min edge height: 7,..,15,..,127              */
	spar.min_edge_g = 12; /* min edge gradient, =6,..,15,..,min_edge_h    */
	spar.min_chain = 8; /* min edge contour length, =4,..,24 pixels     */
	spar.restr_search = 0; /* 1 = restricted_searching_mode                */
	spar.min_rate = 552; /* min matching rate, =160,..,1023, 1024==100%  */
	spar.contour_width = 5; /* min edge contour width, =3,5,7,9 pixels      */
	spar.min_scale = 1024; /* min scale, 512-1024(=100%), keep <=max_scale!*/
	spar.max_scale = 1024; /* max scale, 1024(=100%)-1536, keep <200% !    */
	spar.rot_sect_start = 0; /* rotation sector start, -180..179 deg         */
	spar.rot_sect_width = 90; /* rotation sector width, 0..360 deg            */
	spar.save_edge_image = 0; /* save edge image (show search contours)       */
	spar.dr_clr = SCLR; /* drawing color [4,255]                        */

	spar.max_pm_items = nobj; /* # of searched objects */

	//    srh_x = 10;
	//    srh_y = 10;
	//    srh_dx = srh_img.dx - 20;
	//    srh_dy = srh_img.dy - 20;

	srh_x = 100;
	srh_y = 100;
	srh_dx = 500;
	srh_dy = 350;

	img_set(&pat_img, 0);
	img_set(&srh_img, 0);

	img_copy(&bmp_img, &pat_img);
	img_copy(&bmp_img, &srh_img);

	/*----------------------------------------------------------------------*/
	/*                             Learn stage                              */
	/*----------------------------------------------------------------------*/

	/* Learn pattern */
	rc = or_learn(&or_hnd, &pat_img, NULL, &edg_img, pat_x, pat_y, pat_dx, pat_dy, &lpar);
	if (rc) {
		printf("or_demo: or_learn error = %d\n", rc);
		goto done;
	}
	//    printf("or_learn OK, press a key...\n");
	//    getchar();

	/*----------------------------------------------------------------------*/
	/*                              Search stage                            */
	/*----------------------------------------------------------------------*/

	/* Search pattern */
	if (spar.max_pm_items > res_size) /* truncate              */
		spar.max_pm_items = res_size;

	time = sys_time();
	rc = or_search(&or_hnd, &srh_img, NULL, srh_x, srh_y, srh_dx, srh_dy, &spar, res_buf, &nres);
	time = sys_time() - time; /* get search execution time */
	if (rc) {
		printf("or_demo: or_search error = %d\n", rc);
		goto done;
	}

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

	do {
		getNetxImage(vrmStatus);
		getMostRecentImage(vrmStatus);

		// Note: p_source_img may be null in case a recoverable error
		// (like a trigger timeout) occured.
		if (vrmStatus->p_source_img) {
			getFrameCounter(vrmStatus);
			cout << "frame grabbed clock():" << clock() << "  m_time_stamp: " << vrmStatus->p_source_img->m_time_stamp << "  frame_counter: " << vrmStatus->frame_counter << endl;
			// see, if we had to drop some frames due to data transfer stalls. if so,
			// output a message
			if (vrmStatus->frames_dropped)
				cout << "- " << vrmStatus->frames_dropped << " frames dropped-" << endl;

			Mat mat = getMat(vrmStatus->p_source_img);
			objs.clear();
			timespec detectTimer = timer_start();
			int detecteSize = 0;
			for (int i = 512; i > 32; i /= 2) {
				detecteSize = i;
				face_cascade->detectMultiScale(mat, objs, 1.1, 2, 0, Size(i, i));
				if (!objs.empty()) {
					break;
				}
			}
			long detectTime = timer_end(detectTimer);

			cout << "detectTime" << detectTime << "ms detectSize: " << detecteSize << endl;
			if (objs.size() > 0) {
				for (vector<Rect>::const_iterator r = objs.begin(); r != objs.end(); r++) {
					cout << "found obj: x=" << r->x << " y=" << r->y << " widh=" << r->width << " height=" << r->height << endl;
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
		}
		if (connect) {
			readFromStream();
			sendMessage();
		}
	} while (1);
#ifdef LINUX
	cvDestroyWindow("result");
#endif
	closeDevice(vrmStatus);
	cout << "exit." << endl;

	if (connect) {
		closeStream();
	}
	vm_lib_close();
	return 0;
}
