#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <net/if.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define WSDD_MCAST_ADDR "239.255.255.250"
#define WSDD_PORT 3702
#define BUF_SIZE 8192
#define DEFAULT_UUID "urn:uuid:4d4a5358-4a30-3643-4d2d-4f4e56494600"

static volatile sig_atomic_t g_run = 1;

static void on_signal(int signo)
{
	(void)signo;
	g_run = 0;
}

static int get_if_ipv4(const char *ifname, char *ip, size_t ip_size)
{
	int fd;
	struct ifreq ifr;
	struct sockaddr_in *addr;

	if (!ifname || !ip || ip_size < INET_ADDRSTRLEN) {
		return -1;
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname);

	if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
		close(fd);
		return -1;
	}

	addr = (struct sockaddr_in *)&ifr.ifr_addr;
	if (!inet_ntop(AF_INET, &addr->sin_addr, ip, ip_size)) {
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

static int append(char *buf, size_t size, size_t *off, const char *fmt, ...)
{
	va_list ap;
	int n;

	if (*off >= size) {
		return -1;
	}

	va_start(ap, fmt);
	n = vsnprintf(buf + *off, size - *off, fmt, ap);
	va_end(ap);

	if (n < 0 || (size_t)n >= size - *off) {
		return -1;
	}

	*off += (size_t)n;
	return 0;
}

static void make_msg_id(char *buf, size_t size)
{
	struct timespec ts;

	clock_gettime(CLOCK_REALTIME, &ts);
	snprintf(buf, size, "urn:uuid:%08lx-%04lx-%04lx-%04lx-%012lx",
	         (unsigned long)ts.tv_sec,
	         (unsigned long)(ts.tv_nsec & 0xffff),
	         (unsigned long)((ts.tv_nsec >> 16) & 0xffff),
	         (unsigned long)(getpid() & 0xffff),
	         (unsigned long)((ts.tv_sec ^ ts.tv_nsec) & 0xffffffffffffUL));
}

static int extract_message_id(const char *req, char *out, size_t out_size)
{
	const char *p;
	const char *gt;
	const char *e;
	size_t len;

	p = strstr(req, "MessageID");
	if (!p) {
		out[0] = '\0';
		return 0;
	}

	gt = strchr(p, '>');
	if (!gt) {
		out[0] = '\0';
		return 0;
	}

	p = gt + 1;
	while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
		p++;
	}

	e = strstr(p, "</");
	if (!e) {
		out[0] = '\0';
		return 0;
	}

	while (e > p && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r' || e[-1] == '\n')) {
		e--;
	}

	len = (size_t)(e - p);
	if (len >= out_size) {
		len = out_size - 1;
	}

	memcpy(out, p, len);
	out[len] = '\0';
	return 0;
}

static int build_probe_match(char *buf, size_t size, const char *uuid,
                             const char *xaddr, const char *scopes,
                             const char *relates_to)
{
	char msg_id[96];
	size_t off = 0;

	make_msg_id(msg_id, sizeof(msg_id));

	return append(buf, size, &off,
	              "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
	              "<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\" "
	              "xmlns:a=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" "
	              "xmlns:d=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" "
	              "xmlns:dn=\"http://www.onvif.org/ver10/network/wsdl\" "
	              "xmlns:tds=\"http://www.onvif.org/ver10/device/wsdl\">"
	              "<s:Header>"
	              "<a:MessageID>%s</a:MessageID>"
	              "<a:RelatesTo>%s</a:RelatesTo>"
	              "<a:To s:mustUnderstand=\"true\">http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous</a:To>"
	              "<a:Action s:mustUnderstand=\"true\">http://schemas.xmlsoap.org/ws/2005/04/discovery/ProbeMatches</a:Action>"
	              "</s:Header>"
	              "<s:Body>"
	              "<d:ProbeMatches>"
	              "<d:ProbeMatch>"
	              "<a:EndpointReference><a:Address>%s</a:Address></a:EndpointReference>"
	              "<d:Types>dn:NetworkVideoTransmitter tds:Device</d:Types>"
	              "<d:Scopes>%s</d:Scopes>"
	              "<d:XAddrs>%s</d:XAddrs>"
	              "<d:MetadataVersion>1</d:MetadataVersion>"
	              "</d:ProbeMatch>"
	              "</d:ProbeMatches>"
	              "</s:Body>"
	              "</s:Envelope>",
	              msg_id, relates_to && relates_to[0] ? relates_to : msg_id,
	              uuid, scopes, xaddr) < 0 ? -1 : (int)off;
}

static int build_hello(char *buf, size_t size, const char *uuid,
                       const char *xaddr, const char *scopes)
{
	char msg_id[96];
	size_t off = 0;

	make_msg_id(msg_id, sizeof(msg_id));

	return append(buf, size, &off,
	              "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
	              "<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\" "
	              "xmlns:a=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" "
	              "xmlns:d=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" "
	              "xmlns:dn=\"http://www.onvif.org/ver10/network/wsdl\" "
	              "xmlns:tds=\"http://www.onvif.org/ver10/device/wsdl\">"
	              "<s:Header>"
	              "<a:MessageID>%s</a:MessageID>"
	              "<a:To s:mustUnderstand=\"true\">urn:schemas-xmlsoap-org:ws:2005:04:discovery</a:To>"
	              "<a:Action s:mustUnderstand=\"true\">http://schemas.xmlsoap.org/ws/2005/04/discovery/Hello</a:Action>"
	              "</s:Header>"
	              "<s:Body>"
	              "<d:Hello>"
	              "<a:EndpointReference><a:Address>%s</a:Address></a:EndpointReference>"
	              "<d:Types>dn:NetworkVideoTransmitter tds:Device</d:Types>"
	              "<d:Scopes>%s</d:Scopes>"
	              "<d:XAddrs>%s</d:XAddrs>"
	              "<d:MetadataVersion>1</d:MetadataVersion>"
	              "</d:Hello>"
	              "</s:Body>"
	              "</s:Envelope>",
	              msg_id, uuid, scopes, xaddr) < 0 ? -1 : (int)off;
}

