#ifndef NL_QUERY_H
#define NL_QUERY_H

#define ERROR_CODE_FAILED                       -1
#define ERROR_CODE_SUCCESS                      0

#define MAX_NETLINK_BUF_SIZE                    4096

typedef void (*nl_status_cb)(void *data, int linkup);

class netlink
{
    int fd_{-1};
    void *buf_{NULL};
    int buflen_{0};

public:
    netlink();
    ~netlink();

    int query(const char *device);
    int status(const char *device);
    void query_ext(const char *device);

private:
    int nl_open(int family);
    void nl_close();
    int nl_query(const char *device);
    int nl_status(const char *device, nl_status_cb cb, void *data);
};

#endif