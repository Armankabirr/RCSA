#include "ip-registry.h"

namespace ns3 {

std::map<uint32_t, Ipv4Address>&
IpRegistry::GetMap (void)
{
  static std::map<uint32_t, Ipv4Address> m;
  return m;
}

void
IpRegistry::Register (uint32_t nodeId, Ipv4Address addr)
{
  GetMap ()[nodeId] = addr;
}

bool
IpRegistry::HasIp (uint32_t nodeId)
{
  return GetMap ().find (nodeId) != GetMap ().end ();
}

Ipv4Address
IpRegistry::GetIp (uint32_t nodeId)
{
  auto it = GetMap ().find (nodeId);
  if (it != GetMap ().end ())
    {
      return it->second;
    }
  return Ipv4Address ("0.0.0.0");
}

} // namespace ns3
