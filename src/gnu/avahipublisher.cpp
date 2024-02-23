#include "avahipublisher.h"
#include "log.h"

using namespace std;
using namespace pml::dnssd;

void entry_group_callback(AvahiEntryGroup *g, AvahiEntryGroupState state, AVAHI_GCC_UNUSED void *userdata)
{
    ServicePublisher* pPublisher = reinterpret_cast<ServicePublisher*>(userdata);
    pPublisher->EntryGroupCallback(g, state);
}

void client_callback(AvahiClient *c, AvahiClientState state, AVAHI_GCC_UNUSED void * userdata)
{
    ServicePublisher* pPublisher = reinterpret_cast<ServicePublisher*>(userdata);
    pPublisher->ClientCallback(c, state);
}



void ServicePublisher::EntryGroupCallback(AvahiEntryGroup *pGroup, AvahiEntryGroupState state)
{
    for(auto& [sName, info] : m_mServices)
    {
        if(info.pGroup == pGroup)
        {
            switch (state)
            {
                case AVAHI_ENTRY_GROUP_ESTABLISHED :
                    pmlLog(pml::LOG_INFO, "pml::dnssd") << "ServicePublisher: Service '" << sName << "' successfully established." ;
                    break;
                case AVAHI_ENTRY_GROUP_COLLISION :
                    Collision(info);
                    break;
                case AVAHI_ENTRY_GROUP_FAILURE :
                    pmlLog(pml::LOG_ERROR, "pml::dnssd") << "ServicePublisher: Entry group failure: " << avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(pGroup))) ;
                    break;
                case AVAHI_ENTRY_GROUP_UNCOMMITED:
                    pmlLog(pml::LOG_TRACE, "pml::dnssd") << "ServicePublisher: Service '" << sName << "' uncommited." ;
                    break;
                case AVAHI_ENTRY_GROUP_REGISTERING:
                    pmlLog(pml::LOG_TRACE, "pml::dnssd") << "ServicePublisher: Service '" << sName << "o' registering." ;
                    break;
            }
            break;
        }
    }
}

bool ServicePublisher::AddService(const std::string& sName, const std::string& sService, unsigned short nPort, const std::map<std::string, std::string>& mTxt)
{
    if(m_pClient)
    {
        avahiInfo info;
        info.sName = sName;
        info.sService = sService;
        info.nPort = nPort;
        info.psName = avahi_strdup(info.sName.c_str());
        info.mTxt = mTxt;
        return CreateService(info);
    }
    return false;
}

bool ServicePublisher::CreateService(avahiInfo& info)
{
    pmlLog(pml::LOG_DEBUG, "pml::dnssd") << "ServicePublisher: Adding service " << info.sName ;

    int ret;
    if(info.pGroup == nullptr)
    {
        pmlLog(pml::LOG_DEBUG, "pml::dnssd") << "ServicePublisher: Create new entry group";
        info.pGroup = avahi_entry_group_new(m_pClient, entry_group_callback, reinterpret_cast<void*>(this));
        if(info.pGroup == nullptr)
        {
            pmlLog(pml::LOG_ERROR, "pml::dnssd") << "ServicePublisher: avahi_entry_group_new() failed: " << avahi_strerror(avahi_client_errno(m_pClient)) ;
            return false;
        }
    }
        
    

    if(info.mTxt.empty() == false)
    {
        AvahiStringList* pList = GetTxtList(info);
        if(!pList)
        {
            pmlLog(pml::LOG_ERROR, "pml::dnssd") << "ServicePublisher: Failed to create list" ;
            return false;
        }    
    
        if ((ret = avahi_entry_group_add_service_strlst(info.pGroup, AVAHI_IF_UNSPEC, AVAHI_PROTO_INET, (AvahiPublishFlags)0, info.psName, info.sService.c_str(), NULL, NULL, info.nPort, pList)) < 0)
        {
            if (ret == AVAHI_ERR_COLLISION)
            {
                return Collision(info);
            }
            else
            {
                pmlLog(pml::LOG_ERROR, "pml::dnssd") << "ServicePublisher: Failed to add '" << info.sService << "' service: " << avahi_strerror(ret);
                return false;
            }
        }
        avahi_string_list_free(pList);
    }
    if ((ret = avahi_entry_group_commit(info.pGroup)) < 0)
    {
        pmlLog(pml::LOG_ERROR, "pml::dnssd") << "ServicePublisher: Failed to commit entry group: " << avahi_strerror(ret);
        return false;
    }
    return true;
}

