#include "request-app.h"
#include "cluster-info.h"
#include "neighbor-table.h"
#include "resource-info.h"
#include "ip-registry.h"
#include "metrics-collector.h"
#include "ns3/simulator.h"
#include "ns3/node.h"
#include "ns3/node-list.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/inet-socket-address.h"
#include <vector>
#include <algorithm>
#include <limits>
#include <sstream>

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (RequestApp);

TypeId
RequestApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RequestApp")
    .SetParent<Application> ()
    .SetGroupName ("RCSA_SIM")
    .AddConstructor<RequestApp> ();
  return tid;
}

RequestApp::RequestApp ()
  : m_nodeId (0), m_port (8888), m_maxHops (3), m_socket (0), m_nextRequestId (1)
{
}

RequestApp::~RequestApp ()
{
  m_socket = 0;
}

void
RequestApp::Setup (uint32_t nodeId, uint16_t port, uint32_t maxHops, Time timeout)
{
  m_nodeId = nodeId;
  m_port = port;
  m_maxHops = maxHops;
  m_timeout = timeout;
}

void
RequestApp::StartApplication (void)
{
  if (!m_socket)
    {
      TypeId tid = UdpSocketFactory::GetTypeId ();
      m_socket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
      m_socket->Bind (local);
      m_socket->SetRecvCallback (MakeCallback (&RequestApp::ReceiveMessage, this));
    }
}

void
RequestApp::StopApplication (void)
{
  if (m_socket)
    {
      m_socket->Close ();
    }
}

static bool
FindMinimalAllocation (const std::vector<std::pair<uint32_t,double>> &candidates,
                        double requestedAmount,
                        std::vector<uint32_t> &chosenOut,
                        double &totalOut)
{
  size_t n = candidates.size ();
  if (n > 20)
    {
      std::vector<std::pair<uint32_t,double>> sorted = candidates;
      std::sort (sorted.begin (), sorted.end (),
                 [](auto &a, auto &b){ return a.second < b.second; });
      double sum = 0.0;
      std::vector<uint32_t> chosen;
      for (auto const &c : sorted)
        {
          chosen.push_back (c.first);
          sum += c.second;
          if (sum >= requestedAmount)
            {
              chosenOut = chosen;
              totalOut = sum;
              return true;
            }
        }
      return false;
    }

  double bestTotal = std::numeric_limits<double>::max ();
  std::vector<uint32_t> bestChosen;
  bool found = false;

  for (uint32_t mask = 1; mask < (1u << n); ++mask)
    {
      double sum = 0.0;
      std::vector<uint32_t> chosen;
      for (size_t i = 0; i < n; ++i)
        {
          if (mask & (1u << i))
            {
              sum += candidates[i].second;
              chosen.push_back (candidates[i].first);
            }
        }
      if (sum >= requestedAmount && sum < bestTotal)
        {
          bestTotal = sum;
          bestChosen = chosen;
          found = true;
        }
    }

  if (found)
    {
      chosenOut = bestChosen;
      totalOut = bestTotal;
    }
  return found;
}

bool
RequestApp::TryFindReachableCluster (uint32_t resourceType, uint32_t &chIdOut)
{
  Ptr<Node> selfNode = GetNode ();
  Ptr<ClusterInfo> myInfo = selfNode->GetObject<ClusterInfo> ();
  Ptr<NeighborTable> myTable = selfNode->GetObject<NeighborTable> ();

  if (myInfo && myInfo->HasCluster ())
    {
      Ptr<Node> chNode = NodeList::GetNode (myInfo->GetChId ());
      if (chNode)
        {
          Ptr<ResourceInfo> chRes = chNode->GetObject<ResourceInfo> ();
          if (chRes && chRes->GetResourceType () == resourceType)
            {
              chIdOut = myInfo->GetChId ();
              return true;
            }
        }
    }

  if (myTable)
    {
      std::vector<NeighborEntry> sameType = myTable->GetNeighborsByType (resourceType);
      for (auto const &n : sameType)
        {
          Ptr<Node> nNode = NodeList::GetNode (n.nodeId);
          if (!nNode) { continue; }
          Ptr<ClusterInfo> nInfo = nNode->GetObject<ClusterInfo> ();
          if (nInfo && nInfo->HasCluster ())
            {
              chIdOut = nInfo->GetChId ();
              return true;
            }
        }
    }

  return false;
}

