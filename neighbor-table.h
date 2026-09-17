#ifndef NEIGHBOR_TABLE_H
#define NEIGHBOR_TABLE_H

#include "ns3/object.h"
#include "ns3/nstime.h"
#include <map>
#include <vector>

namespace ns3 {

struct NeighborEntry
{
  uint32_t nodeId;
  uint32_t resourceType;
  double   resourceAmount;
  Time     firstHeard;
  Time     lastHeard;
};

class NeighborTable : public Object
{
public:
  static TypeId GetTypeId (void);

  NeighborTable ();
  virtual ~NeighborTable ();

  void UpdateNeighbor (uint32_t nodeId, uint32_t resourceType,
                        double resourceAmount, Time now);

  bool HasNeighbor (uint32_t nodeId) const;
  NeighborEntry GetNeighbor (uint32_t nodeId) const;

  std::vector<NeighborEntry> GetAllNeighbors (void) const;
  std::vector<NeighborEntry> GetNeighborsByType (uint32_t resourceType) const;

  Time GetConnectionDuration (uint32_t nodeId, Time now) const;

  void RemoveStale (Time now, Time maxAge);

  uint32_t GetSize (void) const;

private:
  std::map<uint32_t, NeighborEntry> m_neighbors;
};

} // namespace ns3

#endif /* NEIGHBOR_TABLE_H */
