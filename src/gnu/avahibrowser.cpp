#include "avahibrowser.h"

#include <iostream>
#include <mutex>

#include "log.h"

#include "mdns.h"
#include "zcposter.h"



void client_callback(AvahiClient * pClient, AvahiClientState state, AVAHI_GCC_UNUSED void * userdata)
{
    pml::log::log(pml::log::Level::kTrace, "pml::dnssd") << "avahi browser client_callback";
    auto pBrowser = reinterpret_cast<pml::dnssd::ServiceBrowser*>(userdata);
    pBrowser->ClientCallback(pClient, state);
}

void type_callback(AvahiServiceTypeBrowser* stb, AvahiIfIndex interface, AvahiProtocol protocol, AvahiBrowserEvent event, const char* type, const char* domain, AvahiLookupResultFlags flags, void* userdata)
{
    pml::log::log(pml::log::Level::kTrace, "pml::dnssd") << "avahi browser type_callback";
    auto pBrowser = reinterpret_cast<pml::dnssd::ServiceBrowser*>(userdata);
    pBrowser->TypeCallback(interface, protocol, event, type, domain);
}

void browse_callback(AvahiServiceBrowser *b, AvahiIfIndex interface, AvahiProtocol protocol, AvahiBrowserEvent event, const char *name, const char *type, const char *domain, AVAHI_GCC_UNUSED AvahiLookupResultFlags flags, void* userdata)
{
    pml::log::log(pml::log::Level::kTrace, "pml::dnssd") << "avahi browser browse_callback";
    auto pBrowser = reinterpret_cast<pml::dnssd::ServiceBrowser*>(userdata);
    pBrowser->BrowseCallback(b, interface, protocol, event, name, type, domain);
}

void resolve_callback(AvahiServiceResolver *r, AVAHI_GCC_UNUSED AvahiIfIndex interface, AVAHI_GCC_UNUSED AvahiProtocol protocol, AvahiResolverEvent event,const char *name, const char *type, const char *domain, const char *host_name, const AvahiAddress *address, uint16_t port, AvahiStringList *txt,AvahiLookupResultFlags flags,AVAHI_GCC_UNUSED void* userdata)
{
    pml::log::log(pml::log::Level::kTrace, "pml::dnssd") << "avahi browser reslove_callback";
    auto pBrowser = reinterpret_cast<pml::dnssd::ServiceBrowser*>(userdata);
    pBrowser->ResolveCallback(r, event, name, type, domain,host_name, address,port,txt);

}




namespace pml::dnssd
{

    ServiceBrowser::ServiceBrowser(const std::string& sDomain) :
        m_sDomain(sDomain)
    {

    }

    ServiceBrowser::~ServiceBrowser()
    {
        Stop();
        DeleteAllServices();

    }

    void ServiceBrowser::DeleteAllServices()
    {
        std::scoped_lock lock(m_mutex);
        m_mServices.clear();
    }


    bool ServiceBrowser::StartBrowser()
    {
        std::scoped_lock lock(m_mutex);
        if(!m_bStarted)
        {
            int error;

            pml::log::log(pml::log::Level::kDebug, "pml::dnssd") << "ServiceBrowser: Create Threaded poll object." ;
            /* Allocate main loop object */
            if (!(m_pThreadedPoll = avahi_threaded_poll_new()))
            {
                pml::log::log(pml::log::Level::kError, "pml::dnssd") << "ServiceBrowser: Failed to create Threaded poll object." ;
                return false;
            }

            /* Allocate a new client */
            pml::log::log(pml::log::Level::kDebug, "pml::dnssd") << "ServiceBrowser: Allocate a new client." ;
            avahi_client_new(avahi_threaded_poll_get(m_pThreadedPoll), AVAHI_CLIENT_NO_FAIL, client_callback, reinterpret_cast<void*>(this), &error);
            avahi_threaded_poll_start(m_pThreadedPoll);
            m_bStarted = true;
        }

        return true;
    }