bool ServicePublisher::Collision(avahiInfo& info)
{
    /* A service name collision with a local service happened. Let's
     * pick a new name */
    char *n = avahi_alternative_service_name(info.psName);
    avahi_free(info.psName);
    info.psName = n;
    pmlLog(pml::LOG_DEBUG, "pml::dnssd") << "ServicePublisher: Service name collision, renaming service to " << info.sName ;
    avahi_entry_group_reset(info.pGroup);
    return CreateService(info);
}

void ServicePublisher::Stop()
{
    if(m_pThreadedPoll)
    {
        avahi_threaded_poll_stop(m_pThreadedPoll);
    }

    if (m_pClient)
    {
        avahi_client_free(m_pClient);
        m_pClient = 0;
    }
    if (m_pThreadedPoll)
    {
        avahi_threaded_poll_free(m_pThreadedPoll);
        m_pThreadedPoll = 0;
    }
    for(auto& [sName, info] : m_mServices)
    {
        avahi_free(info.psName);
        avahi_entry_group_free(info.pGroup);
    }
    m_mServices.clear();
}

void ServicePublisher::ThreadQuit()
{
    avahi_threaded_poll_quit(m_pThreadedPoll);
    m_pThreadedPoll = NULL;
    Stop();
}

void ServicePublisher::ClientCallback(AvahiClient* pClient, AvahiClientState state)
{
    if(pClient)
    {
        /* Called whenever the client or server state changes */
        switch (state)
        {
        case AVAHI_CLIENT_S_RUNNING:
            pmlLog(pml::LOG_DEBUG, "pml::dnssd") << "ServicePublisher: Client: Running" ;
             m_pClient = pClient;
            break;
        case AVAHI_CLIENT_FAILURE:
            pmlLog(pml::LOG_ERROR, "pml::dnssd") << "ServicePublisher: Client failure: " <<  avahi_strerror(avahi_client_errno(pClient)) ;
            ThreadQuit();
            break;
        case AVAHI_CLIENT_S_COLLISION:
        /* Let's drop our registered services. When the server is back
         * in AVAHI_SERVER_RUNNING state we will register them
         * again with the new host name. */
        case AVAHI_CLIENT_S_REGISTERING:
            /* The server records are now being established. This
             * might be caused by a host name change. We need to wait
             * for our own records to register until the host name is
             * properly esatblished. */
             pmlLog(pml::LOG_DEBUG, "pml::dnssd") << "ServicePublisher: Client: Collison or registering" ;
            for(const auto& [sName, info] : m_mServices)
            {
                avahi_entry_group_reset(info.pGroup);
            }
            break;
        case AVAHI_CLIENT_CONNECTING:
            pmlLog(pml::LOG_DEBUG, "pml::dnssd") << "ServicePublisher: Client: Connecting" ;
        }
    }
}


bool ServicePublisher::Start()
{
    m_pClient = nullptr;
    int error;

    if (!(m_pThreadedPoll = avahi_threaded_poll_new()))
    {
        pmlLog(pml::LOG_ERROR, "pml::dnssd") << "ServicePublisher: Failed to create thread poll object." ;
        Stop();
        return false;
    }

    
    /* Allocate a new client */
    m_pClient = avahi_client_new(avahi_threaded_poll_get(m_pThreadedPoll), (AvahiClientFlags)0, client_callback, reinterpret_cast<void*>(this), &error);
    /* Check wether creating the client object succeeded */
    if (!m_pClient)
    {
        pmlLog(pml::LOG_ERROR, "pml::dnssd") << "ServicePublisher: Failed to create client: " << avahi_strerror(error) ;
        Stop();
        return false;
    }
    pmlLog(pml::LOG_DEBUG, "pml::dnssd") << "ServicePublisher: Started" ;
    
    /* Run the main loop */
    avahi_threaded_poll_start(m_pThreadedPoll);
    return true;
}

ServicePublisher::ServicePublisher()=default;

