/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 Natale Patriciello, <natale.patriciello@gmail.com>
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

#include "tcp-mod276.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpModule");
NS_OBJECT_ENSURE_REGISTERED (TcpModule);

TypeId
TcpModule::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpModule")
    .SetParent<TcpNewReno> ()
    .AddConstructor<TcpModule> ()
    .SetGroupName ("Internet")
  ;
  return tid;
}

TcpModule::TcpModule (void)
  : TcpNewReno ()
{
  NS_LOG_FUNCTION (this);
}

TcpModule::TcpModule (const TcpModule& sock)
  : TcpNewReno (sock)
{
  NS_LOG_FUNCTION (this);
}

TcpModule::~TcpModule (void)
{
  NS_LOG_FUNCTION (this);
}

Ptr<TcpCongestionOps>
TcpModule::Fork (void)
{
  return CopyObject<TcpModule> (this);
}

void
TcpModule::CongestionAvoidance (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (segmentsAcked > 0)
    {
      double adder = static_cast<double> (tcb->m_segmentSize * tcb->m_segmentSize) / tcb->m_cWnd.Get();
      adder = std::max (1.0, adder);
      tcb->m_cWnd += static_cast<uint32_t> (adder);
      NS_LOG_INFO ("In CongAvoid, updated to cwnd " << tcb->m_cWnd <<
                   " ssthresh " << tcb->m_ssThresh);
    }
}


uint32_t
TcpModule::SlowStart (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (segmentsAcked >= 1)
    {
      tcb->m_cWnd += tcb->m_ssThresh / (tcb -> m_segmentSize * 10000) ;
      NS_LOG_INFO ("In SlowStart, updated to cwnd " << tcb->m_cWnd << " ssthresh " << tcb->m_ssThresh);
      return segmentsAcked - 1;
    }

  return 0;
}

std::string
TcpModule::GetName () const
{
  return "TcpModule";
}

uint32_t
TcpModule::GetSsThresh (Ptr<const TcpSocketState> tcb,
                           uint32_t bytesInFlight)
{
  NS_LOG_FUNCTION (this << tcb << bytesInFlight);

  uint32_t ssThresh = tcb->m_segmentSize * tcb->m_cWnd / (bytesInFlight + 1) ;
  NS_LOG_INFO ("Calculated cwnd = " << tcb->m_cWnd <<
                " resulting (in segment) ssThresh=" << ssThresh);

  return ssThresh ;
}

void
TcpModule::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (tcb->m_cWnd < tcb->m_ssThresh)
    {
      segmentsAcked = SlowStart (tcb, segmentsAcked);
    }

  if (tcb->m_cWnd >= tcb->m_ssThresh)
    {
      CongestionAvoidance (tcb, segmentsAcked);
    }

}

} // namespace ns3
