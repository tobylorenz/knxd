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

#include "tpdu.h"
#include "apdu.h"

/* TPDU */

TPDUPtr
TPDU::fromPacket (const EIB_AddrType address_type, const eibaddr_t destination_address, const CArray & o6, TracePtr tr)
{
  TPDUPtr tpdu;
  if (o6.size() >= 1)
    {
      if (address_type == GroupAddress)
        {
          if ((o6[0] & 0xFC) == 0x00)
            {
              if (destination_address == 0)
                tpdu = TPDUPtr(new T_Data_Broadcast_PDU ()); // @todo T_Data_SystemBroadcast
              else
                tpdu = TPDUPtr(new T_Data_Group_PDU ());
            }
          else if ((o6[0] & 0xFC) == 0x04)
            tpdu = TPDUPtr(new T_Data_Tag_Group_PDU ());
        }
      else
        {
          if ((o6[0] & 0xFC) == 0x00)
            tpdu = TPDUPtr(new T_Data_Individual_PDU ());
          else if ((o6[0] & 0xC0) == 0x40)
            tpdu = TPDUPtr(new T_Data_Connected_PDU ());
          else if (o6[0] == 0x80)
            tpdu = TPDUPtr(new T_Connect_PDU ());
          else if (o6[0] == 0x81)
            tpdu = TPDUPtr(new T_Disconnect_PDU ());
          else if ((o6[0] & 0xC3) == 0xC2)
            tpdu = TPDUPtr(new T_ACK_PDU ());
          else if ((o6[0] & 0xC3) == 0xC3)
            tpdu = TPDUPtr(new T_NAK_PDU ());
        }
    }
  if (tpdu && tpdu->init (o6, tr))
    return tpdu;

  return nullptr;
}

/* T_Data_Broadcast_PDU */

bool
T_Data_Broadcast_PDU::init (const CArray & c, TracePtr tr)
{
  if (c.size() < 1)
    return false;

  lsdu = c;
  return N_Data_Broadcast_PDU::init(c, tr);
}

CArray
T_Data_Broadcast_PDU::ToPacket () const
{
  assert (lsdu.size() > 0);

  CArray tpdu (lsdu);
  tpdu[0] = tpdu[0] & 0x03;
  return tpdu;
}

std::string
T_Data_Broadcast_PDU::decode (TracePtr tr) const
{
  std::string s;
  s = N_Data_Broadcast_PDU::decode(tr);
  s += " T_Data_Broadcast";
  return s;
}

/* T_Data_SystemBroadcast_PDU */

bool
T_Data_SystemBroadcast_PDU::init (const CArray & c, TracePtr tr)
{
  if (c.size() < 1)
    return false;

  lsdu = c;
  return N_Data_SystemBroadcast_PDU::init(c, tr);
}

CArray
T_Data_SystemBroadcast_PDU::ToPacket () const
{
  assert (lsdu.size() > 0);

  CArray tpdu (lsdu);
  tpdu[0] = tpdu[0] & 0x03;
  return tpdu;
}

std::string
T_Data_SystemBroadcast_PDU::decode (TracePtr tr) const
{
  std::string s;
  s = N_Data_SystemBroadcast_PDU::decode(tr);
  s += " T_Data_SystemBroadcast";
  return s;
}

/* T_Data_Group_PDU */

bool
T_Data_Group_PDU::init (const CArray & c, TracePtr tr)
{
  if (c.size() < 1)
    return false;

  lsdu = c;
  return N_Data_Group_PDU::init(c, tr);
}

CArray
T_Data_Group_PDU::ToPacket () const
{
  assert (lsdu.size() > 0);

  CArray tpdu (lsdu);
  tpdu[0] = tpdu[0] & 0x03;
  return tpdu;
}

std::string
T_Data_Group_PDU::decode (TracePtr tr) const
{
  std::string s;
  s = N_Data_Group_PDU::decode(tr);
  s += " T_Data_Group";
  return s;
}

/* T_Data_Tag_Group_PDU */

bool
T_Data_Tag_Group_PDU::init (const CArray & c, TracePtr tr)
{
  if (c.size() < 1)
    return false;

  lsdu = c;
  return N_Data_Group_PDU::init(c, tr);
}

CArray
T_Data_Tag_Group_PDU::ToPacket () const
{
  assert (lsdu.size() > 0);

  CArray tpdu (lsdu);
  tpdu[0] = tpdu[0] & 0x03;
  return tpdu;
}

std::string
T_Data_Tag_Group_PDU::decode (TracePtr tr) const
{
  std::string s;
  s = N_Data_Group_PDU::decode(tr);
  s += " T_Data_Tag_Group";
  return s;
}

