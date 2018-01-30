#include <thread>
#include <cstring>
#include "base/Buffer.h"
#include <functional>
#include <arpa/inet.h>
#include <memory>
#include <iostream>
#include <cassert>
#include "event/EventCycle.h"
#include <base/Utilities.h>
#include <event/Channel.h>
#include <event/Poller.h>
#include "net/TcpConnection.h"
#include "unistd.h"
#include <sys/eventfd.h>
#include <cstdlib>
#include <sys/socket.h>
#include "net/Socket.h"
#include "net/InetAddr.h"
#include "net/Listener.h"

using namespace std;
using namespace std::placeholders;
using namespace musketeer;

int efd = 0;

struct Owner
{
    Listener listener;

    EventCycle* ec;

    Owner(EventCycle* e)
        : listener(bind(&Owner::ProcessConnected, this, _1),
                    InetAddr("127.0.0.1", 8000), e, 1000),
          ec(e)
    {
        listener.Listen();
    }

    void ProcessConnected(TcpConnectionPtr conn)
    {
        cout << "new conn " << conn->Status() << endl;
        conn->Close();
    }

    /*void ProcessListenReadEvent()
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
    }*/
};

void threadFunc()
{
    EventCycle* cycle = new EventCycle(Poller::MEpoll);

    Owner o(cycle);

    cycle->Loop();
}

int main()
{
    thread t(threadFunc);

    sleep(1);

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
        int ret = ::write(sock.Getfd(), "Hello,hahahh,testtestxxxxx666",
                        sizeof("Hello,hahahh,testtestxxxxx666") - 1);
        cout << "write ret=" << ret << endl;
        sleep(1);
    }

    t.join();

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
