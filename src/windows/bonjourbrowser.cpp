#include "bonjourbrowser.h"
#include <thread>
#include "zcposter.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <iostream>
#include "log.h"

using namespace std;
using namespace pml::dnssd;


void DNSSD_API IterateServiceTypes( DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *serviceName, const char *regtype, const char *replyDomain, void *context )
{
    Log::Get(Log::LOG_DEBUG) << "BonjourBrowser: IterateServiceTypes" << endl;
	ServiceBrowser* pBrowser = (ServiceBrowser *) context;
    pBrowser->IterateTypes(sdRef, flags, interfaceIndex, errorCode, serviceName, regtype, replyDomain, context);
}


void DNSSD_API IterateServiceInstances( DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *serviceName, const char *regtype, const char *replyDomain, void *context )
{
    Log::Get(Log::LOG_DEBUG) << "BonjourBrowser: IterateServiceInstances" << endl;

	ServiceBrowser* pBrowser = (ServiceBrowser *) context;
	pBrowser->IterateInstances(sdRef, flags, interfaceIndex, errorCode, serviceName, regtype, replyDomain, context );
}

void DNSSD_API ResolveInstance( DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *fullname, const char *hosttarget, uint16_t port, uint16_t txtLen, const unsigned char *txtRecord, void *context )
{
    Log::Get(Log::LOG_DEBUG) << "BonjourBrowser: ResolveInstance" << endl;
	ServiceBrowser* pBrowser = (ServiceBrowser *) context;
	pBrowser->Resolve(sdRef, flags, interfaceIndex, errorCode, fullname, hosttarget, port, txtLen, txtRecord, context );
}


void DNSSD_API GetAddress( DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *hostname, const struct sockaddr *address, uint32_t ttl, void *context )
{
    Log::Get(Log::LOG_DEBUG) << "BonjourBrowser: GetAddress" << endl;
	ServiceBrowser* pBrowser = (ServiceBrowser *) context;
	pBrowser->Address(sdRef, flags, interfaceIndex, errorCode, hostname, address, ttl, context );
}




ServiceBrowser::ServiceBrowser()
{

}


void ServiceBrowser::AddService(const std::string& sService, std::shared_ptr<ZCPoster> pPoster)
{
    lock_guard<mutex> lock(m_mutex);
    m_mServiceBrowse.insert(make_pair(sService, pPoster));
}

void ServiceBrowser::RemoveService(const std::string& sService)
{
    lock_guard<mutex> lock(m_mutex);
    m_mServiceBrowse.erase(sService);
}



bool ServiceBrowser::StartBrowser()
{
    Log::Get(Log::LOG_DEBUG) << "BonjourBrowser: Start" << endl;


	DNSServiceRef client = NULL;
	DNSServiceErrorType err = DNSServiceBrowse(&client, 0, 0, "_services._dns-sd._udp", "", IterateServiceTypes, this );
	if ( err == 0 )
	{
        m_mClientToFd[client] = DNSServiceRefSockFD(client);
        Log::Get(Log::LOG_DEBUG) << "BonjourBrowser: StartThread" << endl;

        thread th(RunSelect, this);
        th.detach();

		return true;
	}
    Log::Get(Log::LOG_ERROR) << "BonjourBrowser: Failed to start" << endl;
    return false;
}

