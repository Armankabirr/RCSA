#include "cluster-app.h"
#include "cluster-info.h"
#include "cluster-scoring.h"
#include "neighbor-table.h"
#include "resource-info.h"
#include "ns3/simulator.h"
#include "ns3/node.h"
#include "ns3/node-list.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (ClusterApp);

TypeId
ClusterApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ClusterApp")
    .SetParent<Application> ()
    .SetGroupName ("RCSA_SIM")
    .AddConstructor<ClusterApp> ();
  return tid;
}

ClusterApp::ClusterApp ()
  : m_nodeId (0),
    m_resourceType (0),
    m_evalInterval (Seconds (1.0))
{
}

ClusterApp::~ClusterApp ()
{
}

void
ClusterApp::Setup (uint32_t nodeId, uint32_t resourceType, Time evalInterval)
{
  m_nodeId = nodeId;
  m_resourceType = resourceType;
  m_evalInterval = evalInterval;
}

void
ClusterApp::StartApplication (void)
{
  if (!GetNode ()->GetObject<ClusterInfo> ())
    {
      Ptr<ClusterInfo> info = CreateObject<ClusterInfo> ();
      GetNode ()->AggregateObject (info);
    }

  m_evalEvent = Simulator::Schedule (m_evalInterval, &ClusterApp::EvaluateClustering, this);
}

void
ClusterApp::StopApplication (void)
{
  Simulator::Cancel (m_evalEvent);
}

// Helper: reassign every member of a losing CH over to the winning CH.
static void
TransferMembers (Ptr<ClusterInfo> losingChInfo, uint32_t losingChId,
                  Ptr<ClusterInfo> winningChInfo, uint32_t winningChId)
{
  std::set<uint32_t> oldMembers = losingChInfo->GetMembers ();
  for (uint32_t memberId : oldMembers)
    {
      Ptr<Node> memberNode = NodeList::GetNode (memberId);
      if (!memberNode) { continue; }
      Ptr<ClusterInfo> memberInfo = memberNode->GetObject<ClusterInfo> ();
      if (!memberInfo) { continue; }

      memberInfo->BecomeMember (winningChId);
      winningChInfo->AddMember (memberId);
    }
}

void
ClusterApp::EvaluateClustering (void)
{
  Ptr<Node> selfNode = GetNode ();
  Ptr<ClusterInfo> myInfo = selfNode->GetObject<ClusterInfo> ();
  Ptr<NeighborTable> myTable = selfNode->GetObject<NeighborTable> ();
  Ptr<ResourceInfo> myRes = selfNode->GetObject<ResourceInfo> ();

  if (myInfo && myTable && myRes)
    {
      Time now = Simulator::Now ();
      std::vector<NeighborEntry> sameTypeNeighbors =
          myTable->GetNeighborsByType (m_resourceType);

      for (auto const &neighbor : sameTypeNeighbors)
        {
          uint32_t nvjId = neighbor.nodeId;

          Ptr<Node> nvjNode = NodeList::GetNode (nvjId);
          if (!nvjNode) { continue; }
          Ptr<ClusterInfo> nvjInfo = nvjNode->GetObject<ClusterInfo> ();
          if (!nvjInfo) { continue; }

          bool iAmCH = myInfo->IsCH ();
          bool jIsCH = nvjInfo->IsCH ();

          // Case: both are CHs (Algorithm 1, lines 5-11)
          if (iAmCH && jIsCH)
            {
              if (myInfo->GetChId () == nvjInfo->GetChId ())
                {
                  continue; // already same cluster
                }

              double myScore = ClusterScoring::ComputeScore (
                  myRes->GetResourceAmount (),
                  myTable->GetConnectionDuration (nvjId, now));
              double jScore = ClusterScoring::ComputeScore (
                  neighbor.resourceAmount,
                  myTable->GetConnectionDuration (nvjId, now));

              bool iWin = ClusterScoring::AWins (m_nodeId, myScore, nvjId, jScore);

              if (iWin)
                {
                  // My cluster absorbs NVj's cluster entirely: NVj's
                  // existing members all get reassigned to me too.
                  TransferMembers (nvjInfo, nvjId, myInfo, m_nodeId);
                  myInfo->AddMember (nvjId);
                  nvjInfo->BecomeMember (m_nodeId);
                }
              else
                {
                  TransferMembers (myInfo, m_nodeId, nvjInfo, nvjId);
                  nvjInfo->AddMember (m_nodeId);
                  myInfo->BecomeMember (nvjId);
                }
              continue;
            }

          // Case: NVj is not CH, I am CH (Algorithm 1, lines 13-16)
          if (!jIsCH && iAmCH)
            {
              if (nvjInfo->HasCluster () && nvjInfo->GetChId () != m_nodeId)
                {
                  continue; // NVj already belongs to a different cluster; skip
                }
              nvjInfo->BecomeMember (m_nodeId);
              myInfo->AddMember (nvjId);
              continue;
            }

          // Case: NVj is CH, I am not CH (Algorithm 1, lines 17-20)
          if (jIsCH && !iAmCH)
            {
              if (myInfo->HasCluster () && myInfo->GetChId () != nvjId)
                {
                  continue; // I already belong to a different cluster; skip
                }
              myInfo->BecomeMember (nvjId);
              nvjInfo->AddMember (m_nodeId);
              continue;
            }

          // Case: neither is CH (Algorithm 1, lines 21-23)
          if (!iAmCH && !jIsCH)
            {
              if (myInfo->HasCluster () || nvjInfo->HasCluster ())
                {
                  continue; // one of us already joined a cluster elsewhere
                }

              double myScore = ClusterScoring::ComputeScore (
                  myRes->GetResourceAmount (),
                  myTable->GetConnectionDuration (nvjId, now));
              double jScore = ClusterScoring::ComputeScore (
                  neighbor.resourceAmount,
                  myTable->GetConnectionDuration (nvjId, now));

              bool iWin = ClusterScoring::AWins (m_nodeId, myScore, nvjId, jScore);

              if (iWin)
                {
                  myInfo->BecomeCH (m_nodeId);
                  myInfo->AddMember (nvjId);
                  nvjInfo->BecomeMember (m_nodeId);
                }
              else
                {
                  nvjInfo->BecomeCH (nvjId);
                  nvjInfo->AddMember (m_nodeId);
                  myInfo->BecomeMember (nvjId);
                }
            }
        }
    }

  m_evalEvent = Simulator::Schedule (m_evalInterval, &ClusterApp::EvaluateClustering, this);
}

} // namespace ns3
