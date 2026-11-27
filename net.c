// Network for the cover download (phase 8): IOP ps2dev9 + netman + smap, lwIP on the EE, DHCP or a fixed IP,
// HTTPS through wolfSSL (ps2sdk ports). Bring-up as ps2sdk samples/network/tcpip-dhcp/ps2ip.c (docs/sources.md);
// HTTP/1.0, one connection per file, body read to EOF and checked against Content-Length.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "net.h"

int http_parse(const char *buf, int n, int *status, int *body, int *clen)
{
	int end = -1;
	for (int i = 0; i + 3 < n; i++)
		if (!memcmp(buf + i, "\r\n\r\n", 4)) { end = i; break; }
	if (end < 0 || n < 12 || memcmp(buf, "HTTP/1.", 7) || !isdigit((unsigned char)buf[9])) return 0;
	*status = atoi(buf + 9);
	*body = end + 4;
	*clen = -1;
	for (int i = 0; i < end; i++) // header lines: "\r\n" + name, case-insensitive
		if (buf[i] == '\n' && i + 16 < end && !strncasecmp(buf + i + 1, "content-length:", 15))
			*clen = atoi(buf + i + 16);
	return 1;
}


typedef int (*http_recv_fn)(void *context, int socket, void *buffer, int length, int flags);
typedef void (*http_wait_fn)(void *context, unsigned milliseconds);


static int http_body_length_valid(int status, int content_length, int body_length, int required)
{
    if (status != 200) return 1;
    if (content_length < 0) return !required;
    return content_length == body_length;
}

static int http_read_cancelable(int socket, char *buffer, int capacity, int *received,
                                net_cancel_fn cancelled, void *cancel_context,
                                http_recv_fn recv_fn, http_wait_fn wait_fn, void *io_context,
                                volatile int *progress)
{
    int used = 0, elapsed_ms = 0;
    if (!buffer || capacity <= 0 || !received || !recv_fn || !wait_fn)
        return NET_ERR_PROTO;
    *received = 0;
    if (progress) *progress = 0;
    while (used < capacity && elapsed_ms < 5000) {
        int r;
        if (cancelled && cancelled(cancel_context)) {
            *received = used;
            return NET_ERR_CANCELLED;
        }
        r = recv_fn(io_context, socket, buffer + used, capacity - used, 0x08);
        if (r == 0) break;
        if (r > 0) {
            if (r > capacity - used) {
                *received = used;
                return NET_ERR_PROTO;
            }
            used += r;
            if (progress) *progress = used;
            elapsed_ms = 0;
        } else {
            wait_fn(io_context, 20);
            elapsed_ms += 20;
        }
    }
    *received = used;
    if (cancelled && cancelled(cancel_context)) return NET_ERR_CANCELLED;
    return 0;
}

/* Socket callbacks keep the connect/send state machine host-testable. */
enum { HTTP_IO_AGAIN = -2 };
typedef struct {
    int (*set_nonblocking)(void *context, int socket);
    int (*connect)(void *context, int socket, const void *address, int address_length);
    int (*wait_writable)(void *context, int socket, unsigned milliseconds);
    int (*socket_error)(void *context, int socket);
    int (*send)(void *context, int socket, const char *buffer, int length);
} http_connect_ops;

/* connect returns 0 when connected, 1 while pending, or a negative error.
   wait_writable returns 1 when writable, 0 on timeout, or a negative error.
   socket_error returns 0 when connected, 1 while pending, or a negative error.
   send returns a byte count, HTTP_IO_AGAIN when it would block, or a negative error. */
