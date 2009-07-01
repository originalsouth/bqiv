%define name qiv
%define version 1.6
#%define release 0


Summary: Quick Image Viewer (qiv)
Name: %name
Version: %version
Release: %release
Copyright: GPL
Group: Amusements/Graphics
Source0: %name-%version.tgz
Source1: Public_key_pbarber.asc
#Patch0: %name-%version-%release_Default_path.patch
ExcludeArch: alpha sparc PPC
	# Just because I don't know if this
	# will compile or not.  Feel free
	# to remove this line if you are confident
	# you can build to these architectures. 
Buildroot: %{_tmppath}/%{name}-root
Prereq: sed imlib

%description
	Quick Image Viewer (qiv) is a very small and fast GDK/Imlib image
viewer designed to replace the classic image viewers like xv or
xloadimage. It features setting an image as an x11 background with a
user-definable background color, fullscreen viewing, a screensaver mode,
brightness/contrast/gamma correction, real transparency, zoom, slideshow,
and more. 

%prep 
%setup
%build
mv Makefile Makefile.old
sed "s#/usr/local#${RPM_BUILD_ROOT}#" Makefile.old >Makefile
make
%install
mkdir -p ${RPM_BUILD_ROOT}/usr
mkdir -p ${RPM_BUILD_ROOT}/usr/bin
mkdir -p ${RPM_BUILD_ROOT}/usr/man
mkdir -p ${RPM_BUILD_ROOT}/usr/man/man1
install -c -m 755 qiv $RPM_BUILD_ROOT/usr/bin/qiv
install -c -m 644 qiv.1 ${RPM_BUILD_ROOT}/usr/man/man1/qiv.1
gzip -9f ${RPM_BUILD_ROOT}/usr/man/man1/qiv.1
%pre
%post
%preun
%postun


%clean
rm -rf ${RPM_BUILD_ROOT}

%files
%defattr(-,root,root)
%doc README
%doc README.CHANGES
%doc README.COPYING
%doc README.INSTALL
%doc intro.jpg
/usr/bin/qiv
/usr/man/man1/qiv.1.gz
     
%changelog
* Thu Jan 18 2001 Adam Kopacz / kLoGraFX <adam.k@klografx.de>
- killed "pre8" string, -v (gzip) - we dont neet verbose mode *g*.
- release qiv 1.6
* Sat Jan 06 2001 [df]
- Put 'm' back in help text (oops!)
- Rewrote part of the extension filtering code.
- Move the program used to compress man pages
- (gzip -9vf) up to the top so that it can be
- disabled easily.
- Some HP-UX compilation fixes.
* Fri Jan 05 2001 [df]
- Change zoom_in/zoom_out so that they will break
- out of maxpect/scale_down mode, and make
- maxpect/scale_down override each other.
- A whole bunch of other cleanups.
- Updated keys help text for rotation/moving.
- Fixed bug that caused the image not to update
- when brightness/contrast/gamma was re-adjusted
- to 0/0/0 (with either the Enter key or B/C/G.)
- Merged README.SPEED_UP into README.
- Don't reset image size when rotating, and scale
- properly according to maxpect/scale_down.
- Fold filter.c into main.c.
- Big cleanup: move everything pertaining to the
- image into a qiv_image struct and pass that as
- an argument to most functions. Got rid of a lot
- of global variables.
* Thu Jan 04 2001 [df]
- gzip manpage with -9 (This is required for
- Debian packages, and is a good idea anywhere).
- Replace strcpy/strcat with strncpy/snprintf in
- rreaddir and move2trash to prevent a few more
- possible overflows.
- Clean up initialization code.
- Redo color handling: remove GDK deprecated
- functions and internal color list. Now you can
- use any color from X's rgb.txt or equivalent
- (however, you may have to change '123456' to
- '#123456').
* Wed Jan 03 2001 [df]
- Fix Makefile to handle fonts with spaces in their names.
- Use actual font height when drawing the help
- screen and statusbar instead of hardcoding it.
- Plug text_font and statusbar_background_gc memory leaks.
- Use helpstrs in show_help to avoid duplicating the help text.
- Change raw keysyms to symbolic names in event.c.
* Tue Jan 02 2001 [df]
- Use NULL for the sentinel value in image_extentions.
- Updated man page.
- Replace random() with rand(), since the
- HAVE_RANDOM check didn't really work
- properly, rand is guaranteed to be there,
- and it's acceptible for the amount of
- randomness we need. This facilitated some
- more Makefile improvements.
- Reorganized the source layout a bit.
- Optional code now lives in the lib/ dir.
- Resync Makefile.Solaris to Makefile, and make some minor fixes to each.
* Mon Jan 01 2001 [df]
- Updated -e to work properly with the new centering stuff. This is now 
- considered a feature, not a workaround ;-)
- Put x and y hints into the window when we
- create it (should eliminate the "window
- pops up and then centers itself" problem.)
- Removed last traces of wm_handle kludge, and set StaticGravity instead.
* Sun Dec 31 2000 [df]
- Only call gdk_show_window on load, not every single time we update.
- Replaced strsort with qsort. (should be faster, doesn't have an illegal 
- name ;-)
- Replaced sprintf with snprintf and g_snprintf to prevent any possible
- overflows.
- Cleaned up some extraneous blank lines in output.
* Tue Dec 05 2000 [ak]
- another PPC fix.
* Mon Nov 27 2000 [ak]
- changed "char c" to "int c" in options.c not worked with char on Linux/ppc
- (mailed by Jiri Masik <masik@darbujan.fzu.cz>)
* Sat Sep 23 2000 Adam Kopacz / kLoGraFX  <adam.k@klografx.de>
- Released v1.5 - new homepage: www.klografx.net/qiv/
* Sat Aug 5 2000 Adam Kopacz / kLoGraFX  <adam.k@klografx.de>
- cleaned up readmes..
* Sat Aug 5 2000 Adam Kopacz / kLoGraFX  <adam.k@klografx.de>
- changed up/down and left/right moving of the image, now the stuff works 
- like in ACDSee (Win).
* Sun Jul 30 2000 Adam Kopacz / kLoGraFX  <adam.k@klografx.de>
- added dlimits for moving.
* Fri Jul 28 2000 Adam Kopacz / kLoGraFX  <adam.k@klografx.de>
- added solaris 2.5 patch from francois.petitjean@bureauveritas.com
* Wed Jul 25 2000 Adam Kopacz / kLoGraFX  <adam.k@klografx.de>
- added image moving with left/right/up/down-keys! :)
- tried to fix reset_display_settings (b/g/c) but it dont work.. hm
- added zoom_out limits (64x64). changed keys: now K and L rotate the image.
* Sun Jun 25 2000 Darren Smith <darren.smith@verinet.com>
- Sending a SIGUSR1 to qiv will cause the program to flip to next
- picture. SIGUSR2 will move to previous.
* Thu Jun 15 2000 Adam Kopacz / kLoGraFX  <adam.k@klografx.de>
- re-added b/c/g to titlebar/infoline. Painted new intro.jpg *g*
* Fri Apr 29 2000 image shuffling option (--shuffle) provides 
- random image order while preserving next and previous functionality.
* Thu Apr 13 2000 Adam Kopacz / kLoGraFX  <adam.k@klografx.de>
- cleaned up infoscreen/added more info and kicked "show_info".
* Tue Apr 3 2000 Darren Smith <darren.smith@verinet.com>
- The delay time in slideshow mode used to include the time
- it took for a picture to load. Pressing '?' in non-fullscreen mode
- results in the help being printed to stdout.
