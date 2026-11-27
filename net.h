#pragma once
enum { NET_ERR_MODULES = -1, NET_ERR_LINK = -2, NET_ERR_DHCP = -3, NET_ERR_DNS = -4, NET_ERR_CONNECT = -5,
       NET_ERR_TLS = -6, NET_ERR_PROTO = -7, NET_ERR_BUSY = -8, NET_ERR_CANCELLED = -9 };
typedef int (*net_cancel_fn)(void *context);

int net_dev9(void);     // ps2dev9 once: shared by the cover download, the HDD and the UDP sources (phase 14); 1 if loaded
extern volatile int net_busy; // 1 once udpbd / udpfs gave the adapter to Neutrino's own smap: no lwIP after that
int net_up(const char *ip, const char *mask, const char *gw, const char *dns); // ip "dhcp" or dotted; 0 or NET_ERR_*
int net_up_cancelable(const char *ip, const char *mask, const char *gw, const char *dns,
                      net_cancel_fn cancelled, void *cancel_context);
void net_down(void);        // before running another program: EE stack off netman (no frames DMAed into EE RAM)
const char *net_ip(void);   // the interface's address now (fixed or DHCP), "" when down
extern volatile int net_got; // bytes received so far by the running https_get / http_req (progress bars)
extern char net_why[64]; // the last https_get failure (dns / socket / connect + errno), for the log
extern int net_mtu;       // MTU set on the SMAP interface by net_up (0 = interface not found); for the log
// GET https://<host><path>, whole response into buf. Returns the HTTP status, or NET_ERR_*; on 200 the body is
// buf[*body .. *body + *len) and was checked against Content-Length.
int https_get(const char *host, const char *path, char *buf, int max, int *body, int *len);
int http_req(const char *method, const char *ip, int port, const char *path, char *buf, int max, int *body, int *len);
int http_req_cancelable(const char *method, const char *ip, int port, const char *path,
                        char *buf, int max, int *body, int *len,
                        net_cancel_fn cancelled, void *cancel_context);
#define http_get(...) http_req("GET", __VA_ARGS__) // plain HTTP, LAN host by IP
#define http_get_cancelable(ip, port, path, buf, max, body, len, cancelled, context) \
    http_req_cancelable("GET", ip, port, path, buf, max, body, len, cancelled, context)
int url_split(const char *url, char *host, int hn, int *port, char *path, int pn); // 1 https, 0 http, -1 bad
int http_location(const char *buf, int body, char *out, int n);                     // redirect target, 1 if found
int net_fetch(const char *url, char *buf, int max, int *body, int *len); // https:// or http://IP:port, redirects followed
int http_parse(const char *buf, int n, int *status, int *body, int *clen); // 1 if a full header was found
