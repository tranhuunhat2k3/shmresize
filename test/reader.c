#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>

// Define the syscall number for shmresize
#define SYS_shmresize 463  
#define INITIAL_SIZE 2000
void print_memory_maps()
{
    printf("Địa chỉ ảo của vùng nhớ dùng chung trong tiến trình (%d):\n", getpid());
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "cat /proc/%d/maps | grep \"SYS_SHMRESIZE\"", getpid());
    system(cmd);
    printf("\n");
}
// Function to call shmresize using the syscall interface
int shmresize(int shmid, size_t new_size) {
    return syscall(SYS_shmresize, shmid, new_size);
}
int main() {
    key_t key = ftok("shmfile", 68);      // Arbitrary key for shared memory

    // Create a shared memory segment
    int shmid = shmget(key, 4822, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("Failed to create shared memory segment");
        return 1;
    }

    // Attach to the shared memory segment
    char* shmaddr = (char*)shmat(shmid, NULL, 0);
    if (shmaddr == (char*)-1) {
        perror("Failed to attach shared memory segment");
        shmctl(shmid, IPC_RMID, NULL);  // Cleanup
        return 1;
    }
    printf("Đã attach thành công!\n");
    printf("Shmid = [%d]\n", shmid);
    printf("Địa chỉ ảo bắt đầu của vùng nhớ dùng chung: [%p]\n", (void*)shmaddr);
    print_memory_maps();
    printf("Dữ liệu đọc được từ writer: %s \n", shmaddr);
    printf("Dữ liệu đọc được từ writer (trang thứ 2): %s \n", shmaddr + 4096);
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "ipcs -m");
    system(cmd);
    shmdt(shmaddr);             // Detach from shared memory
    shmctl(shmid, IPC_RMID, NULL);  // Mark shared memory for deletion

    return 0;
}
