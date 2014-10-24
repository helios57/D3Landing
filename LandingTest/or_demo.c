/*----------------------------------------------------------------------*/
/**
* or_demo.c
* Object recognition demo program.
*
* This file contains demo program, which reads image from BMP file,
* performs object recognition (both learn & serch stage) and measures
* execution speed.
*
* Usage:
*   or_demo [nobj [bmp_file]]
* where:
*   nobj     = max # of searched objects (default=1)
*   bmp_file = source BMP file (program generated image if missing)
*
*/
/*----------------------------------------------------------------------*/

/* CCC */
#define VRM_COPY_PROTECTION  1  /* enable copy protection: 0=off, 1=on  */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "vm_lib.h"
#if VRM_COPY_PROTECTION
  #include <vrmusbcam2.h>
#endif

/*............. Macros and typedefs */
#if !defined(mem_alloc)
  #define mem_alloc(size)   malloc(sizeof(char) * (size))
  #define mem_free(ptr)     free(ptr)
#endif

#if !defined(GETCH) || !defined(KBHIT)  /* [[[ */
  #define KBHIT()  _kbhit()
  #define GETCH()  _getch()
#endif  /* ]]] */

#define LCLR   4    /* learn drawing color  */
#define SCLR   8    /* search drawing color */

#define SCR_DX          640
#define SCR_DY          480
#define SCR_PITCH       SCR_DX
#define OR_ALLOC_ERROR  104

/*............. Local functions */
int or_demo(
    image  *dr_img,         /* [in] drawing image, NULL:don't draw      */
    char   *bmp_file,       /* [in] name of BMP file                    */
    int     nobj,           /* [in] # of searched objects               */
    int     res_size,       /* [in] max size of res_buf in elements     */
    OR_RES  res_buf[],      /* [out] result buffer                      */
    int    *nres );         /* [out] number of detected results         */
void img_set (
    image  *img,            /* [in] image                               */
    int     clr );          /* [in] pixel value (color) to set          */
void img_copy (
    image *img1,            /* input image                              */
    image *img2 );          /* output image                             */
void gen_test_img (
    image  *img,            /* [i/o] generated test image               */
    int     bgnd_clr,       /* [in] bgnd pixel value                    */
    int     obj_clr );      /* [in] object pixel value                  */
int  draw_box_marker (
    image  *dr_img,         /* input draw image (VC format)             */
    int     x,              /* input x-coordinate of box center         */
    int     y,              /* input y-coordinate of box center         */
    int     size,           /* input box size (dx and dy)               */
    int     clr );          /* input drawing color                      */


/****** main = Main demo function = 03.2008 *****************************/
int main(int argc, char *argv[])
{
    int   rc = 0;
    int   nobj = 1;         /* # of searched objects        */
    char*  bmp_file=0;

#define RSIZ  10            /* size of result buffer        */
    int     res_size = RSIZ;
    OR_RES  res_buf[RSIZ];  /* result buffer                */
    int     nres = 0;       /* number of detected results   */

    int key = 0;

//    int i;
//    unsigned long time;


/*----------------------------------------------------------------------*/
/*                                  Setup                               */
/*----------------------------------------------------------------------*/

/*............. Greet user */
    printf("---------------------------------------------------------------------\n");
    printf("Object recognition demo program %s/%s\n",vm_lib_vers_str(),__DATE__);
    printf("---------------------------------------------------------------------\n");
    printf("Usage:\n");
    printf("  or_demo [nobj [bmp_file]]\n");
    printf("where:\n");
    printf("  nobj     = max # of searched objects (default=1)\n");
    printf("  bmp_file = source BMP file (program generated image if missing)\n");
    printf("---------------------------------------------------------------------\n");

/*............. Get command line arguments */
    if(argc >= 2)   /* get # of searched objects */
    {
      nobj = atoi(argv[1]);
    }
    if(argc >= 3)   /* get BMP file name from command line */
    {
      //strcpy(bmp_file, argv[2]);
        bmp_file=argv[2];
    }

    if(nobj <= 0) nobj = 1;
    if(nobj > RSIZ) nobj = RSIZ;

    printf("# of objects = %d\n",nobj);
    if(bmp_file)
      printf("BMP file = %s\n",bmp_file);
    else
      printf("BMP file = program generated image\n");

/*............. Init VM_LIB */
#if VRM_COPY_PROTECTION
    VRmCreateVMLIBKey(&key);
#endif
    rc = vm_lib_open(key);
    if(rc)
    {
      printf("VM_LIB open error = %d\n",rc);
      goto done;
    }

/*----------------------------------------------------------------------*/
/*                        Execute OR demo function                      */
/*----------------------------------------------------------------------*/
    rc = or_demo(
            NULL,       /*  dr_img    = [in] drawing image, NULL:don't draw  */
            bmp_file,   /* *bmp_file  = [in] name of BMP file                */
            nobj,       /*  nobj      = [in] # of searched objects           */
            res_size,   /*  res_size  = [in] max size of res_buf in elements */
            res_buf,    /*  res_buf[] = [out] result buffer                  */
           &nres );     /* *nres      = [out] number of detected results     */
    if(rc) goto done;


/*............. Exit */
done:
    vm_lib_close();
    printf("RC=%d\n",rc);
    return 0;
}

