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

/**
 *
 * FASE 1 DO TRABALHO NS3
 * =======================
 *
 * Integrantes:
 *
 * - Rafael David Quirino
 * - Misael Mateus Oliveira de Moraes
 * - Eduardo Barros Pimenta
 *
 * => We create a wifi infrastructure network with 1 AP node (node 0)
 *    and as many STA nodes as provided by the user (default: 10), as
 *    well as many data streams as provided by the user (default: 5)
 *    as long as it stays below the number os STA nodes.
 *
 *    All parameters where included from the homework specification.
 */

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"

#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/internet-stack-helper.h"

using namespace ns3;



NS_LOG_COMPONENT_DEFINE ("Fase_1");



//-------------------------------------------------------------------------------------------------
// Helper functions to simulate traffic (data streams)
//-------------------------------------------------------------------------------------------------

/*
 *
 */
void ReceivePacket (Ptr<Socket> socket)
{
  while (socket->Recv ())
    {
      NS_LOG_UNCOND ("Received one packet!");
    }
}



/*
 *
 */
static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize,
                             uint32_t pktCount, Time pktInterval )
{
  if (pktCount > 0)
    {
      socket->Send (Create<Packet> (pktSize));
      Simulator::Schedule (pktInterval, &GenerateTraffic,
                           socket, pktSize,pktCount - 1, pktInterval);
    }
  else
    {
      socket->Close ();
    }
}

//-------------------------------------------------------------------------------------------------




/*
 *
 */
int main (int argc, char *argv[])
{
  //-----------------------------------------------------------------------------------------------
  // Parameters and Arguments
  //-----------------------------------------------------------------------------------------------

  std::string phyMode ("DsssRate11Mbps");
  uint32_t packetSize = 1000; // bytes
  uint32_t numPackets = 1;    // packets
  double interval = 1.0;      // seconds
  bool verbose = false;

  uint32_t numStaNodes = 10;
  uint32_t numStreams  = 5;

  CommandLine cmd;
  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("numPackets", "number of packets generated", numPackets);
  cmd.AddValue ("interval", "interval (seconds) between packets", interval);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  
  cmd.AddValue ("numStaNodes", "Number of STA nodes in simulation", numStaNodes);
  cmd.AddValue ("numStreams", "Number of data streams in simulation", numStreams);
  
  cmd.Parse (argc, argv);

  // Convert to time object
  Time interPacketInterval = Seconds (interval);

  // Can't have more streams than (STA) nodes
  if (numStreams > numStaNodes)
    {
      numStreams = numStaNodes;
    }

  //-----------------------------------------------------------------------------------------------



  //-----------------------------------------------------------------------------------------------
  // Node container
  //-----------------------------------------------------------------------------------------------

  NodeContainer c;
  c.Create (numStaNodes+1); // STA nodes plus the AP node

  //-----------------------------------------------------------------------------------------------



  //-----------------------------------------------------------------------------------------------
  // Wifi model (helper)
  //-----------------------------------------------------------------------------------------------

  // The below set of helpers will help us to put together the wifi NICs we want
  WifiHelper wifi;

  // Wifi Standard
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

  //-----------------------------------------------------------------------------------------------



  //-----------------------------------------------------------------------------------------------
  // Wifi physical model (Yans)
  //-----------------------------------------------------------------------------------------------

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();

  // Parameters as stated in the homework pdf 
  wifiPhy.Set ("RxSensitivity", DoubleValue (-96.0) );  // Energy Detection Threshold (which is deprecated)
  wifiPhy.Set ("CcaEdThreshold", DoubleValue (-99.0) ); // Carrier Sense Threshold
  wifiPhy.Set ("TxGain", DoubleValue (22.0) );          // Transmission power

  //-----------------------------------------------------------------------------------------------



  //-----------------------------------------------------------------------------------------------
  // Wifi Channel model
  //-----------------------------------------------------------------------------------------------

  YansWifiChannelHelper wifiChannel;

  // Log Distance Propagation Model, exponent = 2,7 (as defined)
  wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel","Exponent",DoubleValue (2.7));
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a mac and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));

  //-----------------------------------------------------------------------------------------------



  //-----------------------------------------------------------------------------------------------
  // Setup the rest of the mac
  //-----------------------------------------------------------------------------------------------

  Ssid ssid = Ssid ("wifi-default");

  NetDeviceContainer devices;

  // Setup AP (node 0)
  wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid));
  NetDeviceContainer apDevice = wifi.Install (wifiPhy, wifiMac, c.Get (0));
  devices.Add (apDevice);
  
  // Setup STA nodes
  wifiMac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid));
  for (unsigned int i = 1; i <= numStaNodes; i++)
    {
      NetDeviceContainer staDevice = wifi.Install (wifiPhy, wifiMac, c.Get (i));
      devices.Add (staDevice);      
    }

  //-----------------------------------------------------------------------------------------------



  //-----------------------------------------------------------------------------------------------
  // Mobility model
  //-----------------------------------------------------------------------------------------------

  MobilityHelper mobility;

  // ConstantPosition for the AP node
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (c.Get (0));

  // RandomWalk2D for STA nodes
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator");
  // 700m x 700m area
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
    "Bounds", RectangleValue (Rectangle (-350, 350, -350, 350)));

  // Installing mobility model on each STA node
  for (unsigned int i = 1; i < c.GetN (); i++)
    mobility.Install (c.Get (i));

  //-----------------------------------------------------------------------------------------------



  //-----------------------------------------------------------------------------------------------
  // Setting up internet protocol stack
  // and creating a simple socket application
  //-----------------------------------------------------------------------------------------------

  InternetStackHelper internet;
  internet.Install (c);

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);

  // Node 0 will be receiving traffic
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink = Socket::CreateSocket (c.Get (0), tid);
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink->Bind (local);
  recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));

  // Streaming nodes (the first #numStreams nodes, from node 1)
  Ptr<Socket> sources[numStreams];
  for (unsigned int i = 1; i <= numStreams; i++)
    {
      Ptr<Socket> source = Socket::CreateSocket (c.Get (i), tid);
      InetSocketAddress remote = InetSocketAddress (Ipv4Address ("255.255.255.255"), 80);
      source->SetAllowBroadcast (true);
      source->Connect (remote);
      sources[i-1] = source;
    }

  // Tracing
  wifiPhy.EnablePcap ("wifi_fase_1", devices);

  //-----------------------------------------------------------------------------------------------



  //-----------------------------------------------------------------------------------------------
  // SIMULATION (for testing)
  //-----------------------------------------------------------------------------------------------

  // Scheduling packet sending (simulation data streams)
  for (unsigned int i = 0; i < numStreams; i++)
    {
      Simulator::ScheduleWithContext (sources[i]->GetNode ()->GetId (),
                                      Seconds (1.0), &GenerateTraffic,
                                      sources[i], packetSize, numPackets, interPacketInterval);
    }

  Simulator::Stop (Seconds (30.0));
  Simulator::Run ();
  Simulator::Destroy ();

  //-----------------------------------------------------------------------------------------------

  return 0;
}
