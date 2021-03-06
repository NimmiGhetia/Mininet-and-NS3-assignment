#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h" 
#include "ns3/netanim-module.h"
#include "ns3/applications-module.h" 
#include "ns3/animation-interface.h" 
#include "ns3/point-to-point-layout-module.h" 
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h" 
#include "ns3/ipv4-global-routing-helper.h" 
#include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h" 
#include <iostream>
#include <fstream> 
#include <vector> 
#include <string>
#include <cstdlib>
#include <ctime>


using namespace ns3;
using namespace std;

void monitorFlow(FlowMonitorHelper *flowmon,Ptr<FlowMonitor> monitor) ;

static void
CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
 *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t\t" << oldCwnd << "\t\t" << newCwnd << std::endl;
}


class MyApp : public Application
{
public:
  MyApp ();
  virtual ~MyApp ();

  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
};

MyApp::MyApp ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0)
{
}

MyApp::~MyApp ()
{
  m_socket = 0;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

void
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_socket->Send (packet);

  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
}

void
MyApp::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
}


int main(int argc, char* argv[]) {

/* default parameters start */

  std::string bandwidth = "5Mbps";
  std::string delay = "2ms";
  double error_rate = 0.00001;
  ns3::PacketMetadata::Enable();
  int simulation_time = 20; //seconds


/* default parameters end */
  
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpModule::GetTypeId()));

    NS_LOG_COMPONENT_DEFINE("net_layer");
    uint16_t n_nodes = 7;
  
/** configuration parameter for UDP flow only **/
    
/** setting up default configuration **/

    CommandLine cmd;
    cmd.Parse(argc, argv);


    Config::SetDefault("ns3::Ipv4GlobalRouting::RespondToInterfaceEvents", BooleanValue(true));

    /* configuration for NETAnim Animator **/
    ns3::PacketMetadata::Enable();
    std::string animFile = "tcp-flow.xml";
  
/** Network Topology Creation **/    
    NodeContainer nodes;
    NodeContainer link[n_nodes-1];
    Ipv4AddressHelper ipv4;
    NetDeviceContainer device[n_nodes-1];
    Ipv4InterfaceContainer interface[n_nodes-1];
    int i;
    Ipv4Address address[n_nodes];
    nodes.Create(n_nodes);
    
    // creating address for nodes
    for(int i = 0 ; i < n_nodes ; i++){
      string s = "10.1."+ to_string(i+1) + ".0" ;
      address[i] = s.c_str() ;
    }
    

    // joining links in topology

    uint32_t linkCount = 0;
    queue<int> remaining ;
    remaining.push(1) ;
   
    while(true){
        queue<int> newrem ;
        while(!remaining.empty()){
            int curr = remaining.front() ;
            remaining.pop() ;
            int leftchild =  curr*2 ;
            if(leftchild <= n_nodes){ 
              newrem.push(leftchild) ;
              link[linkCount] = NodeContainer (nodes.Get (curr-1), nodes.Get (leftchild-1));
              linkCount++;
            }
            int rightchild = curr*2 + 1 ;
            if(rightchild <= n_nodes){
              newrem.push(rightchild) ;  
              link[linkCount] = NodeContainer (nodes.Get (curr-1), nodes.Get (rightchild-1));
              linkCount++;
            }
      }
      if(!newrem.empty())
        swap(remaining,newrem) ;
      else 
        break ;
    }

/** Point to Point Link Helper which sets data rate and delay value for Link **/
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute ("DataRate", StringValue (bandwidth));
    p2p.SetChannelAttribute ("Delay", StringValue (delay));

 /** Setting up each link between two nodes **/
    for(i=0; i < n_nodes-1 ; i++){
      device[i] = p2p.Install(link[i]) ;
    }
    
    // used for tracing congestion window
    NetDeviceContainer checkcongestion ;
    checkcongestion = device[0] ;  

  /*  Error Model at Reciever end  */
  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  em->SetAttribute ("ErrorRate", DoubleValue (error_rate));
  checkcongestion.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
  

  Ipv4StaticRoutingHelper staticRouting;
  Ipv4GlobalRoutingHelper globalRouting;
  Ipv4ListRoutingHelper list;
  // assigning priorities 
  list.Add(staticRouting, 0);
  list.Add(globalRouting, 10);

  // Install network stacks on the nodes 
  InternetStackHelper internet;
  internet.SetRoutingHelper(list);
  internet.Install(nodes);

