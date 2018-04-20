#include "net/Connector.h"
#include <thread>
#include <cstring>
#include "base/Buffer.h"
#include <functional>
#include <arpa/inet.h>
#include <memory>
#include <iostream>
#include <map>
#include <utility>
#include <cassert>
#include "event/EventCycle.h"
#include <base/Utilities.h>
#include <event/Channel.h>
#include <event/Poller.h>
#include "net/TcpConnection.h"
#include "unistd.h"
#include <sys/eventfd.h>
#include <cstdlib>
#include "base/Manager.h"
#include <sys/socket.h>
#include "event/Poller.h"
#include "net/Socket.h"
#include "net/InetAddr.h"
#include "net/Listener.h"
#include "net/NetWorker.h"
#include "base/CycleThread.h"

using namespace std;
using namespace std::placeholders;
using namespace musketeer;

int main()
{
    if(!gManager.CheckAndSet())
    {
        return 1;
    }

    gManager.InitThreads();

    sleep(10000);

    /*
    Buffer buf(50);
    assert(buf.AvailableSize() == 0);
    assert(buf.AppendableSize() == 50);

    string s("init data\r\nhahaha:hvalue\r\n");

    buf.Append(s);
    assert(buf.AvailableSize() == s.size());

    RawBuf apbuf = buf.AppendablePos();
    RawBuf avbuf = buf.AvailablePos();

    assert(apbuf.second = 50 - s.size());
    assert(avbuf.second == s.size());

    buf.MarkProcessed(make_pair(avbuf.first, 11));

    memcpy(apbuf.first, "newval", 6);
    buf.MarkAppended(6);

    avbuf = buf.AvailablePos();
    apbuf = buf.AppendablePos();

    assert(strncmp(avbuf.first, "hahaha:hvalue\r\nnewval", sizeof("hahaha:hvalue\r\nnewval") - 1) == 0);
    assert(buf.AppendableSize() == 50 - sizeof("init data\r\nhahaha:hvalue\r\nnewval") + 1);

    string cont = std::move(buf.Retrive(buf.AvailableSize()));
    assert(cont == string("hahaha:hvalue\r\nnewval"));
    assert(buf.AvailableSize() == 0);
    assert(buf.AppendableSize() == 50);
    */

    return 0;
}
