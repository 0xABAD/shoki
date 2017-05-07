#include <windows.h>
#include <cstdio>
#include <io.h>     // _open_osfhandle
#include <Fcntl.h>  // _O_TEXT

#pragma comment(lib, "kernel32.lib")

namespace bl {
namespace w32 {

/**
 * Allocate a console for a win32 application.  Useful for debugging.
 *
 * @return True if a console could be allocated and that stdout could
 * be redirected to the console.
 *
 * @require
 */
bool allocate_console()
{
    bool canRedirect = false;
    
    if (AllocConsole()) {
        auto sout = GetStdHandle(STD_OUTPUT_HANDLE);
        auto con  = _open_osfhandle((intptr_t)sout, _O_TEXT);
        auto fp   = _fdopen(con, "w");

        *stdout = *fp;
        canRedirect = setvbuf(stdout, nullptr, _IONBF, 0) == 0;
    }

    return canRedirect;
}

} // end namespace w32
} // end namespace bl