void ServiceBrowser::RunSelect(ServiceBrowser* pBrowser)
{
    int count = 0;
	for ( ; ; )
	{
        if ( pBrowser->m_mClientToFd.size() == 0 )
		{
		    Log::Get(Log::LOG_DEBUG) << "BonjourBrowser: Finished" << endl;
		    for(auto itPoster = pBrowser->GetPosterBegin(); itPoster != pBrowser->GetPosterEnd(); ++itPoster)
            {
                itPoster->second->_Finished();
            }
            break;
        }
		fd_set readfds;
		FD_ZERO(&readfds);
		int nfds = 0;
		for ( auto itClient = pBrowser->m_mClientToFd.cbegin() ; itClient != pBrowser->m_mClientToFd.cend() ; itClient++ )
		{
			FD_SET(itClient->second, &readfds);
			nfds = max((int)itClient->second, nfds);
		}


		Log::Get(Log::LOG_DEBUG) << "BonjourBrowser: Start select: fd =" << pBrowser->m_mClientToFd.size() << " nfds =" << nfds << endl;
		struct timeval tv = { 0, 1000 };

		//mDNSPosixGetFDSet(m, &nfds, &readfds, &tv);
		int result = select(nfds, &readfds, (fd_set*)NULL, (fd_set*)NULL, &tv);

		if ( result > 0 )
		{
		    Log::Get(Log::LOG_DEBUG) << "BonjourBrowser: Select done"  << endl;
            //
            // While iterating through the loop, the callback functions might delete
            // the client pointed to by the current iterator, so I have to increment
            // it BEFORE calling DNSServiceProcessResult
            //
			for ( auto itClient =pBrowser-> m_mClientToFd.cbegin() ; itClient != pBrowser->m_mClientToFd.cend() ; )
			{
                auto jj = itClient++;
				if (FD_ISSET(jj->second, &readfds) )
				{
					DNSServiceErrorType err = DNSServiceProcessResult(jj->first);
                    if ( ++count > 10 )
					{
						break;
					}
				}
			}
		}
		else
		{
		    Log::Get(Log::LOG_DEBUG) << "Result = " << result << endl;
			break;
		}
        if ( count > 10 )
        {
			break;
		}
	}
}

void DNSSD_API ServiceBrowser::IterateTypes( DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *serviceName, const char *regtype, const char *replyDomain, void *context )
{
	// Service types are added to the top level of the tree
	//
	if ( flags & kDNSServiceFlagsAdd && !errorCode )
	{
		string r( regtype );
		size_t n = r.find_last_of('.');
		if ( n != string::npos )
		{
			r = r.substr(0,n);
		}
		n = r.find_last_of('.');
		if ( n != string::npos )
		{
			r = r.substr(0,n);
		}
		string service_type = serviceName;
        service_type += '.';
		service_type += r.c_str();
		auto itServicePost = m_mServiceBrowse.find(service_type);

		if(itServicePost != m_mServiceBrowse.end())
        {
            auto itService = m_setServiceTypes.find( service_type );
            if ( itService == m_setServiceTypes.end() )
            {
                m_setServiceTypes.insert( service_type );
                shared_ptr<dnsService> pService = make_shared<dnsService>(service_type);

                DNSServiceRef client = NULL;
                DNSServiceErrorType err = DNSServiceBrowse( &client,  0,  0,  service_type.c_str(),  "",  IterateServiceInstances,  context );

                if ( err == 0 )
                {
                    m_mClientToFd[client] = DNSServiceRefSockFD(client);
                    m_mServRefToString[client] = pService->sService;
                    m_mServices.insert(make_pair(pService->sService, pService));
                }
                else
                {
                    Log::Get(Log::LOG_ERROR) << "Bonjour Browser: Error trying to browse service type: " << service_type.c_str() << endl;
                }
            }
        }
	}
    if ( !(flags & kDNSServiceFlagsMoreComing ) )
	{
        auto itClient = m_mClientToFd.find( sdRef );
        if ( itClient != m_mClientToFd.end() )
		{
			m_mClientToFd.erase( itClient );
		}
        //m_Tree.Invalidate();
    }
}

void DNSSD_API ServiceBrowser::IterateInstances( DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *serviceName, const char *regtype, const char *replyDomain, void *context )
{
	if ( (flags & kDNSServiceFlagsAdd) && !errorCode )
	{
        auto itServToString = m_mServRefToString.find( sdRef );
        if ( itServToString != m_mServRefToString.end() )
		{
		    auto itService = m_mServices.find(itServToString->second);

            shared_ptr<dnsInstance> pInstance = make_shared<dnsInstance>(serviceName);
            pInstance->sService = itServToString->second;

            DNSServiceRef client = NULL;
            DNSServiceErrorType err = DNSServiceResolve ( &client, 0, interfaceIndex, serviceName, regtype, replyDomain, ResolveInstance, context );

            Log::Get(Log::LOG_DEBUG) << "BonjourBrowser: Resolving instance of " << serviceName << " " << regtype << endl;

            if ( err == 0 )
			{
                m_mClientToFd[client] = DNSServiceRefSockFD(client);
                itService->second->mInstances.insert(make_pair(serviceName,pInstance));

                m_mInstances[client] = pInstance;
            }
			else
			{
			    Log::Get(Log::LOG_DEBUG) << "BonjourBrowser: Error trying to browse service instance: " << serviceName << endl;
            }
        }
        else
        {
		}
    }
    if ( !(flags & kDNSServiceFlagsMoreComing ) )
	{
        m_mClientToFd.erase(sdRef);
        m_mServRefToString.erase( sdRef );
    }
}

