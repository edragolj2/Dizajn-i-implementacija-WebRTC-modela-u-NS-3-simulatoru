#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "sptm-sink.h"
#include "ns3/sptm-header.h"

namespace ns3
{

struct RtpHeader{
        uint8_t version;
        uint8_t padding;
        uint8_t extension;
        uint8_t cc;
        uint8_t marker;
        uint8_t payloadType;
        uint16_t sequenceNumber;
        uint32_t timestamp;
        uint32_t ssrc;
};

NS_LOG_COMPONENT_DEFINE ("SptmSink");

NS_OBJECT_ENSURE_REGISTERED (SptmSink);

TypeId
SptmSink::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SptmSink")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<SptmSink> ()
    .AddAttribute ("Local",
                   "The Address on which to Bind the rx socket.",
                   AddressValue (),
                   MakeAddressAccessor (&SptmSink::m_local),
                   MakeAddressChecker ())
    .AddAttribute ("Protocol",
                   "The type id of the protocol to use for the rx socket.",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&SptmSink::m_tid),
                   MakeTypeIdChecker ())
    .AddTraceSource ("Rx",
                     "A packet has been received",
                     MakeTraceSourceAccessor (&SptmSink::m_rxTrace),
                     "ns3::Packet::AddressTracedCallback")
  ;
  return tid;
}

SptmSink::SptmSink ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_totalRx = 0;
}

SptmSink::~SptmSink()
{
  NS_LOG_FUNCTION (this);
}

uint64_t SptmSink::GetTotalRx () const
{
  NS_LOG_FUNCTION (this);
  return m_totalRx;
}

Ptr<Socket>
SptmSink::GetListeningSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

std::list<Ptr<Socket> >
SptmSink::GetAcceptedSockets (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socketList;
}

void SptmSink::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_socket = nullptr;
  m_socketList.clear ();

  // chain up
  Application::DoDispose ();
}


// Application Methods
void SptmSink::StartApplication ()    // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);
  // Create the socket if not already
  if (!m_socket)
    {
      m_socket = Socket::CreateSocket (GetNode (), m_tid);
      if (m_socket->Bind (m_local) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }
      m_socket->Listen ();
      m_socket->ShutdownSend ();
      if (addressUtils::IsMulticast (m_local))
        {
          Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);
          if (udpSocket)
            {
              // equivalent to setsockopt (MCAST_JOIN_GROUP)
              udpSocket->MulticastJoinGroup (0, m_local);
            }
          else
            {
              NS_FATAL_ERROR ("Error: joining multicast on a non-UDP socket");
            }
        }
    }

  m_socket->SetRecvCallback (MakeCallback (&SptmSink::HandleRead, this));
  m_socket->SetAcceptCallback (
    MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
    MakeCallback (&SptmSink::HandleAccept, this));
  m_socket->SetCloseCallbacks (
    MakeCallback (&SptmSink::HandlePeerClose, this),
    MakeCallback (&SptmSink::HandlePeerError, this));
}

void SptmSink::StopApplication ()     // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);
  while(!m_socketList.empty ()) //these are accepted sockets, close them
    {
      Ptr<Socket> acceptedSocket = m_socketList.front ();
      m_socketList.pop_front ();
      acceptedSocket->Close ();
    }
  if (m_socket)
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}

RtpHeader DeserializeRtpPacket(Ptr<Packet> packet) {

  RtpHeader h;
  //uint8_t buffer[12];
/*
  packet -> RemoveHeader(MakeCallback([&buffer](uint8_t* data, uint32_t size)-> uint32_t {
    memcpy(buffer, data, size);
    return size;
  }));
*/
  uint8_t *buffer = new uint8_t[packet->GetSize()];
  packet -> CopyData(buffer, packet->GetSize());

  h.version = (buffer[0] >> 6) & 0x03;
  h.padding = (buffer[0] >> 5) & 0x01;
  h.extension = (buffer[0] >> 4) & 0x01;
  h.cc = (buffer[0]) & 0x0F;
  h.marker = (buffer[1] >> 7) & 0x01;
  h.payloadType = (buffer[1]) & 0x7F;
  h.sequenceNumber = (buffer[2]<<8)|buffer[3];
  h.timestamp = buffer[4] << 24 | buffer[5] << 16 | buffer[6] << 8 | buffer[7];
  h.ssrc = buffer[8] << 24 | buffer[9] << 16 | buffer[10] << 8 | buffer[11];

  h.version = buffer[0];
  h.padding = buffer[1];
  h.extension = buffer[2];
  h.cc = buffer[3];
  h.marker = buffer[4];
  h.payloadType = buffer[5];
  h.sequenceNumber = buffer[6];
  h.timestamp = buffer[7];
  h.ssrc = buffer[8];
  h.padding = buffer[1];
  h.padding = buffer[1];
  h.padding = buffer[1];



  return h;
}

void SptmSink::HandleRead (Ptr<Socket> socket)
{
  RtpHeader myfinalheader;
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () == 0)
        { //EOF
          break;
        }
 
      m_rxTrace (packet, from);
      //std::cout << "Denin " << packet->GetSize() << " " << packet << '\n';
      SptmHeader sptmHead;
      myfinalheader = DeserializeRtpPacket(packet);
      //std::cout << "primljeno: \n";
      //std::cout << std::hex << myfinalheader.version << " " << myfinalheader.padding << " " << myfinalheader.extension << " " << myfinalheader.cc << " " << myfinalheader.marker << " " << myfinalheader.payloadType << " " << myfinalheader.sequenceNumber << " " << myfinalheader.timestamp << " "<< myfinalheader.ssrc << " \n";
       packet->RemoveHeader(sptmHead);
     
      //
      uint8_t *buffer = new uint8_t[packet->GetSize()];
      packet -> CopyData(buffer, packet->GetSize());
      std::string s = std::string((char*)buffer);
      //std::cout << "Denin " << s << " " << packet->GetSize() << " " << packet <<  '\n';
      //
     
      NS_ASSERT (sptmHead.GetSeq() >= 0 );
 
      m_totalRx += packet->GetSize ();

      NS_LOG_INFO ("SPTM Header detected" << sptmHead);

      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s packet sink received "
                       <<  packet->GetSize () << " bytes from "
                       << InetSocketAddress::ConvertFrom(from).GetIpv4 ()
                       << " port " << InetSocketAddress::ConvertFrom (from).GetPort ()
                       << " total Rx " << m_totalRx << " bytes");
        }
      else if (Inet6SocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s packet sink received "
                       <<  packet->GetSize () << " bytes from "
                       << Inet6SocketAddress::ConvertFrom(from).GetIpv6 ()
                       << " port " << Inet6SocketAddress::ConvertFrom (from).GetPort ()
                       << " total Rx " << m_totalRx << " bytes");
        }
    }
}


void SptmSink::HandlePeerClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}
 
void SptmSink::HandlePeerError (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}
 

void SptmSink::HandleAccept (Ptr<Socket> s, const Address& from)
{
  NS_LOG_FUNCTION (this << s << from);
  s->SetRecvCallback (MakeCallback (&SptmSink::HandleRead, this));
  m_socketList.push_back (s);
}

} // Namespace ns3
