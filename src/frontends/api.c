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

static int read_string(char *dst, char *src);

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
	char buff[1024];
	pid_t pid;
	ssize_t len;
	char user[256], pass[256], *ptr;
	int bufflen;

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

	len = read(clientfd, buff, sizeof buff);
	if (len < 0) {
		exit(EXIT_FAILURE);
	}
	ptr = buff;
	if ((bufflen = read_string(user, ptr)) > len) {
		exit(EXIT_FAILURE);
	}
	ptr += bufflen;
	len -= bufflen;
	if ((bufflen = read_string(pass, ptr)) > len) {
		exit(EXIT_FAILURE);
	}

	dup2(clientfd, 0);
	dup2(clientfd, 1);
	dup2(clientfd, 2);
	execl("/chessh/build/chessh-client", "chessh-client", "-u", user, "-p", pass, "-d", "/chessh-server",
			NULL);
	exit(EXIT_FAILURE);
}

static int read_string(char *dst, char *src) {
	unsigned char len = (unsigned char) src[0];
	memcpy(dst, src+1, len);
	dst[len] = '\0';
	return len+1;
}
