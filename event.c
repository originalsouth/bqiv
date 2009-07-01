/*
  Module       : event.c
  Purpose      : Handle GDK events
  More         : see qiv README
  Policy       : GNU GPL
  Homepage     : http://www.klografx.net/qiv/
*/	

#include <stdio.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include "qiv.h"

static int	jumping;
static char	jcmd[100];
static int	jidx;

void qiv_handle_event(GdkEvent *ev, gpointer data)
{
  gboolean exit_slideshow = FALSE;
  qiv_image *q = data;
  gint i;

  switch(ev->type) {
    case GDK_DELETE:
      qiv_exit(0);
      break;

    case GDK_BUTTON_PRESS:
      jumping=0;		/* abort jump mode if a button is pressed */
      if (cursor == visible_cursor)
	gdk_window_set_cursor(q->win, cursor = invisible_cursor);
      break;

    case GDK_MOTION_NOTIFY:
      if (cursor == invisible_cursor)
	gdk_window_set_cursor(q->win, cursor = visible_cursor);
      break;

    /* Use release instead of press (Fixes bug with junk being sent
     * to underlying xterm window on exit) */
    case GDK_BUTTON_RELEASE:
      exit_slideshow = TRUE;
      switch (ev->button.button) {
	case 1:		/* 1st or 5th button pressed */
	case 5:		/* scroll wheel down emulated by button 5 */
	  next_image(1);
	  qiv_load_image(q);
	  break;
	case 2:		/* 2nd button pressed */
	  qiv_exit(0);
	  break;
	case 3:		/* 3rd or 4th button pressed */
	case 4:		/* scroll wheel up emulated by button 4 */
	  next_image(-1);
	  qiv_load_image(q);
	  break;
      }
      break;

    case GDK_KEY_PRESS:

      exit_slideshow = TRUE;	/* Abort slideshow on any key by default */
      if (cursor != invisible_cursor)
	gdk_window_set_cursor(q->win, cursor = invisible_cursor);
   #ifdef DEBUG
      g_print("*** key:\n");	/* display key-codes */
      g_print("\tstring: %s\n",ev->key.string);
      g_print("\tkeyval: %d\n",ev->key.keyval);
   #endif
      if (jumping) {
	if((ev->key.keyval == GDK_Return) ||
	   (ev->key.keyval == GDK_KP_Enter) ||
	   (jidx == 99)) {     /* 99 digits already typed */
	  jcmd[jidx] = '\0';
	  jump2image(jcmd);
	  qiv_load_image(q);
	  jumping=0;
	}
          /* else record keystroke if not null */
	else if(ev->key.string && *(ev->key.string) != '\0')
	  jcmd[jidx++]=*(ev->key.string);
      } else {
	switch (ev->key.keyval) {

	  /* Help */

	  case '?':
	  case GDK_F1:
	    if (fullscreen) {
              int temp, text_w = 0, text_h;

              for (i = 0; helpstrs[i]; i++) {
                  temp = gdk_text_width(text_font, helpstrs[i], strlen(helpstrs[i]));
                  if (text_w < temp) text_w = temp;
              }

              text_h = i * (text_font->ascent + text_font->descent);

	      snprintf(infotext, sizeof infotext, "(Showing Help)");
	      gdk_draw_rectangle(q->win, q->black_gc, 0,
				 screen_x/2 - text_w/2 - 4,
				 screen_y/2 - text_h/2 - 4,
				 text_w + 7, text_h + 7);
	      gdk_draw_rectangle(q->win, q->status_gc, 1,
				 screen_x/2 - text_w/2 - 3,
				 screen_y/2 - text_h/2 - 3,
				 text_w + 6, text_h + 6);
	      for (i = 0; helpstrs[i]; i++) {
		gdk_draw_text(q->win, text_font, q->black_gc, screen_x/2 - text_w/2,
                  screen_y/2 - text_h/2 - text_font->descent +
                  (i+1) * (text_font->ascent + text_font->descent),
                  helpstrs[i], strlen(helpstrs[i]));
	      }
	    } else {
	      for (i = 0; helpstrs[i] != NULL; i++) {
		printf("%s\n", helpstrs[i]);
	      }
	    }
	    break;

	  /* Exit */
	  
	  case GDK_Escape:
	  case 'Q':
	  case 'q':
            qiv_exit(0);
	    break;

	  /* Fullscreen mode (on/off) */

	  case 'F':
	  case 'f':
	    exit_slideshow = FALSE;
	    gdk_window_withdraw(q->win);
	    if (cursor == invisible_cursor)
	      gdk_window_set_cursor(q->win, cursor = visible_cursor);
	    fullscreen ^= 1;
	    first=1;
	    qiv_load_image(q);
	    break;

	  /* Maxpect on/off */

	  case 'M':
	  case 'm':
            scale_down = 0;
	    maxpect ^= 1;
            snprintf(infotext, sizeof infotext, maxpect ? 
                "(Maxpect: on)" : "(Maxpect: off)");
	    check_size(q, TRUE);
	    update_image(q);
	    break;

	  /* Random on/off */

	  case 'R':
	  case 'r':
	    random_order ^= 1;
            snprintf(infotext, sizeof infotext, random_order ?
                "(Random order: on)" : "(Random order: off)");
	    update_image(q);
	    break;

	  /* Statusbar on/off  */

	  case 'I':
	  case 'i':
	    exit_slideshow = FALSE;
	    statusbar ^= 1;
	    update_image(q);
	    break;

	  /* Slide show on/off */

	  case 'S':
	  case 's':
	    exit_slideshow = FALSE;
	    slide ^= 1;
            snprintf(infotext, sizeof infotext, slide ?
                "(Slideshow: on)" : "(Slideshow: off)");
	    update_image(q);
	    break;

	  /* move image right */

	  case GDK_Left:
	    if (fullscreen) {
		if (q->win_w + q->move_x > 10) {
	          q->move_x = q->move_x - (q->win_w / 100);
	          snprintf(infotext, sizeof infotext, "(Moving right)");
		} else {
	          snprintf(infotext, sizeof infotext, "(Can not move anymore)");
		} 
	    } else {
	      snprintf(infotext, sizeof infotext, "(Moving works only in fullscreen mode)");
	      fprintf(stdout, "qiv: Moving works only in fullscreen mode\n");
	    }
	    update_m_image(q);
	    break;

	  /* move image left */

	  case GDK_Right:
	   if(fullscreen) {
		if (q->win_w - q->move_x > 10) {
	    	    q->move_x = q->move_x + (q->win_w / 100);
	            snprintf(infotext, sizeof infotext, "(Moving left)");
		} else {
	          snprintf(infotext, sizeof infotext, "(Can not move anymore)");
		} 
	    } else {
	      snprintf(infotext, sizeof infotext, "(Moving works only in fullscreen mode)");
	      fprintf(stdout, "qiv: Moving works only in fullscreen mode\n");
	    }
	    update_m_image(q);
	    break;

	  /* move image up */

	  case GDK_Down:
            if (fullscreen) {
                if(q->win_h - q->move_y > 10) {
                    q->move_y = q->move_y + (q->win_h / 100);
                    snprintf(infotext, sizeof infotext, "(Moving up)");
                } else {
                    snprintf(infotext, sizeof infotext, "(Can not move anymore)");
                }		 
            } else {
                snprintf(infotext, sizeof infotext, "(Moving works only in fullscreen mode)");
                fprintf(stdout, "qiv: Moving works only in fullscreen mode\n");
            }
	    update_m_image(q);
	    break;

	  /* move image down */

	  case GDK_Up:
            if(fullscreen) {
                if(q->move_y + q->win_h > 10) {
                    q->move_y = q->move_y - (q->win_h / 100);
                    snprintf(infotext, sizeof infotext, "(Moving down)");
                } else {
                    snprintf(infotext, sizeof infotext, "(Can not move anymore)");
                }		 
            } else {
                snprintf(infotext, sizeof infotext, "(Moving works only in fullscreen mode)");
                fprintf(stdout, "qiv: Moving works only in fullscreen mode\n");
            }
            update_m_image(q);
            break;

	  /* Scale_down */

	  case 'T':
	  case 't':
            maxpect = 0;
	    scale_down ^= 1;
            snprintf(infotext, sizeof infotext, scale_down ?
                "(Scale down: on)" : "(Scale down: off)");
	    check_size(q, TRUE);
	    update_image(q);
	    break;

	  /* Resize + */

	  case GDK_KP_Add:
	  case '+':
	  case '=':
	    snprintf(infotext, sizeof infotext, "(Zoomed in)");
	    zoom_in(q);
	    update_z_image(q);
	    break;

	  /* Resize - */

	  case GDK_KP_Subtract:
	  case '-':
	    snprintf(infotext, sizeof infotext, "(Zoomed out)");
	    zoom_out(q);
	    update_z_image(q);
	    break;

	  /* Reset Image / Original (best fit) size */

	  case GDK_Return:
	  case GDK_KP_Enter:
	    snprintf(infotext, sizeof infotext, "(Original size)");
            reload_image(q);
            check_size(q, TRUE);
	    update_image(q);
	    break;

	  /* Next picture - or loop to the first */

	  case ' ':
	  case GDK_Page_Down:
	  case GDK_KP_Page_Down:
	    snprintf(infotext, sizeof infotext, "(Next picture)");
	    next_image(1);
	    qiv_load_image(q);
	    break;

	  /* Previous picture - or loop back to the last */

	  case GDK_BackSpace:
	  case GDK_Page_Up:
	  case GDK_KP_Page_Up:
	    snprintf(infotext, sizeof infotext, "(Previous picture)");
	    next_image(-1);
	    qiv_load_image(q);
	    break;

	  /* + brightness */

	  case 'B':
	    snprintf(infotext, sizeof infotext, "(More brightness)");
	    q->mod.brightness += 8;
	    update_image(q);
	    break;

	  /* - brightness */

	  case 'b':
	    snprintf(infotext, sizeof infotext, "(Less brightness)");
	    q->mod.brightness -= 8;
	    update_image(q);
	    break;

	  /* + contrast */

	  case 'C':
	    snprintf(infotext, sizeof infotext, "(More contrast)");
	    q->mod.contrast += 8;
	    update_image(q);
	    break;

	  /* - contrast */

	  case 'c':
	    snprintf(infotext, sizeof infotext, "(Less contrast)");
	    q->mod.contrast -= 8;
	    update_image(q);
	    break;

          /* + gamma */

	  case 'G':
	    snprintf(infotext, sizeof infotext, "(More gamma)");
	    q->mod.gamma += 8;
	    update_image(q);
	    break;

	  /* - gamma */

	  case 'g':
	    snprintf(infotext, sizeof infotext, "(Less gamma)");
	    q->mod.gamma -= 8;
	    update_image(q);
	    break;

	  /* Delete image */

	  case GDK_Delete:
	  case 'D':
	  case 'd':
	    if (move2trash() == 0)
		snprintf(infotext, sizeof infotext, "(Deleted last image)");
	    else
		snprintf(infotext, sizeof infotext, "(Delete FAILED)");
	    qiv_load_image(q);
	    break;

	  /* Undelete image */

	  case 'U':
	  case 'u':
	    if (undelete_image() == 0)
		snprintf(infotext, sizeof infotext, "(Undeleted)");
	    else
		snprintf(infotext, sizeof infotext, "(Undelete FAILED)");
	    qiv_load_image(q);
	    break;

          /* Jump to image */

	  case 'J':
	  case 'j':
            jumping=1;
	    jidx=0;
	    break;

	  /* Flip horizontal */

	  case 'H':
	  case 'h':
	    gdk_imlib_flip_image_horizontal(q->im);
	    snprintf(infotext, sizeof infotext, "(Flipped horizontal)");
	    update_image(q);
	    break;

	  /* Flip vertical */

	  case 'V':
	  case 'v':
	    gdk_imlib_flip_image_vertical(q->im);
	    snprintf(infotext, sizeof infotext, "(Flipped vertical)");
	    update_image(q);
	    break;

	  /* Rotate right */

	  case 'k':
	  case 'K':
	    gdk_imlib_rotate_image(q->im, 1);
	    gdk_imlib_flip_image_horizontal(q->im);
	    snprintf(infotext, sizeof infotext, "(Rotated right)");
            swap(&q->orig_w, &q->orig_h);
            swap(&q->win_w, &q->win_h);
            check_size(q, FALSE);
            update_image(q);
	    break;

	  /* Rotate left */

	  case 'l':
	  case 'L':
	    gdk_imlib_rotate_image(q->im, -1);
	    gdk_imlib_flip_image_vertical(q->im);
	    snprintf(infotext, sizeof infotext, "(Rotated left)");
            swap(&q->orig_w, &q->orig_h);
            swap(&q->win_w, &q->win_h);
            check_size(q, FALSE);
            update_image(q);
	    break;

	  /* Center image on background */

	  case 'X':
	  case 'x':
	    to_root=1;
            set_desktop_image(q);
            snprintf(infotext, sizeof infotext, "(Centered image on background)");
            update_image(q);
	    to_root=0;
	    break;

	  /* Tile image on background */

	  case 'Y':
	  case 'y':
	    to_root_t=1;
            set_desktop_image(q);
            snprintf(infotext, sizeof infotext, "(Tiled image on background)");
            update_image(q);
	    to_root_t=0;
	    break;

	  case 'Z':
	  case 'z':
	    to_root_s=1;
	    set_desktop_image(q);
            snprintf(infotext, sizeof infotext, "(Stretched image on background)");
            update_image(q);
	    to_root_s=0;
	    break;

	  /* run qiv-command */

          case '0':
            run_command(q, 0, image_names[image_idx]);
            break;

          case '1':
            run_command(q, 1, image_names[image_idx]);
            break;

          case '2':
            run_command(q, 2, image_names[image_idx]);
            break;

          case '3':
            run_command(q, 3, image_names[image_idx]);
            break;

          case '4':
            run_command(q, 4, image_names[image_idx]);
            break;

          case '5':
            run_command(q, 5, image_names[image_idx]);
            break;

          case '6':
            run_command(q, 6, image_names[image_idx]);
            break;

          case '7':
            run_command(q, 7, image_names[image_idx]);
            break;

          case '8':
            run_command(q, 8, image_names[image_idx]);
            break;

          case '9':
            run_command(q, 9, image_names[image_idx]);
            break;

	  default:
	    exit_slideshow = FALSE;
	    break;
	 }
      }
    default:
      break;
  }
  if (exit_slideshow) {
      slide=0;
  }
}
