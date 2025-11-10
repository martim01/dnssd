#ifndef PML_DNSSD_DNS_H
#define PML_DNSSD_DNS_H

#include <list>
#include <map>
#include <memory>
#include <string>

#include "dnsdlldefine.h"

namespace pml::dnssd
{
    struct DNSSD_EXPORT dnsInstance
    {
        dnsInstance(){}
        dnsInstance(std::string sN) : sName(sN), nPort(0), nUpdate(0){}

        std::string sName;
        std::string sHostName;
        std::string sHostIP;
        std::string sService;
        unsigned long nPort;
        std::string sInterface;
        std::map<std::string, std::string> mTxt;
        std::map<std::string, std::string> mTxtLast;
        size_t nUpdate;

    };


    struct DNSSD_EXPORT dnsService
    {
        dnsService(){}
        dnsService(std::string ss) : sService(ss){}

        ~dnsService()
        {
            mInstances.clear();
        }

        std::string sService;
        std::map<std::string, std::shared_ptr<dnsInstance> > mInstances;

    };
}
#endif