SRCS_C = ftp.c \
		 transfer.c \
		 crypt.c \
		 cJSON.c\
		 data.c

HEADS = ulist.h ftp.h transfer.h crypt.h cJSON.h

TARGET = transfer 


include ./rules.mk
