#ifndef PML_DNSSD_BONJOURPUBLISHER_H
#define PML_DNSSD_BONJOURPUBLISHER_H

#ifdef _WIN32
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#include "dns_sd.h"
#include "dnsdlldefine.h"

namespace pml::dnssd
{
    class ServicePublisher
    {
        public:
            ServicePublisher(std::string sName, std::string sService, unsigned short nPort, std::string sHostname);
            ~ServicePublisher();

            bool Start();
            void Stop();
            void Modify();

            void AddTxt(std::string sKey, std::string sValue, bool bModify);
            void RemoveTxt(std::string sKey, bool bModify);


        private:
            static void RunSelect(ServicePublisher* pPublisher);
            std::vector<unsigned char> MakeTxtRecords();

            std::string m_sName;
            std::string m_sService;
            unsigned short m_nPort;
            std::string m_sHostname;
            char* m_psName;

            DNSServiceRef m_sdRef;
            std::map<DNSServiceRef,int> m_mClientToFd;
            std::map<std::string, std::string> m_mTxt;
            //"_nmos-node._tcp"
            std::mutex m_mutex;
    };
}
#endif
#endif
