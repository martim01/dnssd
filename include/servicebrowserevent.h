#pragma once
#include "dnssddlldefine.h"
#include <string>

struct dnsInstance;

class DNSSD_EXPORT ServiceBrowserEvent
{
    public:
        ServiceBrowserEvent(){}
        virtual ~ServiceBrowserEvent(){}

        virtual void InstanceResolved(dnsInstance* pInstance){}
        virtual void AllForNow(const std::string& sService){}
        virtual void Finished(){}
};
