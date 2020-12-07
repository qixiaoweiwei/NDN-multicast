#include "nl_interface.hpp"
#include<iostream>
static int nl80211_init(struct nl80211_state *state);
static void nl80211_cleanup(struct nl80211_state *state);
static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err, void *arg);
static int finish_handler(struct nl_msg *msg, void *arg);
static int ack_handler(struct nl_msg *msg, void *arg);
static int rate_handler(struct nl_msg *msg, void *arg);
static int get_sta_rate(struct nl80211_state *state, char *itrfc, char *adrs, struct rate* rt);
static int set_mcast_rate(struct nl80211_state *state, char *itrfc, float rate);


static int nl80211_init(struct nl80211_state *state)
{
	int err;

	state->nl_sock = nl_socket_alloc();
	if (!state->nl_sock) {
		fprintf(stderr, "Failed to allocate netlink socket.\n");
		return -ENOMEM;
	}

	if (genl_connect(state->nl_sock)) {
		fprintf(stderr, "Failed to connect to generic netlink.\n");
		err = -ENOLINK;
		goto out_handle_destroy;
	}

	nl_socket_set_buffer_size(state->nl_sock, 8192, 8192);

	err = 1;
	setsockopt(nl_socket_get_fd(state->nl_sock), SOL_NETLINK,
		   NETLINK_EXT_ACK, &err, sizeof(err));

	state->nl80211_id = genl_ctrl_resolve(state->nl_sock, "nl80211");
	if (state->nl80211_id < 0) {
		fprintf(stderr, "nl80211 not found.\n");
		err = -ENOENT;
		goto out_handle_destroy;
	}

	return 0;

 out_handle_destroy:
	nl_socket_free(state->nl_sock);
	return err;
}

static void nl80211_cleanup(struct nl80211_state *state)
{
	nl_socket_free(state->nl_sock);
}

void mac_addr_n2a(char *mac_addr, const unsigned char *arg)
{
	int i, l;

	l = 0;
	for (i = 0; i < ETH_ALEN ; i++) {
		if (i == 0) {
			sprintf(mac_addr+l, "%02x", arg[i]);
			l += 2;
		} else {
			sprintf(mac_addr+l, ":%02x", arg[i]);
			l += 3;
		}
	}
}

int mac_addr_a2n(unsigned char *mac_addr, char *arg)
{
	int i;
        char tmp_arg1[20];
        strcpy(tmp_arg1, arg);
        char *tmp_arg = tmp_arg1;

	for (i = 0; i < ETH_ALEN ; i++) {
		int temp;
		char *cp = strchr(tmp_arg, ':');
		if (cp) {
			*cp = 0;
			cp++;
		}
		if (sscanf(tmp_arg, "%x", &temp) != 1)
			return -1;
		if (temp < 0 || temp > 255)
			return -1;

		mac_addr[i] = temp;
		if (!cp)
			break;
		tmp_arg = cp;
	}
	if (i < ETH_ALEN - 1)
		return -1;

	return 0;
}

static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err,
			 void *arg)
{
	struct nlmsghdr *nlh = (struct nlmsghdr *)err - 1;
	int len = nlh->nlmsg_len;
	struct nlattr *attrs;
	struct nlattr *tb[NLMSGERR_ATTR_MAX + 1];
	int *ret = (int*)arg;
	int ack_len = sizeof(*nlh) + sizeof(int) + sizeof(*nlh);

	*ret = err->error;

	if (!(nlh->nlmsg_flags & NLM_F_ACK_TLVS))
		return NL_STOP;

	if (!(nlh->nlmsg_flags & NLM_F_CAPPED))
		ack_len += err->msg.nlmsg_len - sizeof(*nlh);

	if (len <= ack_len)
		return NL_STOP;

	attrs = (struct nlattr*)((unsigned char *)nlh + ack_len);
	len -= ack_len;

	nla_parse(tb, NLMSGERR_ATTR_MAX, attrs, len, NULL);
	if (tb[NLMSGERR_ATTR_MSG]) {
		len = strnlen((char *)nla_data(tb[NLMSGERR_ATTR_MSG]),
			      nla_len(tb[NLMSGERR_ATTR_MSG]));
		fprintf(stderr, "kernel reports: %*s\n", len,
			(char *)nla_data(tb[NLMSGERR_ATTR_MSG]));
	}

	return NL_STOP;
}

static int finish_handler(struct nl_msg *msg, void *arg)
{
	int *ret = (int*)arg;
	*ret = 0;
	return NL_SKIP;
}

static int ack_handler(struct nl_msg *msg, void *arg)
{
	int *ret = (int*)arg;
	*ret = 0;
	return NL_STOP;
}


