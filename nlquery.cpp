#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <iostream>
#include <cstring>

#include "nlquery.h"

void nl_linkstatus_query_cb(void *data, int linkstatus)
{
    *(int *)data = linkstatus;
}

netlink::netlink()
{
    (void)nl_open(NETLINK_ROUTE);
    buf_ = malloc(MAX_NETLINK_BUF_SIZE);
    if(buf_)
        buflen_ = MAX_NETLINK_BUF_SIZE;
}

netlink::~netlink()
{
    nl_close();
    if(buf_)
    {
        free(buf_);
        buf_ = NULL;
        buflen_ = 0;
    }
}

int netlink::query(const char *device)
{
    int link_status = -1;
    if(!device)
        return ERROR_CODE_FAILED;
    if(nl_query(device))
        (void)nl_status(device, nl_linkstatus_query_cb, &link_status);
    return link_status;
}

int netlink::nl_open(int family)
{
    int fd;
    struct sockaddr_nl sa;

    nl_close();

    memset(&sa, 0 , sizeof(sa));
    sa.nl_family = AF_NETLINK;
    sa.nl_groups = RTNLGRP_LINK; /* RTnetlink multicast groups */

    fd = socket(AF_NETLINK, SOCK_RAW, family);
    if(0 > fd)
    {
        std::cout<<"create netlink socket failed!"<<std::endl;
        return -1;
    }

    if(bind(fd, (struct sockaddr *)&sa, sizeof(sa)))
    {
        std::cout<<"bind netlink addr failed!"<<std::endl;
        close(fd);
        return -1;
    }

    fd_ = fd;
    return fd;
}

void netlink::nl_close()
{
    if(0 <= fd_)
    {
        close(fd_);
        fd_ = -1;
    }
    return ;
}

int netlink::nl_query(const char *device)
{
    if(0 > fd_)
    {
        return ERROR_CODE_FAILED;
    }

    struct msghdr msg;
    struct sockaddr_nl sa;
    struct iovec iov;
    int cnt;

    /* nlmsg header + payload */
    /* payload有多种格式，ifinfomsg是其中一种，用于获取接口信息 */
    struct {
        struct nlmsghdr hdr;
        struct ifinfomsg ifm;
    } __attribute__((packed)) req;

    memset(&sa, 0, sizeof(sockaddr_nl));
    sa.nl_family = AF_NETLINK;

    /* req嵌入到iov中，iov和sa嵌入到msg中 */

    memset(&req, 0, sizeof(req));
    req.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(ifinfomsg)); /* 整个netlink消息的长度（包含消息头） */
    req.hdr.nlmsg_type = RTM_GETLINK;                               /* Types of messages */
    req.hdr.nlmsg_flags = NLM_F_REQUEST; /* 消息标记，它们用以表示消息的类型 */
    req.hdr.nlmsg_seq = 1; /* 消息序列号，用以将消息排队，有些类似TCP协议中的序号（不完全一样），但是netlink的这个字段是可选的，不强制使用*/
    req.hdr.nlmsg_pid = 0; /* 发送端口的ID号，对于内核来说该值就是0，对于用户进程来说就是其socket所绑定的ID号 */
    req.ifm.ifi_family = AF_UNSPEC; /* 协议族 */
    req.ifm.ifi_index = if_nametoindex(device ? device : ""); /* 接口序号 */
    req.ifm.ifi_change = 0xffffffff;  /* IFF_* 格式 */

    iov.iov_base = &req;
    iov.iov_len = sizeof(req);

    memset(&msg, 0, sizeof(msg));

    msg.msg_name = &sa;
    msg.msg_namelen = sizeof(sa);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    cnt = sendmsg(fd_, &msg, 0);
    if(0 > cnt)
    {
        std::cout<<"send msg error!"<<std::endl;
        return ERROR_CODE_FAILED;
    }
    return cnt;
}

int netlink::nl_status(const char *device, nl_status_cb cb, void *data)
{
    /* 变量初始化 */
    int index, len;
    int link_up = -1;
    struct iovec iov;
    struct msghdr msg;
    struct sockaddr_nl sa;
    struct nlmsghdr *nh;
    struct ifinfomsg *info = NULL;

    if(!buf_)
    {
        buf_ = malloc(MAX_NETLINK_BUF_SIZE);
        if(buf_)
            buflen_ = MAX_NETLINK_BUF_SIZE;
        else
            return -1;
    }

    /* device name 转index */
    index = if_nametoindex(device);

    /* recvmsg 中msg拼装，msg中指定好iov buf 、sockaddr_nl（用于保存消息来源地址） */
    iov.iov_base = buf_;
    iov.iov_len = buflen_;
    memset(&msg, 0, sizeof(msghdr));
    msg.msg_name = &sa;
    msg.msg_namelen = sizeof(sa);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    /* recvmsg思路：先尝试获取，读取实际大小但不清空接收缓存，如果本端recv buf不够大，重新malloc */
    len = recvmsg(fd_, &msg, MSG_PEEK | MSG_TRUNC);
    if(len < 1)
    {
        std::cout<<" netlink no msg"<<std::endl;
        return -1;
    }

    if(len > buflen_)
    {
        free(buf_);
        buf_ = NULL;
        buflen_ = 0;

        buf_ = malloc(len);
        if(!buf_)
            return -1;
        buflen_ = len;

        iov.iov_base = buf_;
        iov.iov_len = buflen_;
    }

    /* 第二次获取-清空socket buf */
    len = recvmsg(fd_, &msg, 0);
    if(len < 1)
        return -1;

    /* 接收buf里存的消息格式为: | nlmsghdr + ifinfomsg  + rtattr | ... * N | nlmsghdr + ifinfomsg + rtattr| */
    /* 读取ifinfomsg并解析 */
    nh = (struct nlmsghdr *)buf_;
    for( ; NLMSG_OK(nh, len); nh = NLMSG_NEXT(nh, len))
    {
        if(nh->nlmsg_type != RTM_NEWLINK)
            continue;

        info = (struct ifinfomsg *)NLMSG_DATA(nh);
        if(index != info->ifi_index)
            continue;

        link_up = info->ifi_flags & IFF_RUNNING ? 1 : 0; 
    }
    cb(data, link_up);
    return 0;
}


