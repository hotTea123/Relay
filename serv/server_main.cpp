#include"relay_server.h"
using namespace std;

int main(int argc, const char *argv[]){
    Relay_Server relay_server;
    if(!relay_server.initServer("127.0.0.1", 8067))
        cout << "init server error" << endl;
    relay_server.runServer();
    return 0;
}