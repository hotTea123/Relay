#include"relay_client.h"
using namespace std;

int main(int argc,char **argv){
    if(argc != 3){
        cout << "input error! please input: session_number data_len" << endl;
        return 0;
    }
    Relay_Client relay_client("127.0.0.1", 8067, atoi(argv[1]), atoi(argv[2]));
    relay_client.run_cli();
    return 0;
}