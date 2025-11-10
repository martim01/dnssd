#ifndef PML_DNSSD_BONJOURPUBLISHER_H
#define PML_DNSSD_BONJOURPUBLISHER_H

#ifdef _WIN32
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "dns_sd.h"
#include "dnsdlldefine.h"
#include "servicepublisher.h"

namespace pml::dnssd
{
    class BonjourPublisher : public ServicePublisher
    {
        public:
            BonjourPublisher();
            ~BonjourPublisher();

            bool Start() final;
            void Stop() final;
            

            bool AddService(const std::string& sName, const std::string& sService, unsigned short nPort, const std::map<std::string, std::string>& mTxt) final;

            bool RemoveService(const std::string& sName) final;

            void AddTxt(const std::string& sName, const std::string& sKey, const std::string& sValue, bool bModify) final;
            void SetTxt(const std::string& sName, const std::map<std::string, std::string>& mTxt) final;
            void RemoveTxt(const std::string& sName, const std::string& sKey,bool bModify) final;


        private:
            struct bonjourInfo
            {
                DNSServiceRef sdRef{0};
                std::string sName;
                std::string sService;
                unsigned short nPort = 0;
                std::map<std::string, std::string> mTxt;
            };

            void Modify(const bonjourInfo& info);

            void RunSelect();
            std::vector<unsigned char> MakeTxtRecords(const std::map<std::string, std::string>& mTxt);

            std::string m_sName;
            std::string m_sService;
            unsigned short m_nPort;
            std::string m_sHostname;
            char* m_psName;

            DNSServiceRef m_sdRef;
            std::map<DNSServiceRef,int> m_mClientToFd;
            
            //"_nmos-node._tcp"
            std::mutex m_mutex;
            std::atomic_bool m_bRun{true};
            std::unique_ptr<std::thread> m_pThread = nullptr;

            std::map<std::string, bonjourInfo> m_mServices;
    };
}
#endif
#endif
