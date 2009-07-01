/*
  Module       : utils.c
  Purpose      : Various utilities for qiv
  More         : see qiv README
  Policy       : GNU GPL
  Homepage     : http://www.klografx.net/qiv/
*/	

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <gdk/gdkx.h>
#include <dirent.h>
#include "qiv.h"

/* copy current image to .qiv-trash */
int move2trash(char *filename)
{
  char *ptr,*ptr2;
  char trashfile[FILENAME_LEN];
  int i=0;

  ptr2 = malloc(sizeof(char) * FILENAME_LEN);

  if(!realpath(filename,ptr2)) {
    g_print("Error: Could not move file to trash\n");
    return 1;
  }

  snprintf(trashfile, sizeof trashfile, "%s%s", TRASH_DIR, ptr2);

  ptr = ptr2 = trashfile;
  while((ptr = strchr(ptr,'/'))) {
    *ptr = '\0';
    if(access(ptr2,F_OK)) {
      if(mkdir(ptr2,0700)) {
	g_print("Error: Could not make directory %s\n",ptr2);
	return 1;
      }
    }
    *ptr = '/';
    ptr += 1;
  }

  if(link(filename,trashfile)) {
    g_print("Error: Could not copy file to %s\n",trashfile);
    return 1;
  }

  if(!unlink(filename)) {
    --images;
    for(i=image_idx;i<images;++i) {
      image_names[i] = image_names[i+1];
    }

    /* If deleting the last file out of x */
    if(images == image_idx)
      --image_idx;
    
    /* If deleting the only file left */    
    if(!images)
      gdk_exit(0);
  }
  else {
    g_print("Error: Could not write to %s\n", TRASH_DIR);
    return 1;
  }
  return 0;
}

/* run a command ... */
void run_command(qiv_image *q, int n, char *filename)
{
    static char command[BUF_LEN];
    snprintf(infotext, sizeof infotext, "Running: 'qiv-command %i %s'", n, filename);
    update_image(q);
    snprintf(command, sizeof command, "qiv-command %i %s", n, filename);
    system(command);
}

  
/* 
   This routine jumps x images forward or backward or
   directly to image x
   Enter jf10\n ... jumps 10 images forward
   Enter jb5\n  ... jumps 5 images backward
   Enter jt15\n ... jumps to image 15
*/
void jump2image(char *cmd)
{
  int direction = 0;
  int x;

#ifdef DEBUG
    g_print("*** starting jump2image function: %s\n", cmd);
#endif

  if(cmd[0] == 'f' || cmd[0] == 'F')
    direction = 1;
  else if(cmd[0] == 'b' || cmd[0] == 'B')
    direction = -1;
  else if(!(cmd[0] == 't' || cmd[0] == 'T'))
    return;

  /* get number of images to jump or image to jump to */
  x = atoi(cmd+1);

  if (direction == 1) {
    if ((image_idx + x) > (images-1))
      image_idx = images-1;
    else
      image_idx += x;
  }
  else if (direction == -1) {
    if ((image_idx - x) < 0)
      image_idx = 0;
    else
      image_idx -= x;
  }
  else {
    if (x > images || x < 1)
      return;
    else
      image_idx = x-1;
  }

#ifdef DEBUG
    g_print("*** end of jump2image function\n");
#endif

}

void finish(int sig)
{
  gdk_pointer_ungrab(CurrentTime);
  gdk_keyboard_ungrab(CurrentTime);
  gdk_exit(0);
} 

/*
  Update selected image index image_idx
  Direction determines if the next or the previous
  image is selected.
*/
void next_image(int direction)
{
  static int last_modif = 1;	/* Delta of last change of index of image */
  if (direction) {
    if (random_order)
      image_idx = get_random(random_replace, images, direction);
    else 
      image_idx += direction;
    
    last_modif = direction;
  } 
  else {
    /* Default action */
    if (random_order) {
      /* Select a random image */
      image_idx = get_random(random_replace, images, last_modif);
    } else
      /* Repeat our last direction change */
      image_idx += last_modif;
  }
  /* Correct out-of-bound indices */
  image_idx = (image_idx + images) % images;
}

void usage(char *name, int exit_status)
{
    g_print("qiv (Quick Image Viewer) v%s\n"
	"Usage: %s [options] files ...\n"
	"See 'man qiv' or type '%s --help' for options.\n",
        VERSION, name, name);

    gdk_exit(exit_status);
}

