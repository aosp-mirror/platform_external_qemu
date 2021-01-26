#define GL_GLEXT_PROTOTYPES  // For shader funcs

#include <jawt_md.h>
#include <stdio.h>
#include <string.h>
#include <GL/glx.h>

#include <vector>

static constexpr char kTexturedVertexShader[] = R"(
attribute vec4 a_position;
attribute vec2 a_texcoord;                                                  
varying vec2 v_texcoord;


void main() {
    v_texcoord = a_texcoord.st * vec2(1.0, -1.0);
    uv = in_uv;
    gl_Position = u_modelViewProj * vec4(in_position, 1.0);
}
)";

bool g_inited = false;
GLXContext g_ctx = NULL;
GLuint g_tex;
GLuint g_vertex_shader;

static void attach_shader() {
    g_vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    if (glIsShader(g_vertex_shader) != GL_TRUE) {
        fprintf(stderr, "glCreateShader failed\n");
        return;
    }
        
    const char* shader = kTexturedVertexShader;
    glShaderSource(g_vertex_shader, 1, &shader, nullptr);
    glCompileShader(g_vertex_shader);

    GLint compileStatus;
    glGetShaderiv(g_vertex_shader, GL_COMPILE_STATUS, &compileStatus);

    if (compileStatus != GL_TRUE) {
        GLsizei infoLogLength = 0;
        glGetShaderiv(g_vertex_shader, GL_INFO_LOG_LENGTH, &infoLogLength);
        std::vector<char> infoLog(infoLogLength + 1, 0);
        glGetShaderInfoLog(g_vertex_shader, infoLogLength, nullptr, &infoLog[0]);
        fprintf(stderr, "fail to compile. infolog: [%s]\n", &infoLog[0]);
        return;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, g_vertex_shader);
    glLinkProgram(program);

    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

    if (linkStatus != GL_TRUE) {
        GLsizei infoLogLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
        std::vector<char> infoLog(infoLogLength + 1, 0);
        glGetProgramInfoLog(program, infoLogLength, nullptr, &infoLog[0]);

        fprintf(stderr, "failed to link. infolog: [%s]\n", &infoLog[0]);
        return;
    }

    glUseProgram(program);
}

static void initialize_opengl(JAWT_X11DrawingSurfaceInfo* dsi_x11, jint width, jint height) {
    fprintf(stderr, "%s called\n", __func__);
    XVisualInfo visual_info = {};
    int visualsMatched = 0;

    visual_info.visualid = dsi_x11->visualID;
    XVisualInfo* visualList = XGetVisualInfo(dsi_x11->display, 0, &visual_info, &visualsMatched);
    fprintf(stderr, "visualid=%d visualsMatched=%d\n", dsi_x11->visualID, visualsMatched);
    if (visualList == NULL || visualsMatched == 0) {
        fprintf(stderr, "XGetVisualInfo failed");
        return;
    }

    int visual_idx = -1;
    for (int i = 0; i < visualsMatched; ++i) {
        if (dsi_x11->visualID == visualList[i].visualid) {
            fprintf(stderr, "%d) got match id=%d screen=%d depth=%d bits_per_rgb=%d\n", i,
                    visualList[i].visualid, visualList[i].screen, visualList[i].depth,
                    visualList[i].bits_per_rgb);
            visual_idx = i;
            break;
        }
    }

    if (visual_idx < 0) {
        fprintf(stderr, "Unable to find matching VIsualInfo");
        return;
    }

    g_ctx = glXCreateContext(dsi_x11->display, &visualList[visual_idx], NULL, True);
    glXMakeCurrent(dsi_x11->display, dsi_x11->drawable, g_ctx);

//    attach_shader();

    glGenTextures(1, &g_tex);
    // Request for the exact amount of memory since we'll be reusing the texture.
    glBindTexture(GL_TEXTURE_2D, g_tex);
    // Probably should use 4-byte alignment if we change the format from RGB to RGBA or BGRA.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    fprintf(stderr, "Before glTexImage2D w=%d h=%d ctx=%p tex=%d err=%d\n", width, height, g_ctx, g_tex, glGetError());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, (GLvoid*)NULL);
//    fprintf(stderr, "After glTexImage2D w=%d h=%d ctx=%p tex=%d err=%d\n", width, height, g_ctx, g_tex, glGetError());
    glBindTexture(GL_TEXTURE_2D, 0);
    glXMakeCurrent(dsi_x11->display, None, 0);

    XFree(visualList);

    g_inited = true;
}

