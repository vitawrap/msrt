#pragma once

#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)

namespace ms {
// Namespace for windows-specific functions
namespace nt {

    extern void getArgvUTF8( wchar_t* pcmdline, int * argc, char *** argv );
}
}

#endif
