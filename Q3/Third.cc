#include <bits/stdc++.h>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace std;
using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpCongestionAnalysis3");

class TcpApp : public Application
{
public:
    TcpApp();
    virtual ~TcpApp();

    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId(void);
    void Setup(Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
    virtual void StartApplication(void);
    virtual void StopApplication(void);

    void ScheduleTx(void);
    void SendPacket(void);

    Ptr<Socket> m_socket;
    Address m_peer;
    uint32_t m_packetSize;
    uint32_t m_nPackets;
    DataRate m_dataRate;
    EventId m_sendEvent;
    bool m_running;
    uint32_t m_packetsSent;
};

TcpApp::TcpApp()
    : m_socket(0),
      m_peer(),
      m_packetSize(0),
      m_nPackets(0),
      m_dataRate(0),
      m_sendEvent(),
      m_running(false),
      m_packetsSent(0)
{
}

TcpApp::~TcpApp(){
    m_socket = 0;
}

/* static */
TypeId TcpApp::GetTypeId(void){
    static TypeId tid = TypeId("TcpApp")
                            .SetParent<Application>()
                            .SetGroupName("Tutorial")
                            .AddConstructor<TcpApp>();
    return tid;
}

void TcpApp::Setup(Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate){
    m_socket = socket;
    m_peer = address;
    m_packetSize = packetSize;
    m_nPackets = nPackets;
    m_dataRate = dataRate;
}

void TcpApp::StartApplication(void){
    m_running = true;
    m_packetsSent = 0;
    m_socket->Bind();
    m_socket->Connect(m_peer);
    SendPacket();
}

void TcpApp::StopApplication(void){
    m_running = false;
    if (m_sendEvent.IsRunning()){
        Simulator::Cancel(m_sendEvent);
    }
    if (m_socket){
        m_socket->Close();
    }
}

void TcpApp::SendPacket(void){
    Ptr<Packet> packet = Create<Packet>(m_packetSize);
    m_socket->Send(packet);
    if (++m_packetsSent < m_nPackets){
        ScheduleTx();
    }
}

void TcpApp::ScheduleTx(void){
    if (m_running){
        Time tNext(Seconds(m_packetSize * 8 / static_cast<double>(m_dataRate.GetBitRate())));
        m_sendEvent = Simulator::Schedule(tNext, &TcpApp::SendPacket, this);
    }
}

static void
CwndChange(Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd){
    //NS_LOG_UNCOND(Simulator::Now().GetSeconds() << "\t" << newCwnd);
    *stream->GetStream() << Simulator::Now().GetSeconds() << "\t" << newCwnd << std::endl;
}

int main(int argc,char *argv[]){

    int config = 1;

    //Parsing command line arguments
    CommandLine cmd;
    cmd.AddValue("config","Configuration for the topology",config);
    cmd.Parse(argc,argv);

    //Creating nodes 
    NodeContainer nodes;
    nodes.Create(3);

    //Creating links
    //Between N1 and N3
    PointToPointHelper p2p1;
    p2p1.SetDeviceAttribute("DataRate",StringValue("10Mbps"));
    p2p1.SetChannelAttribute("Delay",StringValue("3ms"));
    //Between N2 and N3
    PointToPointHelper p2p2;
    p2p2.SetDeviceAttribute("DataRate",StringValue("9Mbps"));
    p2p2.SetChannelAttribute("Delay",StringValue("3ms"));

    //Installing network interfaces 
    NetDeviceContainer devices1;
    devices1 = p2p1.Install(nodes.Get(0),nodes.Get(2));
    NetDeviceContainer devices2;
    devices2 = p2p2.Install(nodes.Get(1),nodes.Get(2));

    //Installing protocol stacks
    InternetStackHelper stack;
    stack.Install(nodes);

    //Assigning IP addresses to the devices
    Ipv4AddressHelper address1;
    address1.SetBase("10.0.0.0","255.255.255.0");
    Ipv4InterfaceContainer interfaces1 = address1.Assign(devices1);
    Ipv4AddressHelper address2;
    address2.SetBase("10.0.1.0","255.255.255.0");
    Ipv4InterfaceContainer interfaces2 = address2.Assign(devices2);

    //Initialising node N3 as a TCP sink for connection 1
    uint16_t sinkPort1 = 50000;
    Address sinkAddress1(InetSocketAddress(interfaces1.GetAddress(1), sinkPort1));
    PacketSinkHelper packetSinkHelper1("ns3::TcpSocketFactory", InetSocketAddress(interfaces1.GetAddress(1), sinkPort1));
    ApplicationContainer sinkApp1 = packetSinkHelper1.Install(nodes.Get(2));
    sinkApp1.Start(Seconds(0.));
    sinkApp1.Stop(Seconds(30.));
    //Initialising node N3 as a TCP sink for connection 2
    uint16_t sinkPort2 = 50001;
    Address sinkAddress2(InetSocketAddress(interfaces1.GetAddress(1), sinkPort2));
    PacketSinkHelper packetSinkHelper2("ns3::TcpSocketFactory", InetSocketAddress(interfaces1.GetAddress(1), sinkPort2));
    ApplicationContainer sinkApp2 = packetSinkHelper2.Install(nodes.Get(2));
    sinkApp2.Start(Seconds(0.));
    sinkApp2.Stop(Seconds(30.));
    //Initialising node N3 as a TCP sink for connection 3
    uint16_t sinkPort3 = 50002;
    Address sinkAddress3(InetSocketAddress(interfaces2.GetAddress(1), sinkPort3));
    PacketSinkHelper packetSinkHelper3("ns3::TcpSocketFactory", InetSocketAddress(interfaces2.GetAddress(1), sinkPort3));
    ApplicationContainer sinkApp3 = packetSinkHelper3.Install(nodes.Get(2));
    sinkApp3.Start(Seconds(0.));
    sinkApp3.Stop(Seconds(30.));

    //Assigning TCP types based on the given configuration of the topology
    Ptr<Socket> con1;
    Ptr<Socket> con2;
    Ptr<Socket> con3;
    TypeId reno_original = TypeId::LookupByName("ns3::TcpNewReno");
    TypeId reno_cse = TypeId::LookupByName("ns3::TcpNewRenoCSE");

    if(config==1){
        Config::Set("/NodeList/*/$ns3::TcpL4Protocol/SocketType", TypeIdValue(reno_original));
        con1 = Socket::CreateSocket(nodes.Get(0),TcpSocketFactory::GetTypeId());
        con2 = Socket::CreateSocket(nodes.Get(0),TcpSocketFactory::GetTypeId());
        con3 = Socket::CreateSocket(nodes.Get(1),TcpSocketFactory::GetTypeId());
    }else if(config==2){
        Config::Set("/NodeList/*/$ns3::TcpL4Protocol/SocketType", TypeIdValue(reno_original));
        con1 = Socket::CreateSocket(nodes.Get(0),TcpSocketFactory::GetTypeId());
        con2 = Socket::CreateSocket(nodes.Get(0),TcpSocketFactory::GetTypeId());
        Config::Set("/NodeList/*/$ns3::TcpL4Protocol/SocketType", TypeIdValue(reno_cse));
        con3 = Socket::CreateSocket(nodes.Get(1),TcpSocketFactory::GetTypeId());
    }else{
        Config::Set("/NodeList/*/$ns3::TcpL4Protocol/SocketType", TypeIdValue(reno_cse));
        con1 = Socket::CreateSocket(nodes.Get(0),TcpSocketFactory::GetTypeId());
        con2 = Socket::CreateSocket(nodes.Get(0),TcpSocketFactory::GetTypeId());
        con3 = Socket::CreateSocket(nodes.Get(1),TcpSocketFactory::GetTypeId());
    }

    //TCP source for connection 1
    Ptr<TcpApp> app1 = CreateObject<TcpApp>();
    app1->Setup(con1, sinkAddress1, 3000, 10000, DataRate("1.5Mbps"));
    nodes.Get(0)->AddApplication(app1);
    app1->SetStartTime(Seconds(1.));
    app1->SetStopTime(Seconds(20.));

    //TCP source for connection 2
    Ptr<TcpApp> app2 = CreateObject<TcpApp>();
    app2->Setup(con2, sinkAddress2, 3000, 10000, DataRate("1.5Mbps"));
    nodes.Get(0)->AddApplication(app2);
    app2->SetStartTime(Seconds(5.));
    app2->SetStopTime(Seconds(25.));

    //TCP source for connection 3
    Ptr<TcpApp> app3 = CreateObject<TcpApp>();
    app3->Setup(con3, sinkAddress3, 3000, 10000, DataRate("1.5Mbps"));
    nodes.Get(1)->AddApplication(app3);
    app3->SetStartTime(Seconds(15.));
    app3->SetStopTime(Seconds(30.));

    AsciiTraceHelper AsciiTraceHelper;
    Ptr<OutputStreamWrapper> stream1 = AsciiTraceHelper.CreateFileStream("Outputs_3/"+to_string(config)+"-1.cwnd");
    con1->TraceConnectWithoutContext("CongestionWindow", MakeBoundCallback(&CwndChange, stream1));

    Ptr<OutputStreamWrapper> stream2 = AsciiTraceHelper.CreateFileStream("Outputs_3/"+to_string(config)+"-2.cwnd");
    con2->TraceConnectWithoutContext("CongestionWindow", MakeBoundCallback(&CwndChange, stream2));

    Ptr<OutputStreamWrapper> stream3 = AsciiTraceHelper.CreateFileStream("Outputs_3/"+to_string(config)+"-3.cwnd");
    con3->TraceConnectWithoutContext("CongestionWindow", MakeBoundCallback(&CwndChange, stream3));

    Simulator::Stop(Seconds(30));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
