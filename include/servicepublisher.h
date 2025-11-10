
#ifndef PML_DNSSD_SERVICE_PUBLISHER_H
#define PML_DNSSD_SERVICE_PUBLISHER_H

#include <map>
#include <string>

namespace pml::dnssd
{
    class ServicePublisher
    {
        public:
            ServicePublisher()=default;
            virtual ~ServicePublisher()=default;

            virtual bool Start()=0;
            virtual void Stop()=0;
            

            virtual bool AddService(const std::string& sName, const std::string& sService, unsigned short nPort, const std::map<std::string, std::string>& mTxt)=0;

            virtual bool RemoveService(const std::string& sName)=0;

            virtual void AddTxt(const std::string& sName, const std::string& sKey, const std::string& sValue, bool bModify)=0;
            virtual void SetTxt(const std::string& sName, const std::map<std::string, std::string>& mTxt)=0;
            virtual void RemoveTxt(const std::string& sName, const std::string& sKey,bool bModify)=0;

    };
}
#endif // PML_DNSSD_SERVICE_PUBLISHER_H