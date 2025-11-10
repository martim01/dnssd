#ifndef PML_DNSSD_SERVICE_PUBLISHER_H
#define PML_DNSSD_SERVICE_PUBLISHER_H

#include <map>
#include <set>
#include <string>

#include <avahi-common/alternative.h>
#include <avahi-client/client.h>
#include <avahi-common/error.h>
#include <avahi-common/malloc.h>
#include <avahi-client/publish.h>
#include <avahi-common/thread-watch.h>
#include <avahi-common/timeval.h>

#include "dnsdlldefine.h"
#include "servicepublisher.h"

static void entry_group_callback(AvahiEntryGroup *g, AvahiEntryGroupState state, AVAHI_GCC_UNUSED void *userdata);
static void client_callback(AvahiClient *c, AvahiClientState state, AVAHI_GCC_UNUSED void * userdata);

namespace pml::dnssd
{
    struct avahiInfo
    {
        AvahiEntryGroup* pGroup = nullptr;
        std::string sName;
        std::string sService;
        unsigned short nPort = 0;
        char* psName = nullptr;

        std::map<std::string, std::string> mTxt;
    };
    
    class AvahiPublisher : public ServicePublisher
    {
        public:
            AvahiPublisher();
            ~AvahiPublisher();

            bool Start() final;
            void Stop() final;
            

            bool AddService(const std::string& sName, const std::string& sService, unsigned short nPort, const std::map<std::string, std::string>& mTxt) final;

            bool RemoveService(const std::string& sName) final;

            void AddTxt(const std::string& sName, const std::string& sKey, const std::string& sValue, bool bModify) final;
            void SetTxt(const std::string& sName, const std::map<std::string, std::string>& mTxt) final;
            void RemoveTxt(const std::string& sName, const std::string& sKey,bool bModify) final;

            void EntryGroupCallback(AvahiEntryGroup* pGroup, AvahiEntryGroupState state);
            void ClientCallback(AvahiClient* pClient, AvahiClientState state);

        private:

            bool CreateService(avahiInfo& info);
            void Modify(const avahiInfo& info);
            bool Collision(avahiInfo& info);

            void ThreadQuit();

            AvahiStringList* GetTxtList(const avahiInfo& info);

            AvahiClient* m_pClient = nullptr;
            AvahiThreadedPoll* m_pThreadedPoll = nullptr;

            

            std::map<std::string, avahiInfo> m_mServices;

    };
}
#endif // PML_DNSSD_SERVICE_PUBLISHER_H