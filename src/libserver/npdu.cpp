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

#include "npdu.h"

NPDUPtr
NPDU::fromPacket (const EIB_AddrType address_type, const eibaddr_t destination_address, const CArray & o6, TracePtr tr)
{
  NPDUPtr npdu;
  if (o6.size() >= 1)
    {
      if (address_type == GroupAddress)
        {
          if (destination_address == 0)
            npdu = NPDUPtr(new N_Data_Broadcast_PDU ()); // @todo N_Data_SystemBroadcast
          else
            npdu = NPDUPtr(new N_Data_Group_PDU ());
        }
      else
        npdu = NPDUPtr(new N_Data_Individual_PDU ());
    }
  if (npdu && npdu->init (o6, tr))
    return npdu;

  return nullptr;
}

/* N_Data_Individual_PDU */

bool
N_Data_Individual_PDU::init (const CArray & o6, TracePtr tr)
{
  return true;
}

CArray
N_Data_Individual_PDU::ToPacket () const
{
  CArray npdu;
  return npdu;
}

std::string
N_Data_Individual_PDU::decode (TracePtr tr) const
{
  std::string s;
  // @todo s = L_Data_PDU::decode(tr);
  s = "N_Data_Individual";
  return s;
}

/* N_Data_Group_PDU */

bool
N_Data_Group_PDU::init (const CArray & o6, TracePtr tr)
{
  return true;
}

CArray
N_Data_Group_PDU::ToPacket () const
{
  CArray npdu;
  return npdu;
}

std::string
N_Data_Group_PDU::decode (TracePtr tr) const
{
  std::string s;
  // @todo s = L_Data_PDU::decode(tr);
  s = "N_Data_Group";
  return s;
}

/* N_Data_Broadcast_PDU */

bool
N_Data_Broadcast_PDU::init (const CArray & o6, TracePtr tr)
{
  return true;
}

CArray
N_Data_Broadcast_PDU::ToPacket () const
{
  CArray npdu;
  return npdu;
}

std::string
N_Data_Broadcast_PDU::decode (TracePtr tr) const
{
  std::string s;
  // @todo s = L_Data_PDU::decode(tr);
  s = "N_Data_Broadcast";
  return s;
}

/* N_Data_SystemBroadcast_PDU */

bool
N_Data_SystemBroadcast_PDU::init (const CArray & o6, TracePtr tr)
{
  return true;
}

CArray N_Data_SystemBroadcast_PDU::ToPacket () const
{
  CArray npdu;
  return npdu;
}

std::string N_Data_SystemBroadcast_PDU::decode (TracePtr tr) const
{
  std::string s;
  s = L_SystemBroadcast_PDU::decode(tr);
  s += " N_Data_SystemBroadcast";
  return s;
}
