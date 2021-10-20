#ifndef TCP_NEWRENO_CSE
#define TCP_NEWRENO_CSE

#include "tcp-congestion-ops.h"
using namespace ns3;
using namespace std;

class TcpNewRenoCSE : public TcpCongestionOps
{
    public:
        static TypeId GetTypeId (void);
        TcpNewRenoCSE();
        TcpNewRenoCSE (const TcpNewRenoCSE& sock);
        ~TcpNewRenoCSE ();
        string GetName () const;
        void IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
        uint32_t GetSsThresh (Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight);
        Ptr<TcpCongestionOps> Fork ();
    protected:
        uint32_t SlowStart (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
        void CongestionAvoidance (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
};

#endif