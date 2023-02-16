#pragma once
#pragma warning(disable : 4996)

struct addrinfo;

int create_dgram_socket(const char *address, const char *port, addrinfo *res_addr);
