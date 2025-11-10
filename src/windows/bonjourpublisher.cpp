#ifdef _WIN32
#include "bonjourpublisher.h"
#include <sstream>
#include <thread>

#include "log.h"

namespace pml::dnssd
{

    BonjourPublisher::BonjourPublisher()=default;

    BonjourPublisher::~BonjourPublisher()
    {
        Stop();
    }


    bool BonjourPublisher::AddService(const std::string& sName, const std::string& sService, unsigned short nPort, const std::map<std::string, std::string>& mTxt)
    {   
        std::scoped_lock lg(m_mutex);

        pml::log::log(pml::log::Level::kDebug, "pml::dnssd::BonjourBrowser") << "Add Service " << sService;

        bonjourInfo info;
        info.sName = sName;
        info.nPort = nPort;
        info.mTxt = mTxt;
        info.sService = sService;


        DNSServiceErrorType error;
        DNSServiceRef serviceRef;

        const std::vector<unsigned char> vRecords(MakeTxtRecords(mTxt));

        error = DNSServiceRegister(&info.sdRef,
                                    0,                  // no flags
                                    0,                  // all network interfaces
                                    sName.c_str(),  // name
                                    sService.c_str(),       // service type
                                    "",                 // register in default domain(s)
                                    nullptr,               // use default host name
                                    htons(nPort),        // port number
                                    vRecords.size(),                  // length of TXT record
                                    !vRecords.empty()? &vRecords[0] : nullptr,               // no TXT record
                                    nullptr, // call back function
                                    nullptr);              // no context

        if (error == kDNSServiceErr_NoError)
        {
            pml::log::log(pml::log::Level::kDebug, "pml::dnssd::BonjourBrowser") << "Service " << sService << " added";
            m_mClientToFd[info.sdRef] = DNSServiceRefSockFD(info.sdRef);

            m_mServices.try_emplace(sName, info);
        

            return true;
        }
        else
        {
            pml::log::log(pml::log::Level::kError, "pml::dnssd::BonjourPublisher") << "Failed to add service " << sService << " : " << error;
            return false;
        }
    }

    bool BonjourPublisher::RemoveService(const std::string& sName)
    {
        if(auto itService = m_mServices.find(sName); itService != m_mServices.end())
        {
            DNSServiceRefDeallocate(itService->second.sdRef);
            m_mServices.erase(itService);
            return true;
        }
        return false;
    }


    bool BonjourPublisher::Start()
    {

        pml::log::log(pml::log::Level::kDebug, "pml::dnssd::BonjourBrowser") << "BojourPubliser: Start";
        m_pThread = std::make_unique<std::thread>([this](){RunSelect();});
        return true;
    }

    void BonjourPublisher::Stop()
    {
        std::scoped_lock lg(m_mutex);
        m_bRun = false;
        if(m_pThread)
        {
            m_pThread->join();
            m_pThread = nullptr;
        }
        
    }

    void BonjourPublisher::Modify(const bonjourInfo& info)
    {
        std::scoped_lock lg(m_mutex);
        if(info.sdRef)
        {
            // try and find a record that matches
            const std::vector<unsigned char> txt_records = MakeTxtRecords(info.mTxt);

            DNSServiceErrorType errorCode = DNSServiceUpdateRecord(info.sdRef, NULL, 0, (std::uint16_t)txt_records.size(), &txt_records[0], 0);

            if (errorCode != kDNSServiceErr_NoError)
            {
                pml::log::log(pml::log::Level::kError, "pml::dnssd::BonjourPublisher") << "Modify failed with error "<< errorCode;
            }
            else
            {
                pml::log::log(pml::log::Level::kDebug, "pml::dnssd::BonjourPublisher") << "Modify succeeded";
            }
        }
    }

    void BonjourPublisher::AddTxt(const std::string& sName, const std::string& sKey, const std::string& sValue, bool bModify)
    {
        if(auto itService= m_mServices.find(sName); itService != m_mServices.end())
        {
            itService->second.mTxt[sKey] = sValue;
            if(bModify)
            {
                Modify(itService->second);
            }
        }
    }

    void BonjourPublisher::RemoveTxt(const std::string& sName, const std::string& sKey,bool bModify)
    {
        if(auto itService= m_mServices.find(sName); itService != m_mServices.end())
        {
            itService->second.mTxt.erase(sKey);
            if(bModify)
            {
                Modify(itService->second);
            }
        }
    }

    void BonjourPublisher::SetTxt(const std::string& sName, const std::map<std::string, std::string>& mTxt)
    {
        if(auto itService= m_mServices.find(sName); itService != m_mServices.end())
        {
            itService->second.mTxt = mTxt;

            Modify(itService->second);
        }
    }

    std::vector<unsigned char> BonjourPublisher::MakeTxtRecords(const std::map<std::string, std::string>& mTxt)
    {
        std::vector<unsigned char> vText;

        for(const auto& [sKey, sValue] : mTxt)
        {
            std::stringstream ss;
            ss << sKey << "=" << sValue;
            vText.push_back(ss.str().size());
            for(size_t i = 0; i < ss.str().size(); i++)
            {
                vText.push_back(ss.str()[i]);
            }

        }

        return vText;
    }


    void BonjourPublisher::RunSelect()
    {
        while(m_bRun)
        {
            if (m_mClientToFd.size() == 0 )
            {
                m_bRun = false;
            }
            else
            {
                fd_set readfds;
                FD_ZERO(&readfds);
                int nfds = 0;
                for(const auto& [sdRef, fd] : m_mClientToFd)
                {
                    FD_SET(fd, &readfds);
                    nfds = std::max(fd, nfds);
                }


                pml::log::log(pml::log::Level::kDebug, "pml::dnssd::BonjourPublisher") << "Start select: fd =" << m_mClientToFd.size() << " nfds =" << nfds;
                struct timeval tv = { 0, 1000 };

                int result = select(nfds, &readfds, (fd_set*)nullptr, (fd_set*)nullptr, &tv);
                int count = 0;
                if ( result > 0 )
                {
                    pml::log::log(pml::log::Level::kDebug, "pml::dnssd::BonjourPublisher") << "Select done";
                    std::scoped_lock lg(m_mutex);

                    //
                    // While iterating through the loop, the callback functions might delete
                    // the client pointed to by the current iterator, so I have to increment
                    // it BEFORE calling DNSServiceProcessResult
                    //
                    for ( auto itClient = m_mClientToFd.cbegin() ; itClient != m_mClientToFd.cend() ; )
                    {
                        auto jj = ++itClient;
                        if (FD_ISSET(jj->second, &readfds) )
                        {
                            if(auto err = DNSServiceProcessResult(jj->first); err != kDNSServiceErr_NoError)
                            {
                                pml::log::log(pml::log::Level::kError, "pml::dnssd::BonjourPublisher") << "BonjourPublisher: DNSServiceProcessResult returned error " << err ;
                            }
                            if ( ++count > 10 )
                            {
                                break;
                            }
                        }
                    }
                }
                else
                {
                    pml::log::log(pml::log::Level::kDebug, "pml::dnssd::BonjourPublisher") << "Result = " << result;
                    break;
                }
            }
        }

        for(auto& [sName, info] : m_mServices)
        {
            DNSServiceRefDeallocate(info.sdRef);
        }
    }
}

#endif