void parse_bitrate(struct nlattr *bitrate_attr, char *buf, int buflen)
{
	int rate = 0;
	char *pos = buf;
	struct nlattr *rinfo[NL80211_RATE_INFO_MAX + 1];
	//static struct nla_policy rate_policy[NL80211_RATE_INFO_MAX + 1] = {
	//	[NL80211_RATE_INFO_BITRATE] = { .type = NLA_U16 },
	//	[NL80211_RATE_INFO_BITRATE32] = { .type = NLA_U32 }
	//};

	static struct nla_policy rate_policy[NL80211_RATE_INFO_MAX + 1];
	for(int i = 0; i < NL80211_RATE_INFO_MAX + 1; i++){
		if(i == NL80211_RATE_INFO_BITRATE)
			rate_policy[i].type = NLA_U16;
		if(i == NL80211_RATE_INFO_BITRATE32)
			rate_policy[i].type = NLA_U32;
	}

	if (nla_parse_nested(rinfo, NL80211_RATE_INFO_MAX,
			     bitrate_attr, rate_policy)) {
		snprintf(buf, buflen, "failed to parse nested rate attributes!");
		return;
	}

	if (rinfo[NL80211_RATE_INFO_BITRATE32])
		rate = nla_get_u32(rinfo[NL80211_RATE_INFO_BITRATE32]);
	else if (rinfo[NL80211_RATE_INFO_BITRATE])
		rate = nla_get_u16(rinfo[NL80211_RATE_INFO_BITRATE]);
	if (rate > 0)
		pos += snprintf(pos, buflen - (pos - buf),
				"%d.%d", rate / 10, rate % 10);
	else
		pos += snprintf(pos, buflen - (pos - buf), "(unknown)");

}

static int rate_handler(struct nl_msg *msg, void *arg)
{
    	struct rate* rt = (struct rate*)arg;
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = (struct genlmsghdr*)nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *sinfo[NL80211_STA_INFO_MAX + 1];
	//static struct nla_policy stats_policy[NL80211_STA_INFO_MAX + 1] = {
	//	[NL80211_STA_INFO_TX_BITRATE] = { .type = NLA_NESTED },
	//	[NL80211_STA_INFO_RX_BITRATE] = { .type = NLA_NESTED }
	//};
	static struct nla_policy stats_policy[NL80211_STA_INFO_MAX + 1];
	for(int i = 0; i < NL80211_STA_INFO_MAX + 1; i++){
		if(i == NL80211_STA_INFO_TX_BITRATE)
			stats_policy[i].type = NLA_NESTED;
		if(i == NL80211_STA_INFO_RX_BITRATE)
			stats_policy[i].type = NLA_NESTED;
	}
	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	if (!tb[NL80211_ATTR_STA_INFO]) {
		fprintf(stderr, "sta stats missing!\n");
		return NL_SKIP;
	}
	if (nla_parse_nested(sinfo, NL80211_STA_INFO_MAX,
			     tb[NL80211_ATTR_STA_INFO],
			     stats_policy)) {
		fprintf(stderr, "failed to parse nested attributes!\n");
		return NL_SKIP;
	}

	if (sinfo[NL80211_STA_INFO_TX_BITRATE]) {
		char buf[100];
		parse_bitrate(sinfo[NL80211_STA_INFO_TX_BITRATE], buf, sizeof(buf));
        rt->tx_rate = atof(buf);
	}
	if (sinfo[NL80211_STA_INFO_RX_BITRATE]) {
		char buf[100];
		parse_bitrate(sinfo[NL80211_STA_INFO_RX_BITRATE], buf, sizeof(buf));
        rt->rx_rate = atof(buf);
	}
	//printf("tx_rate = %f, rx_rate = %f\n", rt->tx_rate, rt->rx_rate);
	return NL_SKIP;
}

static int get_sta_rate(struct nl80211_state *state, char *itrfc, char *adrs, struct rate* rt)
{
	struct nl_cb *cb;
	struct nl_cb *s_cb;
	struct nl_msg *msg;
	int err;
    	signed long long devidx = if_nametoindex(itrfc);
    	unsigned char mac_addr[ETH_ALEN];
	if(devidx == 0){
		printf("error! no such device!\n");
		return 2;
	}
	msg = nlmsg_alloc();
	if (!msg) {
		fprintf(stderr, "failed to allocate netlink message\n");
		return 2;
	}

	cb = nl_cb_alloc(NL_CB_DEFAULT);
	s_cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (!cb || !s_cb) {
		fprintf(stderr, "failed to allocate netlink callbacks\n");
		err = 2;
		goto out;
	}

	genlmsg_put(msg, 0, 0, state->nl80211_id, 0, 0, NL80211_CMD_GET_STATION, 0);

	NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, devidx);

	if (mac_addr_a2n(mac_addr, adrs)) {
		fprintf(stderr, "invalid mac address\n");
		goto out;
	}

	NLA_PUT(msg, NL80211_ATTR_MAC, ETH_ALEN, mac_addr);

	nl_socket_set_cb(state->nl_sock, s_cb);

	err = nl_send_auto_complete(state->nl_sock, msg);
	if (err < 0)
		goto out;

	err = 1;

	nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);
	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, rate_handler, rt);

	while (err > 0)
		nl_recvmsgs(state->nl_sock, cb);
 out:
     //	printf("I am here!\n");
	nl_cb_put(cb);
	nl_cb_put(s_cb);
	nlmsg_free(msg);
	printf("err = %d\n", err);
	return err;
 nla_put_failure:
	fprintf(stderr, "building message failed\n");
	return 2;
}

