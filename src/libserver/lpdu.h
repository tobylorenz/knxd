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
 * @ingroup KNX_03_03_02
 * Data Link Layer General
 * @{
 */

#ifndef LPDU_H
#define LPDU_H

#include <memory>

#include "trace.h"

/** enumeration of Layer 2 frame types */
enum LPDU_Type
{
  /** unknown LPDU */
  L_Unknown = 0,
  /** L_Data */
  L_Data,
  /** L_SystemBroadcast */
  L_SystemBroadcast,
  /** L_Poll_Data */
  L_Poll_Data,
  /** L_Poll_Update */
  L_Poll_Update,
  /** L_Busmon */
  L_Busmon,
  /** L_Service_Information */
  L_Service_Information,
  /** L_Management */
  L_Management,
};

/** Message Priority */
enum EIB_Priority : uint8_t
{
  PRIO_LOW    = 3, // 11b
  PRIO_NORMAL = 1, // 01b
  PRIO_URGENT = 2, // 10b
  PRIO_SYSTEM = 0, // 00b
};

/** Address Type */
enum EIB_AddrType : uint8_t
{
  IndividualAddress = 0,
  GroupAddress = 1
};

/** abstract LPDU base class */
class LPDU
{
public:
  LPDU () = default;
  virtual ~LPDU () = default;

  /** decode content as string */
  virtual std::string decode (TracePtr tr) const = 0;

  /** get frame type */
  virtual LPDU_Type getLType () const = 0;
};

using LPDUPtr = std::unique_ptr<LPDU>;

/** L_Data PDU */
class L_Data_PDU:public LPDU
{
public:
  /** acknowledge request (0=optional, 1=mandatory) */
  uint8_t ack_request = 0;

  /** address type */
  EIB_AddrType address_type = IndividualAddress;

  /* destination address */
  eibaddr_t destination_address = 0;

  /** standard/extended frame format (0=Extended, 1=Standard) */
  uint8_t frame_format = 0;

  /** octet count */
  uint8_t octet_count = 0;

  /** priority */
  EIB_Priority priority = PRIO_LOW;

  /* source address */
  eibaddr_t source_address = 0;

  /** LSDU extended control field (octet 5, hop_count_type) */
  uint8_t lsdu_ctrl = 0;

  /** LSDU (octet 6 to N) */
  CArray lsdu {};

  /* status */
  uint8_t l_status = 0;

  /** is repeated */
  bool repeated = false;

  /** checksum ok */
  bool valid_checksum = true;

  /** length ok */
  bool valid_length = true;
  uint8_t hop_count_type = 0x06; // @todo belongs to NPDU

  L_Data_PDU () = default;

  virtual std::string decode (TracePtr tr) const override;
  virtual LPDU_Type getLType () const override
  {
    return L_Data;
  }
};

using LDataPtr = std::unique_ptr<L_Data_PDU>;

/** L_SystemBroadcast PDU */
class L_SystemBroadcast_PDU:public LPDU
{
public:
  /** acknowledge request (0=optional, 1=mandatory) */
  uint8_t ack_request = 0;

  /** address type */
  EIB_AddrType address_type = IndividualAddress;

  /** destination address */
  eibaddr_t destination_address = 0;

  /** standard/extended frame format */
  uint8_t frame_format = 0;

  /** octet count */
  uint8_t octet_count = 0;

  /** priority */
  EIB_Priority priority = PRIO_LOW;

  /** source address */
  eibaddr_t source_address = 0;

  /** LSDU extended control field (octet 5, hop_count_type) */
  uint8_t lsdu_ctrl = 0;

  /** LSDU (octet 6 to N) */
  CArray lsdu {};

  /* status */
  uint8_t l_status = 0;

  L_SystemBroadcast_PDU () = default;

  virtual std::string decode (TracePtr tr) const override;
  virtual LPDU_Type getLType () const override
  {
    return L_SystemBroadcast;
  }
};

/** L_Poll_Data PDU */
class L_Poll_Data_PDU:public LPDU
{
public:
  /* destination address */
  eibaddr_t destination_address = 0;
  uint8_t no_of_expected_poll_data = 0;
  CArray poll_data_sequence;
  uint8_t l_status = 0;

  L_Poll_Data_PDU () = default;

  virtual std::string decode (TracePtr tr) const override;
  virtual LPDU_Type getLType () const override
  {
    return L_Poll_Data;
  }
};

/** L_Poll_Update PDU */
class L_Poll_Update_PDU:public LPDU
{
public:
  uint8_t poll_data;

  L_Poll_Update_PDU () = default;

  virtual std::string decode (TracePtr tr) const override;
  virtual LPDU_Type getLType () const override
  {
    return L_Poll_Update;
  }
};

/** L_Busmon PDU */
class L_Busmon_PDU:public LPDU
{
public:
  uint8_t l_status;
  /** content of the TP1 frame */
  CArray lpdu;
  uint32_t time_stamp;

  L_Busmon_PDU ();

  virtual std::string decode (TracePtr tr) const override;
  virtual LPDU_Type getLType () const override
  {
    return L_Busmon;
  }
};

using LBusmonPtr = std::unique_ptr<L_Busmon_PDU>;

/** interface for callback for busmonitor frames */
class L_Busmonitor_CallBack
{
public:
  L_Busmonitor_CallBack(std::string& n) : name(n) { }
  std::string& name;
  /** callback: a bus monitor frame has been received */
  virtual void send_L_Busmonitor (LBusmonPtr l) = 0;
};

/** L_Service_Information PDU */
class L_Service_Information_PDU:public LPDU
{
public:
  L_Service_Information_PDU () = default;

  virtual std::string decode (TracePtr tr) const override;
  virtual LPDU_Type getLType () const override
  {
    return L_Service_Information;
  }
};

/** L_Management PDU */
class L_Management_PDU:public LPDU
{
public:
  L_Management_PDU () = default;

  virtual std::string decode (TracePtr tr) const override;
  virtual LPDU_Type getLType () const override
  {
    return L_Management;
  }
};

#endif

/** @} */
