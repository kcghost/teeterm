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
#include <string.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <limits.h>

uint8_t buf[BUFSIZ];
/* Ensure ts starts initialized to zero */
struct termios ts;
char ptyNames[2][PATH_MAX];

void cleanup(int signo)
{
	if(unlink(ptyNames[0]) < 0) {
		perror("Could not unlink pty*0");
		if(unlink(ptyNames[1]) < 0) {
			perror("Could not unlink pty*1");
		}
		exit(1);
	}
	if(unlink(ptyNames[1]) < 0) {
		perror("Could not unlink pty*1");
		exit(1);
	}
	exit(0);
}

void usage(char *self) {
	printf("Usage: %s [-n pty_suffix] [ARGS]\n", self);
}

int main(int argc, char* argv[])
{
	int child_pty;
	int master[2];
	int slave[2];

	//char ptyNames[2][PATH_MAX];
	int customname;

	fd_set readfds;
	int nfds;
	int count;

	int i;
	pid_t pid;

	if(argc < 2) {
		usage(argv[0]);
		return 1;
	}

	customname = !strcmp("-n", argv[1]);
	if (customname && argc < 4) {
		usage(argv[0]);
		return 1;
	}
	
	for (i = 0; i <= 1; ++i) {
		strcpy(ptyNames[i], "pty");
		if (customname) {
			strcat(ptyNames[i], argv[2]);
		}
		strcat(ptyNames[i], (i==0 ? "0" : "1"));
		printf("customname: %d, ptyNames[%d]: %s\n", customname, i, ptyNames[i]);
	}

	cfmakeraw(&ts);

	signal(SIGCHLD, cleanup);
	signal(SIGINT, cleanup);

	if((pid = forkpty(&child_pty, NULL, &ts, NULL)) < 0) {
		perror("Could not fork");
		return 1;
	} else if(pid == 0) {
		int index = customname ? 3 : 1;
		printf("Command: %s", argv[index]);
		execvp(argv[index], &(argv[index]));
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
			fcntl(master[i], F_SETFL, O_NONBLOCK);
			puts(ttyname(slave[i]));
			if(symlink(ttyname(slave[i]), ptyNames[i]) < 0) {
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
