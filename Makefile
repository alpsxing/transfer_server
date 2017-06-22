SRCS_C = ftp.c \
		 transfer.c \
		 crypt.c \
		 cJSON.c\
		 data.c \
		 encode.c

HEADS = ulist.h ftp.h transfer.h crypt.h cJSON.h encode.h

TARGET = transfer 


include ./rules.mk