/****** or_demo = OR demo function = 08.2010 ****************************/
/*----------------------------------------------------------------------*/
/**
* OR demo function.
*
* The function executes one object recognition pass in image, read
* from the input BMP file. If bmp_file is missing (blank string), the
* function uses progra-generated test image to search pattern(s).
*
* @param  dr_img    [in] OR drawing image, NULL:don't draw
* @param  bmp_file  [in] name of BMP file (NULL or "": use generate test image)
* @param  nobj      [in] number of searched objects (>= 1)
* @param  res_size  [in] max size of res_buf in elements
* @param  res_buf   [out] result buffer (max size = res_size elements)
* @param  nres      [out] number of detected results, stored in res_buf (<=res_size)
*
* @return
*       0     = Success
* <br>  104   = Memory allocation error
* <br>  Other = Error
*
*/
/*----------------------------------------------------------------------*/
int or_demo(
    image  *dr_img,         /* [in] drawing image, NULL:don't draw      */
    char   *bmp_file,       /* [in] name of BMP file                    */
    int     nobj,           /* [in] # of searched objects               */
    int     res_size,       /* [in] max size of res_buf in elements     */
    OR_RES  res_buf[],      /* [out] result buffer                      */
    int    *nres )          /* [out] number of detected results         */
{

/* DDD */
#define DUMP_OR_RESULTS  1

    int     rc = 0;
    OR_HND  or_hnd;         /* OR library handle            */
    char   *edg_buf = NULL; /* edge-mask image buffer       */
    char   *pat_buf = NULL; /* pattern image buffer         */
    char   *srh_buf = NULL; /* search image buffer          */
    image   bmp_img = { 0,0,0,0 };

    int     pat_x,pat_y;    /* pattern coordinates          */
    int     pat_dx,pat_dy;  /* pattern dimensionss          */
    int     srh_x,srh_y;    /* search area coordinates      */
    int     srh_dx,srh_dy;  /* search area dimensionss      */

//    image   dr_img;         /* overlay (drawing) image      */
    image   pat_img;        /* pattern image                */
    image   srh_img;        /* search image                 */
    image   edg_img;        /* edge-mask image              */

    LEARN_PAR  lpar;        /* learn parameter structure    */
    SEARCH_PAR spar;        /* search parameter structure   */

    int i,j;
    int img_buf_size;
    int scr_dx;
    int scr_dy;
    int scr_pitch;
    int bgnd_clr = 200;     /* bgnd pixels in generated test image      */
    int obj_clr = 30;       /* fgnd pixels in generated test image      */

    unsigned long time;


/*----------------------------------------------------------------------*/
/*                                  Setup                               */
/*----------------------------------------------------------------------*/

/*............. Init OR handle */
    or_init(&or_hnd);

/*............. Read BMP file into image */
/* Supported BMP formats: 16/256 color BMP file! */
    if(bmp_file && strlen(bmp_file))
    {
      rc = read_bmp_file(bmp_file, &bmp_img);
      if(rc)
      {
        printf("read_bmp_file error = %d\n",rc);
        goto done;
      }
      else
      {
        printf("BMP image size: (%d x %d)\n",bmp_img.dx, bmp_img.dy);
      }
    }
/*............. Generate test image in 'bmp_img' */
    else
    {
      bmp_img.st = (long)mem_alloc(SCR_PITCH * SCR_DY);
      if(!bmp_img.st)
      {
        printf("Memory allocation error\n");
        rc = OR_ALLOC_ERROR;
        goto done;
      }
      bmp_img.dx = SCR_DX;
      bmp_img.dy = SCR_DY;
      bmp_img.pitch = SCR_PITCH;

      gen_test_img(&bmp_img, bgnd_clr, obj_clr);
    }


    scr_dx = bmp_img.dx;
    scr_dy = bmp_img.dy;
    scr_pitch = scr_dx;

    pat_x = 125;
    pat_y = 125;
    pat_dx = 75;
    pat_dy = 75;
//    pat_dx = 275;
//    pat_dy = 275;

/*----------------------------------------------------------------------*/
/*                        Allocate image buffers                        */
/*----------------------------------------------------------------------*/
    img_buf_size = scr_dy * scr_pitch;
    rc = OR_ALLOC_ERROR;

/*............. Allocate edge-mask image buffer */
    edg_buf = (char *)mem_alloc(img_buf_size);
    if(!edg_buf) goto done;

    pat_buf = (char *)mem_alloc(img_buf_size);
    if(!pat_buf) goto done;

    srh_buf = (char *)mem_alloc(img_buf_size);
    if(!srh_buf) goto done;

    rc = 0;


/*............. Setup images */
    pat_img.st    = (long)pat_buf;
    pat_img.dx    = scr_dx;
    pat_img.dy    = scr_dy;
    pat_img.pitch = scr_pitch;

    srh_img.st    = (long)srh_buf;
    srh_img.dx    = scr_dx;
    srh_img.dy    = scr_dy;
    srh_img.pitch = scr_pitch;

    edg_img.st    = (long)edg_buf;
    edg_img.dx    = pat_dx;
    edg_img.dy    = pat_dy;
    edg_img.pitch = scr_pitch;

/*............. Learn control parameters */
    lpar.min_edge_h = 50;       /* min edge height: 7,..,15,..,127              */
    lpar.min_edge_g = 12;       /* min edge gradient: 6,..,15,..,min_edge_h     */
    lpar.min_chain = 8;         /* min edge contour length: 4,..,24 pixels      */
    lpar.save_edge_image = 0;   /* save edge image (show pattern contours)      */
    lpar.lrn_mode = 0;          /* learn mode:                                  */
                                /*  0: normal, no edge masking, edg_img ignored */
                                /*  1: create edge-mask image in edg_img        */
                                /*  2: use edg_img to mask unwanted learn       */
                                /*     pattern edges                            */
    lpar.dr_clr = LCLR;         /* drawing color [4,255]                        */


/*............. Search control parameters */
    spar.min_edge_h = 50;       /* min edge height: 7,..,15,..,127              */
    spar.min_edge_g = 12;       /* min edge gradient, =6,..,15,..,min_edge_h    */
    spar.min_chain = 8;         /* min edge contour length, =4,..,24 pixels     */
    spar.restr_search = 0;      /* 1 = restricted_searching_mode                */
    spar.min_rate = 552;        /* min matching rate, =160,..,1023, 1024==100%  */
    spar.contour_width = 5;     /* min edge contour width, =3,5,7,9 pixels      */
    spar.min_scale = 1024;      /* min scale, 512-1024(=100%), keep <=max_scale!*/
    spar.max_scale = 1024;      /* max scale, 1024(=100%)-1536, keep <200% !    */
    spar.rot_sect_start = -180; /* rotation sector start, -180..179 deg         */
    spar.rot_sect_width = 360;  /* rotation sector width, 0..360 deg            */
    spar.save_edge_image = 0;   /* save edge image (show search contours)       */
    spar.dr_clr = SCLR;         /* drawing color [4,255]                        */

    spar.max_pm_items = nobj;   /* # of searched objects */

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
//    img_copy(&img, &srh_img);


/*----------------------------------------------------------------------*/
/*                             Learn stage                              */
/*----------------------------------------------------------------------*/

/* Draw pattern rectangle */
    rc = or_draw_box(dr_img, pat_x, pat_y, pat_dx, pat_dy, LCLR);
    if(rc) goto done;

/* Learn pattern */
    rc = or_learn(&or_hnd, &pat_img, dr_img, &edg_img,
                  pat_x, pat_y, pat_dx, pat_dy,
                  &lpar);
    if(rc)
    {
      printf("or_demo: or_learn error = %d\n",rc);
      goto done;
    }
//    printf("or_learn OK, press a key...\n");
//    getchar();


/*----------------------------------------------------------------------*/
/*                              Search stage                            */
/*----------------------------------------------------------------------*/

/* Draw search rectangle */
    rc = or_draw_box(dr_img, srh_x, srh_y, srh_dx, srh_dy, SCLR);
    if(rc) goto done;

/* Search pattern */
    if(spar.max_pm_items > res_size)    /* truncate              */
      spar.max_pm_items = res_size;

    time = sys_time();
    rc = or_search(&or_hnd, &srh_img, dr_img,
                   srh_x, srh_y, srh_dx, srh_dy,
                   &spar, res_buf, nres);
    time = sys_time() - time;       /* get search execution time */
    if(rc)
    {
      printf("or_demo: or_search error = %d\n",rc);
      goto done;
    }

/* Dump results */
#if DUMP_OR_RESULTS
    printf("--- %d object(s) found, time=%lu ms ---\n", *nres, time);

    for(i=0; i<*nres; i++)
    {
      printf("%02d: x,y=%d,%d rate=%d rot=%d scale=%d\n    corners=(%d,%d) (%d,%d) (%d,%d) (%d,%d)\n",
              i,res_buf[i].x, res_buf[i].y, res_buf[i].rate,
                res_buf[i].rot, res_buf[i].scale,
                res_buf[i].corners[0], res_buf[i].corners[1],
                res_buf[i].corners[2], res_buf[i].corners[3],
                res_buf[i].corners[4], res_buf[i].corners[5],
                res_buf[i].corners[6], res_buf[i].corners[7] );

      /* Draw marker on object's center */
      rc = or_draw_marker(dr_img, res_buf[i].x, res_buf[i].y, 5, SCLR);
      if(rc) goto done;

      /* Draw corners of object's rotated rectangle */
      for(j=0; j<8; j+=2)
      {
        rc = draw_box_marker(dr_img, res_buf[i].corners[j],
                                      res_buf[i].corners[j+1], 9, SCLR);
        if(rc) goto done;
      }
    }
#endif

/*............. Exit */
done:
    if(rc == OR_ALLOC_ERROR)
      printf("Memory allocation error\n");

    or_exit(&or_hnd);
    if(bmp_img.st) mem_free((void *)(int)bmp_img.st);   // free image buffer
    if(edg_buf != NULL) mem_free(edg_buf);
    if(pat_buf != NULL) mem_free(pat_buf);
    if(srh_buf != NULL) mem_free(srh_buf);

    return rc;
}

