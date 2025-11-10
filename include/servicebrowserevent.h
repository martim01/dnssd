#ifndef PML_DNSSD_SERVICEBROWSEREVENT_H
#define PML_DNSSD_SERVICEBROWSEREVENT_H

#include <string>

#include "dnsddlldefine.h"


namespace pml::dnssd
{
    class DNSSD_EXPORT ServiceBrowserEvent
    {
        public:
            ServiceBrowserEvent(){}
            virtual ~ServiceBrowserEvent(){}

            virtual void InstanceResolved(dnsInstance* pInstance){}
            virtual void AllForNow(const std::string& sService){}
            virtual void Finished(){}
    };
}
#endif