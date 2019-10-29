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

#include "link.h"

#include <cstdio>
#include <memory>

#include "router.h"

/* LinkBase */

LinkBase::LinkBase(BaseRouter&, IniSectionPtr& s, TracePtr tr) : cfg(s)
{
  t = TracePtr(new Trace(*tr, s));
  t->setAuxName("Base");
}

const std::string&
LinkBase::name()
{
  return cfg->name;
}

std::string
LinkBase::info(int)
{
  // TODO add more introspection
  std::string res = "cfg:";
  res += cfg->name;
  return res;
}

bool
LinkBase::setup()
{
#if 0
  // Use this code if you want to (ab)use valgrind for tracking which
  // code path called setup the first time.
  if (setup_called)
    {
      // make sure that this doesn't break anything
      char x = *setup_foo;
      *setup_foo = 1;
      *setup_foo = x;
    }
  else
    {
      setup_foo = (char *)malloc(1);
      free((void *)setup_foo);
    }
#endif
  assert (!setup_called);
  setup_called = true;
  return true;
}

void
LinkBase::start()
{
  assert(setup_called);
}

void
LinkBase::stop()
{
}

bool
LinkBase::_link(LinkRecvPtr)
{
  return false;
}

/* LinkRecv */

LinkRecv::LinkRecv(BaseRouter &r, IniSectionPtr& c, TracePtr tr) :
  LinkBase(r,c,tr)
{
  t->setAuxName("Recv");
}

void
LinkRecv::send_L_Data (LDataPtr l)
{
  send->send_L_Data(std::move(l));
}

FilterPtr
LinkRecv::findFilter(const std::string &, bool)
{
  return nullptr;
}

bool
LinkRecv::link(LinkBasePtr next)
{
  assert(next);
  if(!next->_link(std::dynamic_pointer_cast<LinkRecv>(shared_from_this())))
    return false;
  assert(send == next); // _link_ was called
  return true;
}

void
LinkRecv::_link_(LinkBasePtr next)
{
  send = next;
}

/* LinkConnect_ */

LinkConnect_::LinkConnect_(BaseRouter& r, IniSectionPtr& c, TracePtr tr)
  : router(r), LinkRecv(r,c,tr)
{
  t->setAuxName("Conn_");
  //Router& rt = dynamic_cast<Router&>(r);
}

bool
LinkConnect_::setup()
{
  if (!LinkRecv::setup())
    return false;
  DriverPtr dr = driver; // .lock();
  if(dr == nullptr)
    {
      ERRORPRINTF (t, E_ERROR | 61, "No driver in %s. Refusing.", cfg->name);
      return false;
    }

  std::string x = cfg->value("filters","");
  {
    size_t pos = 0;
    size_t comma = 0;
    while(true)
      {
        comma = x.find(',',pos);
        std::string name = x.substr(pos,comma-pos);
        if (name.size())
          {
            FilterPtr link;
            IniSectionPtr s = static_cast<Router&>(router).ini[name];
            name = s->value("filter",name);
            link = static_cast<Router&>(router).get_filter(std::dynamic_pointer_cast<LinkConnect_>(shared_from_this()),
                   s, name);
            if (link == nullptr)
              {
                ERRORPRINTF (t, E_ERROR | 32, "filter '%s' not found.", name);
                return false;
              }
            if(!dr->push_filter(link))
              {
                ERRORPRINTF (t, E_ERROR | 63, "Linking filter '%s' failed.", name);
                return false;
              }
          }
        if (comma == std::string::npos)
          break;
        pos = comma+1;
      }
  }

  LinkBasePtr s = send;
  while (s != nullptr)
    {
      if (!s->setup())
        {
          ERRORPRINTF (t, E_ERROR | 64, "%s: setup %s: failed", cfg->name, s->cfg->name);
          return false;
        }
      if (s == dr)
        break;
      auto ps = std::dynamic_pointer_cast<Filter>(s);
      if (ps == nullptr)
        {
          ERRORPRINTF (t, E_FATAL | 102, "%s: setup %s: no driver", cfg->name, s->cfg->name);
          return false;
        }
      s = ps->send;
    }
  if (s == nullptr)
    {
      ERRORPRINTF (t, E_FATAL | 103, "%s: setup: no driver", cfg->name);
      return false;
    }
  return true;
}

void
LinkConnect_::start()
{
  LinkRecv::start();
  send->start();
}

void
LinkConnect_::stop()
{
  send->stop();
  LinkRecv::stop();
}

void
LinkConnect_::unlink()
{
  /* You can't unlink the root of the chain … */
  assert(false);
}

