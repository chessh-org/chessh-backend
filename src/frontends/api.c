#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

static void start_client(int clientfd);

static int read_string(char *dst, int fd);

int main() {
	int sockfd, opt;
	struct sockaddr_in addr;

	signal(SIGCHLD, SIG_IGN);

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket() failed");
		return 1;
	}

	opt = 1;
	if ((setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
					&opt, sizeof opt)) < 0) {
		perror("setsockopt() failed");
		return 1;
	}

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(1475);
	if (bind(sockfd, (struct sockaddr *) &addr, sizeof addr) < 0) {
		perror("bind() failed");
		return 1;
	}

	if (listen(sockfd, 1024) < 0) {
		perror("listen() failed");
		return 1;
	}

	for (;;) {
		int clientfd;
		struct sockaddr_in addr;
		socklen_t socklen;
		if ((clientfd = accept(sockfd, (struct sockaddr *) &addr, &socklen)) < 0) {
			switch (errno) {
			case EAGAIN: case EINTR:
				continue;
			default:
				perror("accept() failed");
				return 1;
			}
		}
		start_client(clientfd);
	}
}

static void start_client(int clientfd) {
	pid_t pid;
	char user[256], pass[256];

	switch (pid = fork()) {
	case -1:
		perror("fork() failed");
		return;
	case 0:
		break;
	default:
		close(clientfd);
		return;
	}

	if (read_string(user, clientfd) ||
	    read_string(pass, clientfd)) {
		exit(EXIT_FAILURE);
	}

	printf("%s:%s\n", user, pass);

	dup2(clientfd, 0);
	dup2(clientfd, 1);
	dup2(clientfd, 2);
	execl("/chessh/build/chessh-client", "chessh-client", "-u", user, "-p", pass, "-d", "/chessh-server",
			NULL);
	exit(EXIT_FAILURE);
}

static int read_string(char *dst, int fd) {
	unsigned char len;
	if (read(fd, &len, sizeof len) < (ssize_t) sizeof len) {
		return -1;
	}

	dst[len] = '\0';

	while (len > 0) {
		ssize_t this_read;
		this_read = read(fd, dst, len);
		if (this_read < 0) {
			if (errno != EINTR && errno != EAGAIN) {
				return -1;
			}
			continue;
		}
		len -= this_read;
	}

	return 0;
}