    void ServiceBrowser::Stop()
    {
        pml::log::log(pml::log::Level::kDebug, "pml::dnssd") << "ServiceBrowser - stop";
        if(m_pThreadedPoll)
        {
            avahi_threaded_poll_stop(m_pThreadedPoll);
        }

        for(auto pBrowser : m_setBrowser)
        {
            avahi_service_browser_free(pBrowser);
        }

        if(m_pTypeBrowser)
        {
            avahi_service_type_browser_free(m_pTypeBrowser);
            m_pTypeBrowser = 0;
        }
        if(m_pClient)
        {
            avahi_client_free(m_pClient);
            m_pClient = 0;
        }
        if(m_pThreadedPoll)
        {
            avahi_threaded_poll_free(m_pThreadedPoll);
            m_pThreadedPoll = 0;

            std::set<std::shared_ptr<ZCPoster> > setPoster;
            for(auto& [key, pPoster] : m_mServiceBrowse)
            {
                if(setPoster.insert(pPoster).second)
                {
                    pPoster->_Finished();
                }
            }
        }
        m_nWaitingOn = 0;

        pml::log::log(pml::log::Level::kDebug, "pml::dnssd") << "ServiceBrowser - stop - done";
    }


    bool ServiceBrowser::Start(AvahiClient* pClient)
    {
        pml::log::log(pml::log::Level::kDebug, "pml::dnssd") << "ServiceBrowser:Start" ;
        if(!m_bBrowsing)
        {
            //std::scoped_lock lg(m_mutex);
            m_bBrowsing = true;
            m_pClient = pClient;
            Browse();
        }
        return true;
    }

    void ServiceBrowser::Browse()
    {
        pml::log::log(pml::log::Level::kDebug, "pml::dnssd") << "ServiceBrowser:Browse" ;

        for(auto [key, pService] : m_mServiceBrowse)
        {
            if(m_mServices.insert(std::make_pair((key), std::make_shared<dnsService>(dnsService((key))))).second)
            {
                AvahiServiceBrowser* psb = NULL;
                /* Create the service browser */
                if (!(psb = avahi_service_browser_new(m_pClient, AVAHI_IF_UNSPEC, AVAHI_PROTO_INET, (key).c_str(), m_sDomain.c_str(), (AvahiLookupFlags)0, browse_callback, reinterpret_cast<void*>(this))))
                {
                    pml::log::log(pml::log::Level::kError, "pml::dnssd") << "ServiceBrowser: Failed to create service browser" ;
                }
                else
                {
                    pml::log::log(pml::log::Level::kDebug, "pml::dnssd") << "ServiceBrowser: Service '" << key << "' browse" ;
                    m_setBrowser.insert(psb);
                    m_nWaitingOn++;
                }
            }
        }
    }

    void ServiceBrowser::ClientCallback(AvahiClient * pClient, AvahiClientState state)
    {
        pml::log::log(pml::log::Level::kDebug, "pml::dnssd") << "ServiceBrowser:ClientCallback" ;
        switch (state)
        {
            case AVAHI_CLIENT_FAILURE:
                pml::log::log(pml::log::Level::kError, "pml::dnssd") << "ServiceBrowser: ClientCallback: failure" ;
                if (avahi_client_errno(pClient) == AVAHI_ERR_DISCONNECTED)
                {
                    int error;
                    /* We have been disconnected, so let reconnect */
                    pml::log::log(pml::log::Level::kWarning, "pml::dnssd") << "ServiceBrowser:  Disconnected, reconnecting ..." ;

                    avahi_client_free(pClient);
                    m_pClient = NULL;

                    DeleteAllServices();

                    if (!(avahi_client_new(avahi_threaded_poll_get(m_pThreadedPoll), AVAHI_CLIENT_NO_FAIL, client_callback, reinterpret_cast<void*>(this), &error)))
                    {
                        pml::log::log(pml::log::Level::kError, "pml::dnssd") << "ServiceBrowser: Failed to create client object: " << avahi_strerror(avahi_client_errno(m_pClient));// ;
                        Stop();

                    }
                }
                else
                {
                    pml::log::log(pml::log::Level::kError, "pml::dnssd") << "ServiceBrowser: Server connection failure" ;
                    Stop();
                }
                break;
            case AVAHI_CLIENT_S_REGISTERING:
            case AVAHI_CLIENT_S_RUNNING:
            case AVAHI_CLIENT_S_COLLISION:
                pml::log::log(pml::log::Level::kDebug, "pml::dnssd") << "ServiceBrowser: Registering/Running ..." ;
                if (!m_bBrowsing)
                {
                    Start(pClient);
                }
                break;
            case AVAHI_CLIENT_CONNECTING:
                pml::log::log(pml::log::Level::kDebug, "pml::dnssd") << "ServiceBrowser: Waiting for daemon ..." ;
                break;
            default:
                pml::log::log(pml::log::Level::kDebug, "pml::dnssd") << "ServiceBrowser: ClientCallback: " << state ;
        }

    }

