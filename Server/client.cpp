#include "../Socket.hpp"

int main()
{
    Socket sock;
    sock.CreateClient(8501, "127.0.0.1");
    for (int i = 0; i < 5; ++i) {
        std::string str = "wzjjjjjjj";
        sock.Send(str.c_str(), str.size());
        char buf[1024]{};
        sock.Recv(buf, 1023);
        DBG_LOG("%s", buf);
        sleep(1);
    }
    return 0;
}