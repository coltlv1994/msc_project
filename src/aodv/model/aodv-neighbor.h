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

#ifndef AODVNEIGHBOR_H
#define AODVNEIGHBOR_H

#include "ns3/simulator.h"
#include "ns3/timer.h"
#include "ns3/ipv4-address.h"
#include "ns3/callback.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/arp-cache.h"
#include "ns3/node.h"
#include <vector>

namespace ns3
{
namespace aodv
{
class RoutingProtocol;
/**
 * \ingroup aodv
 * \brief maintain list of active neighbors
 */
class Neighbors
{
public:
  /// c-tor
  Neighbors (Time delay);
  /// Neighbor description
  struct Neighbor
  {
    Ipv4Address m_neighborAddress;
    Mac48Address m_hardwareAddress;
    Time m_expireTime;
    bool close;
    uint32_t m_energy;
    float v_value;
    uint32_t m_queuelength;
    float q_value;
    uint32_t mns_energy;
    Time timestamp;


    Neighbor (Ipv4Address ip, Mac48Address mac, Time t) :
      m_neighborAddress (ip), m_hardwareAddress (mac), m_expireTime (t),
      close (false)
    {
      timestamp = Simulator::Now();
    }
    // New, Oct. 19
    Neighbor (Ipv4Address ip, Mac48Address mac, Time t, uint32_t me, float mv, uint32_t mql, uint32_t mns_e) :
    m_neighborAddress (ip), m_hardwareAddress (mac), m_expireTime (t),
    close (false), m_energy (me), v_value(mv), m_queuelength (mql)
  {
    q_value = 0.0;
    mns_energy = mns_e;
  }
  };
  /// Return expire time for neighbor node with address addr, if exists, else return 0.
  Time GetExpireTime (Ipv4Address addr);
  // Return the neighbor list (vector), new Oct. 17
  std::vector<Neighbor> GetNeighborsList (void) ;
  /// Check that node with address addr  is neighbor
  bool IsNeighbor (Ipv4Address addr);
  /// Update expire time for entry with address addr, if it exists, else add new entry
  void Update (Ipv4Address addr, Time expire);
  // New, Oct. 18
  void Update (Ipv4Address addr, Time expire, uint32_t me, float mv, uint32_t mql, uint32_t mns_e);
  void VValueUpdate(void);
  Ipv4Address RouteDecision (Ptr<Node> nd,const Ipv4Address & preHop,const Ipv4Address & dst);
  void mns_gen(void);
  uint32_t GetMnsEnergy(void);
  /// Remove all expired entries
  void Purge ();
  /// Schedule m_ntimer.
  void ScheduleTimer ();
  /// Remove all entries
  void Clear () { m_nb.clear (); }

  /// Add ARP cache to be used to allow layer 2 notifications processing
  void AddArpCache (Ptr<ArpCache>);
  /// Don't use given ARP cache any more (interface is down)
  void DelArpCache (Ptr<ArpCache>);
  /// Get callback to ProcessTxError
  Callback<void, WifiMacHeader const &> GetTxErrorCallback () const { return m_txErrorCallback; }
 
  /// Handle link failure callback
  void SetCallback (Callback<void, Ipv4Address> cb) { m_handleLinkFailure = cb; }
  /// Handle link failure callback
  Callback<void, Ipv4Address> GetCallback () const { return m_handleLinkFailure; }
  std::string PrintOut(void) const;

private:
  /// link failure callback
  Callback<void, Ipv4Address> m_handleLinkFailure;
  /// TX error callback
  Callback<void, WifiMacHeader const &> m_txErrorCallback;
  /// Timer for neighbor's list. Schedule Purge().
  Timer m_ntimer;
  /// vector of entries
  std::vector<Neighbor> m_nb;
  /// Neighbor energy, aver.
  uint32_t mns_energy;
  /// list of ARP cached to be used for layer 2 notifications processing
  std::vector<Ptr<ArpCache> > m_arp;

  /// Find MAC address by IP using list of ARP caches
  Mac48Address LookupMacAddress (Ipv4Address);
  /// Process layer 2 TX error notification
  void ProcessTxError (WifiMacHeader const &);
};

}
}

#endif /* AODVNEIGHBOR_H */
