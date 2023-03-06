CC=gcc
OUT_FILE_NAME=libmdns.a
CDEFINES= -DWIN32 -DNDEBUG -D_WINDOWS -D_USRDLL -DMDNS_DEBUGMSGS=0 -DWIN32_LEAN_AND_MEAN -DUSE_TCP_LOOPBACK -D_CRT_SECURE_NO_DEPRECATE -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1 -DNOT_HAVE_SA_LEN
CFLAGS= -fPIC -O2 -g -Wall $(CDEFINES)
INC= -I.
OBJ_DIR=./obj
OUT_DIR=./lib

$(OUT_FILE_NAME): $(patsubst %.c,$(OBJ_DIR)/%.o,$(wildcard *.c))
	ar -r -o $(OUT_DIR)/$@ $^
	ranlib $(OUT_DIR)/$@

$(OBJ_DIR)/%.o: %.c directories
	$(CC) -c $(INC) $(CFLAGS) -o $@  $<
	
directories:
	@mkdir -p $(OUT_DIR)
	@mkdir -p $(OBJ_DIR)
	
clean:
	rm -f $(OBJ_DIR)/*.o $(OUT_DIR)/$(OUT_FILE_NAME)

rebuild: clean build