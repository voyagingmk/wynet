#include "wynet.h"
using namespace wynet;

WyNet net;

void Stop(int signo)   
{  
    printf("Stop.\n");  
    net.StopLoop();
}  

int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, Stop); 
  //  UDPServer server(9999);
  //  KCPObject kcpObject(9999, &server, &SocketOutput);
    printf("aeGetApiName: %s\n", aeGetApiName());
    Server * server = new Server(net.GetAeLoop(), 9998, 9999);
    net.AddServer(server);
    net.Loop();
    return 0;
}
