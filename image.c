/*
  Module       : image.c
  Purpose      : Routines dealing with image display
  More         : see qiv README
  Policy       : GNU GPL
  Homepage     : http://www.klografx.net/qiv/
*/

#include <stdio.h>
#include <string.h>
#include <gdk/gdkx.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "qiv.h"
#include "xmalloc.h"

static void setup_win(qiv_image *);
static void really_set_static_grav(GdkWindow *);
//static void setup_magnify(qiv_image *, qiv_mgl *); // [lc]
static int used_masks_before=0;
static struct timeval load_before, load_after;
static double load_elapsed;
static GdkCursor *cursor, *visible_cursor, *invisible_cursor;

/*
 *    Load & display image
 */

void qiv_load_image(qiv_image *q)
{
  GdkImlibColor color;
  struct stat statbuf;

  gettimeofday(&load_before, 0);

  if (q->im) {
    /* Discard previous image. To enable caching, s/kill/destroy/. */
    gdk_imlib_kill_image(q->im);
    q->im = NULL;
  }

  stat(image_names[image_idx], &statbuf);
  current_mtime = statbuf.st_mtime;
  q->im = gdk_imlib_load_image(image_names[image_idx]);

  /* this function doesn't seem to work :-(  */
  gdk_imlib_get_image_shape(q->im,&color);
#ifdef DEBUG
  g_print("transparent color (RGB): %d, %d, %d\n", color.r, color.g, color.b);
#endif

  /* turn transparency off */
  /* this function doesn't seem to work, but isn't necessary either  */
/*  if (!transparency) {
    color.r = color.g = color.b = -1;
    gdk_imlib_set_image_shape(q->im,&color);
  }
*/

  if (!q->im) { /* error */
    q->im = NULL;
    q->error = 1;
    q->orig_w = 400;
    q->orig_h = 300;
  } else { /* Retrieve image properties */
    q->error = 0;
    q->orig_w = q->im->rgb_width;
    q->orig_h = q->im->rgb_height;
  }

  check_size(q, TRUE);

  /* desktop-background -> exit */
  if (to_root || to_root_t || to_root_s) {
    if (!q->im) {
      fprintf(stderr, "qiv: cannot load background_image\n");
      qiv_exit(1);
    }
    set_desktop_image(q);
    if(slide)
      return;
    else
      qiv_exit(0);
  }

  if (first) {
    setup_win(q);
    first = 0;
  }

  gdk_window_set_background(q->win, q->im ? &image_bg : &error_bg);

  if (do_grab || (fullscreen && !disable_grab) ) {
    gdk_keyboard_grab(q->win, FALSE, CurrentTime);
    gdk_pointer_grab(q->win, FALSE,
      GDK_BUTTON_PRESS_MASK| GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK,
      NULL, NULL, CurrentTime);
  }
  gettimeofday(&load_after, 0);
  load_elapsed = ((load_after.tv_sec +  load_after.tv_usec / 1.0e6) -
                 (load_before.tv_sec + load_before.tv_usec / 1.0e6));

  update_image(q, FULL_REDRAW);
//    if (magnify && !fullscreen) {  // [lc]
//     setup_magnify(q, &magnify_img);     
//     update_magnify(q, &magnify_img, FULL_REDRAW, 0, 0);
//    }

}

static gchar blank_cursor[1];

