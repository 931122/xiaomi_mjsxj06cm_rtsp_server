#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "mi_unix.h"

int mi_unix_sock_connect_seqpacket(const char *path, int nonblock)
{
	struct sockaddr_un unix_addr;
	int flags = SOCK_SEQPACKET | SOCK_CLOEXEC;
	int unix_sock;

	if (!path) {
		return -1;
	}

	if (nonblock) {
		flags |= SOCK_NONBLOCK;
	}

	unix_sock = socket(AF_UNIX, flags, 0);
	if (unix_sock < 0) {
		perror("socket");
		return -1;
	}

	memset(&unix_addr, 0, sizeof(unix_addr));
	unix_addr.sun_family = AF_UNIX;
	if (strlen(path) >= sizeof(unix_addr.sun_path)) {
		close(unix_sock);
		return -1;
	}
	snprintf(unix_addr.sun_path, sizeof(unix_addr.sun_path), "%s", path);

	if (connect(unix_sock, (struct sockaddr *)&unix_addr, sizeof(unix_addr)) < 0) {
		perror("connect");
		close(unix_sock);
		return -1;
	}

	return unix_sock;
}