static int http_connect_send(int socket, const void *address, int address_length,
                             const char *request, int request_length,
                             net_cancel_fn cancelled, void *cancel_context,
                             const http_connect_ops *ops, void *io_context)
{
    int r, connected = 0, sent = 0, elapsed_ms = 0;
    if (!address || address_length <= 0 || !request || request_length <= 0 || !ops ||
        !ops->set_nonblocking || !ops->connect || !ops->wait_writable ||
        !ops->socket_error || !ops->send)
        return NET_ERR_PROTO;
    if (cancelled && cancelled(cancel_context)) return NET_ERR_CANCELLED;
    if (ops->set_nonblocking(io_context, socket) < 0) return NET_ERR_CONNECT;
    if (cancelled && cancelled(cancel_context)) return NET_ERR_CANCELLED;

    r = ops->connect(io_context, socket, address, address_length);
    if (r < 0) return NET_ERR_CONNECT;
    if (r == 0) {
        connected = 1;
    } else {
        while (elapsed_ms < 5000) {
            if (cancelled && cancelled(cancel_context)) return NET_ERR_CANCELLED;
            r = ops->wait_writable(io_context, socket, 20);
            if (r < 0) return NET_ERR_CONNECT;
            if (r > 0) {
                r = ops->socket_error(io_context, socket);
                if (r == 0) {
                    connected = 1;
                    break;
                }
                if (r < 0) return NET_ERR_CONNECT;
            }
            elapsed_ms += 20;
        }
    }
    if (!connected) return NET_ERR_CONNECT;

    elapsed_ms = 0;
    while (sent < request_length) {
        if (cancelled && cancelled(cancel_context)) return NET_ERR_CANCELLED;
        r = ops->send(io_context, socket, request + sent, request_length - sent);
        if (r > 0) {
            if (r > request_length - sent) return NET_ERR_PROTO;
            sent += r;
            continue;
        }
        if (r != HTTP_IO_AGAIN || elapsed_ms >= 5000) return NET_ERR_CONNECT;
        r = ops->wait_writable(io_context, socket, 20);
        if (r < 0) return NET_ERR_CONNECT;
        elapsed_ms += 20;
    }
    return cancelled && cancelled(cancel_context) ? NET_ERR_CANCELLED : 0;
}

// "https://host/path" or "http://host[:port]/path" (#15: update files) -> 1 https, 0 http, -1 not a URL
int url_split(const char *url, char *host, int hn, int *port, char *path, int pn)
{
	int tls = !strncmp(url, "https://", 8);
	if (!tls && strncmp(url, "http://", 7)) return -1;
	const char *h = url + (tls ? 8 : 7), *sl = strchr(h, '/'), *e = sl ? sl : h + strlen(h), *c = memchr(h, ':', e - h);
	if ((c ? c : e) - h <= 0 || (c ? c : e) - h >= hn) return -1;
	snprintf(host, hn, "%.*s", (int)((c ? c : e) - h), h);
	*port = c ? atoi(c + 1) : tls ? 443 : 80;
	snprintf(path, pn, "%s", sl ? sl : "/");
	return tls;
}

int http_location(const char *buf, int body, char *out, int n) // a redirect's "Location:" header, 1 if found
{
	for (int i = 0; i + 10 < body; i++)
		if (buf[i] == '\n' && !strncasecmp(buf + i + 1, "location:", 9)) {
			const char *v = buf + i + 10;
			while (*v == ' ') v++;
			int k = strcspn(v, "\r\n");
			if (k <= 0 || k >= n) return 0;
			memcpy(out, v, k), out[k] = 0;
			return 1;
		}
	return 0;
}

#ifndef SELFTEST
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <netman.h>
#include <ps2ip.h>
#include <fcntl.h> // Include after ps2ip.h so PS2SDK's tcpip.h socket flags stay authoritative.
#include <netdb.h>
#include <arpa/inet.h>
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>

// wolfSSL seeds its RNG from /dev/urandom, which the PS2 lacks. The link wraps open/read (Makefile -Wl,--wrap) and
// serves that one path from COP0.Count jitter. ponytail: weak entropy, fine for public cover images over TLS;
// real entropy (pad and network timings) if TLS ever carries anything private.
#define URANDOM_FD 4093
int __real_open(const char *path, int flags, ...);
int __real_read(int fd, void *buf, size_t n);
int __wrap_open(const char *path, int flags, ...)
{
	if (!strcmp(path, "/dev/urandom") || !strcmp(path, "/dev/random")) return URANDOM_FD;
	va_list ap;
	va_start(ap, flags);
	int mode = va_arg(ap, int);
	va_end(ap);
	return __real_open(path, flags, mode);
}
int __wrap_read(int fd, void *buf, size_t n)
{
	if (fd != URANDOM_FD) return __real_read(fd, buf, n);
	static unsigned x = 0x9E3779B9;
	x ^= (unsigned)time(NULL) ^ (unsigned)clock();
	for (size_t i = 0; i < n; i++) {
		unsigned c;
		for (int k = 0; k < 8; k++) { // the count's low bits after uneven work: cache, interrupts, other threads
			__asm__ volatile("mfc0 %0, $9" : "=r"(c));
			x = (x ^ c) * 0x01000193;
			for (volatile unsigned j = c & 31; j; j--) {}
		}
		((unsigned char *)buf)[i] = x >> 24;
	}
	return n;
}

