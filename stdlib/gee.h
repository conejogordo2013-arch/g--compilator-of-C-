// C! standard extern declarations header.
// You can compile this file with `gee stdlib/gee.h` to validate header parsing.

extern void println(int64);
extern int64 scanln();
extern int64 alloc(int64);
extern void free(int64);
extern int32 tcp_open(int64, int32);
extern int32 udp_send(int64, int64, int32);
extern int32 system_exit(int32);
extern void print_bool(int32);
extern void print_hex(int64);