static void setup_win(qiv_image *q)
{
  GdkWindowAttr attr;
  GdkPixmap *cursor_pixmap;

  if (!fullscreen) {
    attr.window_type=GDK_WINDOW_TOPLEVEL;
    attr.wclass=GDK_INPUT_OUTPUT;
    attr.event_mask=GDK_ALL_EVENTS_MASK;
    attr.x = center ? q->win_x : 0;
    attr.y = center ? q->win_y : 0;
    attr.width  = q->win_w;
    attr.height = q->win_h;
    attr.wmclass_name = "qiv";
    attr.wmclass_class = "Qiv";   // prevents segfault on amd64 [ld]
    q->win = gdk_window_new(NULL, &attr, GDK_WA_X|GDK_WA_Y|GDK_WA_WMCLASS);

    if (center) {
      gdk_window_set_hints(q->win,
        q->win_x, q->win_y, q->win_w, q->win_h, q->win_w, q->win_h,
        GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE | GDK_HINT_POS);
      /* this call is broken. hack it ourselves... */
      /* gdk_window_set_static_gravities(q->win, TRUE); */
      really_set_static_grav(q->win);
      gdk_window_move_resize(q->win, q->win_x, q->win_y, q->win_w, q->win_h);
    } else {
      gdk_window_set_hints(q->win,
        q->win_x, q->win_y, q->win_w, q->win_h, q->win_w, q->win_h,
        GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE);
      gdk_window_resize(q->win, q->win_w, q->win_h);
    }

    gdk_window_show(q->win);

  } else { /* fullscreen */

    attr.window_type=GDK_WINDOW_TEMP;
    attr.wclass=GDK_INPUT_OUTPUT;
    attr.event_mask=GDK_ALL_EVENTS_MASK;
    attr.x = attr.y = 0;
    attr.width=screen_x;
    attr.height=screen_y;
    q->win = gdk_window_new(NULL, &attr, GDK_WA_X|GDK_WA_Y);
    gdk_window_set_cursor(q->win, cursor);
    gdk_window_show(q->win);
  }

  q->bg_gc = gdk_gc_new(q->win);
  q->text_gc = gdk_gc_new(q->win); /* black is default */
  q->status_gc = gdk_gc_new(q->win);
  gdk_gc_set_foreground(q->bg_gc, &image_bg);
  gdk_gc_set_foreground(q->status_gc, &text_bg);

  cursor_pixmap = gdk_bitmap_create_from_data(q->win, blank_cursor, 1, 1);
  invisible_cursor = gdk_cursor_new_from_pixmap(cursor_pixmap, cursor_pixmap,
                                                &text_bg, &text_bg, 0, 0);
  cursor = visible_cursor = gdk_cursor_new(CURSOR);
  gdk_window_set_cursor(q->win, cursor);
}

void hide_cursor(qiv_image *q)
{
  if (cursor != invisible_cursor)
    gdk_window_set_cursor(q->win, cursor = invisible_cursor);
}

void show_cursor(qiv_image *q)
{
  if (cursor != visible_cursor)
    gdk_window_set_cursor(q->win, cursor = visible_cursor);
}

/* XXX: fix GDK. it's setting bit gravity instead of wm gravity, so we
 * have to go behind its back and kludge this ourselves. */

static void really_set_static_grav(GdkWindow *win)
{
  long dummy;

  XSizeHints *hints = XAllocSizeHints();
  XGetWMNormalHints(GDK_DISPLAY(), GDK_WINDOW_XWINDOW(win), hints, &dummy);
  hints->win_gravity = StaticGravity;
  hints->flags |= PWinGravity;
  XSetWMNormalHints(GDK_DISPLAY(), GDK_WINDOW_XWINDOW(win), hints);
  XFree(hints);
}

/* set image as background */

void set_desktop_image(qiv_image *q)
{
  GdkWindow *root_win = GDK_ROOT_PARENT();
  GdkVisual *gvis = gdk_window_get_visual(root_win);
  GdkPixmap *temp;
  GdkPixmap *m = NULL;
  gchar     *buffer;

  gint root_w = screen_x, root_h = screen_y;
  gint root_x = 0, root_y = 0;

  if (to_root || to_root_t) {
    root_w = q->win_w;
    root_h = q->win_h;
  }

  if (to_root) {
    root_x = (screen_x - root_w) / 2;
    root_y = (screen_y - root_h) / 2;
  }

  gdk_imlib_set_image_modifier(q->im, &q->mod);
  gdk_imlib_changed_image(q->im);
  gdk_imlib_render(q->im, root_w, root_h);
  q->p = gdk_imlib_move_image(q->im);
  m = gdk_imlib_move_mask(q->im);
#ifdef DEBUG
  if (m)  g_print("*** image has transparency\n");
#endif

  if (gvis != gdk_imlib_get_visual()) {
    fprintf(stderr,
        "qiv: Your root window's visual is not the visual Imlib chose;\n"
        "     qiv cannot set the background currently.\n");
    return;
  }
  
  if (to_root_t) {
    gdk_window_set_back_pixmap(root_win, q->p, FALSE);
  } else {
    GdkGC *rootGC;
    buffer = xcalloc(1, screen_x * screen_y);
    rootGC = gdk_gc_new(root_win);
    temp = gdk_pixmap_create_from_data(root_win, buffer, screen_x, 
                                       screen_y, gvis->depth, &image_bg, &image_bg);
    gdk_draw_pixmap(temp, rootGC, q->p, 0, 0, root_x, root_y, root_w, root_h);
    gdk_window_set_back_pixmap(root_win, temp, FALSE);
    gdk_imlib_free_pixmap(temp);
    gdk_gc_destroy(rootGC);
  }

  if(q->p) {
    gdk_imlib_free_pixmap(q->p);
    q->p = NULL;
  }
  gdk_window_clear(root_win);
  gdk_flush();
}

