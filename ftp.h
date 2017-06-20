#ifndef _TRANSFER_FTP_H_
#define _TRANSFER_FTP_H_

int update_ftp_conn();
int ftp_transfer(const char *local_file, const char *remote_path, const char *remote_file);

#endif