    void ServiceBrowser::TypeCallback(AvahiIfIndex interface, AvahiProtocol protocol, AvahiBrowserEvent event, const char* type, const char* domain)
    {
        switch(event)
        {
            case AVAHI_BROWSER_NEW:
                {
                    std::string sService(type);
                    pml::log::log(pml::log::Level::kDebug, "pml::dnssd") << "ServiceBrowser: Service '" << type << "' found in domain '" << domain << "'" ;
                    if(auto itServiceBrowse = m_mServiceBrowse.find(sService); itServiceBrowse != m_mServiceBrowse.end())
                    {
                        m_mutex.lock();
                        if(m_mServices.insert(std::make_pair(sService, std::make_shared<dnsService>(dnsService(sService)))).second)
                        {
                            AvahiServiceBrowser* psb = NULL;
                            /* Create the service browser */
                            if (!(psb = avahi_service_browser_new(m_pClient, interface, protocol, type, domain, (AvahiLookupFlags)0, browse_callback, reinterpret_cast<void*>(this))))
                            {
                                pml::log::log(pml::log::Level::kError, "pml::dnssd") << "ServiceBrowser: Failed to create service browser" ;
                            }
                            else
                            {
                                pml::log::log(pml::log::Level::kDebug, "pml::dnssd") << "ServiceBrowser: Service '" << type << "' browse" ;
                                m_setBrowser.insert(psb);
                                m_nWaitingOn++;
                            }
                        }
                        m_mutex.unlock();
                    }
                }
                break;
            case AVAHI_BROWSER_REMOVE:
                    /* We're dirty and never remove the browser again */
                pml::log::log(pml::log::Level::kDebug, "pml::dnssd") << "ServiceBrowser: Service '" << type << "' removed" ;
                    break;
            case AVAHI_BROWSER_FAILURE:
                pml::log::log(pml::log::Level::kError, "pml::dnssd") << "ServiceBrowser: service_type_browser failed" ;
                Stop();
                break;
            case AVAHI_BROWSER_CACHE_EXHAUSTED:
                pml::log::log(pml::log::Level::kError, "pml::dnssd") << "ServiceBrowser: AVAHI_BROWSER_CACHE_EXHAUSTED" ;
                break;
            case AVAHI_BROWSER_ALL_FOR_NOW:
                pml::log::log(pml::log::Level::kError, "pml::dnssd") << "ServiceBrowser: AVAHI_BROWSER_ALL_FOR_NOW" ;
                CheckStop();
                break;
            default:
                pml::log::log(pml::log::Level::kDebug, "pml::dnssd") << "ServiceBrowser: TypeCallback: " << event ;
        }
    }

