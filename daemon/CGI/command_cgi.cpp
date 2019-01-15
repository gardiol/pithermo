
#include <iostream>

#include "frameworkutils.h"
#include "frameworktimer.h"
#include "udpsocket.h"

using namespace FrameworkLibrary;

int main( int , char** )
{
    int ret = 255;
    printf("Content-type: text/plain\n\n");

    std::string input;
    std::getline( std::cin, input );

    if ( input.length() > 0 )
    {
        UdpSocket sender("CommandSend","127.0.0.1","",5555,0);
        if ( sender.activateInterface() )
            if ( sender.writeData( input.c_str(), static_cast<int>(input.length()) ) == static_cast<int>(input.length()) )
                ret = 0;
    }
    return ret;
}
