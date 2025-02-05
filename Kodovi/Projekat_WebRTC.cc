#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/applications-module.h"
#include "ns3/nix-vector-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/wifi-module.h"
#include "ns3/netanim-module.h"

#include <vector>
#include <sstream>
#include <iostream>
#include <iterator>
#include <fstream>
 
#include "ns3/aodv-module.h"
#include "ns3/olsr-module.h"
#include "ns3/dsdv-module.h"  

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SPTMExample");

PointToPointHelper p2p;
NodeContainer n1n0;
NetDeviceContainer d1d0;
bool useErrorModel = false;

//Create variables to test the results
uint32_t m_bytes_sent = 0;
uint32_t m_bytes_received = 0;

uint32_t m_packets_sent = 0;
uint32_t m_packets_received = 0;

//Create help variable m_time
double m_time = 0;

//Create c++ map for measuring delay time
std::map<uint32_t, double> m_delayTable;
 
/**
   * \brief Each time the packet is sent, this function is called using TracedCallback and information about packet is stored in m_delayTable
   * \param context - used to display full path of the information provided (useful for taking information about the nodes or applications)
   * \param packet - Ptr reference to the packet itself
   */
void SentPacket(std::string context, Ptr<const Packet> p) {
   
    /*
    //HELP LINES USED FOR TESTING
    std::cout << "\n ..................SentPacket....." << p->GetUid() << "..." <<  p->GetSize() << ".......  \n";
    p->Print(std::cout);                
    std::cout << "\n ............................................  \n";  
    */

    //Sum bytes of the packet that was sent
    m_bytes_sent  += p->GetSize();
    m_packets_sent++;

    //Insert in the delay table details about the packet that was sent
   
    m_delayTable.insert (std::make_pair (p->GetUid(), (double)Simulator::Now().GetSeconds()));
}

long double cumulativeVal = 0;
long long int c_counter = 0;
void ReceivedPacket(std::string context, Ptr<const Packet> p, const Address& addr){
   
    /*
    //HELP LINES USED FOR TESTING
    std::cout << "\n ..................ReceivedPacket....." << p->GetUid() << "..." <<  p->GetSize() << ".......  \n";
    p->Print(std::cout);                
    std::cout << "\n ............................................  \n";  
    */

    //Find the record in m_delayTable based on packetID
    std::map<uint32_t, double >::iterator i = m_delayTable.find ( p->GetUid() );
   
    //Get the current time in the temp variable
    double temp = (double)Simulator::Now().GetSeconds();  
    //Display the delay for the packet in the form of "packetID delay" where delay is calculated as the current time - time when the packet was sent
    //std::cout << p->GetUid() << "\t" << "Delay: " << (temp - i->second)*1000 << "ms"<< "\t" << "MAC: " << addr << "\n";
    cumulativeVal += i->second;
    c_counter++;
    std::cout << p->GetUid() << "\t" << i->second << std::endl;
    //Remove the entry from the delayTable to clear the RAM memory and obey memory leakage
    if(i != m_delayTable.end()){
        m_delayTable.erase(i);
    }
    std::cout << "Denin" << cumulativeVal/c_counter << std::endl;
    //Sum bytes and number of packets that were sent
    m_bytes_received += p->GetSize();
    m_packets_received++;
}

void Ratio(){
    std::cout << "Sent (bytes):\t" <<  m_bytes_sent
    << "\tReceived (bytes):\t" << m_bytes_received
    << "\nSent (Packets):\t" <<  m_packets_sent
    << "\tReceived (Packets):\t" << m_packets_received
   
    << "\nRatio (bytes):\t" << (float)m_bytes_received/(float)m_bytes_sent
    << "\tRatio (packets):\t" << (float)m_packets_received/(float)m_packets_sent << "\n";
}

