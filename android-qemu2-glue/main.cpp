#if defined(_WIN32)
// On Windows, link against qtmain.lib which provides a WinMain()
// implementation, that later calls qMain().
#define main qt_main
#endif

// Implemented by vl.c
extern "C" int qemu_main(int argc, char** argv);

extern "C" int main(int argc, char **argv) {
    return qemu_main(argc, argv);
}
