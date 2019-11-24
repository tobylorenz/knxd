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

#ifndef TPDU_H
#define TPDU_H

#include <memory>

#include "common.h"
#include "npdu.h"

/** enumeration of TPDU types */
enum TPDU_Type
{
  /** unknown TPDU */
  T_Unknown = 0,
  /** T_Data_Broadcast */
  T_Data_Broadcast,
  /** T_Data_SystemBroadcast */
  T_Data_SystemBroadcast,
  /** T_Data_Group */
  T_Data_Group,
  /** T_Data_Tag_Group */
  T_Data_Tag_Group,
  /** T_Data_Individual */
  T_Data_Individual,
  /** T_Data_Connected */
  T_Data_Connected,
  /** T_Connect */
  T_Connect,
  /** T_Disconnect */
  T_Disconnect,
  /** T_ACK */
  T_ACK,
  /** T_NAK */
  T_NAK,
};

class TPDU;
using TPDUPtr = std::unique_ptr<TPDU>;

/** abstract TPDU base class */
class TPDU
{
public:
  virtual ~TPDU () = default;

  virtual bool init (const CArray & c, TracePtr tr) = 0;

  /**
   * convert to character array
   *
   * @return Octet 6 to N
   */
  virtual CArray ToPacket () const = 0;

  /** decode content as string */
  virtual std::string decode (TracePtr tr) const = 0;

  /** gets TPDU type */
  virtual TPDU_Type getTType () const = 0;

  /**
   * converts character array to a TPDU
   *
   * @param[in] address_type Address Type (AT) from Octet 5
   * @param[in] destination_address Destination Address
   * @param[in] o6 Octet 6 to N
   * @oaram[in] tr Trace pointer
   * @return TPDU
   */
  static TPDUPtr fromPacket (const EIB_AddrType address_type, const eibaddr_t destination_address, const CArray & o6, TracePtr tr);
};

/** T_Data_Broadcast PDU */
class T_Data_Broadcast_PDU:public TPDU, public N_Data_Broadcast_PDU
{
public:
  T_Data_Broadcast_PDU () = default;
  virtual bool init (const CArray & c, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string decode (TracePtr tr) const override;
  virtual TPDU_Type getTType () const override
  {
    return T_Data_Broadcast;
  }
};

/** T_Data_SystemBroadcast PDU */
class T_Data_SystemBroadcast_PDU:public TPDU, public N_Data_SystemBroadcast_PDU
{
public:
  T_Data_SystemBroadcast_PDU () = default;
  virtual bool init (const CArray & c, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string decode (TracePtr tr) const override;
  virtual TPDU_Type getTType () const override
  {
    return T_Data_SystemBroadcast;
  }
};

/** T_Data_Group PDU */
class T_Data_Group_PDU:public TPDU, public N_Data_Group_PDU
{
public:
  T_Data_Group_PDU () = default;
  virtual bool init (const CArray & c, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string decode (TracePtr tr) const override;
  virtual TPDU_Type getTType () const override
  {
    return T_Data_Group;
  }
};

/** T_Data_Tag_Group PDU */
class T_Data_Tag_Group_PDU:public TPDU, public N_Data_Group_PDU
{
public:
  T_Data_Tag_Group_PDU () = default;
  virtual bool init (const CArray & c, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string decode (TracePtr tr) const override;
  virtual TPDU_Type getTType () const override
  {
    return T_Data_Tag_Group;
  }
};

/** T_Data_Individual PDU */
class T_Data_Individual_PDU:public TPDU, public N_Data_Individual_PDU
{
public:
  T_Data_Individual_PDU () = default;
  virtual bool init (const CArray & c, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string decode (TracePtr tr) const override;
  virtual TPDU_Type getTType () const override
  {
    return T_Data_Individual;
  }
};

/** T_Data_Connected PDU */
class T_Data_Connected_PDU:public TPDU, public N_Data_Individual_PDU
{
public:
  uint8_t sequence_number = 0;

  T_Data_Connected_PDU () = default;
  virtual bool init (const CArray & c, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string decode (TracePtr tr) const override;
  virtual TPDU_Type getTType () const override
  {
    return T_Data_Connected;
  }
};

/** T_Connect PDU */
class T_Connect_PDU:public TPDU, public N_Data_Individual_PDU
{
public:
  T_Connect_PDU () = default;
  virtual bool init (const CArray & c, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string decode (TracePtr tr) const override;
  virtual TPDU_Type getTType () const override
  {
    return T_Connect;
  }
};

/** T_Disconnect PDU */
class T_Disconnect_PDU:public TPDU, public N_Data_Individual_PDU
{
public:

  T_Disconnect_PDU () = default;
  virtual bool init (const CArray & c, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string decode (TracePtr tr) const override;
  virtual TPDU_Type getTType () const override
  {
    return T_Disconnect;
  }
};

/** T_ACK PDU */
class T_ACK_PDU:public TPDU, public N_Data_Individual_PDU
{
public:
  uint8_t sequence_number = 0;

  T_ACK_PDU () = default;
  virtual bool init (const CArray & c, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string decode (TracePtr tr) const override;
  virtual TPDU_Type getTType () const override
  {
    return T_ACK;
  }
};

/** T_NAK PDU */
class T_NAK_PDU:public TPDU, public N_Data_Individual_PDU
{
public:
  uint8_t sequence_number = 0;

  T_NAK_PDU () = default;
  virtual bool init (const CArray & c, TracePtr tr) override;
  virtual CArray ToPacket () const override;
  virtual std::string decode (TracePtr tr) const override;
  virtual TPDU_Type getTType () const override
  {
    return T_NAK;
  }
};

#endif

/** @} */
