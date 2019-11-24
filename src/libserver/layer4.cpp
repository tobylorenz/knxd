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

#include "layer4.h"

#include "tpdu.h"

/***************** GroupSocket *****************/

GroupSocket::GroupSocket (T_Reader<GroupAPDU> *app, LinkConnectClientPtr lc, bool write_only)
  : Layer4commonWO(app, lc, write_only)
{
  TRACEPRINTF (t, 4, "OpenGroupSocket %s", write_only ? "WO" : "RW");
}

GroupSocket::~GroupSocket ()
{
  TRACEPRINTF (t, 4, "CloseGroupSocket");
}

void
GroupSocket::send_L_Data (LDataPtr lpdu)
{
  GroupAPDU c;
  TPDUPtr tpdu = TPDU::fromPacket (lpdu->address_type, lpdu->destination_address, lpdu->lsdu, t);
  if (tpdu->getTType () == T_Data_Group)
    {
      T_Data_Group_PDU *tpdu1 = (T_Data_Group_PDU *) &*tpdu;
      c.source_address = lpdu->source_address;
      c.destination_address = lpdu->destination_address;
      c.lsdu = tpdu1->lsdu;
      app->send(c);
    }
  send_Next();
}

void
GroupSocket::recv_Data (const GroupAPDU & c)
{
  std::unique_ptr<T_Data_Group_PDU> tpdu(new T_Data_Group_PDU);
  tpdu->init(c.lsdu, nullptr);
  std::string s = tpdu->decode (t);
  TRACEPRINTF (t, 4, "Recv GroupSocket %s %s", FormatGroupAddr(c.destination_address), s);
  tpdu->source_address = 0;
  tpdu->destination_address = c.destination_address;
  tpdu->address_type = GroupAddress;
  tpdu->lsdu = tpdu->ToPacket ();
  auto r = recv.lock();
  if (r)
    r->recv_L_Data (std::move(tpdu));
}

/***************** T_Group *****************/

T_Group::T_Group (T_Reader<GroupComm> *app, LinkConnectClientPtr lc, eibaddr_t group_address, bool write_only) :
  Layer4commonWO(app, lc, write_only), group_address(group_address)
{
  t->setAuxName("TGr");
  TRACEPRINTF (t, 4, "OpenGroup %s %s", FormatGroupAddr (group_address),
               write_only ? "WO" : "RW");
}

void
T_Group::send_L_Data (LDataPtr lpdu)
{
  GroupComm c;
  TPDUPtr tpdu = TPDU::fromPacket (lpdu->address_type, lpdu->destination_address, lpdu->lsdu, t);
  if (tpdu->getTType () == T_Data_Group)
    {
      T_Data_Group_PDU *tpdu1 = (T_Data_Group_PDU *) &*tpdu;
      c.lsdu = tpdu1->lsdu;
      c.source_address = lpdu->source_address;
      app->send(c);
    }
  send_Next();
}

void
T_Group::recv_Data (const CArray & c)
{
  std::unique_ptr<T_Data_Group_PDU> tpdu(new T_Data_Group_PDU);
  tpdu->init(c, nullptr);
  std::string s = tpdu->decode (t);
  TRACEPRINTF (t, 4, "Recv Group %s", s);
  tpdu->source_address = 0;
  tpdu->destination_address = group_address;
  tpdu->address_type = GroupAddress;
  tpdu->lsdu = tpdu->ToPacket ();
  auto r = recv.lock();
  if (r)
    r->recv_L_Data (std::move(tpdu));
}

T_Group::~T_Group ()
{
  TRACEPRINTF (t, 4, "CloseGroup");
}

/***************** T_Broadcast *****************/

T_Broadcast::T_Broadcast (T_Reader<BroadcastComm> *app, LinkConnectClientPtr lc, bool write_only)
  : Layer4commonWO(app, lc, write_only)
{
  t->setAuxName("TBr");
  TRACEPRINTF (t, 4, "OpenBroadcast %s", write_only ? "WO" : "RW");
}

T_Broadcast::~T_Broadcast ()
{
  TRACEPRINTF (t, 4, "CloseBroadcast");
}

void
T_Broadcast::send_L_Data (LDataPtr lpdu)
{
  BroadcastComm c;
  TPDUPtr tpdu = TPDU::fromPacket (lpdu->address_type, lpdu->destination_address, lpdu->lsdu, t);
  if (tpdu->getTType () == T_Data_Broadcast)
    {
      T_Data_Broadcast_PDU *tpdu1 = (T_Data_Broadcast_PDU *) &*tpdu;
      c.lsdu = tpdu1->lsdu;
      c.source_address = lpdu->source_address;
      app->send(c);
    }
  send_Next();
}

