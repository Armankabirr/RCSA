#ifndef CLUSTER_APP_H
#define CLUSTER_APP_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/nstime.h"

namespace ns3 {

/**
 * \brief Implements Algorithm 1 (Resource Cluster construction) from
 * Choi et al., Sensors 2024, 24, 2175. Periodically evaluates same-type
 * neighbors from the NeighborTable and applies the CH comparison/merge/
 * join logic to update this vehicle's ClusterInfo state.
 */
class ClusterApp : public Application
{
public:
  static TypeId GetTypeId (void);

  ClusterApp ();
  virtual ~ClusterApp ();

  void Setup (uint32_t nodeId, uint32_t resourceType, Time evalInterval);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void EvaluateClustering (void);

  uint32_t m_nodeId;
  uint32_t m_resourceType;
  Time m_evalInterval;
  EventId m_evalEvent;
};

} // namespace ns3

#endif /* CLUSTER_APP_H */
