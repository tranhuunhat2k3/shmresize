# shmresize
## Mô tả chức năng hàm
Viết hàm ksys_shmresize() vào trong source code của cơ chế Shared Memory, hàm này có tác dụng là mở rộng hoặc giảm phân đoạn shared memory segment mà không làm mất dữ liệu cũ trước đó và shmid của phân đoạn shared memory segment không thay đổi để tránh ảnh hưởng tới việc attach/dettach của các tiến trình khác cùng giao tiếp.
## [Hướng dẫn build kernel](build_kernel.md)
## Lưu đồ thuật toán
<img src="images/flowchart.png" alt="Flowchart Image" width="500">
<details>
  <summary>Giải thích luồng thuật toán:</summary>
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
