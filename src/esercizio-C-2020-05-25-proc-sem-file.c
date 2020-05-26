#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define FILE_SIZE	1024
#define N			4

int fd;
sem_t *proc_sem, *file_mutex;
void err_exit(char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

void child(int n)
{
	int fd = open("output.txt", O_RDWR, 00644);
	if (fd == -1)
		err_exit("open() error");
	while (1) {
		if (sem_wait(proc_sem))
			err_exit("sem_wait() error");
		if (sem_wait(file_mutex))
			err_exit("sem_wait() error");
		int r, i = 0;
		char byte;
		while ((r = read(fd, &byte, sizeof(char))) > 0) {
			i++;
			if (r == -1)
				err_exit("read() error");
			if (byte == 0) {
				byte = 'A' + n;
				if (lseek(fd, -1, SEEK_CUR) == (off_t) -1)
					err_exit("lseek() error");
				if (write(fd, &byte, sizeof(char) == -1))
					err_exit("write() error");
				puts("written");
				if (sem_post(file_mutex))
					err_exit("sem_post() error");
				break;
			}
		}
		if (i >= FILE_SIZE -1) {
			close(fd);
			exit(EXIT_SUCCESS);
		}

		//printf("Child %d has taken control of file\n", n);
		/*int i;
		for (i = 0; i < FILE_SIZE; i++) {
			if (lseek(fd, i, SEEK_SET) == (off_t) -1)
				err_exit("lseek() error");
			char byte;
			int r = read(fd, &byte, sizeof(char));
			//printf("Child %d found byte = %d at offset %d\n", n, byte, *i);
			lseek(fd, -1, SEEK_CUR);
			if (r == -1) {
				err_exit("read() error");
			} else {
				if (!byte) {
					//printf("Child %d writing at offset  =  %d\n", n, *i);
					char new = 'A' + n;
					if (write(fd, &new, sizeof(char)) == -1)
						err_exit("write() error");
					break;
				}
			}
		}
		//printf("Child %d releasing control of file with i = %d\n", n, *i);
		if (sem_post(file_mutex))
			err_exit("sem_wait() error");
		if (i >= FILE_SIZE - 1) {
			close(fd);
			//printf("Child %d exiting\n", n);
			exit(EXIT_SUCCESS);
		}
		*/
	}

	exit(EXIT_SUCCESS);
}

int main()
{
	fd = creat("output.txt", 00644);
	if (fd == -1)
		err_exit("creat() error");
	if (ftruncate(fd, FILE_SIZE))
		err_exit("ftruncate() error");
	if (close(fd))
		err_exit("close() error");

	void *map = mmap(NULL, 2 * sizeof(sem_t) + sizeof(int), PROT_READ | PROT_WRITE,
			MAP_ANONYMOUS | MAP_SHARED, -1, 0);
	if (map == MAP_FAILED)
		err_exit("mmap() error");

	proc_sem = (sem_t *) map;
	file_mutex = proc_sem + 1;
	//i = (int *) (file_mutex + 1);
	//*i = 0;
	if (sem_init(proc_sem, 1, 0))
		err_exit("sem_init() error on proc_sem");
	if (sem_init(file_mutex, 1, 1))
		err_exit("sem_init() error on file_mutex");

	for (int i = 0; i < N; i++) {
		switch (fork()) {
		case 0:
			child(i);
			break;
		case -1:
			err_exit("fork() error");
			break;
		default:
			break;
		}
	}

	for (int i = 0; i < FILE_SIZE + N; i++){
		if (sem_post(proc_sem))
			err_exit("sem_post() error");
	}

	for (int i = 0; i < N; i++){
		if (wait(NULL) == -1)
			err_exit("wait() error");
	}

	return 0;
}