float get_rate(const std::string local, const std::string remote)
{
	char itrfc[20], adrs[20];
	strcpy(itrfc, local.c_str());
	strcpy(adrs, remote.c_str());

	struct nl80211_state nlstate;
	struct rate* rt = (struct rate*)malloc(sizeof(struct rate));
	rt->tx_rate = 0;
	rt->rx_rate = 0;
	if (nl80211_init(&nlstate)){
		printf("nl80211_init error!\n");
		return -1;
	}
		
	if (get_sta_rate(&nlstate, itrfc, adrs, rt)){
		printf("get_sta_rate error!\n");
		return -1;
		
	}
	std::cout<<"rt->tx_rate is "<<rt->tx_rate<<"   rt->rx_rate is "<<rt->rx_rate<<std::endl; 
	nl80211_cleanup(&nlstate);
	return  rt->rx_rate;			// successful
	
}

int valid_handler(struct nl_msg *msg, void *arg)
{
	return NL_OK;
}

static int set_mcast_rate(struct nl80211_state *state, char *itrfc, float rate)
{
	struct nl_cb *cb;
	struct nl_cb *s_cb;
	struct nl_msg *msg;
	int err;
        signed long long devidx = if_nametoindex(itrfc);
	if(devidx == 0){
		printf("error!no such device!\n");
		return 0;
	}
	msg = nlmsg_alloc();
	if (!msg) {
		fprintf(stderr, "failed to allocate netlink message\n");
		return 2;
	}

	cb = nl_cb_alloc(NL_CB_DEFAULT);
	s_cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (!cb || !s_cb) {
		fprintf(stderr, "failed to allocate netlink callbacks\n");
		err = 2;
		goto out;
	}

	genlmsg_put(msg, 0, 0, state->nl80211_id, 0, 0, NL80211_CMD_SET_MCAST_RATE, 0);

	NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, devidx);

	NLA_PUT_U32(msg, NL80211_ATTR_MCAST_RATE, (int)(rate * 10));

	nl_socket_set_cb(state->nl_sock, s_cb);

	err = nl_send_auto_complete(state->nl_sock, msg);
	if (err < 0){
		printf("error!set mcast rate unsuccessful!\n");
		goto out;
	}


	err = 1;

	nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);
	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, valid_handler, NULL);
	while (err > 0){
		nl_recvmsgs(state->nl_sock, cb);
	}


 out:
        printf("I am here!\n");
	nl_cb_put(cb);
	nl_cb_put(s_cb);
	nlmsg_free(msg);
        printf("err = %d\n", err);
	return err;
 nla_put_failure:
	fprintf(stderr, "building message failed\n");
	return 2;
}

int set_rate(const std::string local, float rate)
{
	char itrfc[20];
	strcpy(itrfc, local.c_str());
	std::cout<<"rate is "<<rate<<std::endl;
	if (rate < 2.0)
	 	rate = 1.0;
	else if (rate < 6.0)
		rate = 2.0;
	else if (rate < 9.0)
		rate = 6.0;
	else if (rate < 12.0)
		rate = 9.0;
	else if (rate < 18.0)
		rate = 12.0;
	else if (rate < 24.0)
		rate = 18.0;
	else if (rate < 36.0)
		rate = 24.0;
	else if (rate < 48.0)
		rate = 36.0;
	else if (rate < 54.0)
		rate = 48.0;
	else
		rate = 54.0;
 	struct nl80211_state nlstate;
 	if (nl80211_init(&nlstate)){
		printf("nl80211_init error\n");
		return -1;						// error
	}
	if (set_mcast_rate(&nlstate, itrfc, rate)){
		printf("set_mcast_rate error!\n");
		return -1;						// error
	}
	nl80211_cleanup(&nlstate);
	std::cout<<"set multicast_rate "<<rate<<std::endl;
	return 1;							// success
}