/****** img_set = Set image pixels to constant = 08.2006 ****************/
/*----------------------------------------------------------------------*/
/*
* The function sets all 'img' pixels to 'clr'.
*
* Parameters:
*   img = [in] image
*   clr = [in] pixel value (color) to set
*/
/*----------------------------------------------------------------------*/
void img_set (
    image  *img,            /* [in] image                               */
    int     clr )           /* [in] pixel value (color) to set          */
{
    char *p  = (char *)(int)img->st;
    int   dx = img->dx;
    int   dy = img->dy;

    if(!p || dx <= 0 || dy <= 0) return;

    while(dy--)
    {
      memset(p, clr, dx);
      p += img->pitch;
    }
    return;
}

/****** img_copy - Image copy = 01.2008 *********************************/
/*----------------------------------------------------------------------*/
/*
* The function copies 'img1' to the top/left 'img2' area, eventually
* truncated:
*  - If img1 is greater than img2, then the function truncates and copies
*    img1 to the top left img2 corner.
*  - If img1 is smaller than img2, then the function copies img1 to the
*    top left img2 corner and leaves unchanged the remaining img2 pixels.
*
* @param  img1     Input image
* @param  img2     Output image
*
* @return
*       None
*/
/*----------------------------------------------------------------------*/
void img_copy (
    image *img1,        /* input image                                  */
    image *img2 )       /* output image                                 */
{
    int   dx = (img1->dx < img2->dx) ? img1->dx : img2->dx;
    int   dy = (img1->dy < img2->dy) ? img1->dy : img2->dy;
    char *p1 = (char *)(int)img1->st;
    char *p2 = (char *)(int)img2->st;

    if(!p1 || !p2 || dx <= 0 || dy <= 0) return;

    while(dy--)
    {
      memcpy(p2,p1,dx);
      p1 += img1->pitch;
      p2 += img2->pitch;
    }
    return;
}

