#ifndef GRYLTOOLS_SYSTEMCHECK_H
#define GRYLTOOLS_SYSTEMCHECK_H

/*! There we perform the compilation system checking and set appropriate Macros.
 *  Currently we must check if system is POSIX or Win32
 */ 
// Check if POSIX
#if defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))
    #include <unistd.h>
    #if defined (_POSIX_VERSION)
        // OS is PoSiX-Compliant.
        #define _GRYLTOOL_POSIX
    #endif 
#endif 

// Check for Windows if not POSIX. Prefer POSIX over Windows.
// In this case, system is WINDOWS and NOT Posix
#if defined __WIN32 && !(defined _GRYLTOOL_POSIX)
    #define _GRYLTOOL_WIN32
#endif


#endif // GRYLTOOLS_SYSTEMCHECK_H