void show_help(char *name, int exit_status)
{
    int i;

    g_print("qiv (Quick Image Viewer) v%s\n"
	"Usage: %s [options] files ...\n\n",
        VERSION, name);

    g_print(
          "General options:\n"
          "    --help, -h           This help screen\n"
          "    --display x          Open qiv window on display x\n"
          "    --center, -e         Disable window centering\n"
          "    --root, -x           Set centered desktop background and exit\n"
          "    --root_t, -y         Set tiled desktop background and exit\n"
          "    --root_s, -z         Set stretched desktop background and exit\n"
          "    --maxpect, -m        Zoom to screen size and preserve aspect ratio\n"
          "    --scale_down, -t     Shrink image(s) larger than the screen to fit\n"
          "    --fullscreen, -f     Use fullscreen window on start-up\n"
          "    --brightness, -b x   Set brightness to x (-32..32)\n"
          "    --contrast, -c x     Set contrast to x (-32..32)\n"
          "    --gamma, -g x        Set gamma to x (-32..32)\n"
          "    --no_filter, -n      Do not filter images by extension\n"
          "    --no_statusbar, -i   Disable statusbar in fullscreen_mode\n"
          "    --transparency, -p   Enable transparency for transparent images\n"
          "    --do_grab, -a        Grab the pointer in windowed mode\n"
          "    --version, -v        Print version information and exit\n"
          "    --bg_color, -o x     Set root background color to x\n"
          "    --recursive, -u x    Recursively retrieve all files from directory x\n"
          "\n"
          "Slideshow options:\n"
          "    --slide, -s          Start slideshow immediately\n"
          "    --random, -r         Random order\n"
          "    --shuffle, -S        Shuffled order\n"
          "    --delay, -d x        Wait x seconds between images [default=%d]\n"
          "\n"
          "Keys:\n", SLIDE_DELAY/1000);

    /* skip header and blank line */
    for (i=0; helpkeys[i]; i++)
        g_print("    %s\n", helpkeys[i]);

    g_print("\nValid image extensions:");

    for (i=0; image_extensions[i]; i++)
	g_print("%s%s", (i%8) ? " " : "\n    ", image_extensions[i]);
    g_print("\n\n");
    
    g_print("Homepage: http://www.klografx.net/qiv/\n"
	    "Please mail bug reports and comments to Adam Kopacz <Adam.K@klografx.de>\n");

    gdk_exit(exit_status);
}

/* returns a random number from the integers 0..num-1, either with
   replacement (replace=1) or without replacement (replace=0) */

int get_random(int replace, int num, int direction)
{
  static int index = -1;

  
  static int *rindices = NULL;  /* the array of random intgers */

  int n,m,p,q;

  if (!rindices)
    rindices = (int *) malloc((unsigned) num*sizeof(int));

  if (index < 0)         /* no more indices left in this cycle. Build a new */
    {		         /* array of random numbers, by not sorting on random keys */
      index = num-1;

      for (m=0;m<num;m++)
	{
	  rindices[m] = m; /* Make an array of growing numbers */
	}

      for (n=0;n<num;n++)   /* simple insertion sort, fine for num small */
	{
	  p=(int)(((float)rand()/RAND_MAX) * (num-n)) + n ; /* n <= p < num */
	  q=rindices[n];
	  rindices[n]=rindices[p]; /* Switch the two numbers to make random order */
	  rindices[p]=q;
	}
    }

  if (shuffle) {
    index = index - direction;
    index = (index + num) % num;
  }

  return rindices[shuffle?index:index--];
}

/* Recursively gets all files from a directory */

int rreaddir(const char *dirname, char **files)
{
    DIR *d,*tmp;
    struct dirent *entry;
    char cdirname[FILENAME_LEN], name[FILENAME_LEN];
    int i=0;

    strncpy(cdirname, dirname, sizeof cdirname);
    cdirname[FILENAME_LEN-1] = '\0';

    d = opendir(cdirname);
    if(d) {
        while((entry = readdir(d))) {
            if(!strcmp(entry->d_name,".") ||
                !strcmp(entry->d_name,"..")) {
                continue;
            }
            snprintf(name, sizeof name, "%s/%s", cdirname, entry->d_name);
            files[i++] = strdup(name);
            if((tmp = opendir(name))) {
                closedir(tmp);
                --i;
                i += rreaddir(name,&files[i]);
            }
        }
        closedir(d);
    }
    return i;
}

gboolean color_alloc(const char *name, GdkColor *color)
{
    gboolean result;

    result = gdk_color_parse(name, color);

    if (!result) {
        fprintf(stderr, "qiv: can't parse color '%s'\n", name);
        name = "black";
    }

    result = gdk_colormap_alloc_color(cmap, color, FALSE, TRUE);

    if (!result) {
        fprintf(stderr, "qiv: can't alloc color '%s'\n", name);
        color->pixel = 0;
    }

    return result;
}

void swap(int *a, int *b)
{
    int temp;
    temp = *a;
    *a = *b;
    *b = temp;
}
