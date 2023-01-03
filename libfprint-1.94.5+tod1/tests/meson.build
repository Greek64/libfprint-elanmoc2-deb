envs = environment()
# Enable debug messages and abort on warnings
envs.set('G_DEBUG', 'fatal-warnings')
envs.set('G_MESSAGES_DEBUG', 'all')

# Setup paths
envs.set('MESON_SOURCE_ROOT', meson.source_root())
envs.set('MESON_BUILD_ROOT', meson.build_root())
envs.prepend('LD_LIBRARY_PATH', join_paths(meson.build_root(), 'libfprint'))

# Set FP_DEVICE_EMULATION so that drivers can adapt (e.g. to use fixed
# random numbers rather than proper ones)
envs.set('FP_DEVICE_EMULATION', '1')

# Set a colon-separated list of native drivers we enable in tests
envs.set('FP_DRIVERS_WHITELIST', ':'.join([
    'virtual_image',
    'virtual_device',
    'virtual_device_storage',
]))

envs.set('NO_AT_BRIDGE', '1')

drivers_tests = [
    'aes2501',
    'aes3500',
    'elan',
    'elan-cobo',
    'elanmoc',
    'elanspi',
    'synaptics',
    'upektc_img',
    'uru4000-msv2',
    'uru4000-4500',
    'vfs0050',
    'vfs301',
    'vfs5011',
    'vfs7552',
    'goodixmoc',
    'nb1010',
    'egis0570',
    'fpcmoc',
]

if get_option('introspection')
  conf = configuration_data()
  conf.set('SRCDIR', meson.source_root())
  conf.set('BUILDDIR', meson.build_root())
  configure_file(configuration: conf,
      input: 'create-driver-test.py.in',
      output: 'create-driver-test.py')
endif

if get_option('introspection')
    envs.prepend('GI_TYPELIB_PATH', join_paths(meson.build_root(), 'libfprint'))
    virtual_devices_tests = [
        'virtual-image',
        'virtual-device',
    ]

    python3 = find_program('python3')
    unittest_inspector = find_program('unittest_inspector.py')
    umockdev_test = find_program('umockdev-test.py')

    foreach vdtest: virtual_devices_tests
        driver_name = '_'.join(vdtest.split('-'))
        if driver_name in drivers
            base_args = files(vdtest + '.py')
            suite = ['virtual-driver']

            r = run_command(unittest_inspector, files(vdtest + '.py'), check: true)
            unit_tests = r.stdout().strip().split('\n')

            if r.returncode() == 0 and unit_tests.length() > 0
                suite += vdtest
            else
                unit_tests = [vdtest]
            endif

            foreach ut: unit_tests
                ut_suite = suite
                ut_args = base_args
                if unit_tests.length() > 1
                    ut_args += ut
                    ut_suite += ut.split('.')[0]
                endif
                test(ut,
                    python3,
                    args: ut_args,
                    suite: ut_suite,
                    depends: libfprint_typelib,
                    env: envs,
                )
            endforeach
        else
            test(vdtest,
                find_program('sh'),
                args: ['-c', 'exit 77']
            )
        endif
    endforeach

    foreach driver_test: drivers_tests
        driver_name = driver_test.split('-')[0]
        driver_envs = envs
        driver_envs.set('FP_DRIVERS_WHITELIST', driver_name)

        if (driver_name in supported_drivers and
            gusb_dep.version().version_compare('>= 0.3.0'))
            test(driver_test,
                python3,
                args: [
                    umockdev_test.full_path(),
                    meson.current_source_dir() / driver_test,
                ],
                env: driver_envs,
                suite: ['drivers'],
                timeout: 15,
                depends: libfprint_typelib,
            )
        else
            test(driver_test,
                find_program('sh'),
                args: ['-c', 'exit 77']
            )
        endif
    endforeach
else
    warning('Skipping all driver tests as introspection bindings are missing')
    test('virtual-image',
        find_program('sh'),
        args: ['-c', 'exit 77']
    )

    foreach driver_test: drivers_tests
        test(driver_test,
            find_program('sh'),
            args: ['-c', 'exit 77']
        )
    endforeach
endif

test_utils = static_library('fprint-test-utils',
    sources: [
        'test-utils.c',
        'test-device-fake.c',
    ],
    dependencies: libfprint_private_dep,
    install: false)

unit_tests = [
    'fpi-device',
    'fpi-ssm',
    'fpi-assembling',
]

