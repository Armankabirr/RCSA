#include "cluster-info.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (ClusterInfo);

TypeId
ClusterInfo::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ClusterInfo")
    .SetParent<Object> ()
    .SetGroupName ("RCSA_SIM")
    .AddConstructor<ClusterInfo> ();
  return tid;
}

ClusterInfo::ClusterInfo ()
  : m_hasCluster (false),
    m_isCH (false),
    m_chId (0)
{
}

ClusterInfo::~ClusterInfo ()
{
}

bool
ClusterInfo::HasCluster (void) const
{
  return m_hasCluster;
}

bool
ClusterInfo::IsCH (void) const
{
  return m_hasCluster && m_isCH;
}

uint32_t
ClusterInfo::GetChId (void) const
{
  return m_chId;
}

void
ClusterInfo::Reset (void)
{
  m_hasCluster = false;
  m_isCH = false;
  m_chId = 0;
  m_members.clear ();
}

void
ClusterInfo::BecomeCH (uint32_t selfId)
{
  m_hasCluster = true;
  m_isCH = true;
  m_chId = selfId;
  m_members.clear ();
}

void
ClusterInfo::BecomeMember (uint32_t chId)
{
  m_hasCluster = true;
  m_isCH = false;
  m_chId = chId;
  m_members.clear (); // members list only meaningful for a CH
}

void
ClusterInfo::AddMember (uint32_t memberId)
{
  m_members.insert (memberId);
}

void
ClusterInfo::RemoveMember (uint32_t memberId)
{
  m_members.erase (memberId);
}

std::set<uint32_t>
ClusterInfo::GetMembers (void) const
{
  return m_members;
}

uint32_t
ClusterInfo::GetMemberCount (void) const
{
  return static_cast<uint32_t> (m_members.size ());
}

} // namespace ns3
