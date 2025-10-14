#ifndef X_UNIX_ERR_HPP
#define X_UNIX_ERR_HPP 1

void errMsg(const char *fmt, ...);
void errExit(const char *fmt, ...);
void err_exit(const char *fmt, ...);
void errExitEn(int errNum,const char *fmt, ...);

#endif