#define IRX(m) extern unsigned char m##_irx[]; extern unsigned int size_##m##_irx
IRX(ps2dev9); IRX(netman); IRX(smap);
volatile int net_busy;
int net_mtu;
char net_why[64]; // the last https_get failure, for the log
volatile int net_got; // bytes of the response so far (#15: the update's progress bar)
static int net_on; // net_up succeeded: the EE stack is registered with netman

int net_dev9(void)
{
	static int st; // 0 not tried, 1 loaded, -1 failed
	int r = 0;
	if (!st) st = SifExecModuleBuffer(ps2dev9_irx, size_ps2dev9_irx, 0, NULL, &r) >= 0 && r != 1 ? 1 : -1;
	return st == 1;
}

static int wait_for(int (*ok)(void)) // 10 s, as the ps2sdk sample
{
	for (int t = 0; t < 10; t++) {
		if (ok()) return 1;
		sleep(1);
	}
	return ok();
}
static int link_up(void) { return NetManIoctl(NETMAN_NETIF_IOCTL_GET_LINK_STATUS, NULL, 0, NULL, 0) == NETMAN_NETIF_ETH_LINK_STATE_UP; }
static int dhcp_bound(void)
{
	t_ip_info ip;
	return ps2ip_getconfig("sm0", &ip) >= 0 && ip.dhcp_status == DHCP_STATE_BOUND;
}

int ip4addr_aton(const char *cp, struct ip4_addr *addr); // libps2ip (lwIP), not declared by the installed headers

int net_up(const char *ip, const char *mask, const char *gw, const char *dns)
{
	struct { unsigned char *irx; unsigned *size; } mods[] = {{netman_irx, &size_netman_irx}, {smap_irx, &size_smap_irx}};
	if (net_busy) return NET_ERR_BUSY; // udpbd / udpfs: Neutrino's smap already drives the adapter
	if (!net_dev9()) return snprintf(net_why, sizeof(net_why), "ps2dev9"), NET_ERR_MODULES; // shared with the HDD (phase 14)
	for (int i = 0; i < 2; i++) {
		int r = 0, id = SifExecModuleBuffer(mods[i].irx, *mods[i].size, 0, NULL, &r);
		if (id < 0 || r == 1)
			return snprintf(net_why, sizeof(net_why), "%s id %d ret %d", i ? "smap" : "netman", id, r), NET_ERR_MODULES;
	}
	NetManInit();
	int dhcp = !strcasecmp(ip, "dhcp");
	struct ip4_addr a, m, g, d;
	ip4_addr_set_zero(&a), ip4_addr_set_zero(&m), ip4_addr_set_zero(&g), ip4_addr_set_zero(&d);
	// lwIP's own parser: arpa/inet.h's inet_aton is libcglue_inet_aton, which goes through the socket glue that
	// ps2ipInit installs, so before it every fixed address came out 0.0.0.0 and nothing left the PS2 (phase 16)
	if (!dhcp) ip4addr_aton(ip, &a), ip4addr_aton(mask, &m), ip4addr_aton(gw, &g);
	ps2ipInit(&a, &m, &g);
	// applied again through setconfig, as ps2sdk's tcpip-dhcp sample (ethApplyIPConfig) does: with ps2ipInit's addresses
	// alone a fixed IP stayed 0.0.0.0 (PCSX2, phase 16), and nothing left the PS2
	t_ip_info info;
	if (ps2ip_getconfig("sm0", &info) < 0) return NET_ERR_MODULES;
	info.dhcp_enabled = dhcp; // the lease brings a DNS server too
	if (!dhcp) memcpy(&info.ipaddr, &a, 4), memcpy(&info.netmask, &m, 4), memcpy(&info.gw, &g, 4);
	ps2ip_setconfig(&info);
	if (!wait_for(link_up)) return NET_ERR_LINK;
	if (dhcp && !wait_for(dhcp_bound)) return NET_ERR_DHCP;
	// after the lease: [red] dns wins over the router's, and no DNS at all gets 1.1.1.1 (#7)
	const ip_addr_t *cur = dns_getserver(0);
	const char *use = *dns ? dns : !cur || ip_addr_isany(cur) ? "1.1.1.1" : NULL;
	if (use && ip4addr_aton(use, &d)) dns_setserver(0, &d);
	// On the console (SCPH-75001) some full-size frames arrive with bytes 1472-1513 zeroed: 42 bytes at the end of a
	// 1460-byte TCP segment, seen in every Jellyfin answer, never in PCSX2 (no cache model). ps2sdk netman writes back
	// the D-cache over the received frame rounded up to 64 bytes, and the line holding bytes 1472+ is shared with what
	// follows the buffer. A lower MTU makes lwIP advertise a smaller MSS (TCP_CALCULATE_EFF_SEND_MSS), so the peer never
	// sends frames that long. The installed headers do not match libps2ip's struct netif (mtu at 38, the library's
	// SMapIFInit stores it at 40 and the name at 50), so the field is written at the library's offset, after checking it.
	unsigned char *nif = (unsigned char *)netif_find("sm0");
	if (nif && nif[50] == 's' && nif[51] == 'm' && *(unsigned short *)(nif + 40) == 1500)
		*(unsigned short *)(nif + 40) = net_mtu = 1400;
	net_on = 1;
	return 0;
}

