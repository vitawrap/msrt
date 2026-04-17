#include "nt_utils.hpp"

#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
#include <windows.h>
#include <shellapi.h>
#pragma comment(lib, "Shell32")

namespace ms {
namespace nt {

    // Source - https://stackoverflow.com/a/74999569
    void getArgvUTF8( wchar_t* pcmdline, int * argc, char *** argv )
    {
      // win32 only has widechar commandline availability
      wchar_t ** wargv = CommandLineToArgvW( pcmdline, argc );
      if (!wargv) { *argc = 0; *argv = NULL; return; }
      
      int n = 0;
      for (int i = 0;  i < *argc;  i++)
        n += WideCharToMultiByte( CP_UTF8, 0, wargv[i], -1, NULL, 0, NULL, NULL ) + 1;
      
      // all in one argv[] array with ptrs to/and all the UTF-8 strings
      *argv =  (char**) malloc( (*argc + 1) * sizeof(char *) + n );
      if (!*argv) { *argc = 0; return; }
      
      char * arg = (char *)&((*argv)[*argc + 1]);
      for (int i = 0;  i < *argc;  i++)
      {
        (*argv)[i] = arg;
        arg += WideCharToMultiByte( CP_UTF8, 0, wargv[i], -1, arg, n, NULL, NULL ) + 1;
      }
      (*argv)[*argc] = NULL;
    }

}
}


#endif
