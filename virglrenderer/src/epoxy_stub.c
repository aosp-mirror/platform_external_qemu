#include "igl.h"

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

bool epoxy_is_desktop_gl()
{
    const char *es_prefix = "OpenGL ES";
    const char *version;
    version = (const char *)glGetString(GL_VERSION);
    /* If we didn't get a version back, there are only two things that
     * could have happened: either malloc failure (which basically
     * doesn't exist), or we were called within a glBegin()/glEnd().
     * Assume the second, which only exists for desktop GL.
     */
    if (!version) {
	fprintf(stderr, "can't get gl\n");
	abort();
        return 1;
    }
    fprintf(stderr, "gl get version %s\n", version);
    return strncmp(es_prefix, version, strlen(es_prefix));
}

int epoxy_gl_version()
{
    const char *version = (const char *)glGetString(GL_VERSION);
    GLint major, minor, factor;
    int scanf_count;

    if (!version)
        return 0;

    /* skip to version number */
    while (!isdigit(*version) && *version != '\0')
        version++;

    /* Interpret version number */
    scanf_count = sscanf(version, "%i.%i", &major, &minor);
    if (scanf_count != 2) {
        fprintf(stderr, "Unable to interpret GL_VERSION string: %s\n",
                version);
        abort();
    }

    if (minor >= 10)
        factor = 100;
    else
        factor = 10;

    return factor * major + minor;
}

bool
epoxy_extension_in_string(const char *extension_list, const char *ext)
{
    const char *ptr = extension_list;
    int len;

    if (!ext)
        return 0;

    len = strlen(ext);

    if (extension_list == NULL || *extension_list == '\0')
        return 0;

    /* Make sure that don't just find an extension with our name as a prefix. */
    while (1) {
        ptr = strstr(ptr, ext);
        if (!ptr)
            return 0;

        if (ptr[len] == ' ' || ptr[len] == 0)
            return 1;
        ptr += len;
    }
}

bool epoxy_has_gl_extension(const char* ext)
{
    if (epoxy_gl_version() < 30) {
        const char *exts = (const char *)glGetString(GL_EXTENSIONS);
        if (!exts)
            return 0;
        return epoxy_extension_in_string(exts, ext);
    } else {
        int num_extensions;
        int i;

        glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
        if (num_extensions == 0)
            return 0;

        for (i = 0; i < num_extensions; i++) {
            const char *gl_ext = (const char *)glGetStringi(GL_EXTENSIONS, i);
            if (!gl_ext)
                return 0;
            if (strcmp(ext, gl_ext) == 0)
                return 1;
        }

        return 0;
    }
    return 0;
}
