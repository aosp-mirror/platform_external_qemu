# Remaining steps to getting the full system

Required components:

1. Gralloc device opened
2. Queue primed
3. SF window created
4. SF initialized
5. HWC thread started
6. SF thread started

Required mechanisms?

1. Some way to initialize the emugl config, android pipe, etc.
2. Where does this virtual display device live? Which DLL?
3. Some way to set a window 'current' once it is created, or when eglMakeCurrent is referenced (or when a window is acquired)
4. SF opengl composition

We're going to be mostly running tests with this, so also, it's better to add the test code with our own build scripts.

If we need to run real apps as an actual GLES frontend, consider using dyn linking.

