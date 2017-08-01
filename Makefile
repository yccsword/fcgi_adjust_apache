include $(APP_DIR)/app.mk

TARGET = libfcgitools.so


#Fcgi
FCGI_INCLUDE = -I$(APP_DIR)/httpd/third-part_support/fcgi/include
CFLAGS += $(FCGI_INCLUDE)
ARCH = $(shell getconf LONG_BIT)
ifeq ("$(ARCH)","32")
LDFLAGS += -L$(APP_DIR)/httpd/third-part_support/fcgi/lib -lfcgi
else
LDFLAGS += -L$(APP_DIR)/httpd/third-part_support/fcgi/lib64 -lfcgi
endif

LDFLAGS += -L$(APP_DIR)/lib/fcgi -lmultipart -liconv

SOURCE= \
	lib_fcgitools.c \
	md5.c

OBJS = $(SOURCE:%.c=obj/%.o)

$(TARGET): $(OBJS)
	$(CC) -fPIC -shared -o $@ $(OBJS) $(LDFLAGS)

obj/%.o:%.c
	@mkdir -p obj
	$(CC) -fPIC $(CFLAGS) -c $< -o $@ 
	
install:
	cp -f $(TARGET) $(ROOTFS_DIR)/apache/lib/
	cp -f libmultipart.so $(ROOTFS_DIR)/apache/lib/
	cp -f libiconv.so $(ROOTFS_DIR)/apache/lib/libiconv.so.2

clean:
	rm -fr obj $(OBJS) $(TARGET)
