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

#define INITIAL_SIZE 3000 
// Function to call shmresize using the syscall interface
int shmresize(int shmid, size_t new_size)
{
    return syscall(SYS_shmresize, shmid, new_size);
}
int main()
{
    char initial_data[6000];
    for (int i = 0; i < 4822; i++)
    {
        initial_data[i] = 'A';
    }
    initial_data[4823] = 'B';
    initial_data[4824] = '\0';

    key_t key = ftok("shmfile", 68);      // Arbitrary key for shared memory

    // Create a shared memory segment
    int shmid = shmget(key, INITIAL_SIZE, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("Failed to create shared memory segment");
        return 1;
    }
    printf("Đã tạo vùng nhớ dùng chung.\n");
    // Attach to the shared memory segment
    char* shmaddr = (char*)shmat(shmid, NULL, 0);
    if (shmaddr == (char*)-1) {
        perror("Failed to attach shared memory segment");
        shmctl(shmid, IPC_RMID, NULL);  // Cleanup
        return 1;
    }
    printf("Shmid = [%d]\n", shmid);
    printf("Kích thước: [%d] bytes\n", INITIAL_SIZE);
    char* text = "Tran Huu Nhat 20214333";
    strncpy(shmaddr, text, 23);
    printf("Dữ liệu khởi tạo ban đầu:%s\n", shmaddr);
    printf("Địa chỉ ảo bắt đầu của vùng nhớ dùng chung: [%p]\n", (void*)shmaddr);
    printf("Địa chỉ ảo của vùng nhớ dùng chung trong tiến trình (%d):\n", getpid());
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "cat /proc/%d/maps | grep \"rw-s\"", getpid());
    system(cmd);
    printf("\n");
    //Detach from shared memory
    if (shmdt(shmaddr) == -1)
    {
        perror("shmdt failed");
        return 1;
    }
    //printf("Initial data to shared memory: %s\n",initial_data);
    size_t data_size = strlen(initial_data);
    printf("Kích thước chuỗi toàn ký tự 'A' = [%zu] bytes (Lớn hơn kích thước ban đầu)\n", data_size);

    //Resize the shared memory segment using shmresize
    if (data_size > INITIAL_SIZE)
    {
        if (shmresize(shmid, data_size) == -1)
        {
            perror("Failed to resize shared memory segment");
            return 1;
        }
    }
    printf("Mở rộng vùng nhớ thành công!\n");
    printf("Shmid = [%d]\n", shmid);
    printf("Kích thước: [%zu] bytes\n", data_size);
    //Attach to the shared memory segment
    shmaddr = (char*)shmat(shmid, NULL, 0);
    if (shmaddr == (char*)-1) {
        perror("Failed to attach shared memory segment");
        shmctl(shmid, IPC_RMID, NULL); //Cleanup
        return 1;
    }
    printf("Kiểm tra dữ liệu cũ: %s\n", shmaddr);
    strncpy(shmaddr, initial_data, data_size);
    strncpy(shmaddr, text, 23);
    printf("Đã ghi chuỗi toàn ký tự 'A' vào trong vùng nhớ.\n");
    printf("Địa chỉ ảo bắt đầu của vùng nhớ dùng chung mới: %p\n", (void*)shmaddr);
    printf("Địa chỉ ảo của vùng nhớ dùng chung trong tiến trình (%d):\n", getpid());
    snprintf(cmd, sizeof(cmd), "cat /proc/%d/maps", getpid());
    system(cmd);
    printf("\n");
    //Detach from shared memory
    if (shmdt(shmaddr) == -1) {
        perror("shmdt failed");
        return 1;
    }

    return 0;
}