void zoom_in(qiv_image *q)
{
  int zoom_percentage;
  int w_old, h_old;

  /* first compute current zoom_factor */
  if (maxpect || scale_down || fixed_window_size) {
    zoom_percentage=myround((1.0-(q->orig_w - q->win_w)/(double)q->orig_w)*100);
    zoom_factor=(zoom_percentage - 100) / 10;
  }

  maxpect = scale_down = 0;

  zoom_factor++;
  w_old = q->win_w;
  h_old = q->win_h;
  q->win_w = (gint)(q->orig_w * (1 + zoom_factor * 0.1));
  q->win_h = (gint)(q->orig_h * (1 + zoom_factor * 0.1));

  /* adapt image position */
  q->win_x -= (q->win_w - w_old) / 2;
  q->win_y -= (q->win_h - h_old) / 2;

  if (fullscreen) {
    if (center)
      center_image(q);
    else
      correct_image_position(q);
  } else {
    correct_image_position(q);
  }
}

void zoom_out(qiv_image *q)
{
  int zoom_percentage;
  int w_old, h_old;

  /* first compute current zoom_factor */
  if (maxpect || scale_down || fixed_window_size) {
    zoom_percentage=myround((1.0-(q->orig_w - q->win_w)/(double)q->orig_w)*100);
    zoom_factor=(zoom_percentage - 100) / 10;
  }

  maxpect = scale_down = 0;

  w_old = q->win_w;
  h_old = q->win_h;

  if(zoom_factor > -9 && q->win_w > MIN(64, q->orig_w) && q->win_h > MIN(64, q->orig_h)) {
    zoom_factor--;
    q->win_w = (gint)(q->orig_w * (1 + zoom_factor * 0.1));
    q->win_h = (gint)(q->orig_h * (1 + zoom_factor * 0.1));

    /* adapt image position */
    q->win_x -= (q->win_w - w_old) / 2;
    q->win_y -= (q->win_h - h_old) / 2;

    if (fullscreen) {
      if (center)
        center_image(q);
      else
        correct_image_position(q);
    } else {
      correct_image_position(q);
    }
  } else {
    snprintf(infotext, sizeof infotext, "(Cannot zoom out anymore)");
    fprintf(stderr, "qiv: cannot zoom out anymore\n");
  }
}

void zoom_maxpect(qiv_image *q)
{
#ifdef GTD_XINERAMA
  double zx = (double)preferred_screen->width / (double)q->orig_w;
  double zy = (double)preferred_screen->height / (double)q->orig_h;
#else
  double zx = (double)screen_x / (double)q->orig_w;
  double zy = (double)screen_y / (double)q->orig_h;
#endif
  q->win_w = (gint)(q->orig_w * MIN(zx, zy));
  q->win_h = (gint)(q->orig_h * MIN(zx, zy));
  center_image(q);
}

/*
  Set display settings to startup values
  which are used whenever a new image is loaded.
*/

