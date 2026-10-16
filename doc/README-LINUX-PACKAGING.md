Linux Packaging
===============

## Tarball (.tgz)

For any Linux distribution

 - Strip the executable from debug info!

`strip bin/picross_solver`

 - Tar zip the folder:

`tar --owner=root --group=root --numeric-owner --mtime='2026-10-14' -czvf picross-solver-0.4.0-linux-x86_64.tgz picross-solver-0.4.0-linux-x86_64/`

 - Check:

`tar -tzvf picross-solver-0.4.0-linux-x86_64.tgz`

## Debian (.deb)

For Debian, Ubuntu...

 - Populate the following folder hierarchy, with the contents in the resource repository

```
├── DEBIAN
│   ├── control
│   └── postinst
└── usr
    ├── bin
    │   └── picross_solver
    └── share
        ├── applications
        │   └── picross-solver.desktop
        └── icons
            └── hicolor
                ├── 128x128/apps
                │   └── picross-solver.png
                ├── 16x16/apps
                │   └── picross-solver.png
                ├── 24x24/apps
                │   └── picross-solver.png
                ├── 256x256/apps
                │   └── picross-solver.png
                ├── 32x32/apps
                │   └── picross-solver.png
                ├── 48x48/apps
                │   └── picross-solver.png
                └── 64x64/apps
                    └── picross-solver.png
```

 - Update the `control` and `picross-solver.desktop` files.

 - Strip the binary of debug symbols: `strip usr/bin/picross_solver`.

 - For the dependencies, either manually with `ldd picross_solver` then `dpkg -S /usr/lib/x86_64-linux-gnu/libc.so.6`. Or, using `dpkg-shlibdeps`:

```
mkdir debian && touch debian/control
dpkg-shlibdeps -O -e usr/bin/picross_solver
```

 - Create the package:

`dpkg-deb --build --root-owner-group picross-solver_0.4.0_amd64/`

 - Check it with `lintian` (sudo apt install lintian).

## RPM

For Fedora, RedHat, AlmaLinux, etc.

 - Populate the following `rpmbuild` folder:

```
rpmbuild/
├── BUILD
├── RPMS
├── SOURCES
│   └── picross-solver-0.4.0
│       ├── LICENSE
│       ├── THIRD_PARTY_LICENSES
│       ├── bin
│       │   └── picross_solver
│       └── share
│           ├── applications
│           │   └── picross-solver.desktop
│           └── icons
│               └── hicolor
│                   ├── 128x128/apps
│                   │       └── picross-solver.png
│                   ├── 16x16/apps
│                   │       └── picross-solver.png
│                   ├── 24x24/apps
│                   │       └── picross-solver.png
│                   ├── 256x256/apps
│                   │       └── picross-solver.png
│                   ├── 32x32/apps
│                   │       └── picross-solver.png
│                   ├── 48x48/apps
│                   │       └── picross-solver.png
│                   └── 64x64/apps
│                           └── picross-solver.png
├── SPECS
│   └── picross-solver.spec
└── SRPMS
```

 - Strip the binary.

 - `picross-solver.desktop` is identical to the one used for the .deb package.

 - Edit the spec file `picross-solver.spec`

 - Build the RPM package (`_topdir` must be an absolute path, without the ~ shortcut):

```
rpmbuild -bb --nodeps --define "_topdir /home/pierre/repos/.../packages/rpmbuild" ./rpmbuild/SPECS/picross-solver.spec

```

 - Check the RPM package with `rpmlint`.
