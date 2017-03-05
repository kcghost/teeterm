/* teeterm
 *
 * Split I/O of one process into two pseudoterminals
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <termios.h>
#include <unistd.h>
#include <pty.h>
#include <signal.h>
#include <errno.h>

uint8_t buf[BUFSIZ];
/* Ensure ts starts initialized to zero */
struct termios ts;

void cleanup(int signo)
{
	if(unlink("pty0") < 0) {
		perror("Could not unlink pty0");
		if(unlink("pty1") < 0) {
			perror("Could not unlink pty1");
		}
		exit(1);
	}
	if(unlink("pty1") < 0) {
		perror("Could not unlink pty1");
		exit(1);
	}
	exit(0);
}

int main(int argc, char* argv[])
{
	int child_pty;
	int master[2];
	int slave[2];

	fd_set readfds;
	int nfds;
	int count;

	int i;
	pid_t pid;

	if(argc < 2) {
		printf("Usage: teeterm COMMAND [ARGS]\n");
		return 1;
	}

	cfmakeraw(&ts);

	signal(SIGCHLD, cleanup);
	signal(SIGINT, cleanup);

	if((pid = forkpty(&child_pty, NULL, &ts, NULL)) < 0) {
		perror("Could not fork");
		return 1;
	} else if(pid == 0) {
		execvp(argv[1], &(argv[1]));
		return 1;
	} else {
		nfds = child_pty;
		for(i = 0; i < 2; i++) {
			if(openpty(&master[i], &slave[i], NULL, &ts, NULL) < 0) {
				perror("Could not open pty");
				return 1;
			}
			if(master[i] > nfds) {
				nfds = master[i];
			}
			puts(ttyname(slave[i]));
			if(symlink(ttyname(slave[i]), (i == 0) ? "pty0" : "pty1") < 0) {
				perror("Could not set pty link");
				return 1;
			}
		}

		while(1) {
			FD_ZERO(&readfds);
			FD_SET(child_pty, &readfds);
			FD_SET(master[0], &readfds);
			FD_SET(master[1], &readfds);
			if(select(nfds + 1, &readfds, NULL, NULL, NULL) > 0) {
				if(FD_ISSET(master[0], &readfds)) {
					if((count = read(master[0], buf, sizeof(buf))) > 0) {
						write(child_pty, buf, count);
					}
				}

				if(FD_ISSET(master[1], &readfds)) {
					if((count = read(master[1], buf, sizeof(buf))) > 0) {
						write(child_pty, buf, count);
					}
				}

				if(FD_ISSET(child_pty, &readfds)) {
					if((count = read(child_pty, buf, sizeof(buf))) > 0) {
						write(master[0], buf, count);
						write(master[1], buf, count);
					}
				}
			} else if(errno != EINTR) {
				perror("Error monitoring file descriptors");
				return 1;
			}
		}
	}

	return 0;
}
