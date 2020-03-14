#include "ns3/core-module.h"
#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"

#include "ns3/mmwave-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/mmwave-point-to-point-epc-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store.h"

using namespace ns3;
using namespace mmwave;

Ipv4Address setup_remote(Ptr<Node> pgw, Ipv4StaticRoutingHelper ipv4RoutingHelper, InternetStackHelper internet) {

	// Create a remote server 
	NodeContainer nodes;
	nodes.Create (1);
	internet.Install (nodes);
    Ptr<Node> server = nodes.Get(0);

	// Create the Internet
	PointToPointHelper p2ph;
	p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
	p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
	p2ph.SetChannelAttribute ("Delay", TimeValue (MicroSeconds (1)));

    // setup links
	NetDeviceContainer serverDevices = p2ph.Install (pgw, server);

    // assign IPs
	Ipv4AddressHelper ipv4h;
	ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
	Ipv4InterfaceContainer serverIF = ipv4h.Assign (serverDevices);
	Ipv4Address server_addr = serverIF.GetAddress (1);

    // setup routing
	Ptr<Ipv4StaticRouting> serverStaticRouting = ipv4RoutingHelper.GetStaticRouting (nodes.Get(0)->GetObject<Ipv4> ());
	serverStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

    // setup echo server
    UdpEchoServerHelper echoServer (9);
    ApplicationContainer serverApps = echoServer.Install (server);
    serverApps.Start (Seconds (1.0));
    serverApps.Stop (Seconds (10.0));

    return server_addr;
}

void configure_ue(Ptr<Node> ueNode, Ptr<MmWavePointToPointEpcHelper> epcHelper, 
                    UdpEchoClientHelper client, Ipv4StaticRoutingHelper ipv4RoutingHelper) {

    // Set the default gateway for the UE
	Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
	ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);

    ApplicationContainer clientApp = client.Install (ueNode);
    clientApp.Start (Seconds (2.0));
    clientApp.Stop (Seconds (10.0));
}

MobilityHelper setup_static_mob(Vector v) {
	MobilityHelper mobh;
	Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
	enbPositionAlloc->Add(v);
	mobh.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobh.SetPositionAllocator(enbPositionAlloc);

    return mobh;
}

void setup_enb_ue(Ptr<MmWaveHelper> mmwaveHelper, InternetStackHelper internet, 
                    Ptr<MmWavePointToPointEpcHelper> epcHelper,
	                NodeContainer ueNodes, NodeContainer enbNodes) {

	// Install LTE Devices to the nodes
	NetDeviceContainer enbDevs = mmwaveHelper->InstallEnbDevice (enbNodes);
	NetDeviceContainer ueDevs = mmwaveHelper->InstallUeDevice (ueNodes);

	// Install the IP stack on the UEs
	// Assign IP address to UEs, and install applications
	internet.Install (ueNodes);
	Ipv4InterfaceContainer ueIpIface;
	ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueDevs));

	mmwaveHelper->AttachToClosestEnb (ueDevs, enbDevs);
	mmwaveHelper->EnableTraces ();
}

NS_LOG_COMPONENT_DEFINE ("ECHOEXAMPLE");

int main (int argc, char *argv[]) {
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    
    Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue (1024 * 1024));
	Config::SetDefault ("ns3::MmWavePhyMacCommon::ResourceBlockNum", UintegerValue(1));
	Config::SetDefault ("ns3::MmWavePhyMacCommon::ChunkPerRB", UintegerValue(72));

    // declare common
	Ipv4StaticRoutingHelper ipv4RoutingHelper;
	InternetStackHelper internet;

	Ptr<MmWaveHelper> mmwaveHelper = CreateObject<MmWaveHelper> ();
	mmwaveHelper->SetSchedulerType ("ns3::MmWaveFlexTtiMacScheduler");
	Ptr<MmWavePointToPointEpcHelper>  epcHelper = CreateObject<MmWavePointToPointEpcHelper> ();
	mmwaveHelper->SetEpcHelper (epcHelper);
	Ptr<Node> pgw = epcHelper->GetPgwNode ();

    // setup remote host
    Ipv4Address server_addr = setup_remote(pgw, ipv4RoutingHelper, internet);

	NodeContainer ueNodes;
	NodeContainer enbNodes;
	enbNodes.Create(2);
	ueNodes.Create(2);

	// Install Mobility Model
	MobilityHelper enbmobility = setup_static_mob(Vector(0.0, 0.0, 0.0));;
	MobilityHelper uemobility = setup_static_mob(Vector(30.0, 0.0, 0.0));;

	uemobility.Install (enbNodes);
	uemobility.Install (ueNodes);

    // setup UE ENB common
    setup_enb_ue(mmwaveHelper, internet, epcHelper, ueNodes, enbNodes);

    // setup echo client
    UdpEchoClientHelper echoClient (server_addr, 9);
    echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
    echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.)));
    echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

    configure_ue(ueNodes.Get(0), epcHelper, echoClient, ipv4RoutingHelper);
    configure_ue(ueNodes.Get(1), epcHelper, echoClient, ipv4RoutingHelper);

    // start simulation
    Simulator::Run ();

    // cleanup
    Simulator::Destroy ();
    return 0;
 }
