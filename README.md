# shmresize
### Mô tả chức năng hàm
Viết hàm ksys_shmresize() vào trong source code của cơ chế Shared Memory, hàm này có tác dụng là mở rộng hoặc giảm phân đoạn shared memory segment mà không làm mất dữ liệu cũ trước đó và shmid của phân đoạn shared memory segment không thay đổi để tránh ảnh hưởng tới việc attach/dettach của các tiến trình khác cùng giao tiếp.
### [Hướng dẫn build kernel](build_kernel.md)
### Lưu đồ thuật toán
<img src="images/flowchart.png" alt="Flowchart Image" width="500">
<details>
  <summary>Giải thích luồng thuật toán</summary>
  # Quy Trình Hoạt Động của Hàm `ksys_shmresize`

1. **Bắt đầu**: Hàm `ksys_shmresize` bắt đầu.
2. **Kiểm tra `new_size`**: Kiểm tra xem `new_size` có nằm trong giới hạn cho phép hay không. Nếu không, trả về lỗi `-EINVAL`.
3. **Lấy đối tượng `shmid_kernel`**: Lấy cấu trúc `shmid_kernel` tương ứng với `shmid`. Nếu không lấy được, trả về lỗi.
4. **Khóa đối tượng IPC**: Khóa đối tượng để tránh truy cập đồng thời.
5. **Kiểm tra giới hạn namespace**: Kiểm tra xem việc thay đổi kích thước có vượt quá giới hạn của namespace hay không. Nếu có, trả về lỗi `-ENOSPC`.
6. **Tạo tệp mới**: Tạo một tệp mới với kích thước `new_size`. Nếu thất bại, trả về lỗi.
7. **`old_size > 0?`**: Kiểm tra xem có dữ liệu cũ cần sao chép hay không.
8. **Cấp phát bộ nhớ**: Cấp phát bộ nhớ kernel cho bộ đệm tạm thời. Nếu thất bại, trả về lỗi `-ENOMEM`.
9. **Đọc dữ liệu**: Đọc dữ liệu từ tệp cũ vào bộ đệm. Nếu thất bại, giải phóng bộ nhớ và tệp mới, sau đó trả về lỗi.
10. **Đặt lại vị trí ghi**: Đặt lại vị trí ghi cho tệp mới về 0.
11. **Ghi dữ liệu**: Ghi dữ liệu từ bộ đệm vào tệp mới. Nếu thất bại, giải phóng bộ nhớ và tệp mới, sau đó trả về lỗi.
12. **Giải phóng bộ nhớ**: Giải phóng bộ nhớ đã cấp phát cho bộ đệm.
13. **Cập nhật thông tin**: Cập nhật kích thước (`shm_segsz`) và tổng số trang sử dụng (`shm_tot`).
14. **Giải phóng tệp cũ**: Giải phóng tệp cũ.
15. **Gán tệp mới**: Gán tệp mới cho `shp->shm_file`.
16. **Giải phóng khóa IPC**: Giải phóng khóa.
17. **Trả về 0 (thành công)**: Hàm kết thúc thành công.

</details>
<details>
  <summary>Triển khai hàm</summary>
Trước tiên ta phải tải mã nguồn nhân linux về để chỉnh sửa mã nguồn, sau đó sẽ tiến hành build lại kernel sau đó áp dụng kernel mới để kiểm tra hoạt động của hàm mới.
  
