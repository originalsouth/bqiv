/*
  Module       : options.c
  Purpose      : Read and evaluate commandline options
  More         : see qiv README
  Homepage     : http://www.klografx.net/qiv/
*/	

#include "qiv.h"
#include <string.h>
#include <ctype.h>
#ifdef HAVE_GETOPT_LONG
#include <getopt.h>
#else
#include "lib/getopt.h"
#endif

extern char *optarg;
extern int optind, opterr, optopt;
extern int rreaddir(const char *);

static char *short_options = "hexyzmtb:c:g:nipavo:srSd:u:fw:IMN";
static struct option long_options[] =
{
    {"help",		0, NULL, 'h'},
    {"center",		0, NULL, 'e'},
    {"root",		0, NULL, 'x'},
    {"root_t",		0, NULL, 'y'},
    {"root_s",		0, NULL, 'z'},
    {"maxpect",		0, NULL, 'm'},
    {"scale_down",	0, NULL, 't'},
    {"brightness",	1, NULL, 'b'},
    {"contrast",	1, NULL, 'c'},
    {"gamma",		1, NULL, 'g'},
    {"no_filter",	0, NULL, 'n'},
    {"no_statusbar",	0, NULL, 'i'},
    {"transparency",	0, NULL, 'p'},
    {"do_grab",		0, NULL, 'a'},
    {"version",		0, NULL, 'v'},
    {"bg_color",	1, NULL, 'o'},
    {"slide",		0, NULL, 's'},
    {"random",		0, NULL, 'r'},
    {"shuffle",         0, NULL, 'S'},
    {"delay",		1, NULL, 'd'},
    {"recursive",   	1, NULL, 'u'},
    {"fullscreen",	0, NULL, 'f'},
    {"width",		1, NULL, 'w'},
    {"ignore_path_sort",0, NULL, 'I'},
    {"merged_case_sort",0, NULL, 'M'},
    {"numeric_sort",	0, NULL, 'N'},
    {0,			0, NULL, 0}
};

static int numeric_sort = 0, merged_case_sort = 0, ignore_path_sort = 0;

/* This array makes it easy to sort filenames into merged-case order
 * (e.g. AaBbCcDdEeFf...). */
static unsigned char casemap[256] = {
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
    0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
    0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
    0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
    0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
    0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,
    0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
    0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
    0x40,0x41,0x43,0x45,0x47,0x49,0x4B,0x4D, /* @ABCDEFG */
    0x4F,0x51,0x53,0x55,0x57,0x59,0x5B,0x5D, /* HIJKLMNO */
    0x5F,0x61,0x63,0x65,0x67,0x69,0x6B,0x6D, /* PQRSTUVW */
    0x6F,0x71,0x73,0x75,0x76,0x77,0x78,0x79, /* XYZ[\]^_ */
    0x7A,0x42,0x44,0x46,0x48,0x4A,0x4C,0x4E, /* `abcdefg */
    0x50,0x52,0x54,0x56,0x58,0x5A,0x5C,0x5E, /* hijklmno */
    0x60,0x62,0x64,0x66,0x68,0x6A,0x6C,0x6E, /* pqrstuvw */
    0x70,0x72,0x74,0x7B,0x7C,0x7D,0x7E,0x7F, /* xyz{|}~  */
    0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,
    0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,
    0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,
    0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,
    0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,
    0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,
    0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,
    0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,
    0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,
    0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,
    0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,
    0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,
    0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,
    0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,
    0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,
    0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF
};