static int make_socket(const char *if_ip)
{
	int fd;
	int yes = 1;
	struct sockaddr_in addr;
	struct ip_mreq mreq;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		return -1;
	}

	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
#ifdef SO_REUSEPORT
	setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes));
#endif

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(WSDD_PORT);

	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		close(fd);
		return -1;
	}

	memset(&mreq, 0, sizeof(mreq));
	mreq.imr_multiaddr.s_addr = inet_addr(WSDD_MCAST_ADDR);
	mreq.imr_interface.s_addr = inet_addr(if_ip);
	if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
		close(fd);
		return -1;
	}

	setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &mreq.imr_interface, sizeof(mreq.imr_interface));
	return fd;
}

static void usage(const char *argv0)
{
	fprintf(stderr,
	        "usage: %s --ifs eth0 [--onvif-port 1000] [--uuid urn:uuid:...] [--scopes \"...\"]\n",
	        argv0);
}

int main(int argc, char **argv)
{
	static const struct option opts[] = {
		{ "ifs", required_argument, NULL, 'i' },
		{ "onvif-port", required_argument, NULL, 'p' },
		{ "uuid", required_argument, NULL, 'u' },
		{ "scopes", required_argument, NULL, 's' },
		{ "help", no_argument, NULL, 'h' },
		{ NULL, 0, NULL, 0 }
	};
	const char *ifname = NULL;
	const char *uuid = DEFAULT_UUID;
	const char *scopes = "onvif://www.onvif.org/name/MJSXJ06CM onvif://www.onvif.org/Profile/S";
	int onvif_port = 1000;
	char if_ip[INET_ADDRSTRLEN];
	char xaddr[128];
	char req[BUF_SIZE];
	char res[BUF_SIZE];
	int fd;
	int opt;

	while ((opt = getopt_long(argc, argv, "i:p:u:s:h", opts, NULL)) != -1) {
		switch (opt) {
		case 'i':
			ifname = optarg;
			break;
		case 'p':
			onvif_port = atoi(optarg);
			break;
		case 'u':
			uuid = optarg;
			break;
		case 's':
			scopes = optarg;
			break;
		default:
			usage(argv[0]);
			return opt == 'h' ? 0 : 1;
		}
	}

	if (!ifname) {
		usage(argv[0]);
		return 1;
	}

	if (get_if_ipv4(ifname, if_ip, sizeof(if_ip)) < 0) {
		fprintf(stderr, "get ip for %s failed: %s\n", ifname, strerror(errno));
		return 1;
	}

	snprintf(xaddr, sizeof(xaddr), "http://%s:%d/onvif/device_service", if_ip, onvif_port);

	fd = make_socket(if_ip);
	if (fd < 0) {
		fprintf(stderr, "wsdd socket failed: %s\n", strerror(errno));
		return 1;
	}

	{
		struct sigaction sa;
		memset(&sa, 0, sizeof(sa));
		sa.sa_handler = on_signal;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		sigaction(SIGINT, &sa, NULL);
		sigaction(SIGTERM, &sa, NULL);
	}

	{
		struct sockaddr_in dst;
		int len = build_hello(res, sizeof(res), uuid, xaddr, scopes);

		memset(&dst, 0, sizeof(dst));
		dst.sin_family = AF_INET;
		dst.sin_addr.s_addr = inet_addr(WSDD_MCAST_ADDR);
		dst.sin_port = htons(WSDD_PORT);
		if (len > 0) {
			sendto(fd, res, (size_t)len, 0, (struct sockaddr *)&dst, sizeof(dst));
		}
	}

	printf("onvif_wsdd listening on %s %s\n", ifname, xaddr);

	while (g_run) {
		struct sockaddr_in peer;
		socklen_t peer_len = sizeof(peer);
		ssize_t n;
		char relates_to[256];
		int len;

		n = recvfrom(fd, req, sizeof(req) - 1, 0, (struct sockaddr *)&peer, &peer_len);
		if (n < 0) {
			if (errno == EINTR) {
				continue;
			}
			break;
		}
		req[n] = '\0';

		if (!strstr(req, "Probe") && !strstr(req, "Resolve")) {
			continue;
		}

		extract_message_id(req, relates_to, sizeof(relates_to));
		len = build_probe_match(res, sizeof(res), uuid, xaddr, scopes, relates_to);
		if (len > 0) {
			sendto(fd, res, (size_t)len, 0, (struct sockaddr *)&peer, peer_len);
			printf("probe match -> %s:%u relates_to=%s\n",
			       inet_ntoa(peer.sin_addr), ntohs(peer.sin_port), relates_to);
			fflush(stdout);
		}
	}

	close(fd);
	return 0;
}
