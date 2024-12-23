# Hướng Dẫn Build Kernel Với IPC Mới

Quá trình build và cài đặt lại kernel trên Linux có thể mất nhiều thời gian, tùy thuộc vào cấu hình của hệ thống sử dụng. Dưới đây là hướng dẫn từng bước chi tiết:

## Bước 1: Cài Đặt Các Gói Yêu Cầu

Trước tiên, cần cài đặt các gói công cụ và thư viện cần thiết để build kernel. Trên các hệ điều hành dựa trên Debian (như Ubuntu), có thể cài đặt các gói này bằng lệnh:

```bash
sudo apt update
sudo apt install build-essential libncurses-dev bison flex libssl-dev libelf-dev
```

## Bước 2: Tải Mã Nguồn Kernel

Có thể tải mã nguồn kernel từ [kernel.org](https://kernel.org) hoặc từ các kho mã nguồn của hệ điều hành sử dụng.

Ví dụ, để tải kernel phiên bản 6.11.7 từ kernel.org, thực hiện:

```bash
wget https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.5.7.tar.xz
tar -xvf linux-6.5.7.tar.xz
cd linux-6.5.7
```

## Bước 3: Áp Dụng Các Thay Đổi Tùy Theo Mục Đích

Sửa đổi mã nguồn của kernel theo các thay đổi cần thực hiện, ví dụ như trong `ipc/shm.c`. Hãy đảm bảo rằng đã lưu lại tất cả các thay đổi trước khi tiến hành các bước tiếp theo.

## Bước 4: Cấu Hình Kernel

Kernel cần được cấu hình trước khi build. Có một số tùy chọn cấu hình, tùy thuộc vào hệ thống:

### Sao chép cấu hình hiện tại của hệ thống
Nếu bạn không muốn thay đổi nhiều, sao chép cấu hình hiện tại của hệ thống:

```bash
cp /boot/config-$(uname -r) .config
```

### Sử dụng giao diện cấu hình menu (tùy chọn nâng cao)

```bash
make menuconfig
```

### Cấu hình mặc định

```bash
make defconfig
```

## Bước 5: Build Kernel

Sau khi cấu hình, bắt đầu quá trình build kernel. Lệnh sau đây sẽ build kernel cùng các mô-đun liên quan:

```bash
make -j$(nproc)
```

Lệnh `-j$(nproc)` sẽ sử dụng số lượng luồng tối đa mà CPU của bạn hỗ trợ để tăng tốc quá trình build.

## Bước 6: Cài Đặt Kernel và Mô-đun

Sau khi quá trình build hoàn tất, cài đặt các mô-đun kernel bằng cách chạy:

```bash
sudo make modules_install
```

Tiếp theo, cài đặt kernel vào thư mục `/boot`:

```bash
sudo make install
```

## Bước 7: Cập Nhật Bootloader

Hầu hết các hệ thống hiện đại sử dụng GRUB làm bootloader, nên bạn cần cập nhật GRUB để nhận diện kernel mới:

```bash
sudo update-grub
```

## Bước 8: Khởi Động Lại Hệ Thống

Bây giờ bạn có thể khởi động lại hệ thống và chọn kernel mới trong menu GRUB. Kiểm tra rằng kernel mới đã được áp dụng thành công bằng lệnh:

```bash
uname -r
```

Kernel mới sẽ được hiển thị cùng với các thay đổi mà chúng ta đã áp dụng.
