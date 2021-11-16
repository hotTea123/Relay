#include"relay_client.h"
using namespace std;

int main(int argc,char **argv){
    Relay_Client relay_client(atoi(argv[1]), "127.0.0.1", 8067);
    relay_client.run_cli();
    return 0;
}