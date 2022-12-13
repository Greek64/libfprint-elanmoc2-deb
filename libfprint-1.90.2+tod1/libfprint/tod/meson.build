tod_soversion = 1
tod_subpath = versioned_libname / 'tod-@0@'.format(tod_soversion)
tod_modules_prefix = get_option('libdir') / tod_subpath

tod_conf = configuration_data()
tod_conf.set_quoted('TOD_DRIVERS_DIR',
    get_option('prefix') / get_option('libdir') / tod_subpath)
configure_file(output: 'tod-config.h', configuration: tod_conf)

gmodule_dep = dependency('gmodule-2.0', version: '>=' + glib_min_version)
deps += gmodule_dep

mapfile = files('libfprint-tod.ver')

libfprint_tod_private = static_library('fprint-tod-private',
    sources: [
        'tod-shared-loader.c',
    ],
    include_directories: include_directories('..'),
    link_with: libfprint_private,
    dependencies: deps,
    install: false,
)

tod_sources = []
foreach source: libfprint_private_sources
    tod_sources += '..' / source
endforeach

libfprint_tod = library(versioned_libname.split('lib')[1] + '-tod',
    sources: [
        tod_sources,
    ],
    soversion: tod_soversion,
    include_directories: include_directories('..'),
    link_args: [
        '-Wl,--version-script,@0@/@1@'.format(meson.source_root(), mapfile[0]),
        '-Wl,--unresolved-symbols=ignore-in-object-files'
    ],
    link_depends: mapfile,
    link_with: [libfprint_private],
    dependencies: deps,
    install: true)

deps += declare_dependency(
    link_with: [
        libfprint_tod,
        libfprint_tod_private,
    ]
)

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
