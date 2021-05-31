libnemo_image_converter_sources = [
    'image-converter.c',
    'nemo-image-converter.c',
    'nemo-image-resizer.c',
    'nemo-image-rotator.c',
]

libnemo_image_converter = library('nemo-image-converter',
    libnemo_image_converter_sources,
    include_directories: rootInclude,
    dependencies: [
        libnemo,
        gtk3,
    ],
    install: true,
    install_dir: libnemo_extension_dir,
)