void DNSSD_API ServiceBrowser::Resolve( DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *fullname, const char *hosttarget, uint16_t port, uint16_t txtLen, const unsigned char *txtRecord, void *context )
{
    if ( !errorCode )
	{
        auto itInstance = m_mInstances.find( sdRef );
        if ( itInstance != m_mInstances.end() )
		{
		    itInstance->second->sHostName = hosttarget;

            DNSServiceRef client = NULL;
			MIB_IFROW IfRow;
			IfRow.dwIndex = interfaceIndex;
			DWORD result  = GetIfEntry ( &IfRow );
			string sAdapter = "Unknown";
			if ( result == 0 )
			{
			    sAdapter = (char*)IfRow.bDescr;
                DNSServiceErrorType err = DNSServiceGetAddrInfo( &client, kDNSServiceFlagsTimeout, interfaceIndex, kDNSServiceProtocol_IPv4, hosttarget, GetAddress, context );

                if ( err == 0 )
				{
                    m_mClientToFd[client] = DNSServiceRefSockFD(client);
                    m_mInstances[client] = itInstance->second;
					Log::Get(Log::LOG_DEBUG)  << "BonjourBrowser: Looking up " << itInstance->second->sHostName << " on " << sAdapter << endl;
                }
				else
				{
					Log::Get(Log::LOG_ERROR) << "BonjourBrowser: Error looking up address info for "<<  itInstance->second->sHostName << endl;
                }
			}
			uint8_t lolo = (port >> 0) & 0xFF;
            uint8_t lohi = (port >> 8) & 0xFF;
			itInstance->second->nPort = (lolo << 8) | (lohi << 0);
			itInstance->second->sInterface = sAdapter;

            if  ( errorCode == 0 )
			{
                size_t pos = 0;
                for ( ; ; )
				{
                    if ( pos >= txtLen )
					{
						break;
					}
                    int length = txtRecord[pos] & 0xff;
                    pos++;
                    if ( length == 0 )
                    {
						break;
					}
					stringstream ssText;
					for(int i = 0; i < length; i++)
                    {
                        ssText << txtRecord[pos];
                        ++pos;
                    }
                    size_t nFind = ssText.str().find("=");
                    if(nFind != string::npos)
                    {
                        itInstance->second->mTxt.insert(make_pair(ssText.str().substr(0,nFind), ssText.str().substr(nFind+1)));
                    }
                }
            }
        }
	}
    if ( !(flags & kDNSServiceFlagsMoreComing ) )
	{
        auto itClient = m_mClientToFd.find( sdRef );
        if ( itClient != m_mClientToFd.end() )
		{
            m_mClientToFd.erase( itClient );
		}
		m_mInstances.erase(sdRef);
    }
}
void DNSSD_API ServiceBrowser::Address( DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *hostname, const struct sockaddr *address, uint32_t ttl, void *context )
{
    if ( !errorCode )
	{
        auto itInstance = m_mInstances.find( sdRef );
        if ( itInstance != m_mInstances.end() )
		{
		    const sockaddr_in *in = (const sockaddr_in *) address;
            char *ip = inet_ntoa( in->sin_addr );
            itInstance->second->sHostIP = ip;
        }
        auto itPoster = m_mServiceBrowse.find(itInstance->second->sService);
        if(itPoster != m_mServiceBrowse.end())
        {
            itPoster->second->_InstanceResolved(itInstance->second);
        }

    }
    if ( !(flags & kDNSServiceFlagsMoreComing ) )
	{
        m_mClientToFd.erase(sdRef);
        m_mInstances.erase(sdRef);
    }
}



map<string, shared_ptr<dnsService> >::const_iterator ServiceBrowser::GetServiceBegin()
{
    lock_guard<mutex> lock(m_mutex);
    return m_mServices.begin();
}

map<string, shared_ptr<dnsService> >::const_iterator ServiceBrowser::GetServiceEnd()
{
    lock_guard<mutex> lock(m_mutex);
    return m_mServices.end();
}

map<string, shared_ptr<dnsService> >::const_iterator ServiceBrowser::FindService(const string& sService)
{
    lock_guard<mutex> lock(m_mutex);
    return m_mServices.find(sService);
}
