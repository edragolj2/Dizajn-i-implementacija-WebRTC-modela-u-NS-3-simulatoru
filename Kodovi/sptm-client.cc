#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "sptm-client.h"
#include "ns3/sptm-header.h"
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <arpa/inet.h>
#include <bitset>


namespace ns3 {

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
        //uint32_t ccrc;
        //ccrc(0x87654321)
       
        RtpHeader() : version(0b10), padding(0b0), extension(0b0), cc(0b0000), marker(0b0), payloadType(96), sequenceNumber(12345),
                      timestamp(67890), ssrc(0x12345678)  {}
};

NS_LOG_COMPONENT_DEFINE ("SptmClient");

NS_OBJECT_ENSURE_REGISTERED (SptmClient);

TypeId
SptmClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SptmClient")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<SptmClient> ()
    .AddAttribute ("MaxPackets",
                   "The maximum number of packets the application will send",
                   UintegerValue (5),
                   MakeUintegerAccessor (&SptmClient::m_count),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("FlowNumber",
                   "The number of flow",
                   UintegerValue (1),
                   MakeUintegerAccessor (&SptmClient::m_flow),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Interval",
                   "The time to wait between packets", TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&SptmClient::m_interval),
                   MakeTimeChecker ())
    .AddAttribute ("RemoteAddress",
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&SptmClient::m_peerAddress),
                   MakeAddressChecker ())
    .AddAttribute ("RemotePort", "The destination port of the outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&SptmClient::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize",
                   "Size of packets generated. The minimum packet size is 16 bytes which is the size of the header carrying the sequence number and the time stamp.",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&SptmClient::m_size),
                   MakeUintegerChecker<uint32_t> (16,1500))
    .AddTraceSource ("Tx",
                     "A packet has been sent",
                     MakeTraceSourceAccessor (&SptmClient::m_txTrace),
                     "ns3::Packet::AddressTracedCallback")
  ;
  return tid;
}

SptmClient::SptmClient ()
{
  NS_LOG_FUNCTION (this);
  m_sent = 0;
  m_socket = 0;
  m_flow = 0;
  m_sendEvent = EventId ();
}

SptmClient::~SptmClient ()
{
  NS_LOG_FUNCTION (this);
}

void
SptmClient::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

void
SptmClient::SetRemote (Address addr)
{
  NS_LOG_FUNCTION (this << addr);
  m_peerAddress = addr;
}
 

void
SptmClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
SptmClient::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (!m_socket)
  {

    TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
    m_socket = Socket::CreateSocket (GetNode (), tid);
    if (Ipv4Address::IsMatchingType(m_peerAddress) == true)
      {
        if (m_socket->Bind () == -1)
          {
            NS_FATAL_ERROR ("Failed to bind socket");
          }
        m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
      }
    else if (InetSocketAddress::IsMatchingType (m_peerAddress) == true)
      {
        if (m_socket->Bind () == -1)
          {
            NS_FATAL_ERROR ("Failed to bind socket");
          }
        m_socket->Connect (m_peerAddress);
      }
    else
      {
        NS_ASSERT_MSG (false, "Incompatible address type: " << m_peerAddress);
      }
  }

  m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  m_socket->SetAllowBroadcast (true);
  m_sendEvent = Simulator::Schedule (Seconds (0.0), &SptmClient::Send, this);
}

void
SptmClient::StopApplication (void)
{
  NS_LOG_FUNCTION (this);
  Simulator::Cancel (m_sendEvent);
}