const char *net_ip(void) // the address the interface has now (fixed or the DHCP lease), "" before net_up
{
	static char s[16];
	t_ip_info ip;
	*s = 0;
	if (net_on && ps2ip_getconfig("sm0", &ip) >= 0) snprintf(s, sizeof(s), "%s", inet_ntoa(ip.ipaddr));
	return s;
}

// before another program: the IOP's netman keeps DMAing every received frame (LAN broadcasts too) into the EE stack's
// buffers, which the next program's code then occupies. After the launcher's network was up (phase 16: achievements
// on every boot) Black never started; deregistering the EE side stops the copies
void net_down(void)
{
	if (!net_on) return;
	net_on = 0;
	ps2ipDeinit();
	NetManDeinit();
}

static int tls_recv(WOLFSSL *ssl, char *b, int sz, void *ctx) // wolfSSL's reads: 8 s of silence and it gives up
{
	(void)ssl;
	for (int ms = 0; ms < 8000; ms += 20) {
		int r = recv(*(int *)ctx, b, sz, 0x08); // lwIP's MSG_DONTWAIT (see http_req)
		if (r > 0) return r;
		if (r == 0) return WOLFSSL_CBIO_ERR_CONN_CLOSE;
		usleep(20000);
	}
	return WOLFSSL_CBIO_ERR_TIMEOUT;
}

