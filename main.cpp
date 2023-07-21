#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <string.h>

#include "nlquery.h"

void outputHelp()
{
    std::cout<<"-i : enter your ethernet port name"<<std::endl;
    std::cout<<"-h : print help"<<std::endl;
}

int main(int argc, char **argv)
{
    int opt;
    netlink nl;
    int link_status = -1;
    char dev[16];
    bool dev_f = false;

    strncpy(dev, "eth0", 16);

    while((opt = getopt(argc, argv, "i:h")) != -1)
    {
        switch(opt)
        {
            case 'i':
                strncpy(dev, optarg, 16);
                dev_f = true;
                break;
            case 'h':
            default:
                outputHelp();
                return 0;
        }
    }

    if(!dev_f)
        return 0;

    nl.query_ext(dev);
    do
    {
        link_status = nl.status(dev);
        nl.printstate();
        // sleep(2);
    } while (link_status != -1);

    return 0;
}