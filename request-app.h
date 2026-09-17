#ifndef REQUEST_APP_H
#define REQUEST_APP_H

#include "ns3/application.h"
#include "ns3/socket.h"
#include "ns3/nstime.h"
#include <map>

namespace ns3 {

struct PendingRequest
{
  uint32_t requesterId;
  uint32_t resourceType;
  double   requestedAmount;
  Time     startTime;
};

/**
 * \brief Implements Algorithm 2 (Intra-Resource Search) and Algorithm 3/4
 * (Inter-Resource Search) from Choi et al., Sensors 2024, 24, 2175,
 * plus the allocation logic from Section 4.2.3.
 *
 * Intra-search: requester reaches a same-type cluster directly.
 * Inter-search: requester relays through different-type neighbors,
 * hop by hop, until a node that can reach the requested type is found,
 * which then forwards a normal intra-search request to that cluster's CH.
 * The CH always responds directly to the ORIGINAL requester's IP
 * (looked up via IpRegistry), not back through the relay chain.
 */
class RequestApp : public Application
{
public:
  static TypeId GetTypeId (void);

  RequestApp ();
  virtual ~RequestApp ();

  void Setup (uint32_t nodeId, uint16_t port, uint32_t maxHops, Time timeout);

  bool MakeRequest (uint32_t requestedType, double requestedAmount);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ReceiveMessage (Ptr<Socket> socket);
  void HandleRequestMessage (const std::string &payload);
  void HandleInterRequestMessage (const std::string &payload);
  void HandleResponseMessage (const std::string &payload);
  void CheckTimeout (uint32_t requestId);

  void SendRequestToCh (uint32_t chId, uint32_t requestId, uint32_t requesterId,
                         uint32_t resourceType, double requestedAmount);
  void SendFailureToRequester (uint32_t requestId, uint32_t requesterId);
  bool TryFindReachableCluster (uint32_t resourceType, uint32_t &chIdOut);
  bool SelectBestRelay (uint32_t excludeNodeId, uint32_t &nextHopOut);

  uint32_t m_nodeId;
  uint16_t m_port;
  uint32_t m_maxHops;
  Time m_timeout;
  Ptr<Socket> m_socket;
  uint32_t m_nextRequestId;
  std::map<uint32_t, PendingRequest> m_pending;
};

} // namespace ns3

#endif /* REQUEST_APP_H */
