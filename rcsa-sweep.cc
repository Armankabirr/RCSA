#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/olsr-module.h"
#include "ns3/resource-info.h"
#include "ns3/beacon-app.h"
#include "ns3/neighbor-table.h"
#include "ns3/cluster-app.h"
#include "ns3/cluster-info.h"
#include "ns3/request-app.h"
#include "ns3/ip-registry.h"
#include "ns3/metrics-collector.h"
#include "ns3/manhattan-grid-mobility-model.h"

using namespace ns3;

int
main (int argc, char *argv[])
{
  // Defaults match the paper's baseline (Section 5.1 / Table 1).
  double gridSize = 2000.0;
  double blockSize = 1000.0;
  double density = 100.0;        // vehicles per km^2, Table 1: [50,250]
  double speedKmh = 40.0;        // Table 1: [20,60]
  double requesterRatioPercent = 20.0; // Table 1: [5,50]%
  uint32_t numResourceTypes = 3; // Table 1: [1,5]
  double simTime = 45.0; // extended to 45s to allow all requesters to complete at max density (250/km²) // extended to allow all requesters to complete even at high density
  double requestTime = 15.0;
  double requestInterarrivalSeconds = 0.1; // default: spread requests 0.1s apart
  uint16_t requestPort = 8888;
  uint32_t maxHops = 5;
  double timeoutSeconds = 6.0;

  CommandLine cmd;
  cmd.AddValue ("density", "Vehicle density per km^2", density);
  cmd.AddValue ("speedKmh", "Average vehicle speed in km/h", speedKmh);
  cmd.AddValue ("requesterRatio", "Percent of vehicles that make a request", requesterRatioPercent);
  cmd.AddValue ("numResourceTypes", "Number of resource types", numResourceTypes);
  cmd.AddValue ("simTime", "Simulation time before requests fire", simTime);
  cmd.AddValue ("maxHops", "Max relay hops for inter-resource search", maxHops);
  cmd.AddValue ("timeoutSeconds", "Request timeout in seconds", timeoutSeconds);
  cmd.AddValue ("requestInterarrival", "Request inter-arrival time in seconds (0=all simultaneous)", requestInterarrivalSeconds);
  cmd.Parse (argc, argv);

  MetricsCollector::Reset ();

  double areaKm2 = (gridSize / 1000.0) * (gridSize / 1000.0);
  uint32_t numVehicles = static_cast<uint32_t> (density * areaKm2);

  NodeContainer rsuNodes;
  int numLines = static_cast<int> (gridSize / blockSize) + 1;
  rsuNodes.Create (numLines * numLines);

  MobilityHelper rsuMobility;
  Ptr<ListPositionAllocator> rsuPositions = CreateObject<ListPositionAllocator> ();
  for (int i = 0; i < numLines; ++i)
    for (int j = 0; j < numLines; ++j)
      rsuPositions->Add (Vector (i * blockSize, j * blockSize, 0.0));
  rsuMobility.SetPositionAllocator (rsuPositions);
  rsuMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  rsuMobility.Install (rsuNodes);

  NodeContainer vehicleNodes;
  vehicleNodes.Create (numVehicles);

  MobilityHelper vehicleMobility;
  Ptr<RandomRectanglePositionAllocator> vehiclePositions = CreateObject<RandomRectanglePositionAllocator> ();
  Ptr<UniformRandomVariable> xAlloc = CreateObject<UniformRandomVariable> ();
  xAlloc->SetAttribute ("Min", DoubleValue (0.0));
  xAlloc->SetAttribute ("Max", DoubleValue (gridSize));
  Ptr<UniformRandomVariable> yAlloc = CreateObject<UniformRandomVariable> ();
  yAlloc->SetAttribute ("Min", DoubleValue (0.0));
  yAlloc->SetAttribute ("Max", DoubleValue (gridSize));
  vehiclePositions->SetX (xAlloc);
  vehiclePositions->SetY (yAlloc);
  vehicleMobility.SetPositionAllocator (vehiclePositions);
  vehicleMobility.SetMobilityModel ("ns3::ManhattanGridMobilityModel");
  vehicleMobility.Install (vehicleNodes);

  // Apply the swept speed to every vehicle's mobility model.
  for (uint32_t i = 0; i < numVehicles; ++i)
    {
      Ptr<ManhattanGridMobilityModel> mob =
          vehicleNodes.Get (i)->GetObject<ManhattanGridMobilityModel> ();
      if (mob)
        {
          mob->SetMaxSpeedKmh (speedKmh);
        }
    }

  Ptr<UniformRandomVariable> typeRandom = CreateObject<UniformRandomVariable> ();
  Ptr<UniformRandomVariable> amountRandom = CreateObject<UniformRandomVariable> ();
  amountRandom->SetAttribute ("Min", DoubleValue (50.0));
  amountRandom->SetAttribute ("Max", DoubleValue (500.0));

  std::vector<uint32_t> types (numVehicles);
  std::vector<double> amounts (numVehicles);
  for (uint32_t i = 0; i < numVehicles; ++i)
    {
      types[i] = typeRandom->GetInteger (0, numResourceTypes - 1);
      amounts[i] = amountRandom->GetValue ();
      Ptr<ResourceInfo> res = CreateObject<ResourceInfo> ();
      res->SetResourceType (types[i]);
      res->SetResourceAmount (amounts[i]);
      vehicleNodes.Get (i)->AggregateObject (res);
    }

  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211a);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                  "DataMode", StringValue ("OfdmRate6Mbps"),
                                  "ControlMode", StringValue ("OfdmRate6Mbps"));

  YansWifiChannelHelper channelHelper = YansWifiChannelHelper::Default ();
  Ptr<YansWifiChannel> channel = channelHelper.Create ();

  YansWifiPhyHelper vehiclePhy;
  vehiclePhy.SetChannel (channel);
  vehiclePhy.Set ("TxPowerStart", DoubleValue (16.0));
  vehiclePhy.Set ("TxPowerEnd", DoubleValue (16.0));

  WifiMacHelper mac;
  mac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer vehicleDevices = wifi.Install (vehiclePhy, mac, vehicleNodes);

  YansWifiPhyHelper rsuPhy;
  rsuPhy.SetChannel (channel);
  rsuPhy.Set ("TxPowerStart", DoubleValue (33.0));
  rsuPhy.Set ("TxPowerEnd", DoubleValue (33.0));
  NetDeviceContainer rsuDevices = wifi.Install (rsuPhy, mac, rsuNodes);

  OlsrHelper olsr;
  InternetStackHelper internet;
  internet.SetRoutingHelper (olsr);
  internet.Install (vehicleNodes);
  internet.Install (rsuNodes);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.0.0", "255.255.0.0");
  Ipv4InterfaceContainer vehicleIfaces = ipv4.Assign (vehicleDevices);
  ipv4.Assign (rsuDevices);

  for (uint32_t i = 0; i < numVehicles; ++i)
    {
      uint32_t realId = vehicleNodes.Get (i)->GetId ();
      IpRegistry::Register (realId, vehicleIfaces.GetAddress (i));
    }

  Ptr<UniformRandomVariable> jitter = CreateObject<UniformRandomVariable> ();
  jitter->SetAttribute ("Min", DoubleValue (0.0));
  jitter->SetAttribute ("Max", DoubleValue (1.0));

  std::vector<Ptr<RequestApp>> requestApps (numVehicles);

  for (uint32_t i = 0; i < numVehicles; ++i)
    {
      uint32_t realId = vehicleNodes.Get (i)->GetId ();

      Ptr<BeaconApp> beacon = CreateObject<BeaconApp> ();
      beacon->Setup (realId, types[i], amounts[i], Seconds (1.0));
      vehicleNodes.Get (i)->AddApplication (beacon);
      beacon->SetStartTime (Seconds (1.0 + jitter->GetValue ()));
      beacon->SetStopTime (Seconds (simTime + timeoutSeconds));

      Ptr<ClusterApp> cluster = CreateObject<ClusterApp> ();
      // Keep eval at 1.0s — more frequent evaluation creates unacceptable overhead
      cluster->Setup (realId, types[i], Seconds (1.0));
      vehicleNodes.Get (i)->AddApplication (cluster);
      cluster->SetStartTime (Seconds (5.0 + jitter->GetValue ()));
      cluster->SetStopTime (Seconds (simTime + timeoutSeconds));

      Ptr<RequestApp> request = CreateObject<RequestApp> ();
      request->Setup (realId, requestPort, maxHops, Seconds (timeoutSeconds));
      vehicleNodes.Get (i)->AddApplication (request);
      request->SetStartTime (Seconds (5.0));
      request->SetStopTime (Seconds (simTime + timeoutSeconds));
      requestApps[i] = request;
    }

  // Only a percentage of vehicles act as requesters (Table 1: requester ratio).
  Ptr<UniformRandomVariable> reqTypeRandom = CreateObject<UniformRandomVariable> ();
  Ptr<UniformRandomVariable> reqAmountRandom = CreateObject<UniformRandomVariable> ();
  reqAmountRandom->SetAttribute ("Min", DoubleValue (50.0));
  reqAmountRandom->SetAttribute ("Max", DoubleValue (300.0));

  uint32_t numRequesters = static_cast<uint32_t> (numVehicles * requesterRatioPercent / 100.0);
  for (uint32_t i = 0; i < numRequesters && i < numVehicles; ++i)
    {
      uint32_t reqType = reqTypeRandom->GetInteger (0, numResourceTypes - 1);
      double reqAmount = reqAmountRandom->GetValue ();
      // Spread requests: if interarrival > 0, schedule each requester at
      // requestTime + i*interarrival; otherwise all fire at requestTime.
      double scheduleTime = requestTime + (requestInterarrivalSeconds > 0.0
                                              ? i * requestInterarrivalSeconds
                                              : 0.0);
      Simulator::Schedule (Seconds (scheduleTime), &RequestApp::MakeRequest,
                            requestApps[i], reqType, reqAmount);
    }

  Simulator::Stop (Seconds (simTime + timeoutSeconds + 1.0));
  Simulator::Run ();

  // Single machine-readable summary line for batch collection.
  std::cout << "CSV," << density << "," << speedKmh << "," << requesterRatioPercent
             << "," << numResourceTypes << "," << MetricsCollector::GetTotalRequests ()
             << "," << MetricsCollector::GetTotalSuccesses () << ","
             << MetricsCollector::GetTotalFailures () << ","
             << MetricsCollector::GetSuccessRatio () << ","
             << MetricsCollector::GetTotalPackets () << ","
             << MetricsCollector::GetAverageDelay () << ","
             << MetricsCollector::GetAverageSuccessDelay () << std::endl;

  Simulator::Destroy ();
  return 0;
}
