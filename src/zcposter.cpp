#include "zcposter.h"

namespace pml::dnssd
{
    void ZCPoster::_InstanceResolved(std::shared_ptr<dnsInstance> pInstance)
    {
        std::scoped_lock lg(m_mutex);
        InstanceResolved(pInstance);

    }

    void ZCPoster::_AllForNow(const std::string& sService)
    {
        std::scoped_lock lg(m_mutex);
        AllForNow(sService);
    }

    void ZCPoster::_Finished()
    {
        std::scoped_lock lg(m_mutex);
        Finished();
    }

    void ZCPoster::_RegistrationNodeError()
    {
        std::scoped_lock lg(m_mutex);
        RegistrationNodeError();
    }

    void ZCPoster::_InstanceRemoved(std::shared_ptr<dnsInstance> pInstance)
    {
        std::scoped_lock lg(m_mutex);
        InstanceRemoved(pInstance);
    }
}