/** Assigning Network Base Address to Each node **/
  for(i=0; i < n_nodes-1 ; i++){
    ipv4.SetBase(address[i], "255.255.255.252");
    interface[i] = ipv4.Assign(device[i]);
  }

  // used for tcp flow trace between two nodes
  Ipv4InterfaceContainer interfaces ;
  interfaces = interface[0] ;

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

/** creating tcp flow  **/
  uint16_t sinkPort = 8080;
  Address sinkAddress (InetSocketAddress (interfaces.GetAddress(1), sinkPort));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
  ApplicationContainer sinkApps = packetSinkHelper.Install (nodes.Get (1));
  sinkApps.Start (Seconds (0.));
  sinkApps.Stop (Seconds (simulation_time*10));

  Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (nodes.Get(0), TcpSocketFactory::GetTypeId ());
  
/** writing congestion window change to file tcp.cwnd **/
  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream ("cwnd.txt");
  ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream));

  Ptr<MyApp> app = CreateObject<MyApp> ();
  app->Setup (ns3TcpSocket, sinkAddress, 1040, 1000, DataRate ("1Mbps"));
  nodes.Get (0)->AddApplication (app);
  app->SetStartTime (Seconds (1.));
  app->SetStopTime (Seconds (20.));
  
/** NetAnim Animator Setup **/
    AnimationInterface anim(animFile);
     anim.SetMaxPktsPerTraceFile(500000);
    Ptr<Node> n = nodes.Get(0);
    anim.SetConstantPosition(n, 0, 2);
    n = nodes.Get(1);
    anim.SetConstantPosition(n, 10, 60);
    n = nodes.Get(2);
    anim.SetConstantPosition(n, 20, 40);
    n = nodes.Get(3);
    anim.SetConstantPosition(n, 80, 20);
    n = nodes.Get(4);
    anim.SetConstantPosition(n, 80, 60);
    n = nodes.Get(5);
    anim.SetConstantPosition(n, 40, 40);
    n = nodes.Get(6);
    anim.SetConstantPosition(n, 20, 60);
 

// 1. Install FlowMonitor on all nodes
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  monitor->CheckForLostPackets ();

/** writing flow monitor statics to file tcp_throughtput.txt 
 * flow0 is from source to destination send, received bytes and throughput
 * flow1 is from destination to source send, received bytes and throughput
 * **/
  Simulator::Schedule(Seconds(1),&monitorFlow,&flowmon,monitor) ;

  Simulator::Stop (Seconds (10));
  Simulator::Run ();

  monitorFlow(&flowmon,monitor) ;

  Simulator::Destroy ();
  return 0;
}


void monitorFlow(FlowMonitorHelper *flowmon,Ptr<FlowMonitor> monitor){
  ofstream myfile;
  myfile.open ("tcp_throughput.txt",std::ofstream::app);
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> ((*flowmon).GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i){
    Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
    std::cout << "Flow " << i->first - 1 << " (" << t.sourceAddress << ":"<<t.sourcePort<< " -> " << t.destinationAddress << ":"<<t.destinationPort << ")\n";
    std::cout << " Tx Bytes: " << i->second.txBytes << "\n";
    std::cout << " Rx Bytes: " << i->second.rxBytes << "\n";
    std::cout << " Throughput: " << i->second.rxBytes * 8.0 /(i->second.timeLastRxPacket.GetSeconds()- i->second.timeFirstTxPacket.GetSeconds()) / 1024 / 1024 << " Mbps \n"; 
    
    time_t tt;
    time (&tt);
    struct tm * ti; 
    
    ti = localtime(&tt); 

    myfile << "Timestamp : " << asctime(ti) << "---------------------------------\n" ;
    myfile << "Flow " << i->first - 1 << " (" << t.sourceAddress << ":"<<t.sourcePort<< " -> " << t.destinationAddress << ":"<<t.destinationPort << ")\n";
    myfile << " Tx Bytes: " << i->second.txBytes << "\n";
    myfile << " Rx Bytes: " << i->second.rxBytes << "\n";
    myfile << " Throughput: " << i->second.rxBytes * 8.0 /(i->second.timeLastRxPacket.GetSeconds()- i->second.timeFirstTxPacket.GetSeconds()) / 1024 / 1024 << " Mbps \n"; 
  }
  myfile.close();
  Simulator::Schedule(Seconds(1),&monitorFlow,flowmon,monitor) ;
}