bool
RequestApp::SelectBestRelay (uint32_t excludeNodeId, uint32_t &nextHopOut)
{
  // Priority: (1) neighbors already in a cluster (richer network context),
  // ranked by longest connection duration; (2) unclustered neighbors,
  // also ranked by longest connection duration. Excludes the node we
  // just received the request from, to avoid trivial ping-pong.
  Ptr<Node> selfNode = GetNode ();
  Ptr<NeighborTable> myTable = selfNode->GetObject<NeighborTable> ();
  if (!myTable) { return false; }

  std::vector<NeighborEntry> all = myTable->GetAllNeighbors ();
  Time now = Simulator::Now ();

  uint32_t bestClusteredId = 0;
  double bestClusteredDuration = -1.0;
  uint32_t bestUnclusteredId = 0;
  double bestUnclusteredDuration = -1.0;

  for (auto const &n : all)
    {
      if (n.nodeId == excludeNodeId || !IpRegistry::HasIp (n.nodeId))
        {
          continue;
        }

      Ptr<Node> nNode = NodeList::GetNode (n.nodeId);
      if (!nNode) { continue; }

      double duration = (now - n.firstHeard).GetSeconds ();
      Ptr<ClusterInfo> nInfo = nNode->GetObject<ClusterInfo> ();
      bool inCluster = (nInfo && nInfo->HasCluster ());

      if (inCluster)
        {
          if (duration > bestClusteredDuration)
            {
              bestClusteredDuration = duration;
              bestClusteredId = n.nodeId;
            }
        }
      else
        {
          if (duration > bestUnclusteredDuration)
            {
              bestUnclusteredDuration = duration;
              bestUnclusteredId = n.nodeId;
            }
        }
    }

  if (bestClusteredDuration >= 0.0)
    {
      nextHopOut = bestClusteredId;
      return true;
    }
  if (bestUnclusteredDuration >= 0.0)
    {
      nextHopOut = bestUnclusteredId;
      return true;
    }
  return false;
}

void
RequestApp::SendRequestToCh (uint32_t chId, uint32_t requestId, uint32_t requesterId,
                               uint32_t resourceType, double requestedAmount)
{
  if (!IpRegistry::HasIp (chId)) { return; }

  std::ostringstream msg;
  msg << "REQ," << requestId << "," << requesterId << "," << resourceType
      << "," << requestedAmount;
  std::string msgStr = msg.str ();

  Ptr<Packet> packet = Create<Packet> ((uint8_t*) msgStr.c_str (), msgStr.size ());
  InetSocketAddress dest = InetSocketAddress (IpRegistry::GetIp (chId), m_port);
  m_socket->SendTo (packet, 0, dest);
  MetricsCollector::RecordPacketSent ();
}

void
RequestApp::SendFailureToRequester (uint32_t requestId, uint32_t requesterId)
{
  if (!IpRegistry::HasIp (requesterId)) { return; }

  std::ostringstream msg;
  msg << "RES," << requestId << ",0,0,0";
  std::string msgStr = msg.str ();

  Ptr<Packet> packet = Create<Packet> ((uint8_t*) msgStr.c_str (), msgStr.size ());
  InetSocketAddress dest = InetSocketAddress (IpRegistry::GetIp (requesterId), m_port);
  m_socket->SendTo (packet, 0, dest);
  MetricsCollector::RecordPacketSent ();
}