bool
LinkConnect_::checkAddress (eibaddr_t addr)
{
  return send->checkAddress(addr);
}

bool
LinkConnect_::checkGroupAddress (eibaddr_t addr)
{
  return send->checkGroupAddress(addr);
}

bool
LinkConnect_::hasAddress (eibaddr_t addr)
{
  return send->hasAddress(addr);
}

void
LinkConnect_::addAddress (eibaddr_t addr)
{
  send->addAddress(addr);
}

bool
LinkConnect_::checkSysAddress(eibaddr_t addr)
{
  return static_cast<Router&>(router).checkAddress(addr, nullptr);
}

bool
LinkConnect_::checkSysGroupAddress(eibaddr_t addr)
{
  return static_cast<Router&>(router).checkGroupAddress(addr, nullptr);
}

void
LinkConnect_::set_driver(DriverPtr d)
{
  driver = d;
  link(std::dynamic_pointer_cast<LinkBase>(d));
}

/* LinkConnect */

LinkConnect::LinkConnect(BaseRouter& r, IniSectionPtr& c, TracePtr tr)
  : LinkConnect_(r,c,tr)
{
  t->setAuxName("Conn");
  //Router& rt = dynamic_cast<Router&>(r);
  retry_timer.set <LinkConnect,&LinkConnect::retry_timer_cb> (this);
}

LinkConnect::~LinkConnect()
{
  retry_timer.stop();

  if (addr && addr_local)
    static_cast<Router &>(router).release_client_addr(addr);
}

void
LinkConnect::send_L_Data (LDataPtr l)
{
  send_more = false;
  assert (state == L_up);
  retry_timer.start(send_timeout,0);
  TRACEPRINTF(t, 6, "sending, send_more clear");
  LinkConnect_::send_L_Data(std::move(l));
}

bool
LinkConnect::setup()
{
  if (!LinkConnect_::setup())
    return false;

  ignore = cfg->value("ignore",false);
  may_fail = cfg->value("may-fail",false);
  retry_delay = cfg->value("retry-delay",0);
  max_retries = cfg->value("max-retry",0);
  send_timeout = cfg->value("send-timeout", 10);
  return true;
}

void
LinkConnect::start()
{
  TRACEPRINTF(t, 5, "Starting");
  send_more = true;
  changed = time(NULL);
  LinkConnect_::start();
}

void
LinkConnect::stop()
{
  TRACEPRINTF(t, 5, "Stopping");
  changed = time(NULL);
  LinkConnect_::stop();
}

void
LinkConnect::setAddress(eibaddr_t addr)
{
  this->addr = addr;
  this->addr_local = false;
}

void
LinkConnect::started()
{
  setState(L_up);
  changed = time(NULL);
  TRACEPRINTF(t, 5, "Started");
}

void
LinkConnect::stopped()
{
  setState(L_down);
}

void
LinkConnect::errored()
{
  setState(L_error);
}

void
LinkConnect::recv_L_Data (LDataPtr l)
{
  static_cast<Router&>(router).recv_L_Data(std::move(l), *this);
}

void
LinkConnect::recv_L_Busmonitor (LBusmonPtr l)
{
  static_cast<Router&>(router).recv_L_Busmonitor(std::move(l));
}

bool
LinkConnect::checkSysAddress(eibaddr_t addr)
{
  return static_cast<Router&>(router).checkAddress(addr, std::dynamic_pointer_cast<LinkConnect>(shared_from_this()));
}

bool
LinkConnect::checkSysGroupAddress(eibaddr_t addr)
{
  return static_cast<Router&>(router).checkGroupAddress(addr, std::dynamic_pointer_cast<LinkConnect>(shared_from_this()));
}

void
LinkConnect::send_Next()
{
  send_more = true;
  if (state == L_up)
    retry_timer.stop();
  TRACEPRINTF(t, 6, "sendNext called, send_more set");
  static_cast<Router&>(router).send_Next();
}

