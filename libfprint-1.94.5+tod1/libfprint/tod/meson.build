tod_soversion = 1
tod_subpath = versioned_libname / 'tod-@0@'.format(tod_soversion)
tod_modules_prefix = get_option('libdir') / tod_subpath

tod_conf = configuration_data()
tod_conf.set_quoted('TOD_DRIVERS_DIR',
    get_option('prefix') / get_option('libdir') / tod_subpath)
configure_file(output: 'tod-config.h', configuration: tod_conf)

gmodule_dep = dependency('gmodule-2.0', version: '>=' + glib_min_version)
deps += gmodule_dep

mapfile = configure_file(input: 'libfprint-tod.ver.in',
    output: 'libfprint-tod.ver',
    configuration: {
    'tod_soversion' : tod_soversion,
})

libfprint_tod_private = static_library('fprint-tod-private',
    sources: [
        'tod-shared-loader.c',
        'tod-goodix-wrapper.c',
    ],
    include_directories: include_directories('..'),
    link_with: libfprint_private,
    dependencies: deps,
    install: false,
)

tod_sources = [
    'tod-wrappers.c',
    'tod-symbols.h',
]
foreach source: libfprint_private_sources
    tod_sources += '..' / source
endforeach

libfprint_tod = shared_library(versioned_libname.split('lib')[1] + '-tod',
    c_args: [
        '-DTOD_LIBRARY=1',
        '-include', '@0@'.format(files('tod-symbols.h')[0]),
    ],
    sources: [
        tod_sources,
    ],
    soversion: tod_soversion,
    include_directories: include_directories('..'),
    link_args: [
        '-Wl,--version-script,@0@'.format(mapfile),
        '-Wl,--unresolved-symbols=ignore-in-object-files'
    ],
    link_depends: mapfile,
    link_whole: [libfprint_tod_private],
    dependencies: deps,
    install: true)

tod_dep = declare_dependency(
    link_with: [
        libfprint_tod,
        libfprint_tod_private,
    ])
deps += tod_dep

pkgconfig = import('pkgconfig')
pkgconfig.generate(libfprint_tod,
    name: versioned_libname + '-tod',
    filebase: '@0@-tod-@1@'.format(versioned_libname, tod_soversion),
    description: 'Private Libfprint Touch Drivers API',
    version: meson.project_version() + '+tod@0@'.format(tod_soversion),
    subdirs: tod_subpath,
    requires_private: [
        versioned_libname,
    ],
    variables: [
        'tod_driversdir=${libdir}/@0@'.format(tod_subpath)
    ]
)

tod_local_headers = [
    'tod-macros.h',
]

tod_headers = []
extra_libfprint_headers = [
    'drivers_api.h',
]

foreach header: libfprint_private_headers + extra_libfprint_headers
    tod_headers += '..' / header
endforeach

custom_target('tod_fpi_enums_headers',
  depends: fpi_enums,
  input: fpi_enums_h,
  output: 'fpi-enums.h',
  command: ['cp', '@INPUT@', '-v', '@OUTPUT@'],
  install: true,
  install_dir: get_option('includedir') / tod_subpath)

install_headers(tod_headers,
    subdir: tod_subpath
)

install_headers(tod_local_headers,
    subdir: tod_subpath / 'tod',
)

# Replace the libfprint private instance with the TOD one in this case as we
# contain already all the symbols.
libfprint_private = libfprint_tod