int https_get(const char *host, const char *path, char *buf, int max, int *body, int *len)
{
	static WOLFSSL_CTX *ctx;
	if (!ctx) {
		wolfSSL_Init();
		ctx = wolfSSL_CTX_new(wolfSSLv23_client_method());
		if (!ctx) return NET_ERR_TLS;
		// ponytail: no certificate check (no CA store, and the RTC may be off); the data is public cover art and every
		// image is size-checked after decoding. Embed the host's root CA if anything private ever goes over this.
		wolfSSL_CTX_set_verify(ctx, WOLFSSL_VERIFY_NONE, NULL);
	}
	struct addrinfo hints = {0}, *ai;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(host, "443", &hints, &ai) != 0) return snprintf(net_why, sizeof(net_why), "dns %s", host), NET_ERR_DNS;
	int s = socket(AF_INET, SOCK_STREAM, 0), r = s < 0 ? -1 : connect(s, ai->ai_addr, ai->ai_addrlen);
	if (r < 0) snprintf(net_why, sizeof(net_why), "%s %s errno %d", s < 0 ? "socket" : "connect",
	                    inet_ntoa(((struct sockaddr_in *)ai->ai_addr)->sin_addr), errno);
	freeaddrinfo(ai);
	if (r < 0) { if (s >= 0) close(s); return NET_ERR_CONNECT; }
	WOLFSSL *ssl = wolfSSL_new(ctx);
	int st = NET_ERR_TLS, n = snprintf(buf, max, "GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: TEa.G3\r\nConnection: close\r\n\r\n", path, host);
	snprintf(net_why, sizeof(net_why), "tls %.40s", host); // left as is if the handshake fails or times out
	if (ssl && wolfSSL_UseSNI(ssl, WOLFSSL_SNI_HOST_NAME, host, strlen(host)) == WOLFSSL_SUCCESS &&
	    wolfSSL_set_fd(ssl, s) == WOLFSSL_SUCCESS && (wolfSSL_SSLSetIORecv(ssl, tls_recv), wolfSSL_SetIOReadCtx(ssl, &s), 1) &&
	    wolfSSL_connect(ssl) == WOLFSSL_SUCCESS && wolfSSL_write(ssl, buf, n) == n) {
		snprintf(net_why, sizeof(net_why), "read %.40s", host);
		for (n = net_got = 0; n < max && (r = wolfSSL_read(ssl, buf + n, max - n)) > 0;) net_got = n += r;
		int clen;
		st = NET_ERR_PROTO;
		if (http_parse(buf, n, &st, body, &clen)) {
			*len = n - *body;
			if (st == 200 && clen != *len) st = NET_ERR_PROTO; // truncated, or bigger than buf
		}
	}
	if (ssl) wolfSSL_shutdown(ssl), wolfSSL_free(ssl);
	close(s);
	if (st == 200) net_why[0] = 0;
	return st;
}

// Plain HTTP to a LAN host. Cancellation is sampled before/after connect and send,
// and between every nonblocking receive poll. A blocking connect/send can delay cancellation
// until that syscall returns; live refresh remains disabled pending target validation.
static int http_socket_recv(void *context, int socket, void *buffer, int length, int flags)
{
    (void)context;
    return (int)recv(socket, buffer, (size_t)length, flags);
}

static void http_socket_wait(void *context, unsigned milliseconds)
{
    (void)context;
    usleep(milliseconds * 1000);
}

static int http_socket_set_nonblocking(void *context, int socket)
{
    (void)context;
    return fcntl(socket, F_SETFL, O_NONBLOCK) < 0 ? -1 : 0;
}

static int http_socket_connect(void *context, int socket, const void *address, int address_length)
{
    (void)context;
    int r = connect(socket, (const struct sockaddr *)address, (socklen_t)address_length);
    if (r == 0) return 0;
    if (errno == EINPROGRESS || errno == EALREADY || errno == EWOULDBLOCK) return 1;
    return -1;
}

static int http_socket_wait_writable(void *context, int socket, unsigned milliseconds)
{
    fd_set writes;
    struct timeval timeout;
    int r;
    (void)context;
    FD_ZERO(&writes);
    FD_SET(socket, &writes);
    timeout.tv_sec = (long)(milliseconds / 1000u);
    timeout.tv_usec = (long)((milliseconds % 1000u) * 1000u);
    r = select(socket + 1, NULL, &writes, NULL, &timeout);
    if (r < 0) return errno == EINTR ? 0 : -1;
    return r > 0 && FD_ISSET(socket, &writes) ? 1 : 0;
}

static int http_socket_error(void *context, int socket)
{
    int error = 0;
    socklen_t length = sizeof(error);
    (void)context;
    if (getsockopt(socket, SOL_SOCKET, SO_ERROR, &error, &length) < 0) return -1;
    if (error == 0) return 0;
    if (error == EINPROGRESS || error == EALREADY || error == EWOULDBLOCK) return 1;
    return -1;
}

static int http_socket_send(void *context, int socket, const char *buffer, int length)
{
    int r;
    (void)context;
    r = (int)send(socket, buffer, (size_t)length, MSG_DONTWAIT);
    if (r >= 0) return r;
    if (errno == EWOULDBLOCK || errno == EINTR) return HTTP_IO_AGAIN;
    return -1;
}