void reload_image(qiv_image *q)
{
  if(watch_file) {
    GdkImlibImage *new_im = gdk_imlib_load_image(image_names[image_idx]);
    if(new_im){
      struct stat statbuf;
      stat(image_names[image_idx], &statbuf);
      current_mtime = statbuf.st_mtime;

      gdk_imlib_destroy_image(q->im);
      q->im = new_im;
      q->orig_w = q->im->rgb_width;
      q->orig_h = q->im->rgb_height;
    }
  } else {
    gdk_imlib_destroy_image(q->im);
    q->im = gdk_imlib_load_image(image_names[image_idx]);
  }
  if (!q->im) { /* error */
    q->im = NULL;
    q->error = 1;
    q->orig_w = 400;
    q->orig_h = 300;
  } else { /* Retrieve image properties */
    q->error = 0;
    q->orig_w = q->im->rgb_width;
    q->orig_h = q->im->rgb_height;
  }
  
  q->win_w = (gint)(q->orig_w * (1 + zoom_factor * 0.1));
  q->win_h = (gint)(q->orig_h * (1 + zoom_factor * 0.1));
  reset_mod(q);
  center_image(q);
}

void check_size(qiv_image *q, gint reset)
{
  if (maxpect || (scale_down && (q->orig_w>screen_x || q->orig_h>screen_y))) {
    zoom_maxpect(q);
  } else if (reset || (scale_down && (q->win_w<q->orig_w || q->win_h<q->orig_h))) {
    reset_coords(q);
  }
  if (center) center_image(q);
}

void reset_coords(qiv_image *q)
{
  if (fixed_window_size) {
    double w_o_ratio = (double)(fixed_window_size) / q->orig_w;
    q->win_w = fixed_window_size;
    q->win_h = q->orig_h * w_o_ratio;
  } else {
    if (fixed_zoom_factor) {
      zoom_factor = fixed_zoom_factor; /* reset zoom */
    }
    q->win_w = (gint)(q->orig_w * (1 + zoom_factor * 0.1));
    q->win_h = (gint)(q->orig_h * (1 + zoom_factor * 0.1));
  }
}

/* Something changed the image.  Redraw it. */

