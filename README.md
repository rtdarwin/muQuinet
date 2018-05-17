# muQuinet

muQuinet is an userspace TCP/IP network stack running on Linux TUN virtual network device:

 - can be used by multiple processes/threads cocurrently.
 
 - offer POSIX Sockets compatible APIs, which can be used to override
   corresponding APIs offered by glibc (instruct `ld-linux.so` to use
   our APIs located in DSO(.so file) using `LD_PRELOAD` enivornment
   variable)
   
 - offer epoll supprt (in development)

## How it works

## Build & Install

## Debug

# muQuinet

muQuinet 是一个运行于 Linux TUN 虚拟网络设备上的用户态 TCP/IP 协议栈：

 - 能被多个进程/线程同时使用

 - 提供与 POSIX Sockets 兼容的 APIs。可用于覆盖 glibc 提供的对应 APIs
   （通过设置环境变量 `LD_PRELOAD`，使 `ld-linux.so` 优先使用我们在
   `muquinet-posix.so` 中提供的 APIs）
   
 - 支持 epoll（开发中）

## 工作模式

## 编译 & 安装

## Debug
