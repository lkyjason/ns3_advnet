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

MobilityHelper setup_static_mob(Vector v, Ptr<Node> node) {
	MobilityHelper mobh;
	Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
	enbPositionAlloc->Add(v);
	mobh.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobh.SetPositionAllocator(enbPositionAlloc);
    
	mobh.Install (node);

    return mobh;
}

void configure_ue(Ptr<Node> ueNode, Ptr<MmWavePointToPointEpcHelper> epcHelper, 
                    Ipv4StaticRoutingHelper ipv4RoutingHelper) {

    // Set the default gateway for the UE
	Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
	ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
}

Ipv4Address setup_enb_ue(Ptr<MmWaveHelper> mmwaveHelper, InternetStackHelper internet, 
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

    return ueIpIface.GetAddress(0); 
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

	NodeContainer ueNodes;
	NodeContainer enbNodes;
	enbNodes.Create(2);
	ueNodes.Create(2);

	// Install Mobility Model
	MobilityHelper enb_a = setup_static_mob(Vector(0.0, 0.0, 0.0), enbNodes.Get(0));;
	MobilityHelper enb_b = setup_static_mob(Vector(30.0, 0.0, 0.0), enbNodes.Get(1));;
	MobilityHelper ue_a = setup_static_mob(Vector(1.0, 0.0, 0.0), ueNodes.Get(0));;
	MobilityHelper ue_b = setup_static_mob(Vector(29.0, 0.0, 0.0), ueNodes.Get(1));;

    // setup UE ENB common
    Ipv4Address server_addr = setup_enb_ue(mmwaveHelper, internet, epcHelper, ueNodes, enbNodes);

    // setup echo server
    UdpEchoServerHelper echoServer (9);

    // setup echo client
    UdpEchoClientHelper echoClient (server_addr, 9);
    echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
    echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.)));
    echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

    Ptr<Node> server_node = ueNodes.Get(0);
    Ptr<Node> client_node = ueNodes.Get(1);

    configure_ue(server_node, epcHelper, ipv4RoutingHelper);
    configure_ue(client_node, epcHelper, ipv4RoutingHelper);

    ApplicationContainer serverApps = echoServer.Install (server_node);
    serverApps.Start (Seconds (1.0));
    serverApps.Stop (Seconds (10.0));

    ApplicationContainer clientApp = echoClient.Install (client_node);
    clientApp.Start (Seconds (2.0));
    clientApp.Stop (Seconds (10.0));

    // start simulation
    Simulator::Run ();

    // cleanup
    Simulator::Destroy ();
    return 0;
 }