void update_image(qiv_image *q, int mode)
{
  GdkPixmap *m = NULL;
  double elapsed;
  struct timeval before, after;
  int i;

  if (q->error) {
    g_snprintf(q->win_title, sizeof q->win_title,
        "qiv: ERROR! cannot load image: %s", image_names[image_idx]);
    gdk_beep();

    /* take this image out of the file list */
    --images;
    for(i=image_idx;i<images;++i) {
      image_names[i] = image_names[i+1];
    }

    /* If deleting the last file out of x */
    if(images == image_idx)
      image_idx = 0;
    
    /* If deleting the only file left */    
    if(!images) {
#ifdef DEBUG
      g_print("*** deleted last file in list. Exiting.\n");
#endif
      gdk_exit(0);
    }
    /* else load the next image */
    qiv_load_image(q);
    return;

  } else {
    if (mode == REDRAW || mode == FULL_REDRAW) {
      gdk_imlib_set_image_modifier(q->im, &q->mod);
      gdk_imlib_changed_image(q->im);
    }

    if (mode == MOVED) {
      if (transparency && used_masks_before) {
        /* there should be a faster way to update the mask, but how? */
        gdk_imlib_render(q->im, q->win_w, q->win_h);
        if (q->p) gdk_imlib_free_pixmap(q->p);
        q->p = gdk_imlib_move_image(q->im);
        m = gdk_imlib_move_mask(q->im);    /* creating transparency */
      }

      g_snprintf(q->win_title, sizeof q->win_title,
                 "qiv: %s (%dx%d) %d%% [%d/%d] b%d/c%d/g%d %s",
                 image_names[image_idx], q->orig_w, q->orig_h,
                 myround((1.0-(q->orig_w - q->win_w)/(double)q->orig_w)*100), image_idx+1, images,
                 q->mod.brightness/8-32, q->mod.contrast/8-32, q->mod.gamma/8-32, infotext);
      snprintf(infotext, sizeof infotext, "(-)");

    } // mode == MOVED
    else
    {

      /* calculate elapsed time while we render image */
      gettimeofday(&before, 0);
      gdk_imlib_render(q->im, q->win_w, q->win_h);
      gettimeofday(&after, 0);
      elapsed = ((after.tv_sec +  after.tv_usec / 1.0e6) -
                 (before.tv_sec + before.tv_usec / 1.0e6));

      if (q->p) gdk_imlib_free_pixmap(q->p);
      q->p = gdk_imlib_move_image(q->im);
      m = gdk_imlib_move_mask(q->im);    /* creating transparency */
#ifdef DEBUG
      if (m)  g_print("*** image has transparency\n");
#endif

      g_snprintf(q->win_title, sizeof q->win_title,
                 "qiv: %s (%dx%d) %1.01fs %d%% [%d/%d] b%d/c%d/g%d %s",
                 image_names[image_idx], q->orig_w, q->orig_h, load_elapsed+elapsed,
                 myround((1.0-(q->orig_w - q->win_w)/(double)q->orig_w)*100), image_idx+1, images,
                 q->mod.brightness/8-32, q->mod.contrast/8-32, q->mod.gamma/8-32, infotext);
      snprintf(infotext, sizeof infotext, "(-)");
    }
  }

  gdk_window_set_title(q->win, q->win_title);

  q->text_len = strlen(q->win_title);
  q->text_w = gdk_text_width(text_font, q->win_title, q->text_len);
  q->text_h = text_font->ascent + text_font->descent;

  if (!fullscreen) {
// it's not necessary to differentiate between these cases anymore
// because we know the window position in any case and thus can
// always trust the computed new coordinates and move the window there
//    if (center) {
      gdk_window_set_hints(q->win,
        q->win_x, q->win_y, q->win_w, q->win_h, q->win_w, q->win_h,
        GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE | GDK_HINT_POS);
      really_set_static_grav(q->win);
      gdk_window_move_resize(q->win, q->win_x, q->win_y, q->win_w, q->win_h);
//    } else {
//      gdk_window_set_hints(q->win,
//        q->win_x, q->win_y, q->win_w, q->win_h, q->win_w, q->win_h,
//        GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE);
//      gdk_window_resize(q->win, q->win_w, q->win_h);
//    }
    if (!q->error) {
      gdk_window_set_back_pixmap(q->win, q->p, FALSE);
      /* remove or set transparency mask */
      if (used_masks_before) {
        if (transparency)
          gdk_window_shape_combine_mask(q->win, m, 0, 0);
        else
          gdk_window_shape_combine_mask(q->win, 0, 0, 0);
      }
      else
      {
        if (transparency && m) {
          gdk_window_shape_combine_mask(q->win, m, 0, 0);
          used_masks_before=1;
        }
      }
    }
    gdk_window_clear(q->win);
/**
    if (statusbar_window) {
#ifdef DEBUG
      g_print("*** print statusbar at (%d, %d)\n", MAX(2,q->win_w-q->text_w-10), MAX(2,q->win_h-q->text_h-10));
#endif
//printf(">>> statusbar_w %d %d %d %d\n",
// MAX(2,q->win_w-text_w-10), MAX(2,q->win_h-text_h-10), text_w+5, text_h+5);

      gdk_draw_rectangle(q->win, q->bg_gc, 0,
                         MAX(2,q->win_w-q->text_w-10), MAX(2,q->win_h-q->text_h-10),
                         q->text_w+5, q->text_h+5);
      gdk_draw_rectangle(q->win, q->status_gc, 1,
                         MAX(3,q->win_w-q->text_w-9), MAX(3,q->win_h-q->text_h-9),
                         q->text_w+4, q->text_h+4);
      gdk_draw_text(q->win, text_font, q->text_gc,
                    MAX(5,q->win_w-q->text_w-7), MAX(5,q->win_h-7-text_font->descent),
                    q->win_title, q->text_len);
    }
    gdk_flush();
 **/
  } // if (!fullscreen)
  else
  {
#ifdef GTD_XINERAMA
# define statusbar_x (statusbar_screen->x_org + statusbar_screen->width)
# define statusbar_y (statusbar_screen->y_org + statusbar_screen->height)
#else
# define statusbar_x screen_x
# define statusbar_y screen_y
#endif
    if (mode == FULL_REDRAW) {
      gdk_window_clear(q->win); 
    } else {
      if (q->win_x > q->win_ox)
        gdk_draw_rectangle(q->win, q->bg_gc, 1,
          q->win_ox, q->win_oy, q->win_x - q->win_ox, q->win_oh);
      if (q->win_y > q->win_oy)
        gdk_draw_rectangle(q->win, q->bg_gc, 1,
          q->win_ox, q->win_oy, q->win_ow, q->win_y - q->win_oy);
      if (q->win_x + q->win_w < q->win_ox + q->win_ow)
        gdk_draw_rectangle(q->win, q->bg_gc, 1,
          q->win_x + q->win_w, q->win_oy, q->win_ox + q->win_ow, q->win_oh);
      if (q->win_y + q->win_h < q->win_oy + q->win_oh)
        gdk_draw_rectangle(q->win, q->bg_gc, 1,
          q->win_ox, q->win_y + q->win_h, q->win_ow, q->win_oy + q->win_oh);

      if (q->statusbar_was_on && (!statusbar_fullscreen ||
                                  q->text_ow > q->text_w || q->text_oh > q->text_h))
        gdk_draw_rectangle(q->win, q->bg_gc, 1,
            statusbar_x-q->text_ow-9, statusbar_y-q->text_oh-9,
            q->text_ow+4, q->text_oh+4);
    }

    /* remove or set transparency mask */
    if (used_masks_before) {
      if (transparency)
        gdk_window_shape_combine_mask(q->win, m, q->win_x, q->win_y);
      else
        gdk_window_shape_combine_mask(q->win, 0, q->win_x, q->win_y);
    }
    else
    {
      if (transparency && m) {
        gdk_window_shape_combine_mask(q->win, m, q->win_x, q->win_y);
        used_masks_before=1;
      }
    }

    if (!q->error)
      gdk_draw_pixmap(q->win, q->bg_gc, q->p, 0, 0,
                      q->win_x, q->win_y, q->win_w, q->win_h);

    if (statusbar_fullscreen) {
      gdk_draw_rectangle(q->win, q->bg_gc, 0,
          statusbar_x-q->text_w-10, statusbar_y-q->text_h-10, q->text_w+5, q->text_h+5);
      gdk_draw_rectangle(q->win, q->status_gc, 1,
          statusbar_x-q->text_w-9, statusbar_y-q->text_h-9, q->text_w+4, q->text_h+4);
      gdk_draw_text(q->win, text_font, q->text_gc,
          statusbar_x-q->text_w-7, statusbar_y-7-text_font->descent,
          q->win_title, q->text_len);
    }

    q->win_ox = q->win_x;
    q->win_oy = q->win_y;
    q->win_ow = q->win_w;
    q->win_oh = q->win_h;
    q->text_ow = q->text_w;
    q->text_oh = q->text_h;
    q->statusbar_was_on = statusbar_fullscreen;
  }
  gdk_flush();
}


