#include "TimeWheel.h"
#include <any>
#include <regex>

using std::cout;
using std::endl;


int main() {
    std::string s = "GET /bite/login?user=wzj&pass=123123 HTTP/1.1";
    std::regex e("(GET|HEAD|POST|PUT|DELETE) ([^?]*)\\?(.*) (HTTP/1\\.[01])(?:\n|\r\n)?");
    std::smatch sm;
    bool ret = std::regex_match(s, sm, e);
    if (ret) {
        for (auto &s: sm) {
            cout << s << endl;
        }
    } else return -1;
    return 0;
}
