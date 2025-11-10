#include "dnssd.h"

#ifdef _WIN32
#include "bonjourbrowser.h"
#include "bonjourpublisher.h"
#else
#include "avahibrowser.h"
#include "avahipublisher.h"
#endif // __WIN32__

namespace pml::dnssd
{
    Browser::Browser(const std::string& sDomain) : m_pBrowser(std::unique_ptr<ServiceBrowser>(new ServiceBrowser(sDomain)))
    {

    }

    void Browser::AddService(const std::string& sService, std::shared_ptr<ZCPoster> pPoster)
    {
        m_pBrowser->AddService(sService, pPoster);
    }

    void Browser::RemoveService(const std::string& sService)
    {
        m_pBrowser->RemoveService(sService);
    }

    bool Browser::StartBrowser()
    {
        return m_pBrowser->StartBrowser();
    }

    Browser::~Browser()
    {

    }

    Publisher::Publisher() :
        #ifdef _WIN32
            m_pPublisher(std::unique_ptr<ServicePublisher>(new BonjourPublisher()))
        #else
            m_pPublisher(std::unique_ptr<ServicePublisher>(new AvahiPublisher()))
        #endif // _WIN32
    {
    }

    Publisher::~Publisher()
    {
        Stop();
    }

    bool Publisher::Start()
    {
        return m_pPublisher->Start();
    }

    void Publisher::Stop()
    {
        m_pPublisher->Stop();
    }


    void Publisher::AddTxt(const std::string& sName, const std::string& sKey, const std::string& sValue, bool bModify)
    {
        m_pPublisher->AddTxt(sName, sKey,sValue, bModify);
    }

    void Publisher::RemoveTxt(const std::string& sName, const std::string& sKey, bool bModify)
    {
        m_pPublisher->RemoveTxt(sName, sKey, bModify);
    }

    bool Publisher::AddService(const std::string& sName, const std::string& sService, unsigned short nPort,const std::map<std::string, std::string>& mTxt)
    {
        return m_pPublisher->AddService(sName, sService, nPort, mTxt);
    }

    bool Publisher::RemoveService(const std::string& sName)
    {
        return m_pPublisher->RemoveService(sName);
    }

    void Publisher::SetTxt(const std::string& sName, const std::map<std::string, std::string>& mTxt)
    {
        m_pPublisher->SetTxt(sName, mTxt);
    }
}