void reset_mod(qiv_image *q)
{
  q->mod.brightness = default_brightness;
  q->mod.contrast = default_contrast;
  q->mod.gamma = default_gamma;
}

void destroy_image(qiv_image *q)
{
  if (q->p) gdk_imlib_free_pixmap(q->p);
  if (q->bg_gc) gdk_gc_destroy(q->bg_gc);
  if (q->text_gc) gdk_gc_destroy(q->text_gc);
  if (q->status_gc) gdk_gc_destroy(q->status_gc);
}

void setup_magnify(qiv_image *q, qiv_mgl *m)
{
   GdkWindowAttr mgl_attr;
   GdkGeometry mgl_hints;

   m->win_w=300; m->win_h=200;

//   gdk_flush();
   gdk_window_get_root_origin(q->win, &m->frame_x, &m->frame_y);
// printf("frame %d %d\n", m->frame_x, m->frame_y);

   mgl_attr.window_type=GDK_WINDOW_TOPLEVEL; // Set up attributes for GDK to create a Window
   mgl_attr.wclass=GDK_INPUT_OUTPUT;
   mgl_attr.event_mask=GDK_STRUCTURE_MASK;
   mgl_attr.width=m->win_w;
   mgl_attr.height=m->win_h;
   mgl_attr.override_redirect=TRUE;
//   m->win=gdk_window_new(NULL,&mgl_attr,GDK_WA_X|GDK_WA_Y|GDK_WA_WMCLASS);
   m->win=gdk_window_new(NULL,&mgl_attr,GDK_WA_X|GDK_WA_Y);
   mgl_hints.min_width=m->win_w;
   mgl_hints.max_width=m->win_w;
   mgl_hints.min_height=m->win_h;
   mgl_hints.max_height=m->win_h;

   //gdk_window_set_hints(GdkWindow*window, gint x, gint y,
   //                     gint min_width, gint min_height,
   //                     gint max_width, gint max_height,
   //                     gint flags);
   gdk_window_set_hints(m->win, mgl_attr.x, mgl_attr.y,
                        mgl_attr.width, mgl_attr.height, mgl_attr.width, mgl_attr.height,
                        GDK_HINT_POS | GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE);
   gdk_window_set_decorations(m->win, GDK_DECOR_BORDER);
   gdk_flush();
}