static int http_req_impl(const char *method, const char *ip, int port, const char *path,
                         char *buf, int max, int *body, int *len, int require_content_length,
                         net_cancel_fn cancelled, void *cancel_context)
{
    static const http_connect_ops io = {
        http_socket_set_nonblocking, http_socket_connect, http_socket_wait_writable,
        http_socket_error, http_socket_send
    };
    struct sockaddr_in a = {0};
    int s, r, n, st = NET_ERR_PROTO;
    if (!method || !ip || !path || !buf || max <= 0 || !body || !len)
        return NET_ERR_PROTO;
    *body = *len = 0;
    if (cancelled && cancelled(cancel_context)) return NET_ERR_CANCELLED;
    a.sin_family = AF_INET, a.sin_port = htons(port), a.sin_addr.s_addr = inet_addr(ip);
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) return NET_ERR_CONNECT;
    n = snprintf(buf, max, "%s %s HTTP/1.0\r\nHost: %s\r\nContent-Length: 0\r\nConnection: close\r\n\r\n",
                 method, path, ip);
    if (n < 0 || n >= max) {
        close(s);
        return NET_ERR_PROTO;
    }
    r = http_connect_send(s, &a, sizeof(a), buf, n, cancelled, cancel_context, &io, NULL);
    if (r != 0) {
        close(s);
        return r;
    }
    r = http_read_cancelable(s, buf, max, &n, cancelled, cancel_context,
                             http_socket_recv, http_socket_wait, NULL, &net_got);
    if (r != 0) {
        close(s);
        return r;
    }
    if (http_parse(buf, n, &st, body, &r)) {
        *len = n - *body;
        if (!http_body_length_valid(st, r, *len, require_content_length)) st = NET_ERR_PROTO;
    } else st = NET_ERR_PROTO;
    close(s);
    return st;
}

int http_req_cancelable(const char *method, const char *ip, int port, const char *path,
                        char *buf, int max, int *body, int *len,
                        net_cancel_fn cancelled, void *cancel_context)
{
    return http_req_impl(method, ip, port, path, buf, max, body, len, 1, cancelled, cancel_context);
}

int http_req(const char *method, const char *ip, int port, const char *path, char *buf, int max, int *body, int *len)
{
    return http_req_impl(method, ip, port, path, buf, max, body, len, 0, NULL, NULL);
}
// a URL, following up to 4 redirects (GitHub's release downloads go to another host); same contract as https_get
int net_fetch(const char *url, char *buf, int max, int *body, int *len)
{
	static char u[1024]; // ponytail: one caller at a time (the icon thread's update job)
	char host[128], path[900];
	int port, st = NET_ERR_PROTO;
	snprintf(u, sizeof(u), "%s", url);
	for (int hop = 0; hop < 5; hop++) {
		int tls = url_split(u, host, sizeof(host), &port, path, sizeof(path));
		if (tls < 0) return NET_ERR_PROTO;
		st = tls ? https_get(host, path, buf, max, body, len) : http_req("GET", host, port, path, buf, max, body, len);
		if (st < 300 || st >= 400 || !http_location(buf, *body, u, sizeof(u))) return st;
	}
	return st;
}

#else // host check: `make test`
#include <assert.h>

typedef struct { int cancelled, cancel_on_wait, calls, waits, mode; } http_fixture;
static int fixture_cancel(void *context) { return ((http_fixture *)context)->cancelled; }

typedef struct {
    int cancelled, cancel_on_wait, waits, ready_after;
    int connect_pending, socket_error_pending, send_again, send_chunk;
    int nonblocking_calls, connect_calls, socket_error_calls, send_calls;
    char sent[64];
    int sent_length;
} http_connect_fixture;

static int connect_fixture_cancel(void *context)
{
    return ((http_connect_fixture *)context)->cancelled;
}

static int connect_fixture_nonblocking(void *context, int socket)
{
    http_connect_fixture *f = (http_connect_fixture *)context;
    (void)socket;
    f->nonblocking_calls++;
    return 0;
}