/* T_Data_Individual_PDU */

bool
T_Data_Individual_PDU::init (const CArray & c, TracePtr tr)
{
  if (c.size() < 1)
    return false;

  lsdu = c;
  return N_Data_Individual_PDU::init(c, tr);
}

CArray
T_Data_Individual_PDU::ToPacket () const
{
  assert (lsdu.size() > 0);

  CArray tpdu (lsdu);
  tpdu[0] = tpdu[0] & 0x03;
  return tpdu;
}

std::string
T_Data_Individual_PDU::decode (TracePtr tr) const
{
  std::string s;
  s = N_Data_Individual_PDU::decode(tr);
  s += " T_Data_Individual";
  return s;
}

/* T_Data_Connected_PDU */

bool
T_Data_Connected_PDU::init (const CArray & c, TracePtr tr)
{
  if (c.size() < 1)
    return false;

  lsdu = c;
  sequence_number = (c[0] >> 2) & 0x0f;
  return N_Data_Individual_PDU::init(c, tr);
}

CArray
T_Data_Connected_PDU::ToPacket () const
{
  assert (lsdu.size() > 0);
  assert ((sequence_number & 0xf0) == 0);

  CArray tpdu (lsdu);
  tpdu[0] = 0x40 | ((sequence_number & 0x0f) << 2) | (tpdu[0] & 0x03);
  return tpdu;
}

std::string
T_Data_Connected_PDU::decode (TracePtr tr) const
{
  assert ((sequence_number & 0xf0) == 0);

  std::string s;
  s = N_Data_Individual_PDU::decode(tr);
  s += " T_Data_Connected serno:";
  addHex (s, sequence_number);
  return s;
}

/* T_Connect_PDU */

bool
T_Connect_PDU::init (const CArray & c, TracePtr tr)
{
  if (c.size() != 1)
    return false;

  return N_Data_Individual_PDU::init(c, tr);
}

CArray
T_Connect_PDU::ToPacket () const
{
  CArray tpdu;
  tpdu.resize(1);
  tpdu[0] = 0x80;
  return tpdu;
}

std::string
T_Connect_PDU::decode (TracePtr tr) const
{
  std::string s;
  s = N_Data_Individual_PDU::decode(tr);
  s += " T_Connect";
  return s;
}

/* T_Disconnect_PDU */

bool
T_Disconnect_PDU::init (const CArray & c, TracePtr tr)
{
  if (c.size() != 1)
    return false;

  return N_Data_Individual_PDU::init(c, tr);
}

CArray
T_Disconnect_PDU::ToPacket () const
{
  CArray tpdu;
  tpdu.resize(1);
  tpdu[0] = 0x81;
  return tpdu;
}

std::string
T_Disconnect_PDU::decode (TracePtr tr) const
{
  std::string s;
  s = N_Data_Individual_PDU::decode(tr);
  s += " T_Disconnect";
  return s;
}

/* T_ACK_PDU */

bool
T_ACK_PDU::init (const CArray & c, TracePtr tr)
{
  if (c.size() != 1)
    return false;

  sequence_number = (c[0] >> 2) & 0x0f;
  return N_Data_Individual_PDU::init(c, tr);
}

CArray
T_ACK_PDU::ToPacket () const
{
  assert ((sequence_number & 0xf0) == 0);

  CArray tpdu;
  tpdu.resize(1);
  tpdu[0] = 0xC2 | ((sequence_number & 0x0f) << 2);
  return tpdu;
}

std::string
T_ACK_PDU::decode (TracePtr tr) const
{
  assert ((sequence_number & 0xf0) == 0);

  std::string s;
  s = N_Data_Individual_PDU::decode(tr);
  s += " T_ACK Serno:";
  addHex (s, sequence_number);
  return s;
}

/* T_NAK_PDU */

bool
T_NAK_PDU::init (const CArray & c, TracePtr tr)
{
  if (c.size() != 1)
    return false;

  sequence_number = (c[0] >> 2) & 0x0f;
  return N_Data_Individual_PDU::init(c, tr);
}

CArray
T_NAK_PDU::ToPacket () const
{
  assert ((sequence_number & 0xf0) == 0);

  CArray tpdu;
  tpdu.resize(1);
  tpdu[0] = 0xC3 | ((sequence_number & 0x0f) << 2);
  return tpdu;
}

std::string
T_NAK_PDU::decode (TracePtr tr) const
{
  assert ((sequence_number & 0xf0) == 0);

  std::string s;
  s = N_Data_Individual_PDU::decode(tr);
  s += " T_NAK Serno:";
  addHex (s, sequence_number);
  return s;
}
