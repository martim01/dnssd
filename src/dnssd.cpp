#include "dnssd.h"
#ifdef __WIN32__
#include "bonjourbrowser.h"
#include "bonjourpublisher.h"
#else
#include "avahibrowser.h"
#include "avahipublisher.h"
#endif // __WIN32__

using namespace pml;


Browser::Browser() : m_pBrowser(std::unique_ptr<ServiceBrowser>(new ServiceBrowser()))
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

Publisher::Publisher(std::string sName, std::string sService, unsigned short nPort, std::string sHostname) :
    m_pPublisher(std::unique_ptr<ServicePublisher>(new ServicePublisher(sName, sService, nPort, sHostname)))
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

void Publisher::Modify()
{
    m_pPublisher->Modify();
}

void Publisher::AddTxt(std::string sKey, std::string sValue, bool bModify)
{
    m_pPublisher->AddTxt(sKey,sValue, bModify);
}

void Publisher::RemoveTxt(std::string sKey, std::string sValue, bool bModify)
{
    m_pPublisher->RemoveTxt(sKey, sValue, bModify);
}

