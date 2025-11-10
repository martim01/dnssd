#ifndef PML_DNSSD_H
#define PML_DNSSD_H

#include <map>
#include <memory>
#include <string>

#include "dnsdlldefine.h"


namespace pml::dnssd
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
            Publisher();
            ~Publisher();

            bool Start();
            void Stop();

            bool AddService(const std::string& sName, const std::string& sService, unsigned short nPort,const std::map<std::string, std::string>& mTxt);
            bool RemoveService(const std::string& sName);

            void SetTxt(const std::string& sName, const std::map<std::string, std::string>& mTxt);
            void AddTxt(const std::string& sName, const std::string& sKey, const std::string& sValue, bool bModify);
            void RemoveTxt(const std::string& sName, const std::string& sKey, bool bModify);

        private:
            std::unique_ptr<ServicePublisher> m_pPublisher;
    };
}
#endif // PML_DNSSD_H
