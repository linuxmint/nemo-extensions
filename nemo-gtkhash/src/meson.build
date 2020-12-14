subdir('hash')
# if build_nautilus
#     subdir('nautilus')
# endif
# if build_nemo
#     subdir('nemo')
# endif
# if build_thunar
#     subdir('thunar')
# endif


if build_gtkhash
    executable('gtkhash',
        'callbacks.c',
        'gui.c',
        'hash.c',
        'list.c',
        'main.c',
        'prefs.c',
        include_directories: rootInclude,
        dependencies: [
            gtk3,
        ],
        # $(GTK_CFLAGS) \
        # -DDATADIR="\"$(pkgdatadir)\"" \
        # -DLOCALEDIR="\"$(localedir)\""
        install: true,
    )
endif