void update_magnify(qiv_image *q, qiv_mgl *m, int mode, gint xcur, gint ycur)
{
//   GdkWindowAttr mgl_attr;
//   GdkGeometry mgl_hints;
  register  gint xx, yy;  // win_pos_x, win_pos_y;

/***********
  if (mode == FULL_REDRAW) {
//    printf("> update_magnify: FULL_REDRAW \n");
    m->im=gdk_imlib_crop_and_clone_image(
      q->im, 0, 0, m->win_w, m->win_h);
    gdk_imlib_apply_image(m->im,m->win);

    mgl_hints.min_width=m->win_w;
    mgl_hints.max_width=m->win_w;
    mgl_hints.min_height=m->win_h;
    mgl_hints.max_height=m->win_h;

//    gdk_window_set_hints(m->win, mgl_attr.x, mgl_attr.y,mglw, 
//                         mglw,mglh, mglh, GDK_HINT_POS | GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE);
    gdk_window_set_hints(m->win, xcur+50, ycur-50-m->win_h,m->win_w, 
                         m->win_w,m->win_h, m->win_h, GDK_HINT_POS | GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE);
//    gdk_window_move_resize(m->win,mgl_attr.x, mgl_attr.y, mglw, mglh);

//    gdk_window_set_geometry_hints(magnify_img.win, &mgl_hints, GDK_HINT_POS );
  }
************/ 
  if (mode == REDRAW ) {
    xx=xcur * q->orig_w / q->win_w;    /* xx, yy are the coords of cursor   */
    if (xx <= m->win_w/2)               /* xcur, ycur scaled to the original */
      xx=0;                             /* image; they are changed so that   */
    else                                /* the magnified part is always      */
      if (xx >= q->orig_w - m->win_w/2) /* inside the image.                 */
        xx=q->orig_w - m->win_w;
      else
        xx=xx - m->win_w/2;

    yy=ycur * q->orig_h / q->win_h;
    if (yy <= m->win_h/2)
      yy=0;
    else
      if (yy >= q->orig_h - m->win_h/2) 
        yy=q->orig_h - m->win_h;
      else
        yy=yy - m->win_h/2;

//    printf("MGL: xcur: %d, ycur: %d, xx: %d, yy: %d\n", xcur, ycur, xx, yy);

    if (m->im) {
      gdk_imlib_kill_image(m->im);
      m->im = NULL;
    }

    m->im=gdk_imlib_crop_and_clone_image(q->im, xx, yy, m->win_w, m->win_h);
    gdk_imlib_apply_image(m->im, m->win);
    gdk_window_show(m->win);

    // xcur= m->frame_x + xcur +
    // (xcur < m->win_w/2? 50 : - 50 - m->win_w); 
    // gdk_window_get_root_origin(q->win,              // todo  [lc]
    //        &magnify_img.frame_x, &magnify_img.frame_y);  // call not necessary
    xx= m->frame_x + xcur - 50 - m->win_w;
    yy= m->frame_y + ycur - 50 - m->win_h;
    if (xx < 0) {
      if (xcur < m->win_w - magnify_img.frame_x)
        xx=m->frame_x + xcur + 50;
      else
        xx=0;
    }
    if (yy < 0) {
      if (ycur < m->win_h - magnify_img.frame_y)
        yy=m->frame_y + ycur + 50;
      else
        yy=0;
    }
//    printf("MGL: m->frame_x: %d, m->frame_y: %d, xx: %d, yy: %d\n", m->frame_x, m->frame_y, xx, yy);

//    gdk_window_move_resize(m->win, xx, yy,    m->win_w, m->win_h);
    gdk_window_move(m->win, xx, yy);
  }

// gdk_window_show(m->win);
  gdk_flush();
}

