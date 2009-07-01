/*
  Module       : main.c
  Purpose      : GDK/Imlib Quick Image Viewer (qiv)
  More         : see qiv README
  Homepage     : http://www.klografx.net/qiv/
  Policy       : GNU GPL
*/	

#include <gdk/gdkx.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkmain.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <ctype.h>
#include <string.h>
#include "qiv.h"
#include "main.h"

qiv_image main_img;

static void filter_images(int *, char **);
static int check_extension(const char *);
static void qiv_signal_usr1();
static void qiv_signal_usr2();
static gboolean qiv_handle_timer(gpointer);
static void qiv_timer_restart(gpointer);

#ifdef GTD_XINERAMA
static XineramaScreenInfo * get_preferred_xinerama_screen(void);
#endif

int main(int argc, char **argv)
{
  struct timeval tv;

  //er, we don't set a value for params.dither here?
  //i would probably just let the user's .imrc set it
  //GdkImlibInitParams params;
  //params.flags = PARAMS_DITHER;

  /* Initialize GDK and Imlib */

  gdk_init(&argc,&argv);
  gdk_imlib_init();

  //probably should only use one of the init funcs...
  //gdk_imlib_init_params(&params);

  /* Load things from GDK/Imlib */

  qiv_main_loop = g_main_new(TRUE);
  cmap = gdk_colormap_get_system();
  text_font = gdk_font_load(STATUSBAR_FONT);
  screen_x = gdk_screen_width();
  screen_y = gdk_screen_height();

#ifdef GTD_XINERAMA
  preferred_screen = get_preferred_xinerama_screen();
#endif

  gtk_widget_push_visual(gdk_imlib_get_visual());
  gtk_widget_push_colormap(gdk_imlib_get_colormap());

  /* Randomize seed for 'true' random */

  gettimeofday(&tv,NULL);
  srand(tv.tv_usec*1000000+tv.tv_sec);

  /* Set up our options, image list, etc */

  strncpy(select_dir, SELECT_DIR, sizeof select_dir);
  reset_mod(&main_img);
  options_read(argc, argv, &main_img);
  max_rand_num = images;

  if (filter) /* Filter graphic images */
    filter_images(&images,image_names);
  
  if (!images) { /* No images to display */
    g_print("qiv: cannot load any images.\n");
    usage(argv[0],1);
  }

  /* get colors */

  color_alloc(STATUSBAR_BG, &text_bg);
  color_alloc(ERROR_BG, &error_bg);
  color_alloc(image_bg_spec, &image_bg);

  /* Display first image first, except in random mode */

  image_idx = 0;
  if (random_order)
    next_image(0);

  //disabled because 'params' is never used, see above
  //if (to_root || to_root_t || to_root_s) {
  //  params.flags |= PARAMS_VISUALID;
  //  (GdkVisual*)params.visualid = gdk_window_get_visual(GDK_ROOT_PARENT());
  //}

  /* Setup callbacks */

  gdk_event_handler_set(qiv_handle_event, &main_img, NULL);
  qiv_timer_restart(NULL);

  /* And signal catchers */

  signal(SIGTERM, finish);
  signal(SIGINT, finish);
  signal(SIGUSR1, qiv_signal_usr1);
  signal(SIGUSR2, qiv_signal_usr2);

  /* Load & display the first image */

  qiv_load_image(&main_img);
  
  if(watch_file){
  	gtk_idle_add (qiv_watch_file, &main_img);
  }

  g_main_run(qiv_main_loop); /* will never return */
  return 0;
}


void qiv_exit(int code)
{
  if (text_font) gdk_font_unref(text_font);
  if (cmap) gdk_colormap_unref(cmap);
  destroy_image(&main_img);

  g_main_destroy(qiv_main_loop);
  finish(SIGTERM);		/* deprecated, subject to change */
}


/*
 * functions for handling signals
 */

static void qiv_signal_usr1()
{
  next_image(1);
  qiv_load_image(&main_img);
}

static void qiv_signal_usr2()
{
  next_image(-1);
  qiv_load_image(&main_img);
}


/*
 * Slideshow timer function
 *
 * If this function returns false, the timer is destroyed
 * and qiv_timer_restart() is automatically called, which
 * then starts the timer again. Thus images which takes some
 * time to load will still be displayed for "delay" seconds.
 */
 
static gboolean qiv_handle_timer(gpointer data)
{
  if (*(int *)data || slide) {
    next_image(0);
    qiv_load_image(&main_img);
  }
  return FALSE;
}


/*
 *	Slideshow timer (re)start function
 */
 
static void qiv_timer_restart(gpointer dummy)
{
  g_timeout_add_full(G_PRIORITY_LOW, delay,
		     qiv_handle_timer, &slide,
		     qiv_timer_restart);
}

/* Filter images by extension */

static void filter_images(int *images, char **image_names)
{
  int i = 0;

  while(i < *images) {
    if (check_extension(image_names[i])) {
      i++;
    } else {
      int j = i;
      while(j < *images-1) {
	image_names[j] = image_names[j+1];
	++j;
      }
      --(*images);
    }
  }
}

static int check_extension(const char *name)
{
  char *extn = strrchr(name, '.');
  int i;

  if (extn)
    for (i=0; image_extensions[i]; i++)
      if (strcasecmp(extn, image_extensions[i]) == 0)
        return 1;

  return 0;
}

#ifdef GTD_XINERAMA

static XineramaScreenInfo *
largest_xinerama_screen (XineramaScreenInfo * screens, int nscreens)
{
  XineramaScreenInfo * screen;
  long pixels;
  long maxpixels = 0;
  XineramaScreenInfo * largest_screen = screens;
  
  for (screen = screens; nscreens--; screen++) {
    pixels = screen->width * screen->height;
    if (pixels > maxpixels) {
      maxpixels = pixels;
      largest_screen = screen;
    }
  }
  return largest_screen;
}

static XineramaScreenInfo *
get_preferred_xinerama_screen(void)
{
  static XineramaScreenInfo preferred_screen[1];
  Display * dpy;
  XineramaScreenInfo *screens;
  int nscreens = 0;

  dpy = XOpenDisplay(gdk_get_display());
  if (!dpy) {
    return 0;
  }
  screens = XineramaQueryScreens(dpy, &nscreens);
  if (screens) {
    XineramaScreenInfo * s = largest_xinerama_screen(screens,nscreens);
    *preferred_screen = *s;
  }
  else {
    /* If we don't have Xinerama, fake it: */
    preferred_screen->screen_number = DefaultScreen(dpy);
    preferred_screen->x_org = 0;
    preferred_screen->y_org = 0;
    preferred_screen->width = DisplayWidth(dpy, DefaultScreen(dpy));
    preferred_screen->height = DisplayHeight(dpy, DefaultScreen(dpy));
  }
  XFree(screens);
  XCloseDisplay(dpy);
  return preferred_screen;
}
#endif