ServicePublisher::~ServicePublisher()
{
    Stop();
}



void ServicePublisher::AddTxt(const std::string& sName, const std::string& sKey, const std::string& sValue, bool bModify)
{
    auto itService= m_mServices.find(sName);
    if(itService != m_mServices.end())
    {
        itService->second.mTxt[sKey] = sValue;
        if(bModify)
        {
            Modify(itService->second);
        }
    }
}

void ServicePublisher::SetTxt(const std::string& sName, const std::map<std::string, std::string>& mTxt)
{
    auto itService= m_mServices.find(sName);
    if(itService != m_mServices.end())
    {
        itService->second.mTxt.clear();

        for(const auto& [sKey, sValue] : mTxt)
        {
            itService->second.mTxt[sKey] = sValue;
        }
        Modify(itService->second);
    }
}

void ServicePublisher::RemoveTxt(const std::string& sName, const std::string& sKey, bool bModify)
{
    auto itService= m_mServices.find(sName);
    if(itService != m_mServices.end())
    {
        itService->second.mTxt.erase(sKey);
        if(bModify)
        {
            Modify(itService->second);
        }
    }
}

AvahiStringList* ServicePublisher::GetTxtList(const avahiInfo& info)
{
    pmlLog(pml::LOG_DEBUG, "pml::dnssd") << "ServicePublisher: Create string list" ;
    AvahiStringList* pList = nullptr;
    for(const auto& [sKey, sValue] : info.mTxt)
    {
        pmlLog(pml::LOG_TRACE, "pml::dnssd") << sKey << "=" << sValue ;
        if(pList == nullptr)
        {
            pmlLog(pml::LOG_DEBUG, "pml::dnssd") << "ServicePublisher: Create new string list" ;
            auto sPair = sKey + "="+sValue;
            pList = avahi_string_list_new(sPair.c_str(),nullptr);
        }
        else
        {
            pmlLog(pml::LOG_DEBUG, "pml::dnssd") << "ServicePublisher: String list exists" ;
            pList = avahi_string_list_add_pair(pList, sKey.c_str(), sValue.c_str());
        }
    }
    return pList;
}


void ServicePublisher::Modify(const avahiInfo& info)
{
    pmlLog(pml::LOG_TRACE, "pml::dnssd") << "Modify" ;
    if(m_pThreadedPoll)
    {
        AvahiStringList* pList = GetTxtList(info);
        if(!pList)
        {
            pmlLog(pml::LOG_ERROR, "pml::dnssd") << "ServicePublisher: Failed to create list" ;
        }
        else
        {
            
            if (auto ret = avahi_entry_group_update_service_txt_strlst(info.pGroup, AVAHI_IF_UNSPEC, AVAHI_PROTO_INET, (AvahiPublishFlags)0, info.psName, info.sService.c_str(), NULL, pList); ret < 0)
            {
                if (ret == AVAHI_ERR_COLLISION)
                {
                }
                else
                {
                    pmlLog(pml::LOG_ERROR, "pml::dnssd") << "ServicePublisher: Failed to update '" << info.sName << "' service: " << avahi_strerror(ret) ;
                }
            }
            avahi_string_list_free(pList);
        }
    }
}

bool ServicePublisher::RemoveService(const std::string& sName)
{
    if(auto itInfo = m_mServices.find(sName); itInfo != m_mServices.end() && m_pClient)
    {
        if(auto nRet = avahi_entry_group_reset(itInfo->second.pGroup); nRet < 0)
        {
            pmlLog(pml::LOG_ERROR, "pml::dnssd") << "ServicePublisher: Failed to remove '" << itInfo->second.sName << "' service: " << avahi_strerror(nRet) ;
            return false;
        }

        if(auto nRet = avahi_entry_group_free(itInfo->second.pGroup); nRet < 0)
        {
            pmlLog(pml::LOG_ERROR, "pml::dnssd") << "ServicePublisher: Failed to free '" << itInfo->second.sName << "' service: " << avahi_strerror(nRet) ;
        }
        pmlLog(pml::LOG_INFO, "pml::dnssd") << itInfo->second.sName << " removed";

        m_mServices.erase(itInfo);
        return true;
    }
    return false;
}





