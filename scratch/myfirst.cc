/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 The Boeing Company
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

//
// This script configures two nodes on an 802.11b physical layer, with
// 802.11b NICs in adhoc mode, and by default, sends one packet of 1000
// (application) bytes to the other node.  The physical layer is configured
// to receive at a fixed RSS (regardless of the distance and transmit
// power); therefore, changing position of the nodes has no effect.
//
// There are a number of command-line options available to control
// the default behavior.  The list of available command-line options
// can be listed with the following command:
// ./waf --run "wifi-simple-adhoc --help"
//
// For instance, for this configuration, the physical layer will
// stop successfully receiving packets when rss drops below -97 dBm.
// To see this effect, try running:
//
// ./waf --run "wifi-simple-adhoc --rss=-97 --numPackets=20"
// ./waf --run "wifi-simple-adhoc --rss=-98 --numPackets=20"
// ./waf --run "wifi-simple-adhoc --rss=-99 --numPackets=20"
//
// Note that all ns-3 attributes (not just the ones exposed in the below
// script) can be changed at command line; see the documentation.
//
// This script can also be helpful to put the Wifi layer into verbose
// logging mode; this command will turn on all wifi logging:
//
// ./waf --run "wifi-simple-adhoc --verbose=1"
//
// When you are done, you will notice two pcap trace files in your directory.
// If you have tcpdump installed, you can try this:
//
// tcpdump -r wifi-simple-adhoc-0-0.pcap -nn -tt
//

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/aodv-module.h"
#include "ns3/timer.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <cmath>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiSimpleAdhoc");
Time ts;

void ReceivePacket(Ptr<Socket> socket)
{
  while (socket->Recv())
  {
    NS_LOG_UNCOND("Received one packet!");
    NS_LOG_UNCOND("latop:"<<(Simulator::Now()-ts).GetMilliSeconds());
  }
}

static void GenerateTraffic(Ptr<Socket> socket, uint32_t pktSize,
                            uint32_t pktCount, Time pktInterval, Ptr<Node> ncc)
{
  if (pktCount > 0)
  {
    // here to write new header?
    Ptr<Packet> pk = Create<Packet>(pktSize);
    aodv::addHeader ahp = aodv::addHeader();
    ahp.SetPreviousHop(Ipv4Address("10.1.1.1"));
    ahp.SetPreviousEnergy(ncc->GetSelfEnergy());
    ahp.SetDstAddress(Ipv4Address("10.1.1.1"));
    pk->AddHeader(ahp);
    ncc->SelfEnergyInDe(100, false);
    socket->Send(pk);
    std::cout << "New packet!" << std::endl;
    ts = Simulator::Now();
    Simulator::Schedule(pktInterval, &GenerateTraffic,
                        socket, pktSize, pktCount - 1, pktInterval, ncc);
  }
  else
  {
    socket->Close();
  }
}

int main(int argc, char *argv[])
{
  std::string phyMode("DsssRate1Mbps");
  double rss = -80;           // -dBm
  uint32_t packetSize = 1000; // bytes
  uint32_t numPackets = 500;
  double interval = 5; // seconds
  bool verbose = false;
  int numNodes = 25;
  // double gridsize = 1000.0;
  double mr = 150;
  // uint32_t seed = time(0);
  uint32_t seed = 1509965984; //tested topology, works.
  // std::cout << seed << std::endl;
  ts = Simulator::Now();
  std::srand(seed);

  int ori[5] = {50,150,250,350,450};
  int loca_x[numNodes], loca_y[numNodes];

  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 5; j++) {
      loca_x[i*5+j] = ori[i]+(rand()%40)-80;
      loca_y[i*5+j] = ori[j]+(rand()%40)-80;
      // std::cout << loca_x[i*10+j] << "\t" << loca_y[i*10+j] << std::endl;
    }
  }


  CommandLine cmd;

  cmd.AddValue("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue("rss", "received signal strength", rss);
  cmd.AddValue("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue("numPackets", "number of packets generated", numPackets);
  cmd.AddValue("interval", "interval (seconds) between packets", interval);
  cmd.AddValue("verbose", "turn on all WifiNetDevice log components", verbose);

  cmd.Parse(argc, argv);
  // Convert to time object
  Time interPacketInterval = Seconds(interval);

  // disable fragmentation for frames below 2200 bytes
  Config::SetDefault("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue("2200"));
  // turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("2200"));
  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode",
                     StringValue(phyMode));
  // Set the detection threshold
  // Config::Set("ns3::WifiNetDevice/Phy/EnergyDetectionThreshold", DoubleValue(-82.0));

  NodeContainer c;
  c.Create(numNodes);

  // The below set of helpers will help us to put together the wifi NICs we want
  WifiHelper wifi;
  if (verbose)
  {
    wifi.EnableLogComponents(); // Turn on all Wifi logging
  }
  wifi.SetStandard(WIFI_PHY_STANDARD_80211b);

  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
  // This is one parameter that matters when using FixedRssLossModel
  // set it to zero; otherwise, gain will be added
  wifiPhy.Set("RxGain", DoubleValue(0));
  // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
  wifiPhy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
  // The below FixedRssLossModel will cause the rss to be fixed regardless
  // of the distance between the two stations, and the transmit power
  wifiChannel.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(mr));
  wifiPhy.SetChannel(wifiChannel.Create());

  // Add a mac and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                               "DataMode", StringValue(phyMode),
                               "ControlMode", StringValue(phyMode));
  // Set it to adhoc mode
  wifiMac.SetType("ns3::AdhocWifiMac");
  NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, c);

  // Note that with FixedRssLossModel, the positions below are not
  // used for received signal strength.
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
  for (int i = 0; i < numNodes; i++)
  {
    positionAlloc->Add(Vector(loca_x[i], loca_y[i], 0.0));
    c.Get(i)->SetSelfEnergy(1000000);
    c.Get(i)->SetSelfV_value(0);
  }
  mobility.SetPositionAllocator(positionAlloc);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(c);

  AodvHelper aodv;
  InternetStackHelper internet_stack;
  internet_stack.SetRoutingHelper (aodv);
  internet_stack.Install(c);

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO("Assign IP Addresses.");
  ipv4.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign(devices);

  // Here determines the receiver
  TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
  // create a socket that binds with 
  Ptr<Socket> recvSink = Socket::CreateSocket(c.Get(numNodes - 1), tid);
  InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), 80);
  recvSink->Bind(local);
  recvSink->SetRecvCallback(MakeCallback(&ReceivePacket));

  Ptr<Socket> source = Socket::CreateSocket(c.Get(0), tid);
  InetSocketAddress remote = InetSocketAddress(Ipv4Address("10.1.1.25"), 80);
  source->SetAllowBroadcast(true);
  source->Connect(remote);

  // Tracing
  wifiPhy.EnablePcap("wifi-simple-adhoc", devices);

  // Output what we are doing
  NS_LOG_UNCOND("Testing " << numPackets << " packets sent with receiver rss " << rss);
  // NS_LOG_UNCOND("Size of uint32_t:"<<sizeof(uint32_t)<<"\tSize of float:" << sizeof(float));

  // NS_LOG_UNCOND(sizeof(double));
  // NS_LOG_UNCOND(sizeof(float));
  // NS_LOG_UNCOND(sizeof(unsigned int));
  // NS_LOG_UNCOND(sizeof(unsigned short int));

  Simulator::ScheduleWithContext(source->GetNode()->GetId(),
                                 Seconds(1.0), &GenerateTraffic,
                                 source, packetSize, numPackets, interPacketInterval, c.Get(0));

  Simulator::Run();
  Simulator::Destroy ();

  return 0;
}