if 'virtual_image' in drivers
    unit_tests += [
        'fp-context',
        'fp-device',
    ]
endif

unit_tests_deps = { 'fpi-assembling' : [cairo_dep] }

test_config = configuration_data()
test_config.set_quoted('SOURCE_ROOT', meson.source_root())
test_config_h = configure_file(output: 'test-config.h', configuration: test_config)

foreach test_name: unit_tests
    if unit_tests_deps.has_key(test_name)
        missing_deps = false
        foreach dep: unit_tests_deps[test_name]
            if not dep.found()
                missing_deps = true
                break
            endif
        endforeach

        if missing_deps
            # Create a dummy test that always skips instead
            warning('Test @0@ cannot be compiled due to missing dependencies'.format(test_name))
            test(test_name,
                find_program('sh'),
                suite: ['unit-tests'],
                args: ['-c', 'exit 77'],
            )
            continue
        endif
        extra_deps = unit_tests_deps[test_name]
    else
        extra_deps = []
    endif

    basename = 'test-' + test_name
    test_exe = executable(basename,
        sources: [basename + '.c', test_config_h],
        dependencies: [ libfprint_private_dep ] + extra_deps,
        c_args: common_cflags,
        link_with: test_utils,
    )
    test(test_name,
        test_exe,
        suite: ['unit-tests'],
        env: envs,
    )
endforeach

# Run udev rule generator with fatal warnings
envs.set('UDEV_HWDB', udev_hwdb.full_path())
envs.set('UDEV_HWDB_CHECK_CONTENTS', default_drivers_are_enabled ? '1' : '0')
test('udev-hwdb',
     find_program('test-generated-hwdb.sh'),
     depends: udev_hwdb,
     env: envs)

gdb = find_program('gdb', required: false)
if gdb.found()
    libfprint_wrapper = [
        gdb.path(),
        '-batch',
        '-ex', 'run',
        '--args',
    ]
    add_test_setup('gdb',
        timeout_multiplier: 1000,
        exe_wrapper: libfprint_wrapper,
        env: [
            'LIBFPRINT_TEST_WRAPPER=' + ' '.join(libfprint_wrapper),
        ])
endif

valgrind = find_program('valgrind', required: false)
if valgrind.found()
    glib_share = glib_dep.get_pkgconfig_variable('prefix') / 'share' / glib_dep.name()
    glib_suppressions = glib_share + '/valgrind/glib.supp'
    libfprint_suppressions = '@0@/@1@'.format(meson.source_root(),
        files('libfprint.supp')[0])
    python_suppressions = '@0@/@1@'.format(meson.source_root(),
        files('valgrind-python.supp')[0])
    libfprint_wrapper = [
        valgrind.path(),
        '--tool=memcheck',
        '--leak-check=full',
        '--leak-resolution=high',
        '--error-exitcode=1',
        '--errors-for-leak-kinds=definite',
        '--track-origins=yes',
        '--show-leak-kinds=definite,possible',
        '--show-error-list=yes',
        '--suppressions=' + libfprint_suppressions,
        '--suppressions=' + glib_suppressions,
        '--suppressions=' + python_suppressions,
    ]
    add_test_setup('valgrind',
        timeout_multiplier: 15,
        exe_wrapper: libfprint_wrapper,
        env: [
            'G_SLICE=always-malloc',
            'UNDER_VALGRIND=1',
            'LIBFPRINT_TEST_WRAPPER=' + ' '.join(libfprint_wrapper),
        ])
endif

