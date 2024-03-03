#include "echo.hpp"

int main() {
    Echo server(8501);
    server.Start();
    return 0;
}