#ifdef GTD_XINERAMA
void center_image(qiv_image *q)
{
//  g_print("before: q->win_x = %d, q->win_y = %d, q->win_w = %d\n", q->win_x, q->win_y, q->win_w);
  XineramaScreenInfo * pscr = preferred_screen;

//  g_print("screen_x = %d, pscr->x_org = %d, pscr->width = %d\n", screen_x, pscr->x_org, pscr->width);

  /* Figure out x position */
  if (q->win_w <= pscr->width) {
    /* If image fits on screen try to center image
     * within preferred (sub)screen */
    q->win_x = (pscr->width - q->win_w) / 2 + pscr->x_org;
    /* Ensure image actually lies within screen boundaries */
    if (q->win_x < 0) {
      q->win_x = 0;
    }
    else if (q->win_x + q->win_w > screen_x) {
      q->win_x = screen_x - q->win_w;
    }
  }
  else {
    /* If image wider than screen, just center it over all screens */
    q->win_x = (screen_x - q->win_w) / 2;
//    g_print("q->win_x = %d, screen_x = %d, q->win_w = %d, pscr->width = %d\n", q->win_x, screen_x, q->win_w, pscr->width);
  }

  /* Same thing for y position */
  if (q->win_h <= screen_y) {
    q->win_y = (pscr->height - q->win_h) / 2 + pscr->y_org;
    if (q->win_y < 0) {
      q->win_y = 0;
    }
    else if (q->win_y + q->win_h > screen_y) {
      q->win_y = screen_y - q->win_h;
    }
  }
  else {
    q->win_y = (screen_y - q->win_h) / 2;
  }
//  g_print("after:  q->win_x = %d, q->win_y = %d, q->win_w = %d\n", q->win_x, q->win_y, q->win_w);
}
#else
void center_image(qiv_image *q)
{
  q->win_x = (screen_x - q->win_w) / 2;
  q->win_y = (screen_y - q->win_h) / 2;
}
#endif

void correct_image_position(qiv_image *q)
{
//  g_print("before: q->win_x = %d, q->win_y = %d, q->win_w = %d\n", q->win_x, q->win_y, q->win_w);

  /* try to keep inside the screen */
  if (q->win_w < screen_x) {
    if (q->win_x < 0)
      q->win_x = 0;
    if (q->win_x + q->win_w > screen_x)
      q->win_x = screen_x - q->win_w;
  } else {
    if (q->win_x > 0)
      q->win_x = 0;
    if (q->win_x + q->win_w < screen_x)
      q->win_x = screen_x - q->win_w;
  }
    
  /* don't leave ugly borders */
  if (q->win_h < screen_y) {
    if (q->win_y < 0)
      q->win_y = 0;
    if (q->win_y + q->win_h > screen_y)
      q->win_y = screen_y - q->win_h;
  } else {
    if (q->win_y > 0)
      q->win_y = 0;
    if (q->win_y + q->win_h < screen_y)
      q->win_y = screen_y - q->win_h;
  }
//  g_print("after:  q->win_x = %d, q->win_y = %d, q->win_w = %d\n", q->win_x, q->win_y, q->win_w);
}
