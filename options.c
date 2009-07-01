/*
  Module       : options.c
  Purpose      : Read and evaluate commandline options
  More         : see qiv README
  Homepage     : http://www.klografx.net/qiv/
*/	

#include "qiv.h"
#include <string.h>
#ifdef HAVE_GETOPT_LONG
#include <getopt.h>
#else
#include "lib/getopt.h"
#endif

extern char *optarg;
extern int optind, opterr, optopt, nfiles, read_directory;
extern int rreaddir(const char *, char **);
extern char **files;

static char *short_options = "hexyzmtb:c:g:nipavo:srSd:u:f";
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
    {"recursive",   1, NULL, 'u'},
    {"fullscreen",	0, NULL, 'f'},
    {0,			0, NULL, 0}
};

static int my_strcmp(const void *v1, const void *v2)
{
    return strcmp(*(char **)v1, *(char **)v2);
}

void options_read(int argc, char **argv, qiv_image *q, int *index_return)
{
    int long_index;
    int c;

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
	    case 'u': read_directory=1;
                      if(!(nfiles = rreaddir(optarg,files))) {
                          g_print("Error: %s is not a directory.\n",optarg);
                          gdk_exit(1);
                      }
                      qsort(files, nfiles, sizeof *files, my_strcmp);
		      break;
	    case 'f': fullscreen=1;
		      break;
	    case 0:
	    case '?': usage(argv[0], 1);
		      gdk_exit(0);
	}
    }

    *index_return = optind;
}
