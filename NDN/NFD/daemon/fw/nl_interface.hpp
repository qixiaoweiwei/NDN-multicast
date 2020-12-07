#ifndef __NL_INTERFACE_H
#define __NL_INTERFACE_H
#include <linux/nl80211.h>
#include <net/if.h>
#include <errno.h>
#include <linux/nl80211.h>
#include <net/if.h>
#include <stdio.h>
#include <errno.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <string>



#define ETH_ALEN 6

struct nl80211_state {
	struct nl_sock *nl_sock;
	int nl80211_id;
};

struct rate{
    double tx_rate;
    double rx_rate;
};

void mac_addr_n2a(char *mac_addr, const unsigned char *arg);
int mac_addr_a2n(unsigned char *mac_addr, char *arg);
void parse_bitrate(struct nlattr *bitrate_attr, char *buf, int buflen);
int valid_handler(struct nl_msg *msg, void *arg);
int set_rate(const std::string itrfc, float rate);
float get_rate(const std::string itrfc, const std::string adrs);



#endif