void
LinkConnect::setState(LConnState new_state)
{
  if (state == new_state)
    return;

  LConnState old_state = state;
  const char *osn = stateName();
  state = new_state;
  TRACEPRINTF(t, 5, "%s => %s", osn, stateName());

  if (old_state == L_wait_retry || (old_state == L_up && new_state != L_up))
    retry_timer.stop();

  switch(old_state)
    {
    case L_down:
      switch(new_state)
        {
        case L_going_up:
          start();
          break;
        case L_going_down: // redundant call to stop()
          state = L_down;
          break;
        default:
          goto inval;
        }
      break;
    case L_going_down:
      switch(new_state)
        {
        case L_error:
          state = L_going_down_error;
          break;
        case L_down:
          break;
        default:
          goto inval;
        }
      break;
    case L_up:
      switch(new_state)
        {
        case L_going_up: // redundant call to start()
          state = L_up;
          break;
        case L_going_down:
        case L_down:
          stop();
          break;
        case L_error:
          state = L_up_error;
          break;
        default:
          goto inval;
        }
      break;
    case L_going_up:
      switch(new_state)
        {
        case L_up:
          retries = 0;
          break;
        case L_going_down:
          stop();
          break;
        case L_down:
          goto retry;
        case L_error:
          state = L_up_error;
          break;
        default:
          goto inval;
        }
      break;
    case L_wait_retry:
      switch(new_state)
        {
        case L_going_up:
          start();
          break;
        case L_going_down:
          TRACEPRINTF(t, 5, "retrying halted");
          state = L_down;
          break;
        default:
          goto inval;
        }
      break;
    case L_error:
      switch(new_state)
        {
        case L_wait_retry:
          goto retry;
        case L_error:
          break;
        case L_going_down:
          state = L_error;
          break;
        case L_down:
          goto retry;
        default:
          goto inval;
        }
      break;
    case L_up_error:
      switch(new_state)
        {
        case L_going_down:
          state = L_going_down_error;
          stop();
          break;
        case L_down:
          goto retry;
          break;
        case L_error:
          break;
        default:
          goto inval;
        }
      break;
    case L_going_down_error:
      switch(new_state)
        {
        case L_down:
          goto retry;
        case L_error:
          break;
        case L_going_down:
          state = L_going_down_error;
          break;
        default:
          goto inval;
        }
      break;
    }
  static_cast<Router&>(router).linkStateChanged(std::dynamic_pointer_cast<LinkConnect>(shared_from_this()));
  return;

inval:
  ERRORPRINTF (t, E_ERROR | 60, "invalid transition: %s => %s", osn, stateName());
  abort();
  return;

retry:
  if ((retry_delay > 0 && !max_retries) || retries < max_retries)
    {
      TRACEPRINTF (t, 5, "retry in %d sec", retry_delay);
      state = L_wait_retry;
      retry_timer.start (retry_delay,0);
    }
  else
    {
      if (retry_delay > 0)
        TRACEPRINTF (t, 5, "retrying finished");
      state = L_error;
      static_cast<Router&>(router).linkStateChanged(std::dynamic_pointer_cast<LinkConnect>(shared_from_this()));
    }
}

const char *
LinkConnect::stateName()
{
  switch(state)
    {
    case L_down:
      return "down";
    case L_going_down:
      return ">down";
    case L_up:
      return "up";
    case L_going_up:
      return ">up";
    case L_error:
      return "down/error";
    case L_up_error:
      return "up/error";
    case L_going_down_error:
      return ">down/error";
    case L_wait_retry:
      return "error/retry";
    default:
      abort();
      return "?!?";
    }
}

void
LinkConnect::retry_timer_cb (ev::timer &, int)
{
  if (state == L_up && !send_more)
    {
      ERRORPRINTF (t, E_ERROR | 55, "Driver timed out trying to send (%s)", cfg->name);
      errored();
      return;
    }
  if (state != L_wait_retry)
    return;
  retries++;
  setState(L_going_up);
}

/* LinkConnectClient */

LinkConnectClient::LinkConnectClient(ServerPtr s, IniSectionPtr& c, TracePtr tr)
  : server(s), LinkConnect(s->router, c, tr)
{
  t->setAuxName("ConnC");
  char n[10];
  sprintf(n,"%d",t->seq);
  linkname = t->name + '_' + n;
}

const std::string&
LinkConnectClient::name()
{
  return linkname;
}

/* LinkConnectSingle */

LinkConnectSingle::LinkConnectSingle(ServerPtr s, IniSectionPtr& c, TracePtr tr) :
  LinkConnectClient(s,c,tr)
{
  t->setAuxName("ConnS");
}

bool
LinkConnectSingle::setup()
{
  if (!LinkConnectClient::setup())
    return false;
  if (addr == 0)
    addr = static_cast<Router &>(router).get_client_addr(t);
  if (addr == 0)
    return false;
  return true;
}

bool
LinkConnectSingle::hasAddress (eibaddr_t addr)
{
  return addr == this->addr;
}

void
LinkConnectSingle::addAddress (eibaddr_t addr)
{
  assert (addr == 0 || addr == this->addr);
}

/* Server */

Server::Server(BaseRouter& r, IniSectionPtr& c) :
  LinkConnect(r,c,r.t)
{
  t->setAuxName("Server");
}

