BASE_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
MAKEFILE_DIR := $(dir $(abspath $(firstword $(MAKEFILE_LIST))))
EXTERNAL_DIR := $(BASE_DIR)external

CFLAGS=
LDFLAGS=
CC=g++

SOURCE_DIR = $(BASE_DIR)

LDFLAGS += -lpthread -lftp -lcrypto

SRCS = $(SRCS_C)
OBJ_DIR = obj
OBJS := $(SRCS_C:.c=.o)
OBJS := $(addprefix $(OBJ_DIR)/, $(OBJS))

all : $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

clean :
	@rm -fr $(OBJ_DIR) $(TARGET)

$(OBJ_DIR)/%.o : %.c $(HEADS)
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJS) : | $(OBJ_DIR)

$(OBJ_DIR) :
	mkdir -p $(OBJ_DIR)