Sau đó ta phải viết thêm hàm shmresize với yêu cầu xác định như trên vào trong file mã nguồn của shared memory ipc là 'ipc/shm.c' để hàm có thể hoạt động. Hàm này sẽ hoạt động ở dưới nhân kernel của linux, vì vậy cần khai báo System call tương ứng và khai báo vào Syscall table để có thể gọi từ user space. Bằng việc sử dụng system call number ta có thể sử dụng trực tiếp hàm từ user space bằng việc khai báo thêm thư việt &lt;syscalls.h&gt; thay vì thêm hàm đó vào các thư viện tiêu chuẩn của C.
```bash
#include <linux/slab.h>   // For kmalloc and kfree
#include <linux/mm.h>   // For memory management functions
#include <linux/hugetlb.h>
#include <linux/shm.h>
#include <uapi/linux/shm.h>
#include <linux/init.h>
#include <linux/file.h>
#include <linux/mman.h>
#include <linux/shmem_fs.h>
#include <linux/security.h>
#include <linux/syscalls.h> // For system calls
#include <linux/audit.h>
#include <linux/capability.h>
#include <linux/ptrace.h>
#include <linux/seq_file.h>
#include <linux/rwsem.h>
#include <linux/nsproxy.h>
#include <linux/mount.h>
#include <linux/ipc_namespace.h>
#include <linux/rhashtable.h>
#include <linux/fs.h> // For file operations
#include <linux/fcntl.h>
#include <linux/uaccess.h>
#include <linux/pagemap.h>
#include <linux/rmap.h>
#include <linux/errno.h>
#include “util.h”

// Function to resize the shared memory segment and keep old data
long ksys_shmresize(int shmid, size_t new_size) {
    struct ipc_namespace *ns = current->nsproxy->ipc_ns;
    struct shmid_kernel *shp;
    struct file *new_file;
    loff_t pos = 0;
    ssize_t err;
    size_t old_size;
    size_t numpages = (new_size + PAGE_SIZE - 1) >> PAGE_SHIFT;

    // Basic size validation
    if (new_size < SHMMIN || new_size > ns->shm_ctlmax)
        return -EINVAL;

    // Obtain the shared memory segment object and lock it
    shp = shm_obtain_object_check(ns, shmid);
    if (IS_ERR(shp))
        return PTR_ERR(shp);

    ipc_lock_object(&shp->shm_perm);

    // Verify that the new size does not exceed namespace limits
    if (ns->shm_tot - (shp->shm_segsz >> PAGE_SHIFT) + numpages > ns->shm_ctlall) {
        err = -ENOSPC;
        goto unlock;
    }

    // Create a new file for the resized segment
    new_file = shmem_kernel_file_setup("SYSV_SHMRESIZE", new_size, 0);
    if (IS_ERR(new_file)) {
        err = PTR_ERR(new_file);
        goto unlock;
    }

    // Allocate a temporary buffer to hold old data for copying
    old_size = shp->shm_segsz;
    if (old_size > 0) {
        char *buffer = kmalloc(old_size, GFP_KERNEL);
        if (!buffer) {
            err = -ENOMEM;
            fput(new_file);
            goto unlock;
        }

        // Read data from the old file into the buffer
        err = kernel_read(shp->shm_file, buffer, old_size, &pos);
        if (err < 0) {
            kfree(buffer);
            fput(new_file);
            goto unlock;
        }

        // Reset position for the new file
        pos = 0;

        // Write data from the buffer to the new file
        err = kernel_write(new_file, buffer, old_size, &pos);
        kfree(buffer);

        if (err < 0) {
            fput(new_file);
            goto unlock;
        }
    }

    // Update the segment total and size
    ns->shm_tot = ns->shm_tot - (shp->shm_segsz >> PAGE_SHIFT) + numpages;
    shp->shm_segsz = new_size;

    // Release the old file and replace it with the new one
    fput(shp->shm_file);
    shp->shm_file = new_file;

    err = 0;  // Success

unlock:
    ipc_unlock_object(&shp->shm_perm);
    return err;
}

// System call definition for user-space access
SYSCALL_DEFINE2(shmresize, int, shmid, size_t, new_size) {
    return ksys_shmresize(shmid, new_size);
}

```
  <details>
  <summary># Giải thích chi tiết hơn về một số phần quan trọng</summary>

