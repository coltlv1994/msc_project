/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 IITP RAS
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
 * Based on
 *      NS-2 AODV model developed by the CMU/MONARCH group and optimized and
 *      tuned by Samir Das and Mahesh Marina, University of Cincinnati;
 *
 *      AODV-UU implementation by Erik Nordström of Uppsala University
 *      http://core.it.uu.se/core/index.php/AODV-UU
 *
 * Authors: Elena Buchatskaia <borovkovaes@iitp.ru>
 *          Pavel Boyko <boyko@iitp.ru>
 */

#include "aodv-neighbor.h"
#include "ns3/network-module.h"
#include "ns3/ipv4.h"
#include "ns3/log.h"
#include <algorithm>
#include <cmath>
#include <sstream>


namespace ns3
{
  
NS_LOG_COMPONENT_DEFINE ("AodvNeighbors");

namespace aodv
{
Neighbors::Neighbors (Time delay) : 
  m_ntimer (Timer::CANCEL_ON_DESTROY)
{
  m_ntimer.SetDelay (delay);
  m_ntimer.SetFunction (&Neighbors::Purge, this);
  m_txErrorCallback = MakeCallback (&Neighbors::ProcessTxError, this);
}

bool
Neighbors::IsNeighbor (Ipv4Address addr)
{
  Purge ();
  for (std::vector<Neighbor>::const_iterator i = m_nb.begin ();
       i != m_nb.end (); ++i)
    {
      if (i->m_neighborAddress == addr)
        return true;
    }
  return false;
}

Time
Neighbors::GetExpireTime (Ipv4Address addr)
{
  Purge ();
  for (std::vector<Neighbor>::const_iterator i = m_nb.begin (); i
       != m_nb.end (); ++i)
    {
      if (i->m_neighborAddress == addr)
        return (i->m_expireTime - Simulator::Now ());
    }
  return Seconds (0);
}
//New
std::vector<Neighbors::Neighbor>
Neighbors::GetNeighborsList (void) {
  return m_nb;
}

void
Neighbors::Update (Ipv4Address addr, Time expire)
{
  for (std::vector<Neighbor>::iterator i = m_nb.begin (); i != m_nb.end (); ++i)
    if (i->m_neighborAddress == addr)
      {
        i->timestamp = Simulator::Now();
        i->m_expireTime
          = std::max (expire + Simulator::Now (), i->m_expireTime);
        if (i->m_hardwareAddress == Mac48Address ())
          i->m_hardwareAddress = LookupMacAddress (i->m_neighborAddress);
        return;
      }

  NS_LOG_LOGIC ("Open link to " << addr);
  Neighbor neighbor (addr, LookupMacAddress (addr), expire + Simulator::Now ());
  m_nb.push_back (neighbor);
  Purge ();
}

// New, Oct. 19
void
Neighbors::Update (Ipv4Address addr, Time expire, uint32_t me, float mv, uint32_t mql, uint32_t mns_e)
{
  // NS_LOG_UNCOND("Neighbor: " << addr << "\tEnergy: " << me << "\tNeighbor Aver: " << mns_e);
  // NS_LOG_UNCOND("INVOKED" << me << "\t" << mns_e);
  for (std::vector<Neighbor>::iterator i = m_nb.begin (); i != m_nb.end (); ++i)
    if (i->m_neighborAddress == addr)
      {
        i->m_energy = me; i->m_queuelength = mql; i->v_value = mv; i->mns_energy = mns_e;
        i->timestamp = Simulator::Now();
        i->m_expireTime
          = std::max (expire + Simulator::Now (), i->m_expireTime);
        if (i->m_hardwareAddress == Mac48Address ()) {
          i->m_hardwareAddress = LookupMacAddress (i->m_neighborAddress);
        }
        return;
      }
  NS_LOG_LOGIC ("Open link to " << addr);
  Neighbor neighbor (addr, LookupMacAddress (addr), expire + Simulator::Now (), me,mv, mql,mns_e);
  m_nb.push_back (neighbor);
  Purge ();
}

void Neighbors::VValueUpdate(void) {
  double averE;
  for (std::vector<Neighbor>::iterator i = m_nb.begin (); i != m_nb.end (); ++i) {
    averE += i->m_energy;
    averE /= (double)m_nb.size();
  }

  for (std::vector<Neighbor>::iterator i = m_nb.begin (); i != m_nb.end (); ++i) {
    i->v_value = 0.0;
  }

}

std::string Neighbors::PrintOut(void) const {
  std::stringstream ss;
  Time n = Simulator::Now();
  for (uint32_t index = 0; index<m_nb.size(); index++) {
    m_nb.at(index).m_neighborAddress.Print(ss);
    ss << "\tEnergy:" << m_nb.at(index).m_energy <<"\tAverE:"<<m_nb.at(index).mns_energy<<"\tQ value:"<<(m_nb.at(index).q_value)<<"\t Exp. Time"<<m_nb.at(index).m_expireTime-n<<std::endl;
  }
  return ss.str();

}

Ipv4Address Neighbors::RouteDecision(Ptr<Node> nd,const Ipv4Address & preHop, const Ipv4Address & dst) {
  uint32_t se = nd->GetSelfEnergy();
  float sv = nd->GetSelfV_value();
  float nmax = 0.0;
  uint32_t idx = 0;
  Ipv4Address sr = Ipv4Address("10.1.1.1");
  //Logic problem
  for(uint32_t index=0; index<m_nb.size();index++ ) {
    Neighbors::Neighbor &cr = m_nb.at(index);
    Time cu = Simulator::Now() - cr.timestamp ;
    if(cr.m_neighborAddress.IsEqual(preHop)||cr.m_neighborAddress.IsEqual(sr)) {
      cr.q_value = -1000000;
      continue;
    }
    else {
      if(cr.m_neighborAddress.IsEqual(dst)) {
        cr.q_value = 0;
        continue;
      }
      // NS_LOG_UNCOND(cu);
      float r_t = 0.9*(-1.0+0.5*cr.m_energy/1000000.0+0.5*2.0/3.1415927*atan((float)(cr.m_energy-cr.mns_energy)/1000000.0)+0.5*(-1.0/50.0*cu.GetMilliSeconds()))+0.1*(-1.0+0.5*se/1000000.0+0.5*2.0/3.1415927*atan((float)(se-mns_energy)/1000000.0)-0.5*0.8);
      cr.q_value = r_t + 0.9*(0.7*cr.v_value+0.3*sv);
    }
  }

  for (uint32_t index = 0; index<m_nb.size(); index++) {
    Neighbors::Neighbor &cr = m_nb.at(index);
    if(m_nb.size()==1 ) {
      NS_LOG_UNCOND("Only ONE");
      idx = 0;
      break;
    }
    if(index==0) {
      nmax = cr.q_value; continue;
    }
    else {
      if(cr.q_value>nmax) {
        nmax=cr.q_value;
        idx=index;
      }
    }
  }

  nd->SetSelfV_value(nmax);
  // NS_LOG_UNCOND(nmax);
  return m_nb.at(idx).m_neighborAddress;

  // Neighbors::Neighbor* n_max;

}

uint32_t Neighbors::GetMnsEnergy(void) {
  mns_gen();

  return mns_energy;
}

void Neighbors::mns_gen() {
  if(m_nb.size()==0)
  mns_energy = 0.0;
  else {
    uint32_t averE = 0;
    for(uint32_t index=0;index<m_nb.size();index++) {
      averE+=m_nb.at(index).m_energy;
    }
    mns_energy = (uint32_t)(averE/m_nb.size());
  }
}

struct CloseNeighbor
{
  bool operator() (const Neighbors::Neighbor & nb) const
  {
    return ((nb.m_expireTime < Simulator::Now ()) || nb.close);
  }
};

void
Neighbors::Purge ()
{
  if (m_nb.empty ())
    return;

  CloseNeighbor pred;
  if (!m_handleLinkFailure.IsNull ())
    {
      for (std::vector<Neighbor>::iterator j = m_nb.begin (); j != m_nb.end (); ++j)
        {
          if (pred (*j))
            {
              NS_LOG_LOGIC ("Close link to " << j->m_neighborAddress);
              m_handleLinkFailure (j->m_neighborAddress);
            }
        }
    }
  m_nb.erase (std::remove_if (m_nb.begin (), m_nb.end (), pred), m_nb.end ());
  m_ntimer.Cancel ();
  m_ntimer.Schedule ();
}

void
Neighbors::ScheduleTimer ()
{
  m_ntimer.Cancel ();
  m_ntimer.Schedule ();
}

void
Neighbors::AddArpCache (Ptr<ArpCache> a)
{
  m_arp.push_back (a);
}

void
Neighbors::DelArpCache (Ptr<ArpCache> a)
{
  m_arp.erase (std::remove (m_arp.begin (), m_arp.end (), a), m_arp.end ());
}

Mac48Address
Neighbors::LookupMacAddress (Ipv4Address addr)
{
  Mac48Address hwaddr;
  for (std::vector<Ptr<ArpCache> >::const_iterator i = m_arp.begin ();
       i != m_arp.end (); ++i)
    {
      ArpCache::Entry * entry = (*i)->Lookup (addr);
      if (entry != 0 && (entry->IsAlive () || entry->IsPermanent ()) && !entry->IsExpired ())
        {
          hwaddr = Mac48Address::ConvertFrom (entry->GetMacAddress ());
          break;
        }
    }
  return hwaddr;
}

void
Neighbors::ProcessTxError (WifiMacHeader const & hdr)
{
  Mac48Address addr = hdr.GetAddr1 ();

  for (std::vector<Neighbor>::iterator i = m_nb.begin (); i != m_nb.end (); ++i)
    {
      if (i->m_hardwareAddress == addr)
        i->close = true;
    }
  Purge ();
}
}
}

