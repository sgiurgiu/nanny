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
#include <stdbool.h>

#include "database.h"

static int pfctl_ltoprefix(in_addr_t mask)
{
    int  i;
    for (i = 0; mask !=0; i++) {
        mask >>= 1;
    }
    return i;
}


void kill_states(int dev,struct in_addr address) {
    struct pfioc_state_kill psk;
    struct pf_addr source_addr;
    memset(&psk, 0, sizeof(psk));
    memset(&source_addr, 0, sizeof(source_addr));

    source_addr.v4 = address;
    psk.psk_af = AF_INET;
    
    memcpy(&psk.psk_src.addr.v.a.addr, &source_addr,
        sizeof(psk.psk_src.addr.v.a.addr));
    memset(&psk.psk_src.addr.v.a.mask, 0xff,sizeof(psk.psk_src.addr.v.a.mask));
    
    if (ioctl(dev, DIOCKILLSTATES, &psk)) {
        perror("Cannot kill states");
    }
}

static int update_table(struct in_addr address,unsigned long request, bool kill_connection)
{
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
    pf_table_name = NULL;
	
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

    if(ioctl(dev,request,&io)) {
            perror("Cannot update the table");
            close(dev);
            return -1;
    }
    /*printf("added %d addresses\n",io.pfrio_nadd);*/
    if(kill_connection) {
        kill_states(dev,address);
    }

    close(dev);    
    return 0;
}

static bool is_add_to_block()
{
	char* add_to_block= get_configuration_value("PF_ADD_BLOCK");
    if(!add_to_block) {
		fprintf(stderr, "PF_ADD_BLOCK option is not defined\n");
        return true;
	}
	bool add = strncmp("true",add_to_block,4) == 0 || strncmp("T",add_to_block,1)==0 || strncmp("1",add_to_block,1)==0;
    free(add_to_block);
    return add;
}

int block_address(struct in_addr address) {
	bool add = is_add_to_block();
    int result = update_table(address,add ? DIOCRADDADDRS : DIOCRDELADDRS,true);
	
	return result;
}

int unblock_address(struct in_addr address) {
    
    bool add = is_add_to_block();
    int result = update_table(address,add ? DIOCRDELADDRS: DIOCRADDADDRS,false);
	
	return result;    
}
char is_address_blocked(struct in_addr address) {
    return 0;
}
