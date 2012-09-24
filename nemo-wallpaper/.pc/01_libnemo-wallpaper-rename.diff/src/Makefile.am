## Process this file with automake to produce Makefile.in

## Created by Anjuta - will be overwritten
## If you don't want it to overwrite it,
## 	Please disable it in the Anjuta project configuration

INCLUDES =
libdir = @libdir@/nautilus/extensions-1.0

AM_CFLAGS =\
	`pkg-config --cflags libnautilus-extension`\
	 -Wall

lib_LTLIBRARIES = libnautilus-wallpaper.la

libnautilus_wallpaper_la_SOURCES = \
	nautilus-nwe.c\
	nautilus-nwe.h\
	nautilus-wallpaper-command.c

libnautilus_wallpaper_la_LDFLAGS = \
	`pkg-config --libs libnautilus-extension`

libnautilus_wallpaper_la_LIBADD = 

