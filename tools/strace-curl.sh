#!/bin/sh

strace -esocket,connect,sendto,recvfrom,read,write,close,poll,select,fcntl,getsockopt,setsockopt,getpeername,getsockname curl baidu.com
