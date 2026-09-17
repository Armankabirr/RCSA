#include "resource-info.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (ResourceInfo);

TypeId
ResourceInfo::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ResourceInfo")
    .SetParent<Object> ()
    .SetGroupName ("RCSA_SIM")
    .AddConstructor<ResourceInfo> ();
  return tid;
}

ResourceInfo::ResourceInfo ()
  : m_resourceType (0),
    m_resourceAmount (0.0),
    m_availableAmount (0.0)
{
}

ResourceInfo::~ResourceInfo ()
{
}

void
ResourceInfo::SetResourceType (uint32_t type)
{
  m_resourceType = type;
}

uint32_t
ResourceInfo::GetResourceType (void) const
{
  return m_resourceType;
}

void
ResourceInfo::SetResourceAmount (double amount)
{
  m_resourceAmount = amount;
  m_availableAmount = amount;
}

double
ResourceInfo::GetResourceAmount (void) const
{
  return m_resourceAmount;
}

void
ResourceInfo::SetAvailableAmount (double amount)
{
  m_availableAmount = amount;
}

double
ResourceInfo::GetAvailableAmount (void) const
{
  return m_availableAmount;
}

} // namespace ns3