bool
RequestApp::MakeRequest (uint32_t requestedType, double requestedAmount)
{
  Time now = Simulator::Now ();

  uint32_t chId = 0;
  if (TryFindReachableCluster (requestedType, chId))
    {
      uint32_t requestId = m_nextRequestId++;
      PendingRequest pending = { m_nodeId, requestedType, requestedAmount, now };
      m_pending[requestId] = pending;
      MetricsCollector::RecordRequestSent ();
      SendRequestToCh (chId, requestId, m_nodeId, requestedType, requestedAmount);
      Simulator::Schedule (m_timeout, &RequestApp::CheckTimeout, this, requestId);
      return true;
    }

  // Intra-search failed: fall back to inter-resource search (Algorithm 3/4).
  // Relay through a different-type neighbor, hop by hop.
  Ptr<Node> selfNode = GetNode ();
  Ptr<NeighborTable> myTable = selfNode->GetObject<NeighborTable> ();
  if (!myTable || myTable->GetSize () == 0)
    {
      MetricsCollector::RecordRequestSent ();
      MetricsCollector::RecordFailure (0.0);
      std::cout << "t=" << now.GetSeconds () << "s  Vehicle" << m_nodeId
                 << " request FAILED (no neighbors at all), delay=0s" << std::endl;
      return false;
    }

  // Pick the best relay: prefer clustered neighbors, then longest
  // connection duration (Step 1 fix: was arbitrary "first neighbor").
  uint32_t nextHop = 0;
  if (!SelectBestRelay (m_nodeId, nextHop))
    {
      MetricsCollector::RecordRequestSent ();
      MetricsCollector::RecordFailure (0.0);
      std::cout << "t=" << now.GetSeconds () << "s  Vehicle" << m_nodeId
                 << " request FAILED (no viable relay), delay=0s" << std::endl;
      return false;
    }

  uint32_t requestId = m_nextRequestId++;
  PendingRequest pending = { m_nodeId, requestedType, requestedAmount, now };
  m_pending[requestId] = pending;
  MetricsCollector::RecordRequestSent ();
  Simulator::Schedule (m_timeout, &RequestApp::CheckTimeout, this, requestId);

  std::ostringstream msg;
  msg << "IREQ," << requestId << "," << m_nodeId << "," << requestedType
      << "," << requestedAmount << ",0," << m_maxHops << "," << m_nodeId;
  std::string msgStr = msg.str ();

  Ptr<Packet> packet = Create<Packet> ((uint8_t*) msgStr.c_str (), msgStr.size ());
  InetSocketAddress dest = InetSocketAddress (IpRegistry::GetIp (nextHop), m_port);
  m_socket->SendTo (packet, 0, dest);
  MetricsCollector::RecordPacketSent ();

  return true;
}

void
RequestApp::ReceiveMessage (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      uint8_t buffer[160];
      uint32_t size = packet->CopyData (buffer, sizeof (buffer) - 1);
      buffer[size] = '\0';
      std::string received ((char*) buffer);

      if (received.rfind ("REQ,", 0) == 0)
        {
          HandleRequestMessage (received);
        }
      else if (received.rfind ("IREQ,", 0) == 0)
        {
          HandleInterRequestMessage (received);
        }
      else if (received.rfind ("RES,", 0) == 0)
        {
          HandleResponseMessage (received);
        }
    }
}

void
RequestApp::HandleRequestMessage (const std::string &payload)
{
  std::istringstream iss (payload);
  std::string token;
  std::getline (iss, token, ','); // "REQ"
  std::getline (iss, token, ','); uint32_t requestId = std::stoul (token);
  std::getline (iss, token, ','); uint32_t requesterId = std::stoul (token);
  std::getline (iss, token, ','); // resourceType (implicit: I am CH of that type)
  std::getline (iss, token, ','); double requestedAmount = std::stod (token);

  Ptr<Node> selfNode = GetNode ();
  Ptr<ClusterInfo> myInfo = selfNode->GetObject<ClusterInfo> ();
  Ptr<ResourceInfo> myRes = selfNode->GetObject<ResourceInfo> ();

  std::vector<std::pair<uint32_t,double>> candidates;
  if (myRes)
    {
      candidates.push_back ({m_nodeId, myRes->GetAvailableAmount ()});
    }
  if (myInfo)
    {
      for (uint32_t memberId : myInfo->GetMembers ())
        {
          Ptr<Node> mNode = NodeList::GetNode (memberId);
          if (!mNode) { continue; }
          Ptr<ResourceInfo> mRes = mNode->GetObject<ResourceInfo> ();
          if (mRes)
            {
              candidates.push_back ({memberId, mRes->GetAvailableAmount ()});
            }
        }
    }

  std::vector<uint32_t> chosen;
  double totalAllocated = 0.0;
  bool success = FindMinimalAllocation (candidates, requestedAmount, chosen, totalAllocated);

  if (success)
    {
      double remaining = requestedAmount;
      for (uint32_t id : chosen)
        {
          Ptr<Node> vNode = NodeList::GetNode (id);
          Ptr<ResourceInfo> vRes = vNode->GetObject<ResourceInfo> ();
          if (!vRes) { continue; }
          double take = std::min (vRes->GetAvailableAmount (), remaining);
          vRes->SetAvailableAmount (vRes->GetAvailableAmount () - take);
          remaining -= take;
        }
    }

  // Always respond directly to the TRUE original requester's IP,
  // regardless of who actually forwarded this REQ to us.
  if (IpRegistry::HasIp (requesterId))
    {
      std::ostringstream msg;
      msg << "RES," << requestId << "," << (success ? 1 : 0) << ","
          << totalAllocated << "," << chosen.size ();
      std::string msgStr = msg.str ();
      Ptr<Packet> packet = Create<Packet> ((uint8_t*) msgStr.c_str (), msgStr.size ());
      InetSocketAddress dest = InetSocketAddress (IpRegistry::GetIp (requesterId), m_port);
      m_socket->SendTo (packet, 0, dest);
  MetricsCollector::RecordPacketSent ();
    }
}