void
T_Broadcast::recv_Data (const CArray & c)
{
  std::unique_ptr<T_Data_Broadcast_PDU> tpdu(new T_Data_Broadcast_PDU);
  tpdu->init(c, nullptr);
  std::string s = tpdu->decode (t);
  TRACEPRINTF (t, 4, "Recv Broadcast %s", s);
  tpdu->source_address = 0;
  tpdu->destination_address = 0;
  tpdu->address_type = GroupAddress;
  tpdu->hop_count_type = 0x07;
  tpdu->lsdu = tpdu->ToPacket ();
  auto r = recv.lock();
  if (r)
    r->recv_L_Data (std::move(tpdu));
}

/***************** T_Individual *****************/

T_Individual::T_Individual (T_Reader<CArray> *app, LinkConnectClientPtr lc, eibaddr_t destination_address, bool write_only)
  : Layer4commonWO(app, lc, write_only), destination_address(destination_address)
{
  t->setAuxName("TInd");
  TRACEPRINTF (t, 4, "OpenIndividual %s %s",
               FormatEIBAddr (destination_address).c_str(), write_only ? "WO" : "RW");
}

void
T_Individual::send_L_Data (LDataPtr lpdu)
{
  CArray c;
  TPDUPtr tpdu = TPDU::fromPacket (lpdu->address_type, lpdu->destination_address, lpdu->lsdu, t);
  switch (tpdu->getTType ())
    {
    case T_Data_Broadcast:
    {
      T_Data_Broadcast_PDU *tpdu1 = (T_Data_Broadcast_PDU *) &*tpdu;
      c = tpdu1->lsdu;
      app->send(c);
    }
    break;
    case T_Data_Individual:
    {
      T_Data_Individual_PDU *tpdu1 = (T_Data_Individual_PDU *) &*tpdu;
      c = tpdu1->lsdu;
      app->send(c);
    }
    break;
    default:
      /* ignore */
      break;
    }
  send_Next();
}

void
T_Individual::recv_Data (const CArray & c)
{
  std::unique_ptr<T_Data_Individual_PDU> tpdu(new T_Data_Individual_PDU);
  tpdu->init(c, nullptr);
  std::string s = tpdu->decode (t);
  TRACEPRINTF (t, 4, "Recv Individual %s", s);
  tpdu->source_address = 0;
  tpdu->destination_address = destination_address;
  tpdu->address_type = IndividualAddress;
  tpdu->lsdu = tpdu->ToPacket ();
  auto r = recv.lock();
  if (r)
    r->recv_L_Data (std::move(tpdu));
}

T_Individual::~T_Individual ()
{
  TRACEPRINTF (t, 4, "CloseIndividual");
}

/***************** T_Connection *****************/

T_Connection::T_Connection (T_Reader<CArray> *app, LinkConnectClientPtr lc, eibaddr_t destination_address)
  : Layer4common (app, lc), destination_address(destination_address)
{
  t->setAuxName("TConn");
  TRACEPRINTF (t, 4, "OpenConnection %s", FormatEIBAddr (destination_address));
  timer.set <T_Connection, &T_Connection::timer_cb> (this);
}

T_Connection::~T_Connection ()
{
  TRACEPRINTF (t, 4, "CloseConnection");
  stop ();
  while (!buf.empty ())
    delete buf.get ();
}

void
T_Connection::send_L_Data (LDataPtr lpdu)
{
  TPDUPtr tpdu = TPDU::fromPacket (lpdu->address_type, lpdu->destination_address, lpdu->lsdu, t);
  switch (tpdu->getTType ())
    {
    case T_Data_Connected:
    {
      T_Data_Connected_PDU *tpdu1 = (T_Data_Connected_PDU *) &*tpdu;
      if (tpdu1->sequence_number != recvno && tpdu1->sequence_number != ((recvno - 1) & 0x0f))
        stop();
      else if (tpdu1->sequence_number == recvno)
        {
          tpdu1->lsdu[0] = tpdu1->lsdu[0] & 0x03;
          app->send(tpdu1->lsdu);
          SendAck (recvno);
          recvno = (recvno + 1) & 0x0f;
        }
      else if (tpdu1->sequence_number == ((recvno - 1) & 0x0f))
        SendAck (tpdu1->sequence_number);
    }
    break;
    case T_Connect:
      stop();
      break;
    case T_Disconnect:
      stop();
      break;
    case T_ACK:
    {
      T_ACK_PDU *tpdu1 = (T_ACK_PDU *) &*tpdu;
      if (tpdu1->sequence_number != sendno)
        stop();
      else if (mode != 2)
        stop();
      else
        {
          mode = 1;
          timer.stop();
          sendno = (sendno + 1) & 0x0f;
          SendCheck();
        }
    }
    break;
    case T_NAK:
    {
      T_NAK_PDU *tpdu1 = (T_NAK_PDU *) &*tpdu;
      if (tpdu1->sequence_number != sendno)
        stop();
      else if (repcount >= 3 || mode != 2)
        stop();
      else
        {
          repcount++;
          SendData (sendno, current);
        }
    }
    break;
    default:
      /* ignore */
      ;
    }
  send_Next();
}

