#include "beacon-app.h"
#include "neighbor-table.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/mobility-model.h"
#include "ns3/node.h"
#include <sstream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BeaconApp");
NS_OBJECT_ENSURE_REGISTERED (BeaconApp);

TypeId
BeaconApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BeaconApp")
    .SetParent<Application> ()
    .SetGroupName ("RCSA_SIM")
    .AddConstructor<BeaconApp> ();
  return tid;
}

BeaconApp::BeaconApp ()
  : m_socket (0),
    m_nodeId (0),
    m_resourceType (0),
    m_resourceAmount (0.0),
    m_beaconInterval (Seconds (1.0)),
    m_port (9999)
{
}

BeaconApp::~BeaconApp ()
{
  m_socket = 0;
}

void
BeaconApp::Setup (uint32_t nodeId, uint32_t resourceType, double resourceAmount,
                   Time beaconInterval)
{
  m_nodeId = nodeId;
  m_resourceType = resourceType;
  m_resourceAmount = resourceAmount;
  m_beaconInterval = beaconInterval;
}

void
BeaconApp::StartApplication (void)
{
  // Ensure this node has a NeighborTable to write into.
  if (!GetNode ()->GetObject<NeighborTable> ())
    {
      Ptr<NeighborTable> table = CreateObject<NeighborTable> ();
      GetNode ()->AggregateObject (table);
    }

  if (!m_socket)
    {
      TypeId tid = UdpSocketFactory::GetTypeId ();
      m_socket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
      m_socket->SetAllowBroadcast (true);
      m_socket->Bind (local);
      m_socket->SetRecvCallback (MakeCallback (&BeaconApp::ReceiveBeacon, this));
    }

  m_sendEvent = Simulator::Schedule (m_beaconInterval, &BeaconApp::SendBeacon, this);
}

void
BeaconApp::StopApplication (void)
{
  if (m_socket)
    {
      m_socket->Close ();
    }
  Simulator::Cancel (m_sendEvent);
}

void
BeaconApp::SendBeacon (void)
{
  std::ostringstream msg;
  msg << m_nodeId << "," << m_resourceType << "," << m_resourceAmount;
  std::string msgStr = msg.str ();

  Ptr<Packet> packet = Create<Packet> ((uint8_t*) msgStr.c_str (), msgStr.size ());

  InetSocketAddress remote = InetSocketAddress (Ipv4Address ("255.255.255.255"), m_port);
  m_socket->SendTo (packet, 0, remote);

  m_sendEvent = Simulator::Schedule (m_beaconInterval, &BeaconApp::SendBeacon, this);
}

void
BeaconApp::ReceiveBeacon (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      uint8_t buffer[64];
      uint32_t size = packet->CopyData (buffer, sizeof (buffer) - 1);
      buffer[size] = '\0';
      std::string received ((char*) buffer);

      uint32_t senderId = 0, senderType = 0;
      double senderAmount = 0.0;
      std::istringstream iss (received);
      std::string token;
      std::getline (iss, token, ','); senderId = std::stoul (token);
      std::getline (iss, token, ','); senderType = std::stoul (token);
      std::getline (iss, token, ','); senderAmount = std::stod (token);

      if (senderId != m_nodeId)
        {
          Ptr<NeighborTable> table = GetNode ()->GetObject<NeighborTable> ();
          if (table)
            {
              table->UpdateNeighbor (senderId, senderType, senderAmount,
                                       Simulator::Now ());
            }
        }
    }
}

} // namespace ns3