void
RequestApp::HandleInterRequestMessage (const std::string &payload)
{
  std::istringstream iss (payload);
  std::string token;
  std::getline (iss, token, ','); // "IREQ"
  std::getline (iss, token, ','); uint32_t requestId = std::stoul (token);
  std::getline (iss, token, ','); uint32_t requesterId = std::stoul (token);
  std::getline (iss, token, ','); uint32_t resourceType = std::stoul (token);
  std::getline (iss, token, ','); double requestedAmount = std::stod (token);
  std::getline (iss, token, ','); uint32_t hopCount = std::stoul (token);
  std::getline (iss, token, ','); uint32_t maxHops = std::stoul (token);
  std::getline (iss, token, ','); uint32_t prevHop = std::stoul (token);

  uint32_t chId = 0;
  if (TryFindReachableCluster (resourceType, chId))
    {
      // Found a way in: forward as a normal REQ on behalf of the
      // ORIGINAL requester, so the CH's response goes straight to them.
      SendRequestToCh (chId, requestId, requesterId, resourceType, requestedAmount);
      return;
    }

  if (hopCount >= maxHops)
    {
      SendFailureToRequester (requestId, requesterId);
      return;
    }

  // Relay further: prefer clustered neighbors, then longest connection
  // duration, excluding the one we got this from (Step 1 fix).
  uint32_t nextHop = 0;
  if (!SelectBestRelay (prevHop, nextHop))
    {
      SendFailureToRequester (requestId, requesterId);
      return;
    }

  std::ostringstream msg;
  msg << "IREQ," << requestId << "," << requesterId << "," << resourceType
      << "," << requestedAmount << "," << (hopCount + 1) << "," << maxHops
      << "," << m_nodeId;
  std::string msgStr = msg.str ();

  Ptr<Packet> packet = Create<Packet> ((uint8_t*) msgStr.c_str (), msgStr.size ());
  InetSocketAddress dest = InetSocketAddress (IpRegistry::GetIp (nextHop), m_port);
  m_socket->SendTo (packet, 0, dest);
  MetricsCollector::RecordPacketSent ();
}

void
RequestApp::HandleResponseMessage (const std::string &payload)
{
  std::istringstream iss (payload);
  std::string token;
  std::getline (iss, token, ','); // "RES"
  std::getline (iss, token, ','); uint32_t requestId = std::stoul (token);
  std::getline (iss, token, ','); bool success = (token == "1");
  std::getline (iss, token, ','); double totalAllocated = std::stod (token);
  std::getline (iss, token, ','); uint32_t chosenCount = std::stoul (token);

  auto it = m_pending.find (requestId);
  if (it == m_pending.end ())
    {
      return;
    }

  PendingRequest pending = it->second;
  m_pending.erase (it);

  Time now = Simulator::Now ();
  double delaySeconds = (now - pending.startTime).GetSeconds ();

  if (success)
    {
      MetricsCollector::RecordSuccess (delaySeconds);
      std::cout << "t=" << now.GetSeconds () << "s  Vehicle" << pending.requesterId
                 << " request SUCCESS: got " << pending.requestedAmount
                 << " (allocated total " << totalAllocated << ") from "
                 << chosenCount << " vehicle(s), delay=" << delaySeconds << "s"
                 << std::endl;
    }
  else
    {
      MetricsCollector::RecordFailure (delaySeconds);
      std::cout << "t=" << now.GetSeconds () << "s  Vehicle" << pending.requesterId
                 << " request FAILED, delay=" << delaySeconds << "s" << std::endl;
    }
}

void
RequestApp::CheckTimeout (uint32_t requestId)
{
  auto it = m_pending.find (requestId);
  if (it == m_pending.end ())
    {
      return; // already resolved (success or failure), nothing to do
    }

  PendingRequest pending = it->second;
  m_pending.erase (it);

  Time now = Simulator::Now ();
  double delaySeconds = (now - pending.startTime).GetSeconds ();

  MetricsCollector::RecordFailure (delaySeconds);
  std::cout << "t=" << now.GetSeconds () << "s  Vehicle" << pending.requesterId
             << " request TIMEOUT (no response within " << m_timeout.GetSeconds ()
             << "s), delay=" << delaySeconds << "s" << std::endl;
}

} // namespace ns3
