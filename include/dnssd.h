#pragma once
#include <string>
#include <memory>
#include "dnsdlldefine.h"

namespace pml
{
    namespace dnssd
    {
        class ServiceBrowser;
        class ServicePublisher;
        class ZCPoster;

        class DNSSD_EXPORT Browser
        {
            public:
                Browser(const std::string& sDomain=std::string());
                ~Browser();
                void AddService(const std::string& sService, std::shared_ptr<ZCPoster> pPoster);
                void RemoveService(const std::string& sService);

                bool StartBrowser();

            private:
                std::unique_ptr<ServiceBrowser> m_pBrowser;

        };

        class DNSSD_EXPORT Publisher
        {
            public:
                Publisher(const std::string& sName, const std::string& sService, unsigned short nPort, const std::string& sHostname);

                ~Publisher();

                bool Start();
                void Stop();
                void Modify();

                void AddTxt(const std::string& sKey, const std::string& sValue, bool bModify);
                void RemoveTxt(const std::string& sKey, bool bModify);

            private:
                std::unique_ptr<ServicePublisher> m_pPublisher;
        };
    };
};