std::list<uint64_t> getPacketIDsToLose (uint32_t minId, uint32_t maxId) {
  //DODATI SEED CHANGE
    Ptr<UniformRandomVariable> repeats = CreateObject<UniformRandomVariable>();
    repeats -> SetAttribute("Min", DoubleValue(minId));
    repeats -> SetAttribute("Max", DoubleValue(maxId));
    uint32_t randRepeats = repeats->GetInteger();
    std::list<uint64_t> ret;
    std::cout << "DENIN PAKETI" << randRepeats << std::endl;
    for (uint32_t i = 1; i <= randRepeats; i++) {
            // Create a random number generator for the data rate
      Ptr<UniformRandomVariable> randomVariable = CreateObject<UniformRandomVariable>();
      randomVariable->SetAttribute("Min", DoubleValue(minId));
      randomVariable->SetAttribute("Max", DoubleValue(maxId));

      // Get a random data rate between the min and max values (in kbps)
      uint32_t randId = randomVariable->GetInteger();  
      ret.push_back (randId);
      std::cout << randId << std::endl;
    }
    std::cout << std::endl;
    return ret;
}

void ChgDataRate(Ptr<NetDevice> device) {
  static bool toggle = true;
  if (toggle) {
    p2p.SetDeviceAttribute("DataRate",StringValue("1Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("1ms"));
    std::cout << "1Mbps" << std::endl;
  } else {
    p2p.SetDeviceAttribute("DataRate", StringValue("10bps"));
        p2p.SetChannelAttribute("Delay", StringValue("5ms"));

    std::cout << "500kbps" << std::endl;    
  }
  toggle = !toggle;
  //p2p.Install(n1n0);
  d1d0 = p2p.Install (n1n0);
  //AnyValue val;
  //p2p.GetDeviceAttribute("DataRate", val);
   // std::cout << val.GetBitRate()/1000 << "kbps" << std::endl;

  //std::cout << val << std::endl;
  Simulator::ScheduleWithContext(device->GetNode()->GetId(), Seconds(0.2), &ChgDataRate, device);
}

int main (int argc, char *argv[]) {
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_ALL);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_ALL);
  LogComponentEnable ("SPTMExample", LOG_LEVEL_ALL);

  Packet::EnablePrinting();
  PacketMetadata::Enable ();

  bool      enablePcap = true;
  double    simulationTime = 60*10*1000; //5min  
  //double    numberOfNodes = 4;  
  bool      enableApplication = true;  
 
  CommandLine cmd;
  cmd.AddValue ("simulationTime", "simulationTime", simulationTime);
  cmd.Parse (argc, argv);
 
  //
  // Explicitly create the nodes required by the topology (shown above).
  //
  NodeContainer nodes;
  nodes.Create(2);


  n1n0 = NodeContainer (nodes.Get (1), nodes.Get (0));

  std::string lat = "10ms";// 
  std::string rate = "5Mbps"; // P2P link
 
  InternetStackHelper internet;
  internet.Install (nodes);

  // We create the channels first without any IP addressing information
  NS_LOG_INFO ("Create channels.");
  p2p.SetDeviceAttribute ("DataRate", StringValue (rate));
  p2p.SetChannelAttribute ("Delay", StringValue (lat));
  d1d0 = p2p.Install (n1n0);

    // Later, we add IP addresses.
  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;

  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i1i0 = ipv4.Assign (d1d0);

  NS_LOG_INFO ("Enable static global routing.");
  //
  // Turn on global static routing so we can actually be routed across the network.
  //
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

        //NetAnim
  AnimationInterface anim("dm_zadaca1_netanim.xml");

  if(enableApplication){

      //tcp try 2 -- NE DIRAJ
      uint16_t tcpport = 90;
      PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", InetSocketAddress(i1i0.GetAddress(1), tcpport));
      ApplicationContainer sinkApp = packetSinkHelper.Install(nodes.Get(0)); //1
      sinkApp.Start(Seconds(0));
      sinkApp.Stop(Seconds(1));

      OnOffHelper onOffHelper("ns3::TcpSocketFactory", InetSocketAddress(i1i0.GetAddress(0), tcpport));
      onOffHelper.SetAttribute("DataRate", StringValue("1Mbps"));
      onOffHelper.SetAttribute("PacketSize", UintegerValue(1024));

      ApplicationContainer onOffApp1 = onOffHelper.Install(nodes.Get(1));
      onOffApp1.Start(Seconds(0));
      onOffApp1.Stop(Seconds(1));

      OnOffHelper onOffHelper2("ns3::TcpSocketFactory", InetSocketAddress(i1i0.GetAddress(1), tcpport));
      onOffHelper2.SetAttribute("DataRate", StringValue("1Mbps"));
      onOffHelper2.SetAttribute("PacketSize", UintegerValue(1024));

      ApplicationContainer onOffApp2 = onOffHelper.Install(nodes.Get(0));
      onOffApp2.Start(Seconds(0));
      onOffApp2.Stop(Seconds(1));
    // NE DIRAJ ^

    if (useErrorModel) { //ne diraj
          //ERROOOOOR
    // Run the simulation
/*
   // Set a few attributes
      Config::SetDefault ("ns3::RateErrorModel::ErrorRate", DoubleValue (0.001));
      Config::SetDefault ("ns3::RateErrorModel::ErrorUnit", StringValue ("ERROR_UNIT_PACKET"));
   
      Config::SetDefault ("ns3::BurstErrorModel::ErrorRate", DoubleValue (0.01));
      Config::SetDefault ("ns3::BurstErrorModel::BurstSize", StringValue ("ns3::UniformRandomVariable[Min=1|Max=3]"));
   
      Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (210));
      Config::SetDefault ("ns3::OnOffApplication::DataRate", DataRateValue (DataRate ("448kb/s")));
   
      std::string errorModelType = "ns3::RateErrorModel";
// Allow the user to override any of the defaults and the above
     // Bind()s at run-time, via command-line arguments
      CommandLine cmd;
      cmd.AddValue("errorModelType", "TypeId of the error model to use", errorModelType);
      cmd.Parse (argc, argv);
   
      //
    // Error model
     //
     // Create an ErrorModel based on the implementation (constructor)
     // specified by the default TypeId
   
     ObjectFactory factory;
     factory.SetTypeId (errorModelType);
     Ptr<ErrorModel> em = factory.Create<ErrorModel> ();
     d1d0.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
   
     // Now, let's use the ListErrorModel and explicitly force a loss
     // of the packets with pkt-uids = 11 and 17 on node 2, device 0
     std::list<uint64_t> sampleList = getPacketIDsToLose(0,50);
     // This time, we'll explicitly create the error model we want
     Ptr<ListErrorModel> pem = CreateObject<ListErrorModel> ();
     pem->SetList (sampleList);
     d1d0.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (pem));

*/ //END ERRORR WORKS

    }

      // UDP connection
      std::cout << "Flow 1:\n";
      std::cout << "Sender IP address (client):  " << i1i0.GetAddress (0) << "\n";
      std::cout << "Receiver IP address (server):   " << i1i0.GetAddress (1) << "\n";

      SptmClientHelper sptmClient1 ("ns3::UdpSocketFactory", InetSocketAddress (i1i0.GetAddress (1), 1)); //addr, port
      sptmClient1.SetAttribute("PacketSize", UintegerValue (1200)); //tipična za slučaj
      sptmClient1.SetAttribute("MaxPackets", UintegerValue(855000)); //vezano za veličinu fajla koji se prenosi 40000
      sptmClient1.SetAttribute("Interval", TimeValue (Seconds (0.01))); //tipična za slučaj //33ms za 30fps

      ApplicationContainer apps1 = sptmClient1.Install (nodes.Get (1)); //
      apps1.Start (Seconds (2));
      apps1.Stop (Seconds (simulationTime));

        // Create a packet sptmSink to receive these packets
      SptmSinkHelper sptmSink1 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 1));
      apps1 = sptmSink1.Install (nodes.Get (0));
      apps1.Start (Seconds (2));
      apps1.Stop (Seconds (simulationTime));

      //Simulator::ScheduleWithContext(d1d0.Get(1)->GetNode()->GetId(), Seconds(2), &ChgDataRate, d1d0.Get(0));
   
  }
  Config::Connect("/NodeList/*/ApplicationList/*/$ns3::SptmClient/Tx", MakeCallback(&SentPacket));
  Config::Connect("/NodeList/*/ApplicationList/*/$ns3::SptmSink/Rx", MakeCallback(&ReceivedPacket));
 
  if(enablePcap){
    p2p.EnablePcapAll ("dm_zadaca1_log");
  }

  //
  // Now, do the actual simulation.
  //

  // Run the simulator
  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();
 

  if(enableApplication) {
      Ratio();
  }



  //Finally print the graphs
  Simulator::Destroy();
  Names::Clear();
}