Ptr<Packet> CreateRtpPacket(const RtpHeader &rtpHeader, uint32_t payloadSize) {
  uint8_t buffer[12];
  buffer[0] = (rtpHeader.version << 6) | (rtpHeader.padding << 5) | (rtpHeader.extension << 4) | (rtpHeader.cc);
  buffer[1] = (rtpHeader.marker << 7) | (rtpHeader.payloadType);
  buffer[2] = (rtpHeader.sequenceNumber >> 8);
  buffer[3] = (rtpHeader.sequenceNumber & 0xFF);
  buffer[4] = (rtpHeader.timestamp >> 24) & 0xFF;
  buffer[5] = (rtpHeader.timestamp >> 16) & 0xFF;
  buffer[6] = (rtpHeader.timestamp >> 8) & 0xFF;  
  buffer[7] = (rtpHeader.timestamp) & 0xFF;  
  buffer[8] = (rtpHeader.ssrc >> 24) & 0xFF;
  buffer[9] = (rtpHeader.ssrc >> 16) & 0xFF;  
  buffer[10] = (rtpHeader.ssrc >> 8) & 0xFF;  
  buffer[11] = (rtpHeader.ssrc) & 0xFF;  

  //std::cout << "Denin test 2" << "\n";
  //std::bitset<8> y(buffer[0]);
  //std::cout << y << buffer[1] << buffer[2] << buffer[3] << buffer[4] << buffer[5] << buffer[6] << buffer[7] << buffer[8] << buffer[9] << buffer[10] << buffer[11] << "\n";
  //std::cout << rtpHeader.version << " " << rtpHeader.padding << " " << rtpHeader.extension << " " << rtpHeader.cc << " " << rtpHeader.marker << " " << rtpHeader.payloadType << " ";
  //std::cout << rtpHeader.sequenceNumber << " " << rtpHeader.timestamp << " " << rtpHeader.ssrc;
  //std::cout << "Denin test 2 end" << "\n";


  //std::cout << "TESTIRANJE" << rtpHeader.version << "\n";
  //buffer[0] = htons(rtpHeader.version);
  //std::cout << "END TEST" << buffer[0] << "\n";
/*
  buffer[1] = htons(rtpHeader.padding);
  buffer[2] = rtpHeader.extension;
  buffer[3] = rtpHeader.cc;
  buffer[4] = rtpHeader.marker;
  buffer[5] = rtpHeader.payloadType;
  buffer[6] = rtpHeader.sequenceNumber;
  buffer[7] = rtpHeader.timestamp;
  buffer[8] = htons((rtpHeader.ssrc >> 24) & 0xFF);
  buffer[9] = htons((rtpHeader.ssrc >> 16) & 0xFF);
  buffer[10] = htons((rtpHeader.ssrc >> 8) & 0xFF);
  buffer[11] = htons((rtpHeader.ssrc) & 0xFF);  
*/
//std::cout << "nakon";
//  std::cout << std::hex << buffer[0] << " " << buffer[1] << " " << buffer[2] << " " << buffer[3] << " " << buffer[4] << " " << buffer[5] << " " << buffer[6] << " " << buffer[7] << " " << buffer[8] << " " << buffer[9] << " " << buffer[10] << " " << buffer[11];
 
  Ptr<Packet> packet = Create<Packet>(buffer, 12);
  packet -> AddPaddingAtEnd(payloadSize);
 
  return packet;
}
 

void
SptmClient::Send (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendEvent.IsExpired ());

  SptmHeader sptmHead;
  sptmHead.SetSeq (m_sent);
  sptmHead.SetFlow (m_flow);
 
  std::ostringstream msg;
  msg << "SPTM TEST" << '\0';
  //Ptr<Packet> p = Create<Packet> ((uint8_t*) msg.str().c_str(), msg.str().length());
 
  //std::cout << (uint8_t*) msg.str().c_str() << " " << msg.str().c_str() << " " <<  msg.str().length() << " " << "\n";
  //Ptr<Packet> p = Create<Packet> (m_size-(8+4*2)); // 8+4*2 : the size of the sptmHead header
  RtpHeader h;
  Ptr<Packet> p = CreateRtpPacket(h, 1146); //1200-42
  //p->AddHeader (sptmHead);
//std::cout << p->GetSize() << " " << h.padding << "\n";
  std::stringstream peerAddressStringStream;
  if (Ipv4Address::IsMatchingType (m_peerAddress))
    {
      peerAddressStringStream << Ipv4Address::ConvertFrom (m_peerAddress);
    }
 
  if ((m_socket->Send (p)) >= 0)
    {
      ++m_sent;
      NS_LOG_INFO ("TraceDelay TX " << m_size << " bytes to "
                                    << peerAddressStringStream.str () << " Uid: "
                                    << p->GetUid () << " Time: "
                                    << (Simulator::Now ()).GetSeconds ());

    }
  else
    {
      NS_LOG_INFO ("Error while sending " << m_size << " bytes to "
                                          << peerAddressStringStream.str ());
    }

  if (m_sent < m_count)
    {
      m_sendEvent = Simulator::Schedule (m_interval, &SptmClient::Send, this);
    }
    m_txTrace (p);
}

} // Namespace ns3
