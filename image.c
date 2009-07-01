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
#include "qiv.h"

static void setup_win(qiv_image *);
static void really_set_static_grav(GdkWindow *);

/*
 *	Load & display image
 */

void qiv_load_image(qiv_image *q)
{
  if (q->im) {
    /* Discard previous image. To enable caching, s/kill/destroy/. */
    gdk_imlib_kill_image(q->im);
    q->im = NULL;
  }

  q->im = gdk_imlib_load_image(image_names[image_idx]);

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
    gdk_pointer_grab(q->win, FALSE, GDK_BUTTON_RELEASE_MASK,
      NULL, cursor, CurrentTime);
  }

  update_image(q);
}

static void setup_win(qiv_image *q)
{
  GdkWindowAttr attr;

  if (!fullscreen) {
    attr.window_type=GDK_WINDOW_TOPLEVEL;
    attr.wclass=GDK_INPUT_OUTPUT;
    attr.event_mask=GDK_ALL_EVENTS_MASK;
    attr.x = center ? q->win_x : 0;
    attr.y = center ? q->win_y : 0;
    attr.width  = q->win_w;
    attr.height = q->win_h;
    q->win = gdk_window_new(NULL, &attr, GDK_WA_X|GDK_WA_Y);
    gdk_window_set_cursor(q->win, cursor);

    if (center) {
      gdk_window_set_hints(q->win,
        q->win_x, q->win_y, q->win_w, q->win_h, q->win_w, q->win_h,
        GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE | GDK_HINT_POS);
      /* this call is broken. hack it ourselves... */
      //gdk_window_set_static_gravities(q->win, TRUE);
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

  q->black_gc = gdk_gc_new(q->win);
  q->status_gc = gdk_gc_new(q->win);
  gdk_gc_set_foreground(q->status_gc, &text_bg);
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
    root_w = q->orig_w;
    root_h = q->orig_h;
  }

  if (to_root) {
    root_x = (screen_x - root_w) / 2;
    root_y = (screen_y - root_h) / 2;
  }

  gdk_imlib_set_image_modifier(q->im, &q->mod);
  gdk_imlib_changed_image(q->im);
  gdk_imlib_render(q->im, root_w, root_h);
  q->p = gdk_imlib_move_image(q->im);

  if (transparency)
    m = gdk_imlib_move_mask(q->im);

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
  maxpect = scale_down = 0;
  q->win_w += (gint)(q->orig_w * 0.1);
  q->win_h += (gint)(q->orig_h * 0.1);
  center_image(q);
}

void zoom_out(qiv_image *q)
{
  maxpect = scale_down = 0;
  if(!fullscreen) {
    if(q->win_w > 64 && q->win_h > 64) {
      q->win_w -= (gint)(q->orig_w * 0.1);
      q->win_h -= (gint)(q->orig_h * 0.1);
      center_image(q);
    } else {
      snprintf(infotext, sizeof infotext, "(Can not zoom_out anymore)");
      fprintf(stderr, "qiv: can not zoom_out anymore\n");
    }
  } else {
    q->win_w -= (gint)(q->orig_w * 0.1);
    q->win_h -= (gint)(q->orig_h * 0.1);
    center_image(q);
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
  gdk_imlib_destroy_image(q->im);
  q->im = gdk_imlib_load_image(image_names[image_idx]);
  q->win_w = q->orig_w = q->im->rgb_width;
  q->win_h = q->orig_h = q->im->rgb_height;
  reset_mod(q);
  center_image(q);
}

void check_size(qiv_image *q, gint reset)
{
  if (maxpect || (scale_down && (q->orig_w>screen_x || q->orig_h>screen_y))) {
    zoom_maxpect(q);
  } else if (reset) {
    reset_coords(q);
  }

  center_image(q);
}

void reset_coords(qiv_image *q)
{
  q->win_w = q->orig_w;
  q->win_h = q->orig_h;
  q->move_x = 0;
  q->move_y = 0;
}

/* Something changed the image.  Redraw it. */

void update_image(qiv_image *q)
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
    gdk_imlib_set_image_modifier(q->im, &q->mod);
    gdk_imlib_changed_image(q->im);

    /* calculate elapsed time while we render image */
    gettimeofday(&before, 0);
    gdk_imlib_render(q->im, q->win_w, q->win_h);
    gettimeofday(&after, 0);
    elapsed = ((after.tv_sec +  after.tv_usec / 1.0e6) -
              (before.tv_sec + before.tv_usec / 1.0e6));

    if (q->p) gdk_imlib_free_pixmap(q->p);
    q->p = gdk_imlib_move_image(q->im);

    if (transparency)
        m = gdk_imlib_move_mask(q->im);	/* creating transparency */

    g_snprintf(win_title, sizeof win_title,
        "qiv: %s (%dx%d) %1.01fs %d%% [%d/%d] b%d/c%d/g%d %s",
        image_names[image_idx], q->orig_w, q->orig_h, elapsed,
        (int)((1.0-(q->orig_w - q->win_w)/(double)q->orig_w)*100), image_idx+1, images,
        q->mod.brightness/8-32, q->mod.contrast/8-32, q->mod.gamma/8-32, infotext);
    snprintf(infotext, sizeof infotext, "(-)");
  }

  gdk_window_set_title(q->win, win_title);

  text_len = strlen(win_title);
  text_w = gdk_text_width(text_font, win_title, text_len);
  text_h = text_font->ascent + text_font->descent;

  if (!fullscreen) {
    if (center) {
      gdk_window_move_resize(q->win, q->win_x, q->win_y, q->win_w, q->win_h);
    } else {
      gdk_window_resize(q->win, q->win_w, q->win_h);
    }
    if (!q->error) {
      gdk_window_set_back_pixmap(q->win, q->p, FALSE);
      if (transparency)
        gdk_window_shape_combine_mask(q->win, m, 0, 0);
    }
    gdk_window_clear(q->win);
  }
  else {
    gdk_window_clear(q->win);

    if (!q->error)
        gdk_draw_pixmap(q->win, q->black_gc, q->p, q->move_x, q->move_y,
            q->win_x, q->win_y, q->win_w, q->win_h);

    if (statusbar) {
      gdk_draw_rectangle(q->win, q->black_gc, 0,
          screen_x-text_w-10, screen_y-text_h-10, text_w+5, text_h+5);
      gdk_draw_rectangle(q->win, q->status_gc, 1,
          screen_x-text_w-9, screen_y-text_h-9, text_w+4, text_h+4);
      gdk_draw_text(q->win, text_font, q->black_gc,
          screen_x-text_w-7, screen_y-7-text_font->descent, win_title, text_len);
    }
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
  if (q->black_gc) gdk_gc_destroy(q->black_gc);
  if (q->status_gc) gdk_gc_destroy(q->status_gc);
}

void center_image(qiv_image *q)
{
  q->win_x = (screen_x - q->win_w) / 2;
  q->win_y = (screen_y - q->win_h) / 2;
}
