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

int efd = 0;
/*
typedef pair<TcpConnectionPtr, Buffer> ConnBuf;

struct Owner
{
    Listener listener;
    Buffer readBuf;
    map<int, ConnBuf*> connmap;
    EventCycle* ec;

    Owner(EventCycle* e)
        : listener(bind(&Owner::ProcessConnected, this, _1),
                    InetAddr("127.0.0.1", 8000), e, 1000),
          readBuf(4096),
          connmap(),
          ec(e)
    {
        listener.Listen();
    }

    void ProcessWriteFinished(TcpConnectionPtr conn)
    {
        delete connmap[conn->Index()];
        connmap.erase(conn->Index());
        conn->Close();
    }

    void ProcessRead(TcpConnectionPtr conn, Buffer* buf)
    {
        if(conn->Status() != TcpConnectionStatus::Established)
        {
            assert(buf->AvailableSize() == 0);
            delete connmap[conn->Index()];
            connmap.erase(conn->Index());
            conn->Close();
            return;
        }

        //cout << "thread: " << this_thread::get_id() << " new request size "
        //    << buf->AvailableSize() << endl;
        Buffer* wbuf = conn->GetWriteableBuffer();
        wbuf->Append("HTTP/1.1 200 OK\r\nServer: Haha\r\nContent-Length: 3\r\n\r\nxxx");
        conn->Send(bind(&Owner::ProcessWriteFinished, this, _1));
    }

    void ProcessConnected(TcpConnectionPtr conn)
    {
        if(conn->Status() != TcpConnectionStatus::Established)
        {
            conn->Close();
            return;
        }
        cout << "thread: " << this_thread::get_id() << " new conn "
            << conn->LocalAddr().ToString() << " " << conn->RemoteAddr().ToString() << endl;
        ConnBuf* conbuf = new ConnBuf(conn, 4096);
        conn->SetReadCallback(bind(&Owner::ProcessRead, this, _1, _2), &conbuf->second);
        connmap[conn->Index()] = conbuf;
    }

    void ProcessListenReadEvent()
    {
        bool overload = false;
        InetAddr peeraddr;
        Socket connfd = listenfd.Accept(peeraddr, overload);
        cout << "accept fd = " << connfd.Getfd() << endl;

        if(overload)
        {
            // ...
        }

        cout << "process accept connfd=" << connfd.Getfd() << endl;

        tcpConn = make_shared<TcpConnection>(std::move(connfd), ec, false, nullptr);
        tcpConn->SetReadCallback(bind(&Owner::ProcessConnReadEvent, this, _1));
    }

    void ProcessConnReadEvent(TcpConnectionPtr ptr)
    {
        Buffer* buf = ptr->GetReadableBuffer();
        string s = buf->Retrive(10);
        cout << "Retrive [ " << s << " ]" << endl;
        Buffer* b = ptr->GetWriteableBuffer();
        b->Append(string("response hello"));
        ptr->Send(bind(&Owner::ProcessConnWriteEvent, this, _1));
    }

    void ProcessConnWriteEvent(TcpConnectionPtr ptr)
    {
        musketeer::IOError ior = ptr->CheckIOError();
        cout << "ProcessConnWriteEvent ioError=" << ior.first << " errno=" << ior.second << endl;
    }
};

void threadFunc()
{
    EventCycle* cycle = new EventCycle(Poller::MEpoll);

    Owner o(cycle);

    cycle->Loop();
}*/

void NewConnCB(TcpConnectionPtr conn)
{
    LOG_DEBUG(" hahahah ");
}

int main()
{
    if(!gManager.CheckAndSet())
    {
        return 1;
    }

    gManager.InitThreads();

    /*sleep(1);

    while(1)
    {
    Socket sock = Socket::New(Socket::MIp4);

    //cout << "connect fd = " << sock.Getfd() << endl;

    struct sockaddr_in S;
    S.sin_family = AF_INET;
    S.sin_port = htons(8000);
    S.sin_addr.s_addr = inet_addr("127.0.0.1");

    socklen_t len = static_cast<socklen_t>(sizeof(S));

    connect(sock.Getfd(), (sockaddr*)&S, len);
        //int ret = ::write(sock.Getfd(), "Hello,hahahh,testtestxxxxx666",
        //                sizeof("Hello,hahahh,testtestxxxxx666") - 1);

        cout << "closed connection" << endl;
        sock.Close();
        sleep(1);
    }*/

    sleep(1000);

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

    return 0;
}
