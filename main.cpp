#include <thread>
#include <functional>
#include <arpa/inet.h>
#include <memory>
#include <iostream>
#include <cassert>
#include "event/EventCycle.h"
#include <event/Channel.h>
#include <event/Poller.h>
#include "unistd.h"
#include <sys/eventfd.h>
#include <cstdlib>
#include <sys/socket.h>
#include "net/Socket.h"
#include "net/InetAddr.h"

using namespace std;
using namespace std::placeholders;
using namespace musketeer;

int efd = 0;

struct Owner
{
    Socket listenfd;
    Socket connfd;

    unique_ptr<Channel> listenchan;
    unique_ptr<Channel> connchan;

    EventCycle* ec;

    Owner(EventCycle* e)
        : listenfd(Socket::New(Socket::MIp4)),
          connfd(),
          ec(e)
    {
        InetAddr localAddr("127.0.0.1", 8000);

        listenfd.BindAddr(localAddr);
        listenfd.Listen();

        cout << "listen fd = " << listenfd.Getfd() << endl;

        listenchan.reset(new Channel(e, listenfd.Getfd()));

        listenchan->SetReadCallback(bind(&Owner::ProcessListenReadEvent, this));
        listenchan->EnableReading();
    }

    void ProcessListenReadEvent()
    {
        bool overload = false;
        InetAddr peeraddr;
        connfd = listenfd.Accept(peeraddr, overload);
        cout << "accept fd = " << connfd.Getfd() << endl;

        if(overload)
        {
            // ...
        }

        cout << "process accept connfd=" << connfd.Getfd() << endl;

        connchan.reset(new Channel(ec, connfd.Getfd()));

        connchan->SetReadCallback(bind(&Owner::ProcessConnReadEvent, this));
        connchan->EnableReading();
    }

    void ProcessConnReadEvent()
    {
        char buf[256];
        read(connfd.Getfd(), buf, 256);

        ::write(connfd.Getfd(), "hello\n", sizeof("hello"));

        connchan->Close();
        connfd.Close();
        // next cycle will set this a new one
    }
};

void readCallback()
{
    uint64_t num = 0;
    int r = random() % 10;
    cout << "readCallback: r=" << r << endl;

    if(r >= 7)
    {
        int ret = read(efd, &num, sizeof(num));
        cout << "read: ret=" << ret << " num=" << num << " r=" << r << endl;
    }
}

void threadFunc()
{
    EventCycle* cycle = new EventCycle(Poller::MEpoll);

    Owner o(cycle);

    cycle->Loop();
}


int main()
{
    thread t(threadFunc);

    sleep(2);

    while(1)
    {
        Socket sock = Socket::New(Socket::MIp4);

        cout << "connect fd = " << sock.Getfd() << endl;

        struct sockaddr_in S;
        S.sin_family = AF_INET;
        S.sin_port = htons(8000);
        S.sin_addr.s_addr = inet_addr("127.0.0.1");

        socklen_t len = static_cast<socklen_t>(sizeof(S));

        connect(sock.Getfd(), (sockaddr*)&S, len);

        ::write(sock.Getfd(), "Hello, hahahh, fuck you\n", sizeof("Hello, hahahh, fuck you\n"));

        sleep(1);
    }
    t.join();
}
