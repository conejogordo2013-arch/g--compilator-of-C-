#pragma once

// Adaptación C! de conceptos base de stdio.h clásico.
// Nota: C! no soporta macros C/typedef/union/void* de forma nativa igual a C,
// así que se modela con tipos concretos y punteros como int64 cuando aplica.

struct FILE {
    int32 level;
    int32 flags;
    byte fd;
    byte hold;
    int32 bsize;
    int64 buffer;
    int64 curp;
    int32 istemp;
    int32 token;
};

int32 _IOFBF = 0;
int32 _IOLBF = 1;
int32 _IONBF = 2;

int32 _F_RDWR = 0x0003;
int32 _F_READ = 0x0001;
int32 _F_WRIT = 0x0002;
int32 _F_BUF  = 0x0004;
int32 _F_LBUF = 0x0008;
int32 _F_ERR  = 0x0010;
int32 _F_EOF  = 0x0020;
int32 _F_BIN  = 0x0040;
int32 _F_IN   = 0x0080;
int32 _F_OUT  = 0x0100;
int32 _F_TERM = 0x0200;

int32 EOF = -1;
int32 OPEN_MAX = 20;
int32 SYS_OPEN = 20;
int32 BUFSIZ = 512;
int32 L_ctermid = 5;
int32 L_tmpnam = 13;
int32 SEEK_CUR = 1;
int32 SEEK_END = 2;
int32 SEEK_SET = 0;
int32 TMP_MAX = 65535;

extern void clearerr(struct FILE*);
extern int32 fclose(struct FILE*);
extern int32 fflush(struct FILE*);
extern int32 fgetc(struct FILE*);
extern int32 fputc(int32, struct FILE*);
extern int32 fputs(string, struct FILE*);
extern int64 fread(int64, int64, int64, struct FILE*);
extern int64 fwrite(int64, int64, int64, struct FILE*);
extern int32 fseek(struct FILE*, int64, int32);
extern int64 ftell(struct FILE*);
extern void rewind(struct FILE*);
extern int32 ungetc(int32, struct FILE*);

extern int32 printf(string, ...);
extern int32 fprintf(struct FILE*, string, ...);
extern int32 sprintf(int64, string, ...);
extern int32 scanf(string, ...);
extern int32 fscanf(struct FILE*, string, ...);
extern int32 sscanf(string, string, ...);
extern int32 vprintf(string, int64);
extern int32 vfprintf(struct FILE*, string, int64);

extern int32 puts(string);
extern int64 fgets(int64, int32, struct FILE*);
extern int64 fopen(string, string);
extern int64 freopen(string, string, struct FILE*);
extern void perror(string);
extern int32 rename(string, string);
extern int32 remove(string);
extern int64 tmpfile();
extern int64 tmpnam(int64);
extern int64 strerror(int32);
