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

static void setup_win(qiv_image *);
static void really_set_static_grav(GdkWindow *);
static int used_masks_before=0;
static struct timeval load_before, load_after;
static double load_elapsed;
static GdkCursor *cursor, *visible_cursor, *invisible_cursor;

/*
 *	Load & display image
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
    qiv_exit(0);
  }

  if (first) {
    setup_win(q);
    first = 0;
  }

  gdk_window_set_background(q->win, q->im ? &image_bg : &error_bg);

  if (do_grab || fullscreen) {
    gdk_keyboard_grab(q->win, FALSE, CurrentTime);
    gdk_pointer_grab(q->win, FALSE,
      GDK_BUTTON_PRESS_MASK| GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK,
      NULL, NULL, CurrentTime);
  }
  gettimeofday(&load_after, 0);
  load_elapsed = ((load_after.tv_sec +  load_after.tv_usec / 1.0e6) -
                 (load_before.tv_sec + load_before.tv_usec / 1.0e6));

  update_image(q, FULL_REDRAW);
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
    q->win = gdk_window_new(NULL, &attr, GDK_WA_X|GDK_WA_Y);

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
    buffer = calloc(1, screen_x * screen_y);
    rootGC = gdk_gc_new(root_win);
    temp = gdk_pixmap_create_from_data(root_win, buffer, screen_x, 
	       screen_y, gvis->depth, &image_bg, &image_bg);
    gdk_draw_pixmap(temp, rootGC, q->p, 0, 0, root_x, root_y, root_w, root_h);
    gdk_window_set_back_pixmap(root_win, temp, FALSE);
    gdk_gc_destroy(rootGC);
  }

  gdk_window_clear(root_win);
  gdk_flush();
}

void zoom_in(qiv_image *q)
{
  int zoom_percentage;

  maxpect = scale_down = 0;

  /* first compute current zoom_factor */
  if (fixed_window_size) {
    zoom_percentage=round((1.0-(q->orig_w - q->win_w)/(double)q->orig_w)*100);
    zoom_factor=(zoom_percentage - 100) / 10;
  }

  zoom_factor++;
  q->win_w = (gint)(q->orig_w * (1 + zoom_factor * 0.1));
  q->win_h = (gint)(q->orig_h * (1 + zoom_factor * 0.1));

  /* adapt image position */
  q->win_x = (screen_x - q->win_w) / 2;
  q->win_y = (screen_y - q->win_h) / 2;
}

void zoom_out(qiv_image *q)
{
  int zoom_percentage;

  maxpect = scale_down = 0;

  /* first compute current zoom_factor */
  if (fixed_window_size) {
    zoom_percentage=round((1.0-(q->orig_w - q->win_w)/(double)q->orig_w)*100);
    zoom_factor=(zoom_percentage - 100) / 10;
  }

  if(q->win_w > MIN(64, q->orig_w) && q->win_h > MIN(64, q->orig_h)) {
    zoom_factor--;
    q->win_w = (gint)(q->orig_w * (1 + zoom_factor * 0.1));
    q->win_h = (gint)(q->orig_h * (1 + zoom_factor * 0.1));

    /* adapt image position */
    q->win_x = (screen_x - q->win_w) / 2;
    q->win_y = (screen_y - q->win_h) / 2;
  } else {
    snprintf(infotext, sizeof infotext, "(Can not zoom_out anymore)");
    fprintf(stderr, "qiv: can not zoom_out anymore\n");
  }
}

