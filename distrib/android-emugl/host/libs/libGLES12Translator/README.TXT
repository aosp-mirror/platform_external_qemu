=====gles1=====gles1>>>>>Emulator GLESv1->2 Translator>>>>>gles2=====gles2=====

Not every rendering backend that the emulator supports will support GLESv1.
For example, ANGLE only supports GLESv2.

We need a GLESv1->GLESv2 translation layer in order to take care of GLESv1 use
cases.

This library, libGLES12Translator, is a general solution where if our
currently-utilized GLES backend (such as ANGLE) does not provide libGLES_CM.dll
(the GLESv1 dll), libGLES12Translator will be used instead, and hook into the
underlying libEGL.dll and libGLESv2.dll.

=====gles1=====gles1>>>>>Where this came from originally>>>gles2=====gles2=====

The GLESv1->2 translator in this library is derived from the
mods/graphics_translation in ARC, Android Runtime for Chrome.

ARC is for running Android apps in Chrome and has a GLESv1->2 translator that
is used to translate Android GLESv1 apps to run on the underlying WebGL (or
ANGLE) which is only GLESv2.

More information + the original source can be found from the Chromium git repo
at

https://chromium.googlesource.com/arc/arc/

under the directory mods/graphics_translation.

A bit of history for code archaeology purposes:

This is all based on a previous effort to port GLESv1->2 translator to the
emulator.  That effort itself was based off of the master branch of the ARC
repo around that time.

The closest version of the arc repository has been the master branch as of
around the end of September 2015. Not many changes were happening in ARC around
that time; we can consider this October 6, 2015 version of arc:

https://chromium.googlesource.com/arc/arc/+/016806e172e7149ba42e9b69e0ec876b1e898a4e

as the canonical ARC version off of which we base this GLESv1->2 translator.
In fact, graphics_translation itself had no changes whatsover since Sept 10,
2015 until at least June 23, 2016.

=====gles1=====gles1>>>>>An EGL-less solution>>>>>>>>>>>>>>gles2=====gles2=====

There is one heavy modification to the translator that these CLs do, and it is
to remove the EGL machinery from graphics_translation.  The egl/ subdirectory
of the original graphics_translation has been completely removed.

The idea is that in libOpenglRender on the host, we already have sufficient
information to tell graphics_translation everything without needing
graphics_translattion to have an EGL component.

=====gles1=====gles1>>>>>Current state of the library>>>>>>gles2=====gles2=====

The state of this library right now is that it only builds. It is not hooked up
to anything.  In particular, an attempt at minizing ARC-specific dependencies
was made; RefBase, StrongPointer, and mutex had their usages removed, in favor
of nothing, emugl smart ptr, and android::base::Lock respectively. Note that
this does not guarantee that it works, it is only superficially-correct-looking
in order to get the library to build.
