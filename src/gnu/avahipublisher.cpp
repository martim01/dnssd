#include "avahipublisher.h"
#include "log.h"

using namespace std;
using namespace pml;

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
    m_pGroup = pGroup;
    /* Called whenever the entry group state changes */
    switch (state)
    {
    case AVAHI_ENTRY_GROUP_ESTABLISHED :
        /* The entry group has been established successfully */
        Log() << "mDNS:ServicePublisher\tService '" << m_psName << "' successfully established.";
        break;
    case AVAHI_ENTRY_GROUP_COLLISION :
    {
        Collision();
        break;
    }
    case AVAHI_ENTRY_GROUP_FAILURE :
        Log(LOG_ERROR) << "mDNS:ServicePublisher\tEntry group failure: " << avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(m_pGroup)));
        /* Some kind of failure happened while we were registering our services */
        ThreadQuit();
        break;
    case AVAHI_ENTRY_GROUP_UNCOMMITED:
    case AVAHI_ENTRY_GROUP_REGISTERING:
        ;
    }
}

void ServicePublisher::CreateServices()
{
    if(m_pClient)
    {
        int ret;

        /* If this is the first time we're called, let's create a new
         * entry group if necessary */
        if (!m_pGroup)
        {
            if (!(m_pGroup = avahi_entry_group_new(m_pClient, entry_group_callback, reinterpret_cast<void*>(this))))
            {
                Log(LOG_ERROR) << "mDNS:ServicePublisher\tavahi_entry_group_new() failed: " << avahi_strerror(avahi_client_errno(m_pClient));
                ThreadQuit();
                return;
            }
        }

        /* If the group is empty (either because it was just created, or
         * because it was reset previously, add our entries.  */
        if (avahi_entry_group_is_empty(m_pGroup))
        {
            Log(LOG_DEBUG) << "mDNS:ServicePublisher\tAdding service " << m_psName;

            if(m_mTxt.empty() == false)
            {
                AvahiStringList* pList = GetTxtList();
                if(!pList)
                {

                    Log(LOG_ERROR) << "mDNS:ServicePublisher\tFailed to create list";
                }
                else
                {
                    if ((ret = avahi_entry_group_add_service_strlst(m_pGroup, AVAHI_IF_UNSPEC, AVAHI_PROTO_INET, (AvahiPublishFlags)0, m_psName, m_sService.c_str(), NULL, NULL, m_nPort, pList)) < 0)
                    {
                        if (ret == AVAHI_ERR_COLLISION)
                        {
                            Collision();
                            return;
                        }
                        else
                        {
                            Log(LOG_ERROR) << "mDNS:ServicePublisher\tFailed to add '" << m_sService << "' service: " << avahi_strerror(ret);
                            ThreadQuit();
                            return;
                        }
                    }
                    avahi_string_list_free(pList);
                }
            }
            else
            {
                if ((ret = avahi_entry_group_add_service(m_pGroup, AVAHI_IF_UNSPEC, AVAHI_PROTO_INET, (AvahiPublishFlags)0, m_psName, m_sService.c_str(), NULL, NULL, m_nPort)) < 0)
                {
                    if (ret == AVAHI_ERR_COLLISION)
                    {
                        Collision();
                        return;
                    }
                    else
                    {
                        Log(LOG_ERROR) << "mDNS:ServicePublisher\tFailed to add '" << m_sService << "' service: " << avahi_strerror(ret);
                        ThreadQuit();
                        return;
                    }
                }
            }
            /* Tell the server to register the service */
            if ((ret = avahi_entry_group_commit(m_pGroup)) < 0)
            {
                Log(LOG_ERROR) << "mDNS:ServicePublisher\tFailed to commit entry group: " << avahi_strerror(ret);
                ThreadQuit();
                return;
            }
        }

    }
}

void ServicePublisher::Collision()
{
    /* A service name collision with a local service happened. Let's
     * pick a new name */
    char *n = avahi_alternative_service_name(m_psName);
    avahi_free(m_psName);
    m_psName = n;
    Log(LOG_DEBUG) << "mDNS:ServicePublisher\tService name collision, renaming service to " << m_psName;
    avahi_entry_group_reset(m_pGroup);
    CreateServices();
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
    if(m_psName)
    {
        avahi_free(m_psName);
        m_psName = 0;
    }
}

