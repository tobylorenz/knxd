/*
    EIBD eib bus access and management daemon
    Copyright (C) 2005-2011 Martin Koegler <mkoegler@auto.tuwien.ac.at>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/**
 * @file
 * @ingroup KNX_03_03_04
 * Transport Layer
 * @{
 */

#ifndef LAYER4_H
#define LAYER4_H

#include "link.h"
#include "router.h"

template<class T>
class T_Reader
{
public:
  virtual void send(T &) = 0;
};

template<class T>
class Layer4common:public LineDriver
{
protected:
  T_Reader<T> *app = nullptr;

  Layer4common(T_Reader<T> *app, LinkConnectClientPtr lc) : LineDriver(lc), app(app) {}
  virtual ~Layer4common() = default;
};

template<class T>
class Layer4commonWO:public Layer4common<T>
{
public:
  Layer4commonWO (T_Reader<T> *app, LinkConnectClientPtr lc, bool write_only) : Layer4common<T>(app,lc), write_only(write_only) {}

  virtual bool checkGroupAddress(eibaddr_t) const
  {
    return !write_only;
  }

  bool checkAddress(eibaddr_t addr) const
  {
    return !write_only && addr == this->getAddress();
  }

private:
  bool write_only;
};

/* point-to-multipoint, connectionless (multicast) */

/** informations about a group communication packet */
struct GroupAPDU
{
  /** source address */
  eibaddr_t source_address;

  /** destination address */
  eibaddr_t destination_address;

  /** Layer 4 data */
  CArray tsdu;
};

/** Group Communication socket */
class GroupSocket:public Layer4commonWO<GroupAPDU>
{
public:
  GroupSocket (T_Reader<GroupAPDU> *app, LinkConnectClientPtr lc, bool write_only);
  virtual ~GroupSocket ();

  virtual void send_L_Data (LDataPtr l) override;

  /** send APDU to L3 */
  void recv_Data (const GroupAPDU & c);
};

using GroupSocketPtr = std::shared_ptr<GroupSocket>;

/** informations about a group communication packet */
struct GroupComm
{
  /** source address */
  eibaddr_t source_address;

  /** Layer 4 data */
  CArray tsdu;
};

/** Group Layer 4 connection (Multicast) */
class T_Group:public Layer4commonWO<GroupComm>
{
public:
  T_Group (T_Reader<GroupComm> *app, LinkConnectClientPtr lc, eibaddr_t group_address, bool write_only);
  virtual ~T_Group ();

  virtual void send_L_Data (LDataPtr l) override;

  virtual bool checkGroupAddress(eibaddr_t group_address) const override
  {
    return (group_address == this->group_address);
  }

  /** send APDU to L3 */
  void recv_Data (const CArray & c);

private:
  /** group address */
  eibaddr_t group_address;
};

using T_GroupPtr = std::shared_ptr<T_Group>;

/* point-to-domain, connectionless (broadcast) */

/** information about a broadcast packet */
struct BroadcastComm
{
  /** source address */
  eibaddr_t source_address;

  /** Layer 4 data */
  CArray tsdu;
};

/** Broadcast Layer 4 connection (Broadcast) */
class T_Broadcast:public Layer4commonWO<BroadcastComm>
{
public:
  T_Broadcast (T_Reader<BroadcastComm> *app, LinkConnectClientPtr lc, bool write_only);
  virtual ~T_Broadcast ();

  virtual void send_L_Data (LDataPtr l) override;

  /** send APDU c */
  void recv_Data (const CArray & c);
};

using T_BroadcastPtr = std::shared_ptr<T_Broadcast>;

/* point-to-all-points, connectionless (system broadcast) */

// @todo

/* point-to-point, connectionless */

/** Layer 4 T_Individual connection (one-to-one connectionless) */
class T_Individual:public Layer4commonWO<CArray>
{
public:
  T_Individual (T_Reader<CArray> *app, LinkConnectClientPtr lc, eibaddr_t destination_address, bool write_only);
  virtual ~T_Individual ();

  virtual void send_L_Data (LDataPtr l) override;

  /** send APDU to L3 */
  void recv_Data (const CArray & c);

private:
  /** destination address */
  eibaddr_t destination_address;
};

using T_IndividualPtr = std::shared_ptr<T_Individual>;

/* point-to-point, connection-oriented */

/** implement a client T_Connection (one-to-one connection-oriented) */
class T_Connection:public Layer4common<CArray>
{
public:
  T_Connection (T_Reader<CArray> *app, LinkConnectClientPtr lc, eibaddr_t destination_address);
  virtual ~T_Connection ();

  virtual void send_L_Data (LDataPtr l) override;

  /** send APDU to L3 */
  void recv_Data (const CArray & c);

  void stop();

private:
  ev::timer timer;
  void timer_cb(ev::timer &w, int revents);

  /** input queue */
  Queue < CArray > in;
  CArray current;
  /** buffer queue for layer 3 */
  Queue < L_Data_PDU * >buf;
  /** output queue */
  Queue < CArray > out;
  /** receiving sequence number */
  int recvno = 0;
  /** sending sequence number */
  int sendno = 0;
  /** state */
  int mode = 0;
  /** repeat count of the transmitting frame */
  int repcount;
  /** destination address */
  eibaddr_t destination_address;

  /** sends T_Connect */
  void SendConnect ();
  /** sends T_Disconnect */
  void SendDisconnect ();
  /** send T_ACK */
  void SendAck (int sequence_number);
  /** Sends T_DataConnected */
  void SendData (int sequence_number, const CArray & c);
  /** process the next bit from sendq if mode==1 */
  void SendCheck ();
};

using T_ConnectionPtr = std::shared_ptr<T_Connection>;

/* raw */

/** a raw layer 4 connection packet */
struct TpduComm
{
  /** individual address of the remote device */
  eibaddr_t addr; // @todo source_address or destination_address?

  /** Layer 4 data */
  CArray lsdu;
};

/** Layer 4 raw individual connection */
class T_TPDU:public Layer4common<TpduComm>
{
public:
  T_TPDU (T_Reader<TpduComm> *app, LinkConnectClientPtr lc, eibaddr_t source_address);
  virtual ~T_TPDU ();

  virtual void send_L_Data (LDataPtr l) override;

  /** send APDU to L3 */
  void recv_Data (const TpduComm & c);

private:
  /** source address to use */
  eibaddr_t source_address;
};

using T_TPDUPtr = std::shared_ptr<T_TPDU>;

#endif

/** @} */
