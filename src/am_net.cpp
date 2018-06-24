#include "amulet.h"

#if defined(AM_IOS) || defined(AM_OSX)

struct am_socket {
    int fd;
};

static int create_socket(lua_State *L) {
    am_check_nargs(L, 3);
    const char *domain_str = lua_tostring(L, 1);
    const char *type_str = lua_tostring(L, 2);
    const char *protocol_str = lua_tostring(L, 3);
    if (domain_str == NULL) return luaL_error(L, "missing argument 1 (domain)");
    if (type_str == NULL) return luaL_error(L, "missing argument 2 (type)");
    if (protocol_str == NULL) return luaL_error(L, "missing argument 3 (protocol)");
    int domain = 0;
    int type = 0;
    int protocol = 0;
    if (strcmp(domain_str, "inet") == 0) domain = PF_INET;
    else return luaL_error(L, "unsupported socket domain: %s", domain_str);
    if (strcmp(type_str, "stream") == 0) type = SOCK_STREAM;
    else if (strcmp(type_str, "dgram") == 0) type = SOCK_DGRAM;
    else return luaL_error(L, "unsupported socket type: %s", type_str);
    if (strcmp(protocol_str, "tcp") == 0) protocol = IPPROTO_TCP;
    else if (strcmp(type_str, "udp") == 0) protocol = IPPROTO_UDP;
    else return luaL_error(L, "unsupported socket protocol: %s", protocol_str);

    int fd = socket(domain, type, protocol);
    if (fd == -1) return luaL_error(L, "unable to create socket");
    am_socket *sock = am_new_userdata(L, am_socket);
    sock->fd = fd;
    return 1;
}

static int bind_socket(lua_State *L) {
    am_check_nargs(L, 2);
    am_socket *sock = am_get_userdata(L, am_socket, 1);
    int port = luaL_checkinteger(L, 2);
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.sin_port = htons(port);
    if (bind(sock->fd, (struct sockaddr *)&sa, sizeof sa) == -1) {
        return luaL_error(L, "bind failed");
    }
    return 0;
}

static int listen_socket(lua_State *L) {
    am_check_nargs(L, 2);
    am_socket *sock = am_get_userdata(L, am_socket, 1);
    int backlog = luaL_checkinteger(L, 2);
    if (listen(sock->fd, backlog) == -1) {
        return luaL_error(L, "listen failed");
    }
    return 0;
}

static int accept_socket(lua_State *L) {
    am_check_nargs(L, 1);
    am_socket *accept_sock = am_get_userdata(L, am_socket, 1);
    int fd = accept(accept_sock->fd, NULL, NULL);
    if (fd < 0) return luaL_error(L, "accept failed");
    am_socket *connect_sock = am_new_userdata(L, am_socket);
    connect_sock->fd = fd;
    return 1;
}

static int send_socket(lua_State *L) {
    am_check_nargs(L, 2);
    am_socket *sock = am_get_userdata(L, am_socket, 1);
    if (sock->fd < 0) return luaL_error(L, "invalid socket");
    int type = am_get_type(L, 2);
    char *data = NULL;
    size_t len = 0;
    switch (type) {
        case LUA_TSTRING: {
            data = (char*)lua_tolstring(L, 2, &len);
            break;
        }
        case MT_am_buffer:
        case MT_am_buffer_gc: {
            am_buffer *buf = am_check_buffer(L, 2);
            data = (char*)buf->data;
            len = (size_t)buf->size;
            break;
        }
        default:
            return luaL_error(L, "invalid argument of type %s", am_get_typename(L, type));
    }
    ssize_t sent;
    while (len > 0) {
        sent = send(sock->fd, data, len, 0);
        if (sent < 0) return luaL_error(L, "error sending");
        data += sent;
        len -= sent;
    }
    return 0;
}

static int recv_socket(lua_State *L) {
    am_check_nargs(L, 1);
    am_socket *sock = am_get_userdata(L, am_socket, 1);
    if (sock->fd < 0) return luaL_error(L, "invalid socket");
    ssize_t received;
    char data[1024];
    received = recv(sock->fd, data, 1024, 0);
    if (received < 0) return luaL_error(L, "error receiving data");
    if (received == 0) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushlstring(L, data, received);
    return 1;
}

static int close_socket(lua_State *L) {
    am_check_nargs(L, 1);
    am_socket *sock = am_get_userdata(L, am_socket, 1);
    if (sock->fd > 0) {
        close(sock->fd);
        sock->fd = -1;
    }
    return 0;
}

static int get_host_addr(lua_State *L) {
    struct ifaddrs *interfaces = NULL;
    bool found = false;
    if (getifaddrs(&interfaces) == 0) {
        struct ifaddrs *ptr = interfaces;
        while (ptr != NULL) {
            if (ptr->ifa_addr->sa_family == AF_INET
                && ptr->ifa_addr != NULL 
                && (ptr->ifa_flags & IFF_RUNNING) 
                && !(ptr->ifa_flags & IFF_LOOPBACK)
                && strlen(ptr->ifa_name) >= 2
                && ptr->ifa_name[0] == 'e' && ptr->ifa_name[1] == 'n') 
            {
                //am_debug("%s:%s", ptr->ifa_name, inet_ntoa(((struct sockaddr_in *)ptr->ifa_addr)->sin_addr));
                lua_pushstring(L, inet_ntoa(((struct sockaddr_in *)ptr->ifa_addr)->sin_addr));
                found = true;
                break;
            }
            ptr = ptr->ifa_next;
        }
    }
    freeifaddrs(interfaces);
    if (!found) lua_pushnil(L);
    return 1;
}

static void register_socket_mt(lua_State *L) {
    lua_newtable(L);
    am_set_default_index_func(L);
    am_set_default_newindex_func(L);
    lua_pushcclosure(L, bind_socket, 0);
    lua_setfield(L, -2, "bind");
    lua_pushcclosure(L, listen_socket, 0);
    lua_setfield(L, -2, "listen");
    lua_pushcclosure(L, accept_socket, 0);
    lua_setfield(L, -2, "accept");
    lua_pushcclosure(L, send_socket, 0);
    lua_setfield(L, -2, "send");
    lua_pushcclosure(L, recv_socket, 0);
    lua_setfield(L, -2, "recv");
    lua_pushcclosure(L, close_socket, 0);
    lua_setfield(L, -2, "close");
    am_register_metatable(L, "socket", MT_am_socket, 0);
}

void am_open_net_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"socket", create_socket},
        {"host_addr", get_host_addr},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
    register_socket_mt(L);
}

#else
void am_open_net_module(lua_State *L) {
}
#endif
