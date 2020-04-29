#pragma once

#ifdef __WIN32__
    #ifdef DNSSD_DLL
        #define DNSSD_EXPORT __declspec(dllexport)
    #else
        #define DNSSD_EXPORT __declspec(dllimport)
    #endif // DNSSD_EXPORT
#else
#define DNSSD_EXPORT
#endif