if get_option('tod')
    tod_test_driver_name = 'fake_test_dev_tod'
    tod_ssm_test_driver_name = 'ssm_test_dev_tod'
    tod_envs = envs
    tod_envs.set('FP_TOD_KEEP_MODULES_OPEN', 'TRUE')
    tod_envs.set('FP_VIRTUAL_FAKE_DEVICE', 'yes')
    tod_envs.set('FP_TOD_TEST_DRIVER_NAME', tod_test_driver_name)
    tod_envs.prepend('LD_LIBRARY_PATH',
        meson.build_root() / 'libfprint',
        meson.build_root() / 'libfprint' / 'tod')
    tod_subversion = meson.project_version().split('+tod')[0]
    tod_c_args = [
        '-DTEST_TOD_DRIVER=1',
        '-DTOD_CURRENT_VERSION=@0@'.format(tod_soversion),
        '-DTOD_CURRENT_SUBVERSION="@0@"'.format(tod_subversion),
    ]

    fake_driver = shared_module('device-fake-tod-driver',
        sources: [
            'test-device-fake.c',
            'test-device-fake-tod.c',
        ],
        c_args: tod_c_args,
        link_with: [
            libfprint_tod,
        ],
        include_directories: include_directories('../libfprint'),
        dependencies: deps,
        install: false
    )

    fp_todv1_enums = gnome.mkenums_simple('fp-todv1-enums',
        sources: [
            'tod-drivers/base-fp-device.h',
            'tod-drivers/base-fp-print.h',
            'tod-drivers/base-fpi-device.h',
            'tod-drivers/base-fpi-image.h',
            'tod-drivers/base-fpi-image-device.h',
            'tod-drivers/base-fpi-spi.h',
            'tod-drivers/base-fpi-usb.h',
        ],
        install_header: false)

    test_utils_tod = static_library('fprint-test-utils-tod',
        sources: [
            'test-utils.c',
            'test-utils-tod.c',
            fp_todv1_enums,
        ],
        include_directories: 'tod-drivers',
        dependencies: libfprint_private_dep,
        install: false)

    tod_standalone_tests = [
        'fp-todv1-types',
    ]

    tod_drivers_tests = [
        'fp-context-tod',
        'fp-device-tod',
        'fpi-device',
    ]

    tod_driver_infos = {
        tod_test_driver_name + '_current': {
            'tod-driver': tod_test_driver_name,
            'tod-dir': meson.current_build_dir(),
            'supported-tests': tod_drivers_tests,
        }
    }

    if host_machine.cpu_family() == 'x86_64'
        tod_test_versions = [
            'v1+1.90.1',
            'v1+1.90.2',
            'v1+1.90.3',
            'v1+1.90.5',
            'v1+1.94.0',
            'v1+1.94.3',
        ]

        foreach tod_version: tod_test_versions
            tod_dir = meson.current_source_dir() / 'tod-drivers' / '-'.join([
                'tod', host_machine.cpu_family(), tod_version
            ])
            tod_driver_infos += {
                tod_test_driver_name + '_' + tod_version: {
                    'tod-driver': tod_test_driver_name,
                    'tod-dir': tod_dir,
                    'supported-tests': tod_drivers_tests,
                }
            }

            tod_driver_infos += {
                tod_ssm_test_driver_name + '_' + tod_version: {
                    'tod-driver': tod_ssm_test_driver_name,
                    'tod-dir': tod_dir,
                    'supported-tests': [
                        'fp-context-tod',
                    ],
                }
            }
        endforeach
    endif

    foreach test_name: tod_drivers_tests + tod_standalone_tests
        basename = 'test-' + test_name
        sufix = test_name.endswith('-tod') ? '' : '-tod'
        tod_test_name = test_name + sufix
        tod_suites = ['unit-tests', 'tod']
        test_exe = executable(basename + sufix,
            sources: basename + '.c',
            dependencies: libfprint_private_dep,
            c_args: [
                common_cflags,
                tod_c_args,
            ],
            link_with: test_utils_tod,
        )

        foreach tod_driver, tod_driver_info : tod_driver_infos
            if test_name not in tod_driver_info.get('supported-tests')
                continue
            endif
            tod_test_envs = tod_envs
            tod_test_envs.prepend('FP_DRIVERS_WHITELIST', tod_driver)
            tod_test_envs.set('FP_TOD_DRIVERS_DIR', tod_driver_info.get('tod-dir'))
            tod_test_envs.set('FP_TOD_TEST_DRIVER_NAME', tod_driver)

            test(tod_test_name + '-' + tod_driver,
                test_exe,
                suite: tod_suites + [tod_driver],
                env: tod_test_envs,
                depends: fake_driver,
            )
        endforeach

        if test_name in tod_standalone_tests
            test(tod_test_name,
                test_exe,
                suite: tod_suites,
                env: tod_envs,
            )
        endif
    endforeach

    if find_program('objdump', required: false).found()
        check_libs_symbols = find_program(meson.source_root() /
            'libfprint/tod/tests/check-library-symbols.sh')
        test('check-tod-lib-sybmbols',
            check_libs_symbols,
            args: [libfprint.full_path(), libfprint_tod.full_path()],
            depends: [libfprint, libfprint_tod],
            suite: ['abi-check', tod_suites ],
        )
    endif
endif
