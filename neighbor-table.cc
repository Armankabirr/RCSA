#include "neighbor-table.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (NeighborTable);

TypeId
NeighborTable::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NeighborTable")
    .SetParent<Object> ()
    .SetGroupName ("RCSA_SIM")
    .AddConstructor<NeighborTable> ();
  return tid;
}

NeighborTable::NeighborTable ()
{
}

NeighborTable::~NeighborTable ()
{
}

void
NeighborTable::UpdateNeighbor (uint32_t nodeId, uint32_t resourceType,
                                 double resourceAmount, Time now)
{
  auto it = m_neighbors.find (nodeId);
  if (it == m_neighbors.end ())
    {
      NeighborEntry entry;
      entry.nodeId = nodeId;
      entry.resourceType = resourceType;
      entry.resourceAmount = resourceAmount;
      entry.firstHeard = now;
      entry.lastHeard = now;
      m_neighbors[nodeId] = entry;
    }
  else
    {
      it->second.resourceType = resourceType;
      it->second.resourceAmount = resourceAmount;
      it->second.lastHeard = now;
    }
}

bool
NeighborTable::HasNeighbor (uint32_t nodeId) const
{
  return m_neighbors.find (nodeId) != m_neighbors.end ();
}

NeighborEntry
NeighborTable::GetNeighbor (uint32_t nodeId) const
{
  auto it = m_neighbors.find (nodeId);
  if (it != m_neighbors.end ())
    {
      return it->second;
    }
  NeighborEntry empty = { 0, 0, 0.0, Seconds (0), Seconds (0) };
  return empty;
}

std::vector<NeighborEntry>
NeighborTable::GetAllNeighbors (void) const
{
  std::vector<NeighborEntry> result;
  for (auto const &pair : m_neighbors)
    {
      result.push_back (pair.second);
    }
  return result;
}

std::vector<NeighborEntry>
NeighborTable::GetNeighborsByType (uint32_t resourceType) const
{
  std::vector<NeighborEntry> result;
  for (auto const &pair : m_neighbors)
    {
      if (pair.second.resourceType == resourceType)
        {
          result.push_back (pair.second);
        }
    }
  return result;
}

Time
NeighborTable::GetConnectionDuration (uint32_t nodeId, Time now) const
{
  auto it = m_neighbors.find (nodeId);
  if (it != m_neighbors.end ())
    {
      return now - it->second.firstHeard;
    }
  return Seconds (0);
}

void
NeighborTable::RemoveStale (Time now, Time maxAge)
{
  for (auto it = m_neighbors.begin (); it != m_neighbors.end (); )
    {
      if (now - it->second.lastHeard > maxAge)
        {
          it = m_neighbors.erase (it);
        }
      else
        {
          ++it;
        }
    }
}

uint32_t
NeighborTable::GetSize (void) const
{
  return static_cast<uint32_t> (m_neighbors.size ());
}

} // namespace ns3