void zoom_maxpect(qiv_image *q)
{
  double zx = (double)screen_x / (double)q->orig_w;
  double zy = (double)screen_y / (double)q->orig_h;
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
  center_image(q);
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
  gint text_len, text_w, text_h;
  gchar win_title[BUF_LEN];
  double elapsed;
  struct timeval before, after;

  if (q->error) {
    g_snprintf(win_title, sizeof win_title,
        "qiv: ERROR! cannot load image: %s", image_names[image_idx]);
    gdk_beep();
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
        m = gdk_imlib_move_mask(q->im);	/* creating transparency */
      }

      g_snprintf(win_title, sizeof win_title,
                 "qiv: %s (%dx%d) %d%% [%d/%d] b%d/c%d/g%d %s",
                 image_names[image_idx], q->orig_w, q->orig_h,
                 round((1.0-(q->orig_w - q->win_w)/(double)q->orig_w)*100), image_idx+1, images,
                 q->mod.brightness/8-32, q->mod.contrast/8-32, q->mod.gamma/8-32, infotext);
      snprintf(infotext, sizeof infotext, "(-)");

    } else {

      /* calculate elapsed time while we render image */
      gettimeofday(&before, 0);
      gdk_imlib_render(q->im, q->win_w, q->win_h);
      gettimeofday(&after, 0);
      elapsed = ((after.tv_sec +  after.tv_usec / 1.0e6) -
                 (before.tv_sec + before.tv_usec / 1.0e6));

      if (q->p) gdk_imlib_free_pixmap(q->p);
      q->p = gdk_imlib_move_image(q->im);
      m = gdk_imlib_move_mask(q->im);	/* creating transparency */
#ifdef DEBUG
      if (m)  g_print("*** image has transparency\n");
#endif

      g_snprintf(win_title, sizeof win_title,
                 "qiv: %s (%dx%d) %1.01fs %d%% [%d/%d] b%d/c%d/g%d %s",
                 image_names[image_idx], q->orig_w, q->orig_h, load_elapsed+elapsed,
                 round((1.0-(q->orig_w - q->win_w)/(double)q->orig_w)*100), image_idx+1, images,
                 q->mod.brightness/8-32, q->mod.contrast/8-32, q->mod.gamma/8-32, infotext);
      snprintf(infotext, sizeof infotext, "(-)");
    }
  }

  gdk_window_set_title(q->win, win_title);

  text_len = strlen(win_title);
  text_w = gdk_text_width(text_font, win_title, text_len);
  text_h = text_font->ascent + text_font->descent;

  if (!fullscreen) {
    if (center) {
      gdk_window_set_hints(q->win,
        q->win_x, q->win_y, q->win_w, q->win_h, q->win_w, q->win_h,
        GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE | GDK_HINT_POS);
      really_set_static_grav(q->win);
      gdk_window_move_resize(q->win, q->win_x, q->win_y, q->win_w, q->win_h);
    } else {
      gdk_window_set_hints(q->win,
        q->win_x, q->win_y, q->win_w, q->win_h, q->win_w, q->win_h,
        GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE);
      gdk_window_resize(q->win, q->win_w, q->win_h);
    }
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

    gdk_flush();
    if (statusbar_window) {
      gdk_draw_rectangle(q->win, q->bg_gc, 0,
                         MAX(2,q->win_w-text_w-10), MAX(2,q->win_h-text_h-10),
                         text_w+5, text_h+5);
      gdk_draw_rectangle(q->win, q->status_gc, 1,
                         MAX(3,q->win_w-text_w-9), MAX(3,q->win_h-text_h-9),
                         text_w+4, text_h+4);
      gdk_draw_text(q->win, text_font, q->text_gc,
                    MAX(5,q->win_w-text_w-7), MAX(5,q->win_h-7-text_font->descent),
                    win_title, text_len);
    }
  }
  else {
    if (mode == FULL_REDRAW)
      gdk_window_clear(q->win);
    else {
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
                                  q->text_ow > text_w || q->text_oh > text_h))
        gdk_draw_rectangle(q->win, q->bg_gc, 1,
            screen_x-q->text_ow-9, screen_y-q->text_oh-9,
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
          screen_x-text_w-10, screen_y-text_h-10, text_w+5, text_h+5);
      gdk_draw_rectangle(q->win, q->status_gc, 1,
          screen_x-text_w-9, screen_y-text_h-9, text_w+4, text_h+4);
      gdk_draw_text(q->win, text_font, q->text_gc,
          screen_x-text_w-7, screen_y-7-text_font->descent, win_title, text_len);
    }

    q->win_ox = q->win_x;
    q->win_oy = q->win_y;
    q->win_ow = q->win_w;
    q->win_oh = q->win_h;
    q->text_ow = text_w;
    q->text_oh = text_h;
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

void center_image(qiv_image *q)
{
  q->win_x = (screen_x - q->win_w) / 2;
  q->win_y = (screen_y - q->win_h) / 2;
}
