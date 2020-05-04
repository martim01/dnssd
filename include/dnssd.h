#pragma once
#include <string>
#include <memory>
#include "dnsdlldefine.h"

namespace pml
{

    class ServiceBrowser;
    class ServicePublisher;
    class ZCPoster;

    class DNSSD_EXPORT Browser
    {
        public:
            Browser();
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
            Publisher(std::string sName, std::string sService, unsigned short nPort, std::string sHostname);
            ~Publisher();

            bool Start();
            void Stop();
            void Modify();

            void AddTxt(std::string sKey, std::string sValue, bool bModify);
            void RemoveTxt(std::string sKey, bool bModify);

        private:
            std::unique_ptr<ServicePublisher> m_pPublisher;
    };
};