void
T_Connection::recv_Data (const CArray & c)
{
  t->TracePacket (4, "Recv", c);
  in.push (c);
  SendCheck();
}

void
T_Connection::SendCheck ()
{
  if (mode != 1)
    return;
  repcount = 0;
  if (in.empty())
    return;
  current = in.get();
  SendData (sendno, current);
  mode = 2;
  timer.start(3,0);
}

void
T_Connection::SendConnect ()
{
  TRACEPRINTF (t, 4, "SendConnect");
  std::unique_ptr<T_Connect_PDU> tpdu(new T_Connect_PDU);
  tpdu->source_address = 0;
  tpdu->destination_address = destination_address;
  tpdu->address_type = IndividualAddress;
  tpdu->priority = PRIO_SYSTEM;
  tpdu->lsdu = tpdu->ToPacket ();

  mode = 1;
  sendno = 0;
  recvno = 0;
  repcount = 0;
  auto r = recv.lock();
  if (r)
    r->recv_L_Data (std::move(tpdu));
}

void
T_Connection::SendDisconnect ()
{
  TRACEPRINTF (t, 4, "SendDisconnect");
  std::unique_ptr<T_Disconnect_PDU> tpdu(new T_Disconnect_PDU);
  tpdu->source_address = 0;
  tpdu->destination_address = destination_address;
  tpdu->address_type = IndividualAddress;
  tpdu->priority = PRIO_SYSTEM;
  tpdu->lsdu = tpdu->ToPacket ();
  auto r = recv.lock();
  if (r)
    r->recv_L_Data (std::move(tpdu));
}

void
T_Connection::SendAck (int sequence_number)
{
  TRACEPRINTF (t, 4, "SendACK %d", sequence_number);
  std::unique_ptr<T_ACK_PDU> tpdu(new T_ACK_PDU);
  tpdu->sequence_number = sequence_number;
  tpdu->source_address = 0;
  tpdu->destination_address = destination_address;
  tpdu->address_type = IndividualAddress;
  tpdu->lsdu = tpdu->ToPacket ();
  auto r = recv.lock();
  if (r)
    r->recv_L_Data (std::move(tpdu));
}

void
T_Connection::SendData (int sequence_number, const CArray & c)
{
  std::unique_ptr<T_Data_Connected_PDU> tpdu(new T_Data_Connected_PDU);
  tpdu->init(c, nullptr);
  tpdu->sequence_number = sequence_number;
  TRACEPRINTF (t, 4, "SendData %s", tpdu->decode (t));
  tpdu->source_address = 0;
  tpdu->destination_address = destination_address;
  tpdu->address_type = IndividualAddress;
  tpdu->lsdu = tpdu->ToPacket ();
  auto r = recv.lock();
  if (r)
    r->recv_L_Data (std::move(tpdu));
}

/*
 * States:
 * 0   CLOSED
 * 1   IDLE
 * 2   ACK_WAIT
 *
*/

void T_Connection::timer_cb (ev::timer &, int)
{
  if (mode == 2 && repcount < 3)
    {
      repcount++;
      SendData (sendno, current);
      timer.start(3,0);
    }
  else
    stop();
}

void
T_Connection::stop()
{
  mode = 0;
  timer.stop();
  SendDisconnect ();

  CArray C;
  app->send(C);
}

/***************** T_TPDU *****************/

T_TPDU::T_TPDU (T_Reader<TpduComm> *app, LinkConnectClientPtr lc, eibaddr_t source_address)
  : Layer4common(app, lc), source_address(source_address)
{
  t->setAuxName("TPdu");
  TRACEPRINTF (t, 4, "OpenTPDU %s", FormatEIBAddr (source_address));
}

void
T_TPDU::send_L_Data (LDataPtr lpdu)
{
  TpduComm c;
  c.addr = lpdu->source_address;
  c.lsdu = lpdu->lsdu;
  app->send(c);
  send_Next();
}

void
T_TPDU::recv_Data (const TpduComm & c)
{
  t->TracePacket (4, "Recv TPDU", c.lsdu);
  LDataPtr lpdu = LDataPtr(new L_Data_PDU ());
  lpdu->source_address = source_address;
  lpdu->destination_address = c.addr;
  lpdu->address_type = IndividualAddress;
  lpdu->lsdu = c.lsdu;
  auto r = recv.lock();
  if (r)
    r->recv_L_Data (std::move(lpdu));
}

T_TPDU::~T_TPDU ()
{
  TRACEPRINTF (t, 4, "CloseTPDU");
}
