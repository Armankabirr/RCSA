#ifndef CLUSTER_INFO_H
#define CLUSTER_INFO_H

#include "ns3/object.h"
#include "ns3/nstime.h"
#include <set>

namespace ns3 {

/**
 * \brief Per-vehicle Resource Cluster membership state.
 * Matches the paper's RCH/RCM concept (Section 4.1).
 *
 * A vehicle is either:
 *  - Unclustered (m_hasCluster = false)
 *  - A Resource Cluster Member, RCM (m_hasCluster = true, m_isCH = false,
 *    m_chId = the ID of its Cluster Header)
 *  - A Resource Cluster Header, RCH (m_hasCluster = true, m_isCH = true,
 *    m_chId = its own ID, m_members = set of RCM ids it manages)
 */
class ClusterInfo : public Object
{
public:
  static TypeId GetTypeId (void);

  ClusterInfo ();
  virtual ~ClusterInfo ();

  bool HasCluster (void) const;
  bool IsCH (void) const;
  uint32_t GetChId (void) const;

  // Become an unclustered vehicle (reset state)
  void Reset (void);

  // Become a Cluster Header of a brand-new (or newly re-formed) cluster
  void BecomeCH (uint32_t selfId);

  // Become a Cluster Member of the cluster headed by chId
  void BecomeMember (uint32_t chId);

  // (CH only) manage member list
  void AddMember (uint32_t memberId);
  void RemoveMember (uint32_t memberId);
  std::set<uint32_t> GetMembers (void) const;
  uint32_t GetMemberCount (void) const;

private:
  bool m_hasCluster;
  bool m_isCH;
  uint32_t m_chId;
  std::set<uint32_t> m_members; // only meaningful if m_isCH == true
};

} // namespace ns3

#endif /* CLUSTER_INFO_H */