static int connect_fixture_connect(void *context, int socket, const void *address, int address_length)
{
    http_connect_fixture *f = (http_connect_fixture *)context;
    (void)socket; (void)address; (void)address_length;
    f->connect_calls++;
    return f->connect_pending ? 1 : 0;
}

static int connect_fixture_wait(void *context, int socket, unsigned milliseconds)
{
    http_connect_fixture *f = (http_connect_fixture *)context;
    (void)socket;
    assert(milliseconds == 20);
    f->waits++;
    if (f->cancel_on_wait == f->waits) f->cancelled = 1;
    return f->ready_after > 0 && f->waits >= f->ready_after ? 1 : 0;
}

static int connect_fixture_socket_error(void *context, int socket)
{
    http_connect_fixture *f = (http_connect_fixture *)context;
    (void)socket;
    f->socket_error_calls++;
    if (f->socket_error_pending > 0) {
        f->socket_error_pending--;
        return 1;
    }
    return 0;
}

static int connect_fixture_send(void *context, int socket, const char *buffer, int length)
{
    http_connect_fixture *f = (http_connect_fixture *)context;
    int count = length;
    (void)socket;
    f->send_calls++;
    if (f->send_again > 0) {
        f->send_again--;
        return HTTP_IO_AGAIN;
    }
    if (f->send_chunk > 0 && count > f->send_chunk) count = f->send_chunk;
    if (count <= 0 || f->sent_length + count > (int)sizeof(f->sent)) return -1;
    memcpy(f->sent + f->sent_length, buffer, (size_t)count);
    f->sent_length += count;
    return count;
}

static const http_connect_ops connect_fixture_ops = {
    connect_fixture_nonblocking, connect_fixture_connect, connect_fixture_wait,
    connect_fixture_socket_error, connect_fixture_send
};

static void test_http_connect_send_cancellation(void)
{
    static const char request[] = "GET /catalog";
    http_connect_fixture f = {0};
    f.connect_pending = 1;
    f.ready_after = 1;
    f.socket_error_pending = 1;
    f.send_again = 1;
    f.send_chunk = 4;
    assert(http_connect_send(1, &f, sizeof(f), request, sizeof(request) - 1,
                             connect_fixture_cancel, &f, &connect_fixture_ops, &f) == 0);
    assert(f.nonblocking_calls == 1 && f.connect_calls == 1);
    assert(f.socket_error_calls == 2 && f.sent_length == (int)sizeof(request) - 1);
    assert(!memcmp(f.sent, request, sizeof(request) - 1));

    f = (http_connect_fixture){0};
    f.connect_pending = 1;
    f.cancel_on_wait = 1;
    assert(http_connect_send(1, &f, sizeof(f), request, sizeof(request) - 1,
                             connect_fixture_cancel, &f, &connect_fixture_ops, &f) == NET_ERR_CANCELLED);
    assert(f.waits == 1 && f.sent_length == 0);

    f = (http_connect_fixture){0};
    f.send_again = 1;
    f.cancel_on_wait = 1;
    assert(http_connect_send(1, &f, sizeof(f), request, sizeof(request) - 1,
                             connect_fixture_cancel, &f, &connect_fixture_ops, &f) == NET_ERR_CANCELLED);
    assert(f.waits == 1 && f.sent_length == 0);

    f = (http_connect_fixture){0};
    f.connect_pending = 1;
    assert(http_connect_send(1, &f, sizeof(f), request, sizeof(request) - 1,
                             connect_fixture_cancel, &f, &connect_fixture_ops, &f) == NET_ERR_CONNECT);
    assert(f.waits == 250);

    f = (http_connect_fixture){0};
    f.cancelled = 1;
    assert(http_connect_send(1, &f, sizeof(f), request, sizeof(request) - 1,
                             connect_fixture_cancel, &f, &connect_fixture_ops, &f) == NET_ERR_CANCELLED);
    assert(f.nonblocking_calls == 0 && f.connect_calls == 0);
}
static int fixture_recv(void *context, int socket, void *buffer, int length, int flags)
{
    http_fixture *f = (http_fixture *)context;
    (void)socket; (void)flags; f->calls++;
    if (f->mode == 1 && f->calls == 1) {
        const char payload[] = "abc";
        int count = length < 3 ? length : 3;
        memcpy(buffer, payload, (size_t)count);
        return count;
    }
    if (f->mode == 1) return 0;
    return -1;
}
static void fixture_wait(void *context, unsigned milliseconds)
{
    http_fixture *f = (http_fixture *)context;
    assert(milliseconds == 20);
    f->waits++;
    if (f->cancel_on_wait) f->cancelled = 1;
}

