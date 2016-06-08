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

The set of CLs in this topic are to provide a general GLESv1 emulation solution
whenever the backend does not have GLESv1. Currently, the only use case is for
ANGLE, and we may have to perform ANGLE-specific tweaks to get it all working.

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
