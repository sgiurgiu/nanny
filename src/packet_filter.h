#ifndef PACKET_FILTER_H
#define PACKET_FILTER_H
    
#include <arpa/inet.h>

int block_address(struct in_addr address);
int unblock_address(struct in_addr address);
char is_address_blocked(struct in_addr address);//returns TRUE or FALSE

#endif
