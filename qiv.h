#include <gdk/gdk.h>
#include <gdk_imlib.h>
#include <unistd.h>
#include <stdlib.h>
#ifdef GTD_XINERAMA
# include <X11/Xlib.h>
# include <X11/extensions/Xinerama.h>
#endif

#define VERSION "2.1-pre10"
#define VERSION_FULL "QIV - Quick Image Viewer v2.0 - http://www.klografx.net/qiv/"
#define TRASH_DIR ".qiv-trash"
#define SELECT_DIR ".qiv-select"
#define SLIDE_DELAY 3000 /* milliseconds */
#define IMAGE_BG "black"
#define STATUSBAR_BG "#FFB900"
#define ERROR_BG "#0000FF"
#define DEFAULT_BRIGHTNESS 256
#define DEFAULT_CONTRAST 256
#define DEFAULT_GAMMA 256
#define BUF_LEN 1024

/* FILENAME_LEN is the maximum length of any path/filename that can be
 * handled.  MAX_DELETE determines how many items can be placed into
 * the kill-ring for undelete handling. */

#define FILENAME_LEN 1024
#define MAX_DELETE 1024

typedef struct _qiv_image {
    GdkImlibColorModifier mod; /* Image modifier (for brightness..) */
    GdkPixmap *p; /* Pixmap of the image to display */
    GdkImlibImage *im; /* Image */
    GdkWindow *win; /* Main window for windowed and fullscreen mode */
    int error; /* 1 if Imlib couldn't load image */
    gint win_x, win_y, win_w, win_h; /* window co-ordinates */
    gint orig_w, orig_h; /* Size of original image in pixels */
    GdkGC *bg_gc;     /* image window background */
    GdkGC *text_gc;   /* statusbar text color */
    GdkGC *status_gc; /* statusbar background */

    /* These are used to work out how to redraw in fullscreen mode */
    gint win_ox, win_oy, win_ow, win_oh; /* co-ordinates currently drawn at */
    gint text_ow, text_oh; /* old size of the statusbar */
    int statusbar_was_on; /* true if statusbar was visible last frame */
    int exposed;		/* window became visible */
    int drag;			/* user is currently dragging the image */
    double drag_start_x, drag_start_y; /* position of cursor at drag start */
    int drag_win_x, drag_win_y; /* position of win at drag start */
} qiv_image;

typedef struct _qiv_deletedfile {
    char *filename, *trashfile;
    int pos;
} qiv_deletedfile;

extern int		first;
extern char		infotext[BUF_LEN];
extern GMainLoop	*qiv_main_loop;
extern gint		screen_x, screen_y;
extern GdkFont		*text_font;
extern GdkColormap	*cmap;
extern char		*image_bg_spec;
extern GdkColor		image_bg;
extern GdkColor		text_bg;
extern GdkColor		error_bg;
extern int		images;
extern char		**image_names;
extern int		image_idx;
extern int		max_image_cnt;
extern time_t		current_mtime;
extern qiv_deletedfile	*deleted_files;
extern int		delete_idx;
extern char     select_dir[FILENAME_LEN];

extern int		filter;
extern gint		center;
extern gint		default_brightness;
extern gint		default_contrast;
extern gint		default_gamma;
extern gint		delay;
extern int		readonly;
extern int		random_order;
extern int		random_replace;
extern int		fullscreen;
extern int		maxpect;
extern int		statusbar_fullscreen;
extern int		statusbar_window;
extern int		slide;
extern int		scale_down;
extern int		to_root;
extern int		to_root_t;
extern int		to_root_s;
extern int		transparency;
extern int		do_grab;
extern int      disable_grab;
extern int		max_rand_num;
extern int		fixed_window_size;
extern int		fixed_zoom_factor;
extern int		zoom_factor;
extern int		watch_file;

extern const char	*helpstrs[], **helpkeys, *image_extensions[];

#ifdef GTD_XINERAMA
extern XineramaScreenInfo preferred_screen[1];
extern XineramaScreenInfo statusbar_screen[1];
#endif

/* main.c */

extern void qiv_exit(int);
extern void qiv_load_image();

/* image.c */

/* Modes for update_image */
#define REDRAW 0
#define MOVED  1
#define ZOOMED 2
#define FULL_REDRAW 3

extern void qiv_load_image(qiv_image *);
extern void set_desktop_image(qiv_image *);
extern void zoom_in(qiv_image *);
extern void zoom_out(qiv_image *);
extern void zoom_maxpect(qiv_image *);
extern void reload_image(qiv_image *q);
extern void reset_coords(qiv_image *);
extern void check_size(qiv_image *, gint);
extern void render_to_pixmap(qiv_image *, double *);
extern void update_image(qiv_image *, int);
extern void reset_mod(qiv_image *);
extern void destroy_image(qiv_image *q);
extern void center_image(qiv_image *q);
extern void correct_image_position(qiv_image *q);
extern void hide_cursor(qiv_image *q);
extern void show_cursor(qiv_image *q);

/* event.c */

extern void qiv_handle_event(GdkEvent *, gpointer);

/* options.c */

extern void options_read(int, char **, qiv_image *);

/* utils.c */

extern int  move2trash(void);
extern int  copy2select(void);
extern int  undelete_image(void);
extern void jump2image(char *);
extern void run_command(qiv_image *, char, char *, int *, const char ***);
extern void finish(int);
extern void next_image(int);
extern int checked_atoi(const char *);
extern void usage(char *, int);
extern void show_help(char *, int);
extern int get_random(int, int, int);
extern gboolean color_alloc(const char *, GdkColor *);
extern void swap(int *, int *);
#define myround qiv_round
extern int myround(double);
extern gboolean qiv_watch_file (gpointer);