/****** fill_box = Fill image box with color = 01.2007 ******************/
/*----------------------------------------------------------------------*/
/**
* Fill image box with color.
*
* The function fills a box in the input/output image "img" with
* specified color "clr". The box is clipped inside the image.
*
* @param  img     Input/output image
* @param  x       Input x-coord. of top/left box corner
* @param  y       Input y-coord. of top/left box corner
* @param  dx      Input box width in pixels
* @param  dy      Input box height in pixels
* @param  clr     Input pixel color (value to fill)
*
* @return
*       None
*
*/
/*----------------------------------------------------------------------*/
void fill_box (
    image    *img,          /* input/output image                       */
    int       x,            /* input x-coord. of top/left box corner    */
    int       y,            /* input y-coord. of top/left box corner    */
    int       dx,           /* input box width in pixels                */
    int       dy,           /* input box height in pixels               */
    int       clr )         /* input pixel color                        */
{

    int   x1 = x;
    int   y1 = y;
    int   x2 = x + dx - 1;
    int   y2 = y + dy - 1;
    char *ptr;

/*............. Box outside image? */
    if(x1 >= img->dx || x2 < 0 || y1 >= img->dy || y2 < 0)
       return;

/*............. Clip the box inside the image */
    if(x1 < 0) x1 = 0;
    if(y1 < 0) y1 = 0;
    if(x2 >= img->dx) x2 = img->dx - 1;
    if(y2 >= img->dy) y2 = img->dy - 1;
    dx = x2 - x1 + 1;
    dy = y2 - y1 + 1;

/*............. Fill the box */
    ptr = (char *)(int)img->st + y1*img->pitch + x1;
    while(dy--)
    {
      memset(ptr,clr,dx);
      ptr += img->pitch;
    }

    return;
}

