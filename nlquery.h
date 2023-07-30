#ifndef NL_QUERY_H
#define NL_QUERY_H

#include <string.h>

#define ERROR_CODE_FAILED -1
#define ERROR_CODE_SUCCESS 0

#define MAX_NETLINK_BUF_SIZE 4096

typedef void (*nl_status_cb)(void *data, int linkup);

class netlink
{
    int fd_{-1};
    void *buf_{NULL};
    int buflen_{0};

public:
    netlink();
    ~netlink();

    enum state
    {
        UNKNOWN,
        LINKUP,
        LINKDOWN,
        MAX
    } state_{UNKNOWN},
        prestate_{UNKNOWN};

    int query(const char *device);      // 等于 status + query_ext
    int status(const char *device);     // 等待响应
    void query_ext(const char *device); // 发请求

    void printstate();

private:
    int nl_open(int family);
    void nl_close();
    int nl_query(const char *device);
    int nl_status(const char *device, nl_status_cb cb, void *data);

    std::string enum_state2string(enum state sta);
    void setstate(int linkstatus);
    void set_prev_state(enum state linkstatus);
};

#endif