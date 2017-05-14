#include "packet_filter.h"

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <net/pfvar.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "database.h"

static int pfctl_ltoprefix(in_addr_t mask)
{
    int  i;
    for (i = 0; mask !=0; i++) {
        mask >>= 1;
    }
    return i;
}


int block_address(struct in_addr address) {
	char* pf_table_name = get_configuration_value("PF_TABLE_NAME");
	if(!pf_table_name) {
		fprintf(stderr, "PF_TABLE_NAME option is not defined\n");
	        return 1;
	}
	int dev = open("/dev/pf",O_RDWR);
	if( dev == -1) {
		fprintf(stderr,"Cannot open /dev/pf\n");
		free(pf_table_name);
		return 1;
	}
	struct pfr_table table;
        bzero(&table,sizeof(struct pfr_table));
	strncpy(table.pfrt_name,pf_table_name,strlen(pf_table_name));
	free(pf_table_name);
	
	struct pfr_addr addr;
        bzero(&addr,sizeof(struct pfr_addr));
	addr.pfra_ip4addr = address;
        addr.pfra_af = AF_INET;
        addr.pfra_net = pfctl_ltoprefix(0xffffff00);

	struct pfioc_table io;
	bzero(&io,sizeof(io));
	
	io.pfrio_table = table;
        io.pfrio_buffer = &addr;
        io.pfrio_esize = sizeof(struct pfr_addr);
        io.pfrio_size = 1;

        if(ioctl(dev,DIOCRADDADDRS,&io)) {
                perror("Cannot add address to table");
                close(dev);
                return -1;
        }
        /*printf("added %d addresses\n",io.pfrio_nadd);*/

        close(dev);
	return 0;
}

int unblock_address(struct in_addr address) {
        char* pf_table_name = get_configuration_value("PF_TABLE_NAME");
        if(!pf_table_name) {
                fprintf(stderr, "PF_TABLE_NAME option is not defined\n");
                return 1;
        }
        int dev = open("/dev/pf",O_RDWR);
        if( dev == -1) {
                fprintf(stderr,"Cannot open /dev/pf\n");
                free(pf_table_name);
                return 1;
        }
        struct pfr_table table;
        bzero(&table,sizeof(struct pfr_table));
        strncpy(table.pfrt_name,pf_table_name,strlen(pf_table_name));
        free(pf_table_name);
        
        struct pfr_addr addr;
        bzero(&addr,sizeof(struct pfr_addr));
        addr.pfra_ip4addr = address;
        addr.pfra_af = AF_INET;
        addr.pfra_net = pfctl_ltoprefix(0xffffff00);

        struct pfioc_table io;
        bzero(&io,sizeof(io));
        
        io.pfrio_table = table;
        io.pfrio_buffer = &addr;
        io.pfrio_esize = sizeof(struct pfr_addr);
        io.pfrio_size = 1;

        if(ioctl(dev,DIOCRDELADDRS,&io)) {
                perror("Cannot add address to table");
                close(dev);
                return -1;
        }
        /*printf("added %d addresses\n",io.pfrio_nadd);*/

        close(dev);
    
    return 0;
}
char is_address_blocked(struct in_addr address) {
    return 0;
}