    void ServiceBrowser::BrowseCallback(AvahiServiceBrowser* pBrowser, AvahiIfIndex interface, AvahiProtocol protocol, AvahiBrowserEvent event, const char *name, const char *type, const char *domain)
    {
        /* Called whenever a new services becomes available on the LAN or is removed from the LAN */
        switch (event)
        {
            case AVAHI_BROWSER_FAILURE:
                pml::log::log(pml::log::Level::kError, "pml::dnssd") << "ServiceBrowser: Browser Failure: " << avahi_strerror(avahi_client_errno(m_pClient)) ;
                Stop();
                break;
            case AVAHI_BROWSER_NEW:
                {
                    std::string sService(type);
                    std::string sName(name);

                    pml::log::log(pml::log::Level::kDebug, "pml::dnssd") << "ServiceBrowser: (Browser) NEW: service '" << name << "' of type '" << type << "' in domain '" << domain << "'" ;
                    AvahiServiceResolver* pResolver= avahi_service_resolver_new(m_pClient, interface, protocol, name, type, domain, AVAHI_PROTO_INET, (AvahiLookupFlags)0, resolve_callback, reinterpret_cast<void*>(this));
                    if(!pResolver)
                    {
                        pml::log::log(pml::log::Level::kError, "pml::dnssd") << "ServiceBrowser: Failed to resolve service " << name << ": " << avahi_strerror(avahi_client_errno(m_pClient)) ;
                    }
                    else
                    {
                        m_mResolvers.insert(std::make_pair(std::string(name)+"__"+std::string(type), pResolver));
                        m_nWaitingOn++;
                    }
                }
                break;
            case AVAHI_BROWSER_REMOVE:
                pml::log::log(pml::log::Level::kDebug, "pml::dnssd") << "ServiceBrowser: (Browser) REMOVE: service '" << name << "' of type '" << type << "' in domain '" << domain << "'" ;
                RemoveServiceInstance(type, name);
                break;
            case AVAHI_BROWSER_ALL_FOR_NOW:
                {
                    pml::log::log(pml::log::Level::kDebug, "pml::dnssd") << "ServiceBrowser: (Browser) '" << type << "' in domain '" << domain << "' ALL_FOR_NOW" ;

                    if(auto pPoster = GetPoster(type); pPoster)
                    {
                        pPoster->_AllForNow(type);
                    }

                    if(m_bFree)
                    {
                        if(auto itBrowser = m_setBrowser.find(pBrowser); itBrowser != m_setBrowser.end())
                        {
                            avahi_service_browser_free((*itBrowser));
                            m_setBrowser.erase(itBrowser);
                        }
                        CheckStop();
                }
                }
                break;
            case AVAHI_BROWSER_CACHE_EXHAUSTED:
                {
                    pml::log::log(pml::log::Level::kDebug, "pml::dnssd") << "ServiceBrowser: (Browser) ' CACHE_EXHAUSTED " ;
                }
                break;
            default:
                pml::log::log(pml::log::Level::kDebug, "pml::dnssd") << "ServiceBrowser: BrowseCallback: " << event ;
        }
    }

