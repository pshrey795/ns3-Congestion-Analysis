#include <bits/stdc++.h>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("TcpCongestionAnalysis2");

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

TcpApp::~TcpApp()
{
    m_socket = 0;
}

/* static */
TypeId TcpApp::GetTypeId(void)
{
    static TypeId tid = TypeId("TcpApp")
                            .SetParent<Application>()
                            .SetGroupName("Tutorial")
                            .AddConstructor<TcpApp>();
    return tid;
}

void TcpApp::Setup(Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
    m_socket = socket;
    m_peer = address;
    m_packetSize = packetSize;
    m_nPackets = nPackets;
    m_dataRate = dataRate;
}

void TcpApp::StartApplication(void)
{
    m_running = true;
    m_packetsSent = 0;
    m_socket->Bind();
    m_socket->Connect(m_peer);
    SendPacket();
}

void TcpApp::StopApplication(void)
{
    m_running = false;

    if (m_sendEvent.IsRunning())
    {
        Simulator::Cancel(m_sendEvent);
    }

    if (m_socket)
    {
        m_socket->Close();
    }
}

void TcpApp::SendPacket(void)
{
    Ptr<Packet> packet = Create<Packet>(m_packetSize);
    m_socket->Send(packet);

    if (++m_packetsSent < m_nPackets)
    {
        ScheduleTx();
    }
}

void TcpApp::ScheduleTx(void)
{
    if (m_running)
    {
        Time tNext(Seconds(m_packetSize * 8 / static_cast<double>(m_dataRate.GetBitRate())));
        m_sendEvent = Simulator::Schedule(tNext, &TcpApp::SendPacket, this);
    }
}

static void
CwndChange(Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
    //NS_LOG_UNCOND(Simulator::Now().GetSeconds() << "\t" << newCwnd);
    *stream->GetStream() << Simulator::Now().GetSeconds() << "\t" << newCwnd << std::endl;
}

int main(int argc, char *argv[])
{

    string tcp_type = "ns3::TcpNewReno";
    int type = 0;
    string cdr = "6Mbps";
    string adr = "2Mbps";
    double rate_value = 0;

    // Parsing command line arguments
    CommandLine cmd;
    cmd.AddValue("type", "Channel Data Rate or Application Data Rate", type);
    cmd.AddValue("value","Data Rate",rate_value);
    cmd.Parse(argc, argv);

    if(!type){
        cdr = to_string(rate_value) + "Mbps";
    }else{
        adr = to_string(rate_value) + "Mbps";
    }

    // Creating nodes N1 and N2
    NodeContainer nodes;
    nodes.Create(2);

    // Connection using point to point link
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue(cdr));
    pointToPoint.SetChannelAttribute("Delay", StringValue("3ms"));

    // Installing network interface on nodes
    NetDeviceContainer devices;
    devices = pointToPoint.Install(nodes);

    // Defining error model for node N2
    Ptr<RateErrorModel> rem = CreateObject<RateErrorModel>();
    rem->SetAttribute("ErrorRate", DoubleValue(0.00001));
    devices.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(rem));

    // Installing protocol stacks on devices
    InternetStackHelper stack;
    stack.Install(nodes);

    // Assigning IP addresses to the nodes
    Ipv4AddressHelper address;
    address.SetBase("10.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // Initialising node N2 as a TCP sink
    uint16_t sinkPort = 50000;
    Address sinkAddress(InetSocketAddress(interfaces.GetAddress(1), sinkPort));
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", InetSocketAddress(interfaces.GetAddress(1), sinkPort));
    ApplicationContainer sinkApps = packetSinkHelper.Install(nodes.Get(1));
    sinkApps.Start(Seconds(1.));
    sinkApps.Stop(Seconds(30.));

    // TCP type
    TypeId tid = TypeId::LookupByName(tcp_type);
    Config::Set("/NodeList/*/$ns3::TcpL4Protocol/SocketType", TypeIdValue(tid));

    // Creating TCP socket with the given type on the TCP source
    Ptr<Socket> ns3TcpSocket = Socket::CreateSocket(nodes.Get(0), TcpSocketFactory::GetTypeId());

    // Creating application interface on the TCP source
    Ptr<TcpApp> app = CreateObject<TcpApp>();
    app->Setup(ns3TcpSocket, sinkAddress, 3000, 10000, DataRate(adr));
    nodes.Get(0)->AddApplication(app);
    app->SetStartTime(Seconds(1.));
    app->SetStopTime(Seconds(30.));

    AsciiTraceHelper asciiTraceHelper;
    string outputFile = "Outputs_2/";
    if(!type){
        outputFile += "CDR-";
    }else{
        outputFile += "ADR-";
    }
    outputFile += (to_string(rate_value) + ".cwnd");
    Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream(outputFile);
    ns3TcpSocket->TraceConnectWithoutContext("CongestionWindow", MakeBoundCallback(&CwndChange, stream));

    Simulator::Stop(Seconds(30));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
