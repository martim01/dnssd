#ifndef PML_DNSSD_DEFINE_H
#define PML_DNSSD_DEFINE_H

#ifdef _WIN32
    #ifdef DNSSD_DLL
        #define DNSSD_EXPORT __declspec(dllexport)
    #else
        #define DNSSD_EXPORT __declspec(dllimport)
    #endif // DNSSD_EXPORT
#else
#define DNSSD_EXPORT
#endif

#endif  
