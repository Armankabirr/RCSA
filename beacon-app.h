#ifndef BEACON_APP_H
#define BEACON_APP_H

#include "ns3/application.h"
#include "ns3/socket.h"
#include "ns3/event-id.h"

namespace ns3 {

/**
 * \brief Periodically broadcasts a beacon (ID, resource type, resource
 * amount) via UDP, and listens for beacons from neighboring vehicles.
 * Matches the paper's beacon message description in Section 3.1/4.1.1.
 */
class BeaconApp : public Application
{
public:
  static TypeId GetTypeId (void);

  BeaconApp ();
  virtual ~BeaconApp ();

  void Setup (uint32_t nodeId, uint32_t resourceType, double resourceAmount,
              Time beaconInterval);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void SendBeacon (void);
  void ReceiveBeacon (Ptr<Socket> socket);

  Ptr<Socket> m_socket;
  uint32_t m_nodeId;
  uint32_t m_resourceType;
  double   m_resourceAmount;
  Time     m_beaconInterval;
  EventId  m_sendEvent;
  uint16_t m_port;
};

} // namespace ns3

#endif /* BEACON_APP_H */
