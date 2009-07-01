#ifndef MAIN_H
#define MAIN_H

int		first = 1; /* TRUE iff this is first image */
char		infotext[BUF_LEN];
GdkCursor	*cursor, *visible_cursor, *invisible_cursor;
GMainLoop	*qiv_main_loop;
gint		screen_x, screen_y; /* Size of the screen in pixels */
GdkFont		*text_font; /* font for statusbar and help text */
GdkColormap	*cmap; /* global colormap */
char		*image_bg_spec = IMAGE_BG;
GdkColor	image_bg; /* default background */
GdkColor	text_bg; /* statusbar and help backgrounf */
GdkColor	error_bg; /* for the error window/screen */
int		images;	/* Number of images in current collection */
char		**image_names; /* Filenames of the images */
int		image_idx; /* Index of current image displayed. 0 = 1st image */
int		max_image_cnt; /* # images currently allocated into arrays */
qiv_deletedfile *deleted_files;
int		delete_idx;

/* Options and such */

int	filter = FILTER;
gint	center = CENTER;
gint	default_brightness = DEFAULT_BRIGHTNESS;
gint	default_contrast = DEFAULT_CONTRAST;
gint	default_gamma = DEFAULT_GAMMA;
gint	delay = SLIDE_DELAY; /* delay in slideshow mode in seconds */
int	random_order; /* TRUE if random delay in slideshow */
int	random_replace = 1; /* random with replacement by default */
int	fullscreen; /* TRUE if fullscreen mode */
int	maxpect; /* TRUE if autozoom (fit-to-screen) mode */
int	statusbar = 1; /* TRUE if statusbar is turned on (default) */
int	slide; /* 1=slide show running */
int	scale_down; /* resize down if image x/y > screen */
int	to_root; /* display on root (centered) */
int	to_root_t; /* display on root (tiled) */
int	to_root_s; /* display on root (stretched) */
int	transparency; /* transparency on/off */
int	do_grab; /* grab keboard/pointer (default off) */
int	max_rand_num; /* the largest random number range we will ask for */
int	width_fix_size = 0; /* window width fix size/off */

/* Used for the ? key in fullscreen */

const char *helpstrs[] =
{
    "Quick Image Viewer (qiv) Keys:",
    "",
    "space/PgDn/left mouse/wheel down        next picture",
    "backspace/PgUp/right mouse/wheel up     previous picture",
    "q/ESC/middle mouse                      exit",
    "",
    "0-9                Run 'qiv-command <key> <current-img>'",
    "?                  show keys (in fullscreen mode)",
    "d/D/del            move picture to .qiv-trash",
    "u                  undelete the previously trashed image",
    "+/=                zoom in (10%)",
    "-                  zoom out (10%)",
    "f                  fullscreen mode on/off",
    "m                  scale to screen size on/off",
    "t                  scale down on/off",
    "s                  slide show on/off",
    "r                  random order on/off",
    "b                  - brightness",
    "B                  + brightness",
    "c                  - contrast",
    "C                  + contrast",
    "g                  - gamma",
    "G                  + gamma",
    "arrow keys         move image (in fullscreen mode)",
    "h                  flip horizontal",
    "v                  flip vertical",
    "k                  rotate right",
    "l                  rotate left",
    "jtx<return>        jump to image number x",
    "jfx<return>        jump forward x images",
    "jbx<return>        jump backward x images",
    "enter/return       reset zoom and color settings",
    "i                  statusbar on/off",
    "x                  center image on background",
    "y                  tile image on background",
    "z                  stretch image on background",
    NULL
};

/* For --help output, we'll skip the first two lines. */

const char **helpkeys = helpstrs+2;

/* Used for filtering */

const char *image_extensions[] = {
#ifdef EXTN_JPEG
    ".jpg",".jpeg",
#endif
#ifdef EXTN_GIF
    ".gif",
#endif
#ifdef EXTN_TIFF
    ".tif",".tiff",
#endif
#ifdef EXTN_XPM
    ".xpm",
#endif
#ifdef EXTN_XBM
    ".xbm",
#endif
#ifdef EXTN_PNG
    ".png",".pjpeg",
#endif
#ifdef EXTN_PPM
    ".ppm",
#endif
#ifdef EXTN_PNM
    ".pnm",
#endif
#ifdef EXTN_PGM
    ".pgm",
#endif
#ifdef EXTN_PCX
    ".pcx",
#endif
#ifdef EXTN_BMP
    ".bmp",
#endif
#ifdef EXTN_EIM
    ".eim",
#endif
#ifdef EXTN_TGA
    ".tga",
#endif
    NULL
};

#endif /* MAIN_H */