static void use_glDrawPixels(const jbyte* pixels, jint width, jint height) {
    fprintf(stderr, "%s called\n", __func__);
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho( 0, width, 0, height, 0.1, 1 );
    glPixelZoom( 1, -1 );
    glRasterPos3f(0, height - 1, -0.3);
    glMatrixMode(GL_MODELVIEW);

    glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, pixels);
}

static void drawQuad(const jbyte* pixels, jint width, jint height) {
    fprintf(stderr, "%s called\n", __func__);
    // Upload pixel data to GPU texture
    glBindTexture(GL_TEXTURE_2D, g_tex);
//    fprintf(stderr, "Before glTexSubImage2D w=%d h=%d ctx=%p tex=%d err=%d\n", width, height, g_ctx, g_tex, glGetError());
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, pixels);
//    fprintf(stderr, "After glTexSubImage2D w=%d h=%d ctx=%p tex=%d err=%d\n", width, height, g_ctx, g_tex, glGetError());
    glBindTexture(GL_TEXTURE_2D, 0);

    // match projection to window resolution
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // Texture initially loaded upside-down and flipped
    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glBindTexture(GL_TEXTURE_2D, g_tex);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(0, 0);
    glTexCoord2f(0, 1); glVertex2f(0, height);
    glTexCoord2f(1, 1); glVertex2f(width, height);
    glTexCoord2f(1, 0); glVertex2f(width, 0);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}

static void linux_drawScreenshot(
        JAWT_DrawingSurfaceInfo* dsi,
        const jbyte* pixels,
        jint size,
        jint width,
        jint height) {
    JAWT_X11DrawingSurfaceInfo* dsi_x11;

    /* Get the platform-specific drawing info */
    dsi_x11 = (JAWT_X11DrawingSurfaceInfo*)dsi->platformInfo;

    if (!g_inited) {
        initialize_opengl(dsi_x11, width, height);
    }

    if (!glXMakeCurrent(dsi_x11->display, dsi_x11->drawable, g_ctx)) {
        fprintf(stderr, "glXMakeCurrent failed\n");
    }

#if 0
    use_glDrawPixels(pixels, width, height);
#else
    drawQuad(pixels, width, height);
#endif

    glXSwapBuffers(dsi_x11->display, dsi_x11->drawable);
    glXMakeCurrent(dsi_x11->display, None, 0);
}

// JNI methods must not be mangled
extern "C" {
JNIEXPORT void JNICALL
Java_com_android_emulator_ui_EmulatorPanel_drawScreenshot(JNIEnv *env, jobject canvas, jobject graphics, jbyteArray pixels, jint width, jint height) {
    JAWT awt;
    JAWT_DrawingSurface* ds;
    JAWT_DrawingSurfaceInfo* dsi;
    jboolean result;
    jint lock;


    /* Get the AWT */
    awt.version = JAWT_VERSION_9;
    if (JAWT_GetAWT(env, &awt) == JNI_FALSE) {
        printf("AWT Not found\n");
        return;
    }

    /* Get the drawing surface */
    ds = awt.GetDrawingSurface(env, canvas);
    if (ds == NULL) {
        printf("NULL drawing surface\n");
        return;
    }

    /* Lock the drawing surface */
    lock = ds->Lock(ds);
    if((lock & JAWT_LOCK_ERROR) != 0) {
        printf("Error locking surface\n");
        awt.FreeDrawingSurface(ds);
        return;
    }

    /* Get the drawing surface info */
    dsi = ds->GetDrawingSurfaceInfo(ds);
    if (dsi == NULL) {
        printf("Error getting surface info\n");
        ds->Unlock(ds);
        awt.FreeDrawingSurface(ds);
        return;
    }

    // Draw stuff!
    jint len = env->GetArrayLength(pixels);
    // Use GetPrimitive* instead of GetByteArrayElements to make it more likely to get an uncopied array.
    // Make sure no other JNI calls happens in between GetPrimitive*Critical and ReleasePrimitive*Critical.
    jbyte* native_pixels = static_cast<jbyte*>(env->GetPrimitiveArrayCritical(pixels, NULL));
    linux_drawScreenshot(dsi, native_pixels, len, width, height);
    env->ReleasePrimitiveArrayCritical(pixels, native_pixels, 0);

    /* Free the drawing surface info */
    ds->FreeDrawingSurfaceInfo(dsi);

    /* Unlock the drawing surface */
    ds->Unlock(ds);

    /* Free the drawing surface */
    awt.FreeDrawingSurface(ds);
}
}
