#ifndef PML_DNSSD_BONJOURBROWSER_H
#define PML_DNSSD_BONJOURBROWSER_H

#ifdef _WIN32

#include <atomic>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>


#include "dns_sd.h"
#include "mdns.h"

static void DNSSD_API iterate_service_types( DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *serviceName, const char *regtype,const char *replyDomain, void *context );
static void DNSSD_API iterate_service_instances( DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *serviceName, const char *regtype,const char *replyDomain, void *context );
static void DNSSD_API resolve_instance(  DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *fullname, const char *hosttarget, uint16_t port, uint16_t txtLen, const unsigned char *txtRecord, void *context );
static void DNSSD_API get_address( DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *hostname, const struct sockaddr *address,uint32_t ttl, void *context );


namespace pml::dnssd
{
    struct dnsService;
    struct dnsInstance;
    class ZCPoster;

    class ServiceBrowser
    {

    public:

        ServiceBrowser(const std::string& sDomain);
        ~ServiceBrowser(){}
        void AddService(const std::string& sService, std::shared_ptr<ZCPoster> pPoster);
        void RemoveService(const std::string& sService);

        bool StartBrowser();


        void DNSSD_API IterateTypes( DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *serviceName, const char *regtype,const char *replyDomain, void *context );
        void DNSSD_API IterateInstances( DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *serviceName, const char *regtype,const char *replyDomain, void *context );
        void DNSSD_API Resolve(  DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *fullname, const char *hosttarget, uint16_t port, uint16_t txtLen, const unsigned char *txtRecord, void *context );
        void DNSSD_API Address( DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *hostname, const struct sockaddr *address,uint32_t ttl, void *context );

        void RunSelect();

        const std::map<std::string, std::shared_ptr<dnsService> >& GetServices() const { return m_mServices; }
        const std::map<DNSServiceRef, int>& GetClientToFdMap() const { return m_mClientToFd; }
        const std::map<DNSServiceRef, std::string>& GetServRefToStringMap() const { return m_mServRefToString; }
        const std::map<DNSServiceRef, std::shared_ptr<dnsInstance> >& GetInstancesMap() const { return m_mInstances; }


        std::map<std::string, std::shared_ptr<ZCPoster> >::const_iterator GetPosterBegin()
        {
            return m_mServiceBrowse.begin();
        }
        std::map<std::string, std::shared_ptr<ZCPoster> >::const_iterator GetPosterEnd()
        {
            return m_mServiceBrowse.end();
        }

        protected:

            std::map<std::string, std::shared_ptr<dnsService> > m_mServices;

            std::map<DNSServiceRef,int> m_mClientToFd;
            std::map<DNSServiceRef,std::string> m_mServRefToString;
            std::map<DNSServiceRef, std::shared_ptr<dnsInstance> > m_mInstances;
            std::unordered_set<std::string> m_setServiceTypes;
            std::unordered_set<std::string> m_setServiceInstances;

            std::map<std::string, std::shared_ptr<ZCPoster> > m_mServiceBrowse;

            std::atomic_bool m_bRun{true};
            std::mutex m_mutex;
            std::unique_ptr<std::thread> m_pThread = nullptr;
    };
}
#endif
#endif  