static int my_strcmp(const void *v1, const void *v2)
{
    unsigned char *cp1 = *(unsigned char **)v1;
    unsigned char *cp2 = *(unsigned char **)v2;
    unsigned char *sufptr1, *sufptr2;
    int tmp;

    sufptr1 = cp1 + strlen(cp1);
    while (--sufptr1 > cp1 && *sufptr1 != '.') {}
    sufptr2 = cp2 + strlen(cp2);
    while (--sufptr2 > cp2 && *sufptr2 != '.') {}

    if (ignore_path_sort) {
	unsigned char *slash;
	if ((slash = strrchr(cp1, '/')) != NULL)
	    cp1 = slash + 1;
	if ((slash = strrchr(cp2, '/')) != NULL)
	    cp2 = slash + 1;
    }
    if (numeric_sort) {
	int namelen = 0, diff = 0;

	do {
	    if (sufptr1 && (isdigit(*cp1) || isdigit(*cp2))) {
		unsigned char *ep1, *ep2;

		if (diff)
		    return diff;
		for (ep1 = cp1; isdigit(*ep1); ep1++) {}
		if (ep1 == cp1)
		    return 1;
		for (ep2 = cp2; isdigit(*ep2); ep2++) {}
		if (cp2 == ep2)
		    return -1;
		if ((diff = (ep1 - cp1) - (ep2 - cp2)) == 0) {
		    long val = atol(cp1) - atol(cp2);
		    diff = val < 0? -1 : val > 0? 1 : 0;
		}
		if (diff
		 && sufptr1 - cp1 <= namelen && sufptr2 - cp2 <= namelen)
		    return diff;
		namelen += ep1 - cp1;
		cp1 = ep1;
		cp2 = ep2 - 1;
	    }
	    else {
		if (cp1 == sufptr1) {
		    if (cp2 != sufptr2)
			return -1;
		    sufptr1 = sufptr2 = NULL;
		}
		else if (cp2 == sufptr2)
		    return 1;
		if (merged_case_sort)
		    tmp = casemap[*cp1++] - casemap[*cp2];
		else
		    tmp = *cp1++ - *cp2;
		if (tmp != 0)
		    return tmp;
		if (*cp2 == '/')
		    namelen = 0;
		else
		    namelen++;
	    }
	} while (*cp2++ != '\0');
	return diff;
    }

    do {
	if (cp1 == sufptr1) {
	    if (cp2 != sufptr2)
		return -1;
	}
	else if (cp2 == sufptr2)
	    return 1;
	if (merged_case_sort)
	    tmp = casemap[*cp1++] - casemap[*cp2];
	else
	    tmp = *cp1++ - *cp2;
	if (tmp != 0)
	    return tmp;
    } while (*cp2++ != '\0');

    return 0;
}

void options_read(int argc, char **argv, qiv_image *q)
{
    int long_index, shuffle = 0, need_sort = 0;
    int c, cnt;

    while ((c = getopt_long(argc, argv, short_options,
	    long_options, &long_index)) != -1) {
	switch(c) {
	    case 'h': show_help(argv[0], 0); break;
	    case 'e': center=0;
		      break;
	    case 'x': to_root=1;
		      break;
	    case 'y': to_root_t=1;
		      break;
	    case 'z': to_root_s=1;
		      break;
	    case 't': scale_down=1;
		      break;
	    case 'm': maxpect=1;
		      break;
	    case 'b': q->mod.brightness = (atoi(optarg)+32)*8;
		      if ((q->mod.brightness<0) || (q->mod.brightness>512))
			  usage(argv[0],1);
		      break;
	    case 'c': q->mod.contrast = (atoi(optarg)+32)*8;
		      if ((q->mod.contrast<0) || (q->mod.contrast>512))
			  usage(argv[0],1);
		      break;
	    case 'g': q->mod.gamma = (atoi(optarg)+32)*8;
		      if ((q->mod.gamma<0) || (q->mod.gamma>512))
			  usage(argv[0],1);
		      break;
	    case 'n': filter=0;
		      break;
	    case 'i': statusbar=0;
		      break;
	    case 'p': transparency=1;
		      break;
	    case 'a': do_grab=1;
		      break;
	    case 'v': g_print("qiv (Quick Image Viewer) v%s\n", VERSION);
		      gdk_exit(0);
		      break;
	    case 'o': image_bg_spec = optarg;
		      break;
	    case 's': slide=1;
		      break;
	    case 'r': random_order=1;
		      break;
	    case 'S': random_order=1;
	              shuffle=1;
		      break;
	    case 'd': delay=atoi(optarg)*1000+1;
		      if (delay<1) usage(argv[0],1);
		      break;
	    case 'u': if(rreaddir(optarg) < 0) {
                          g_print("Error: %s is not a directory.\n",optarg);
                          gdk_exit(1);
                      }
                      need_sort=1;
		      break;
	    case 'f': fullscreen=1;
		      break;
	    case 'w': q->win_w = width_fix_size = atoi(optarg);
		      break;
	    case 'I': need_sort = ignore_path_sort = 1;
		      break;
	    case 'M': need_sort = merged_case_sort = 1;
		      break;
	    case 'N': need_sort = numeric_sort = 1;
		      break;
	    case 0:
	    case '?': usage(argv[0], 1);
		      gdk_exit(0);
	}
    }

    if((cnt = argc - optind) > 0) {
	if(images) {
	    if(cnt > max_image_cnt - images) {
		max_image_cnt = images + cnt;
		image_names = (char**)realloc(image_names,max_image_cnt*sizeof(char*));
	    }
	    while (cnt-- > 0)
		image_names[images++] = argv[optind++];
	}
	else {
	    image_names = &argv[optind];
	    images = cnt;
	}
    }

    if(shuffle) {
	char *fn;
	int i, p;
	for (i=0;i<images;i++) { /* simple insertion sort, fine for small num */
	  p = (int)(((float)rand()/RAND_MAX) * (images-i)) + i;
	  fn = image_names[i];
	  image_names[i] = image_names[p];
	  image_names[p] = fn;
	}
    }
    else if(need_sort)
	qsort(image_names, images, sizeof *image_names, my_strcmp);
}