bool
Server::setup()
{
  /* Server::setup() does NOT call LinkConnect::setup() because there is no driver here. */
  return true;
}

void
Server::start()
{
  started();
}

void
Server::stop()
{
  stopped();
}

void
Server::send_L_Data (LDataPtr)
{
  /* Servers don't accept data */
}

bool
Server::hasAddress (eibaddr_t)
{
  return false;
}

void
Server::addAddress (eibaddr_t addr)
{
  ERRORPRINTF(t,E_ERROR | 65,"Tried to add address %s to %s", FormatEIBAddr(addr), cfg->name);
}

bool
Server::checkAddress (eibaddr_t)
{
  return false;
}

bool
Server::checkGroupAddress (eibaddr_t)
{
  return false;
}

/* Filter */

Filter::Filter(const LinkConnectPtr_& c, IniSectionPtr& s)
  : LinkRecv(c->router, s, c->t)
{
  conn = c;
  t->setAuxName(c->t->name);
}

void
Filter::recv_L_Data (LDataPtr l)
{
  auto r = recv.lock();
  if (r != nullptr)
    r->recv_L_Data(std::move(l));
}

void
Filter::recv_L_Busmonitor (LBusmonPtr l)
{
  auto r = recv.lock();
  if (r != nullptr)
    r->recv_L_Busmonitor(std::move(l));
}

bool
Filter::checkSysAddress(eibaddr_t addr)
{
  auto r = recv.lock();
  if (r == nullptr)
    return false;
  return r->checkSysAddress(addr);
}

bool
Filter::checkSysGroupAddress(eibaddr_t addr)
{
  auto r = recv.lock();
  if (r == nullptr)
    return false;
  return r->checkSysGroupAddress(addr);
}

void
Filter::send_Next()
{
  auto r = recv.lock();
  if (r != nullptr)
    r->send_Next();
}

void
Filter::started()
{
  auto r = recv.lock();
  if (r != nullptr)
    r->started();
}

void
Filter::stopped()
{
  auto r = recv.lock();
  if (r != nullptr)
    r->stopped();
}

void
Filter::errored()
{
  auto r = recv.lock();
  if (r != nullptr)
    r->errored();
}

const std::string&
Filter::name()
{
  return cfg->value("filter",cfg->name);
}

bool
Filter::_link(LinkRecvPtr prev)
{
  if (prev == nullptr)
    return false;
  prev->_link_(shared_from_this());
  recv = prev;
  return true;
}

void
Filter::unlink()
{
  /* Remove this filter from the link chain */
  auto r = recv.lock();
  if (r != nullptr)
    {
      if (send != nullptr)
        send->_link(r);
      r->_link_(send);
    }
  send.reset();
  recv.reset();
}

void
Filter::start()
{
  if (send == nullptr) stopped();
  else send->start();
}

void
Filter::stop()
{
  if (send == nullptr) stopped();
  else send->stop();
}

bool
Filter::hasAddress (eibaddr_t addr)
{
  return send->hasAddress(addr);
}

void
Filter::addAddress (eibaddr_t addr)
{
  return send->addAddress(addr);
}

bool
Filter::checkAddress (eibaddr_t addr)
{
  return send->checkAddress(addr);
}

bool
Filter::checkGroupAddress (eibaddr_t addr)
{
  return send->checkGroupAddress(addr);
}

FilterPtr
Filter::findFilter(const std::string & name, bool skip_me)
{
  auto r = recv.lock();
  if (r == nullptr)
    return nullptr;
  if (!skip_me && this->name() == name)
    return std::static_pointer_cast<Filter>(shared_from_this());
  return r->findFilter(name, false);
}

/* Driver */

Driver::Driver(const LinkConnectPtr_& c, IniSectionPtr& s) :
  LinkBase(c->router, s, c->t)
{
  conn = c;
  t->setAuxName("Driver");
}

void
Driver::start()
{
  started();
}

void
Driver::stop()
{
  stopped();
}

bool
Driver::_link(LinkRecvPtr prev)
{
  if (prev == nullptr)
    return false;
  prev->_link_(shared_from_this());
  recv = prev;
  return true;
}

void
Driver::started()
{
  auto r = recv.lock();
  if (r != nullptr)
    r->started();
}

void
Driver::stopped()
{
  auto r = recv.lock();
  if (r != nullptr)
    r->stopped();
}

void
Driver::errored()
{
  auto r = recv.lock();
  if (r != nullptr)
    r->errored();
}

const std::string&
Driver::name()
{
  return cfg->value("driver",cfg->name);
}

void
Driver::recv_L_Data (LDataPtr l)
{
  auto r = recv.lock();
  if (r != nullptr)
    r->recv_L_Data(std::move(l));
}

