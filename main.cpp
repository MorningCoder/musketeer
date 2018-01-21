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

using namespace std;
using namespace std::placeholders;
using namespace musketeer;

int efd = 0;

struct Owner
{
    int listenfd;
    int connfd;

    unique_ptr<Channel> listenchan;
    unique_ptr<Channel> connchan;

    EventCycle* ec;

    Owner(EventCycle* e)
        : ec(e)
    {
        assert((listenfd = ::socket(AF_INET, SOCK_STREAM|SOCK_NONBLOCK, 0)));

        struct sockaddr_in S;
        S.sin_family = AF_INET;
        S.sin_port = htons(8000);
        S.sin_addr.s_addr = inet_addr("127.0.0.1");

        ::bind(listenfd, (sockaddr*)&S, sizeof(S));

        listen(listenfd, 5);

        listenchan.reset(new Channel(e, listenfd));

        listenchan->SetReadCallback(bind(&Owner::ProcessListenReadEvent, this));
        listenchan->EnableReading();
    }

    void ProcessListenReadEvent()
    {
        struct sockaddr_in addr;
        socklen_t len;

        connfd = ::accept4(listenfd, (sockaddr*)&addr, &len, SOCK_NONBLOCK);

        cout << "process accept ret=" << connfd << " errno=" << errno << endl;

        connchan.reset(new Channel(ec, connfd));

        connchan->SetReadCallback(bind(&Owner::ProcessConnReadEvent, this));
        connchan->EnableReading();
    }

    void ProcessConnReadEvent()
    {
        char buf[256];
        int ret = read(connfd, buf, 256);

        buf[255] = '\0';

        cout << "process read ret=" << ret << " errno=" << errno
            << " buf=" << buf << endl;
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
        int sockfd = 0;
        assert((sockfd = ::socket(AF_INET, SOCK_STREAM, 0)));

        struct sockaddr_in S;
        S.sin_family = AF_INET;
        S.sin_port = htons(8000);
        S.sin_addr.s_addr = inet_addr("127.0.0.1");

        socklen_t len = static_cast<socklen_t>(sizeof(S));

        int cret = connect(sockfd, (sockaddr*)&S, len);
        cout << "connect done ret=" << cret << " errno=" << errno << endl;
        
        write(sockfd, "Hello, hahahh, fuck you\n", sizeof("Hello, hahahh, fuck you\n"));

        sleep(100);
    }
    t.join();
}