void ServicePublisher::ThreadQuit()
{
    if(m_pThreadedPoll)
    {
        avahi_threaded_poll_quit(m_pThreadedPoll);
    }
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
            Log(LOG_DEBUG) << "mDNS:ServicePublisher\tClient: Running";
            /* The server has startup successfully and registered its host
             * name on the network, so it's time to create our services */
             m_pClient = pClient;
            CreateServices();
            break;
        case AVAHI_CLIENT_FAILURE:
            Log(LOG_ERROR) << "mDNS:ServicePublisher\tClient failure: " <<  avahi_strerror(avahi_client_errno(pClient));
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
             Log(LOG_DEBUG) << "mDNS:ServicePublisher\tClient: Collison or registering";
            if (m_pGroup)
            {
                avahi_entry_group_reset(m_pGroup);
            }
            break;
        case AVAHI_CLIENT_CONNECTING:
            Log(LOG_DEBUG) << "mDNS:ServicePublisher\tClient: Connecting";
        }
    }
}


bool ServicePublisher::Start()
{
    m_pClient = NULL;
    int error;


    /* Allocate main loop object */
    if (!(m_pThreadedPoll = avahi_threaded_poll_new()))
    {
        Log(LOG_ERROR) << "mDNS:ServicePublisher\tFailed to create thread poll object.";
        Stop();
        return false;
    }

    m_psName = avahi_strdup(m_sName.c_str());
    /* Allocate a new client */
    m_pClient = avahi_client_new(avahi_threaded_poll_get(m_pThreadedPoll), (AvahiClientFlags)0, client_callback, reinterpret_cast<void*>(this), &error);
    /* Check wether creating the client object succeeded */
    if (!m_pClient)
    {
        Log(LOG_ERROR) << "mDNS:ServicePublisher\tFailed to create client: " << avahi_strerror(error);
        Stop();
        return false;
    }
    Log(LOG_DEBUG) << "mDNS:ServicePublisher\tStarted";
    /* After 10s do some weird modification to the service */
    //avahi_thread_poll_get(thread_poll)->timeout_new(avahi_thread_poll_get(thread_poll),avahi_elapse_time(&tv, 1000*10, 0),modify_callback,client);
    /* Run the main loop */
    avahi_threaded_poll_start(m_pThreadedPoll);
    return true;
}

ServicePublisher::ServicePublisher(string sName, string sService, unsigned short nPort, std::string sHostname) :
    m_pClient(0),
    m_pGroup(0),
    m_pThreadedPoll(0),
    m_sName(sName),
    m_sService(sService),
    m_nPort(nPort),
    m_sHostname(sHostname),
    m_psName(0)
{
    Log(LOG_DEBUG) << m_sHostname;

}

ServicePublisher::~ServicePublisher()
{
    Stop();
}



void ServicePublisher::AddTxt(string sKey, string sValue, bool bModify)
{
    m_mTxt[sKey] = sValue;
    if(bModify)
    {
        Modify();
    }
}

void ServicePublisher::RemoveTxt(string sKey, bool bModify)
{
    m_mTxt.erase(sKey);
    if(bModify)
    {
        Modify();
    }
}


AvahiStringList* ServicePublisher::GetTxtList()
{
    Log(LOG_DEBUG) << "mDNS:ServicePublisher\tCreate string list";
    AvahiStringList* pList = NULL;
    for(map<string, string>::iterator itTxt = m_mTxt.begin(); itTxt != m_mTxt.end(); ++itTxt)
    {
        Log(LOG_DEBUG) << itTxt->first << "=" << itTxt->second;
        if(pList == NULL)
        {
            std::string sPair(itTxt->first);
            sPair += "="+itTxt->second;
            pList = avahi_string_list_new(sPair.c_str(),NULL);
        }
        else
        {
            pList = avahi_string_list_add_pair(pList, itTxt->first.c_str(), itTxt->second.c_str());
        }
    }
    return pList;
}


void ServicePublisher::Modify()
{
    Log(LOG_DEBUG) << "Modify";
    if(m_pThreadedPoll)
    {
        AvahiStringList* pList = GetTxtList();
        if(!pList)
        {
            Log(LOG_ERROR) << "mDNS:ServicePublisher\tFailed to create list";
        }
        else
        {
            int ret;
            if ((ret = avahi_entry_group_update_service_txt_strlst(m_pGroup, AVAHI_IF_UNSPEC, AVAHI_PROTO_INET, (AvahiPublishFlags)0, m_psName, m_sService.c_str(), NULL, pList)) < 0)
            {
                if (ret == AVAHI_ERR_COLLISION)
                {
                    return;
                }
                else
                {
                    Log(LOG_ERROR) << "mDNS:ServicePublisher\tFailed to update '" << m_sService << "' service: " << avahi_strerror(ret);
                    ThreadQuit();
                    return;
                }
            }
            avahi_string_list_free(pList);
        }
    }
}