void
Driver::recv_L_Busmonitor (LBusmonPtr l)
{
  auto r = recv.lock();
  if (r != nullptr)
    r->recv_L_Busmonitor(std::move(l));
}

bool
Driver::checkSysAddress(eibaddr_t addr)
{
  auto r = recv.lock();
  if (r == nullptr)
    return false;
  return r->checkSysAddress(addr);
}

bool
Driver::checkSysGroupAddress(eibaddr_t addr)
{
  auto r = recv.lock();
  if (r == nullptr)
    return false;
  return r->checkSysGroupAddress(addr);
}

void
Driver::send_Next()
{
  auto r = recv.lock();
  if (r != nullptr)
    r->send_Next();
}

bool
Driver::link(LinkBasePtr)
{
  return false;
}

void
Driver::_link_(LinkBasePtr)
{
}

bool
Driver::push_filter(FilterPtr filter, bool first)
{
  LinkRecvPtr r;
  LinkBasePtr t;

  // r->t ==> r->filter->t

  if (first)
    {
      // r is the LinkConnect base, t is the following LinkBase-ish thing
      LinkConnectPtr_ c = conn.lock();
      if (c == nullptr)
        return false;
      r = c;
      t = c->send;
    }
  else
    {
      // t is this driver, so r is this->recv
      r = recv.lock();
      if (r == nullptr)
        return false;
      t = shared_from_this();
    }

  // link the first part
  if (!r->link(filter))
    return false;
  // link the second part
  if (!filter->link(t))
    {
      // didn't work, so undo the first.
      r->link(t);
      return false;
    }

#if 0 // this is done by LinkConnect::setup() once the stack is complete
  if (!filter->setup())
    {
      filter->unlink();
      return false;
    }
#endif
  return true;
}

FilterPtr
Driver::findFilter(const std::string & name, bool)
{
  auto r = recv.lock();
  if (r == nullptr)
    return nullptr;
  return r->findFilter(name);
}

bool
Driver::assureFilter(const std::string & name, bool first)
{
  if (findFilter(name) != nullptr)
    return true;

  auto c = conn.lock();
  if (c == nullptr)
    return false;

  std::string sn = this->name() + '.' + name;
  IniSectionPtr s = static_cast<Router&>(c->router).ini.add_auto(sn);
  if (s == nullptr)
    return false;
  auto f = static_cast<Router&>(c->router).get_filter(c, s, name);
  if (f == nullptr)
    return false;
  if (!push_filter(f, first))
    return false;

  // push_filter doesn't call setup()
  if (!f->setup())
    return false;
  return true;
}

/* BusDriver */

BusDriver::BusDriver(const LinkConnectPtr_& c, IniSectionPtr& s) :
  Driver(c,s)
{
  addrs.resize(65536);
  t->setAuxName("BusDriver");
}

bool
BusDriver::hasAddress(eibaddr_t addr)
{
  return addrs[addr];
}

void
BusDriver::addAddress(eibaddr_t addr)
{
  addrs[addr] = true;
}

bool
BusDriver::checkAddress (eibaddr_t)
{
  return true;
}

bool
BusDriver::checkGroupAddress (eibaddr_t)
{
  return true;
}

/* SubDriver */

SubDriver::SubDriver(const LinkConnectClientPtr& c)
  : BusDriver(static_cast<const LinkConnectPtr&>(c), c->cfg)
{
  t->setAuxName("SubDr");
  server = c->server;
}

/* LineDriver */

LineDriver::LineDriver(const LinkConnectClientPtr& c)
  : Driver(c, c->cfg)
{
  t->setAuxName("LineDr");
  server = c->server;
}

bool
LineDriver::setup()
{
  if(!Driver::setup())
    return false;

  auto c = std::dynamic_pointer_cast<LinkConnect>(conn.lock());
  if (c == nullptr)
    return false;

  _addr = c->addr;
  return true;
}

eibaddr_t
LineDriver::getAddress()
{
  return _addr;
}

bool
LineDriver::hasAddress(eibaddr_t addr)
{
  return addr == this->_addr;
}

void
LineDriver::addAddress(eibaddr_t addr)
{
  if (addr != this->_addr)
    ERRORPRINTF(t,E_WARNING | 120,"%s: Addr mismatch: %s vs. %s", this->name(), FormatEIBAddr (addr), FormatEIBAddr (this->_addr));
}

bool
LineDriver::checkAddress(eibaddr_t addr)
{
  return addr == this->_addr;
}

bool
LineDriver::checkGroupAddress (eibaddr_t)
{
  return true;
}
