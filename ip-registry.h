#ifndef IP_REGISTRY_H
#define IP_REGISTRY_H

#include "ns3/ipv4-address.h"
#include <map>

namespace ns3 {

/**
 * \brief Simple global lookup: NS-3 node ID -> its IPv4 address.
 * Populated once after address assignment; used by RequestApp to know
 * where to actually send REQUEST/ALLOCATION packets.
 */
class IpRegistry
{
public:
  static void Register (uint32_t nodeId, Ipv4Address addr);
  static bool HasIp (uint32_t nodeId);
  static Ipv4Address GetIp (uint32_t nodeId);

private:
  static std::map<uint32_t, Ipv4Address>& GetMap (void);
};

} // namespace ns3

#endif /* IP_REGISTRY_H */
