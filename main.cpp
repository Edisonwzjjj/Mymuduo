#include "Buffer.hpp"
#include "Log.hpp"
#include "Socket.hpp"

using std::cout;
using std::endl;

int main() {
    char buf[1024] = "hello world";
    ERROR_LOG("%s", buf);

    return 0;
}
