#include "../platform.h"




/*
 * Name: main
 * Desc: Program entry point.  Where it all begins.
 *
 * NOTES: This is not to be executed manually.  GpuMonEx will execute this process remotely
 *        and will manage it internally.  We will need a 32-bit and 64-bit version of this
 *        process running to hook processes running for the respective CPU architectures.
 *
 *        We won't initialize a window to keep it in the background.
 *
 *       Windows: We need this process to run in the background yet portable, so we still ned
 *       to call the WinMain entry point.  We then call int main after converting the parameters
 *       to the appropriate format.
 *
 *       MacOS and Linux: We can still use the standard int main here.
 */
int main( int argc, char** argv )
{
    return 0;
}

#ifdef _WIN32

/*
 * Name: WinMain
 * Desc: Windows program entry point.  Simply calls the int main implementation above.
 */
int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
    return main( __argc, __argv );
}

#endif