- **numpages = (new_size + PAGE_SIZE - 1) >> PAGE_SHIFT;**: Đoạn code này tính toán số trang bộ nhớ cần thiết để chứa new_size byte. PAGE_SIZE là kích thước của một trang bộ nhớ (thường là 4KB). PAGE_SHIFT là số bit cần dịch phải để chia cho PAGE_SIZE (ví dụ: nếu PAGE_SIZE là 4096 (2^12), thì PAGE_SHIFT là 12). Việc cộng PAGE_SIZE - 1 trước khi dịch phải đảm bảo rằng kết quả được làm tròn lên. Ví dụ: nếu new_size là 4097 byte, thì cần 2 trang.
    
- **shmem_kernel_file_setup("SYSV_SHMRESIZE", new_size, 0);**: Hàm này tạo một tệp tin ẩn danh trong kernel, được sử dụng để lưu trữ dữ liệu của shared memory segment. Tham số đầu tiên là tên (chỉ để debug), tham số thứ hai là kích thước, và tham số thứ ba là cờ (0 trong trường hợp này).

- **kernel_read và kernel_write**: Đây là các hàm kernel space để đọc và ghi dữ liệu vào tệp. Chúng tương tự như read và write trong user space, nhưng hoạt động trong ngữ cảnh kernel.

- **fput**: Hàm này giảm bộ đếm tham chiếu của một đối tượng tệp. Khi bộ đếm tham chiếu đạt 0, tệp sẽ được giải phóng.

- **ipc_lock_object và ipc_unlock_object**: Các hàm này dùng để khóa và giải phóng khóa trên đối tượng IPC (trong trường hợp này là shared memory segment), ngăn chặn các truy cập đồng thời gây ra xung đột dữ liệu.

- **goto unlock;**: Được sử dụng để xử lý lỗi. Khi có lỗi xảy ra, code sẽ nhảy đến nhãn unlock, nơi khóa được giải phóng trước khi hàm trả về lỗi. Điều này rất quan trọng để tránh deadlock.

</details>
Sau khi sửa đổi tệp 'shm.c' để bao gồm chức năng 'shmresize' mới, cần đảm bảo những thay đổi sau trong các phần khác của mã nguồn nhân Linux để tích hợp đầy đủ chức năng mới:

-Define một constant cho ‘shmresize’ system call number (trong include/uapi/linux/ipc.h). Tệp header này chứa các định nghĩa cho các hoạt động của IPC và đảm bảo rằng các chương trình trong không gian người dùng có quyền truy cập vào mã định danh của system call.
```bash
#define SHM_RESIZE 463
```
-Thêm khai báo hàm cho ‘ksys_shmresize’ trong syscalls.h (Khai báo trong include/linux/syscalls.h) để system call mới được công nhận bởi kernel
```bash
asmlinkage long sys_shmresize(int shmid, size_t new_size); 
long ksys_shmresize(int shmid, size_t new_size); 
```
-Đăng ký system call trong các tệp dành riêng cho kiến ​​trúc. Tùy thuộc vào kiến ​​trúc của máy (ví dụ: x86, ARM, v.v.), cập nhật syscall table để đăng ký ‘shmresize’. Điều này cho phép kernel liên kết system call number đến hàm mới.
Với cấu trúc x86 : Sửa đổi \`arch/x86/entry/syscalls/syscall_64.tbl\` để thêm entry cho lệnh gọi hệ thống \`shmresize\`. Thêm một dòng mới với các trường tương ứng.
```bash
463     common	shmresize	sys_shmreszie 
```
-Nếu có ý định sử dụng lệnh gọi \`shmresize\` trực tiếp từ các chương trình trong không gian người dùng, cần phải sửa đổi thư viện chuẩn C (như \`glibc\`) để gọi syscall mới này.

-Ngoài ra, có thể sử dụng \`syscall()\` từ user space để gọi trực tiếp \`shmresize\`.

Sau khi chỉnh sửa xong mã nguồn, tiến hành việc build kernel mới theo hướng dẫn ở trên.
</details>

## Thực hiện chương trình kiểm thử

Thực hiện biên dịch hai chương trình trong folder test sau khi build kernel, một chương trình mở rộng và ghi dữ liệu ra vùng nhớ, chương trình thứ hai để đọc dữ liệu từ vùng nhớ đó.