/****** gen_test_img = Generate test image = 08.2010 ********************/
/*----------------------------------------------------------------------*/
/*
* Generate test image.
*
* The function generates test image with 2 colors - bgnd_clr and obj_clr:
* 1. The function fills the ouput image with color bgnd_clr.
* 2. The function fills 2 rectangular image objects with color obj_clr
*    at coordinates x,y= (140,140) and (300,300) and dimensions
*    dx,dy = 50,40.
*
* @param  img       [i/o] generated test image (min dx,dy=640,480)
* @param  bgnd_clr  [in] bgnd pixel value
* @param  obj_clr   [in] object pixel value
*
* @return
*       None
*
*/
/*----------------------------------------------------------------------*/
void gen_test_img (
    image  *img,            /* [i/o] generated test image               */
    int     bgnd_clr,       /* [in] bgnd pixel value                    */
    int     obj_clr )       /* [in] object pixel value                  */
{
//    char *p  = (char *)(int)img->st;
    int   dx = img->dx;
    int   dy = img->dy;

#define OBJ_DX 40
#define OBJ_DY 40

    if(dx < 400 || dy < 400) return;

    img_set(img, bgnd_clr);

    fill_box(img, 140, 140, OBJ_DX, OBJ_DY, obj_clr);
    fill_box(img, 300, 300, OBJ_DX, OBJ_DY, obj_clr);

    return;
}

/****** draw_box_marker = Draw box marker = 08.2006 *********************/
/*----------------------------------------------------------------------*/
/**
* Draw box marker.
*
* The function draws a rectangle box marker in "dr_img" with specified color.
*
* @param  dr_img  Input drawing image in VC format (NULL: no draw)
* @param  x       Input x-coord. of center box point
* @param  y       Input y-coord. of center box point
* @param  size    Input box size in pixels (width and height)
* @param  clr     Input drawing color
*
* @return
*       0     = OK
* <br>  Other = returned by called functions
*
*/
/*----------------------------------------------------------------------*/
int  draw_box_marker (
    image  *dr_img,         /* input draw image (VC format)             */
    int     x,              /* input x-coordinate of box center         */
    int     y,              /* input y-coordinate of box center         */
    int     size,           /* input box size (dx and dy)               */
    int     clr )           /* input drawing color                      */
{
    return or_draw_box(dr_img, x-size/2, y-size/2, size, size, clr);
}