static void test_http_length_policy(void)
{
    assert(http_body_length_valid(200, 5, 5, 1));
    assert(!http_body_length_valid(200, 4, 5, 1));
    assert(!http_body_length_valid(200, -1, 5, 1));
    assert(http_body_length_valid(200, -1, 5, 0));
    assert(http_body_length_valid(404, -1, 0, 1));
}

static void test_http_read_cancellation(void)
{
    char buffer[8] = {0};
    int received = -1;
    volatile int progress = -1;
    http_fixture f = {0, 1, 0, 0, 0};
    assert(http_read_cancelable(0, buffer, sizeof(buffer), &received,
                                fixture_cancel, &f, fixture_recv, fixture_wait, &f, &progress) == NET_ERR_CANCELLED);
    assert(received == 0 && f.calls == 1 && f.waits == 1);
    f = (http_fixture){0, 0, 0, 0, 1};
    received = -1; progress = -1;
    assert(http_read_cancelable(0, buffer, sizeof(buffer), &received,
                                fixture_cancel, &f, fixture_recv, fixture_wait, &f, &progress) == 0);
    assert(received == 3 && progress == 3 && !memcmp(buffer, "abc", 3));
    f.cancelled = 1; f.calls = 0;
    assert(http_read_cancelable(0, buffer, sizeof(buffer), &received,
                                fixture_cancel, &f, fixture_recv, fixture_wait, &f, &progress) == NET_ERR_CANCELLED);
    assert(f.calls == 0);
}

int main(void)
{
	test_http_length_policy();
	test_http_read_cancellation();
	test_http_connect_send_cancellation();
	static const char ok[] = "HTTP/1.0 200 OK\r\nServer: x\r\ncontent-length: 5\r\n\r\nhello";
	int st, body, clen;
	assert(http_parse(ok, sizeof(ok) - 1, &st, &body, &clen) && st == 200 && clen == 5 && !memcmp(ok + body, "hello", 5));
	static const char nf[] = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
	assert(http_parse(nf, sizeof(nf) - 1, &st, &body, &clen) && st == 404 && clen == 0 && body == sizeof(nf) - 1);
	assert(!http_parse(ok, 20, &st, &body, &clen));                       // header cut short
	assert(!http_parse("FTP/1.0 200\r\n\r\n", 15, &st, &body, &clen));    // not HTTP
	static const char nolen[] = "HTTP/1.0 200 OK\r\n\r\nabc";
	assert(http_parse(nolen, sizeof(nolen) - 1, &st, &body, &clen) && clen == -1);
	char host[64], path[256], loc[64];
	int port;
	assert(url_split("https://github.com/a/b/releases/latest/download/update.txt", host, 64, &port, path, 256) == 1 &&
	       !strcmp(host, "github.com") && port == 443 && !strcmp(path, "/a/b/releases/latest/download/update.txt"));
	assert(url_split("http://192.168.1.5:8000/", host, 64, &port, path, 256) == 0 && !strcmp(host, "192.168.1.5") &&
	       port == 8000 && !strcmp(path, "/"));
	assert(url_split("http://h", host, 64, &port, path, 256) == 0 && port == 80 && !strcmp(path, "/"));
	assert(url_split("ftp://x/", host, 64, &port, path, 256) < 0 && url_split("https:///x", host, 64, &port, path, 256) < 0);
	static const char moved[] = "HTTP/1.1 302 Found\r\nServer: x\r\nlocation: https://objects.example/f?x=1\r\n\r\n";
	assert(http_parse(moved, sizeof(moved) - 1, &st, &body, &clen) && st == 302);
	assert(http_location(moved, body, loc, sizeof(loc)) && !strcmp(loc, "https://objects.example/f?x=1"));
	assert(!http_location(ok, 19, loc, sizeof(loc)));
	puts("net selftest ok");
	return 0;
}
#endif