    void ServiceBrowser::ResolveCallback(AvahiServiceResolver* pResolver, AvahiResolverEvent event,const char *name, const char *type, const char *domain, const char *host_name, const AvahiAddress *address, uint16_t port, AvahiStringList *txt)
    {
        if(pResolver)
        {
            //AvahiLookupResultFlags flags;
            std::string sName(name);
            std::string sService(type);
            std::string sDomain(domain);
            /* Called whenever a service has been resolved successfully or timed out */
            switch (event)
            {
                case AVAHI_RESOLVER_FAILURE:
                    pml::log::log(pml::log::Level::kError, "pml::dnssd") << "ServiceBrowser: (Resolver) Failed to resolve service '" << name << "' of type '" << type << "' in domain '" << domain << "': " << avahi_strerror(avahi_client_errno(m_pClient)) ;
                    m_mResolvers.erase(std::string(name)+"__"+std::string(type));
                    avahi_service_resolver_free(pResolver);
                    break;
                case AVAHI_RESOLVER_FOUND:
                {
                    char a[AVAHI_ADDRESS_STR_MAX];
                    avahi_address_snprint(a, sizeof(a), address);
                    m_mutex.lock();
                    
                    if(auto itService = m_mServices.find(sService); itService != m_mServices.end())
                    {
                        auto itInstance = itService->second->mInstances.find(sName); 
                        if(itInstance == itService->second->mInstances.end())
                        {
                            itInstance = itService->second->mInstances.insert(std::make_pair(sName, std::make_shared<dnsInstance>(dnsInstance(sName)))).first;
                        }
                        else
                        {
                            itInstance->second->nUpdate++;
                            itInstance->second->mTxtLast = itInstance->second->mTxt;
                        }

                        itInstance->second->sHostName = host_name;
                        itInstance->second->nPort = port;
                        itInstance->second->sHostIP = a;
                        itInstance->second->sService = sService;


                        for(AvahiStringList* pIterator = txt; pIterator; pIterator = avahi_string_list_get_next(pIterator))
                        {
                            std::string sPair(reinterpret_cast<char*>(avahi_string_list_get_text(pIterator)));
                            size_t nFind = sPair.find("=");
                            if(nFind != std::string::npos)
                            {
                                itInstance->second->mTxt[sPair.substr(0,nFind)] = sPair.substr(nFind+1);
                            }
                        }
                        
                        if(auto pPoster = GetPoster(sService); pPoster)
                        {
                            pPoster->_InstanceResolved(itInstance->second);
                        }
                        m_mutex.unlock();
                        if(itInstance->second->nUpdate == 0)
                        {
                            pml::log::log(pml::log::Level::kInfo, "pml::dnssd") << "ServiceBrowser: Instance '" << itInstance->second->sName << "' resolved at '" << itInstance->second->sHostIP << "'" ;
                        }
                        else
                        {
                            pml::log::log(pml::log::Level::kInfo, "pml::dnssd") << "ServiceBrowser: Instance '" << itInstance->second->sName << "' updated at '" << itInstance->second->sHostIP << "'" ;
                        }
                    }

                }
                break;
            }
        }

        if(m_bFree)
        {
            m_mResolvers.erase(std::string(name)+"__"+std::string(type));
            avahi_service_resolver_free(pResolver);
            CheckStop();
        }
    }

    void ServiceBrowser::RemoveServiceInstance(const std::string& sService, const std::string& sInstance)
    {
        std::scoped_lock lock(m_mutex);

        if(auto itResolver = m_mResolvers.find((sInstance+"__"+sService)); itResolver != m_mResolvers.end())
        {
            avahi_service_resolver_free(itResolver->second);
            m_mResolvers.erase(itResolver);
        }
        
        if(auto itService = m_mServices.find(sService); itService != m_mServices.end())
        {
            auto pPoster = GetPoster(sService);

            if(auto itInstance = itService->second->mInstances.find(sInstance); itInstance != itService->second->mInstances.end())
            {
                if(pPoster)
                {
                    pPoster->_InstanceRemoved(itInstance->second);
                }
                itService->second->mInstances.erase(sInstance);
            }
        }
    }


    void ServiceBrowser::CheckStop()
    {
        --m_nWaitingOn;
        if(m_nWaitingOn == 0)
        {
            //NodeApi::Get().SignalBrowse();

        }
    }

    std::map<std::string, std::shared_ptr<dnsService> >::const_iterator ServiceBrowser::GetServiceBegin()
    {
        std::scoped_lock lock(m_mutex);
        return m_mServices.begin();
    }

    std::map<std::string, std::shared_ptr<dnsService> >::const_iterator ServiceBrowser::GetServiceEnd()
    {
        std::scoped_lock lock(m_mutex);
        return m_mServices.end();
    }

    std::map<std::string, std::shared_ptr<dnsService> >::const_iterator ServiceBrowser::FindService(const std::string& sService)
    {
        std::scoped_lock lock(m_mutex);
        return m_mServices.find(sService);
    }

    std::shared_ptr<ZCPoster> ServiceBrowser::GetPoster(const std::string& sService)
    {
        auto itPoster = m_mServiceBrowse.find(sService);
        if(itPoster != m_mServiceBrowse.end())
        {
            return itPoster->second;
        }
        return nullptr;
    }


    void ServiceBrowser::AddService(const std::string& sService, std::shared_ptr<ZCPoster> pPoster)
    {
        std::scoped_lock lock(m_mutex);
        m_mServiceBrowse.insert(make_pair(sService, pPoster));
        if(m_bBrowsing)
        {
            Browse();
        }
    }

    void ServiceBrowser::RemoveService(const std::string& sService)
    {
        //@todo RemoveService
    }
}