
#.PHONY : update

export TARGET = test-cmd
export OBJ_PATH = build/usr/bin

CINCLUDES += -Isrc/
CINCLUDES += -I../build/

$(OBJ_PATH)/$(TARGET): src/test.c cmdlist
	@echo "2"
	$(CC) $< -o $@ $(CINCLUDES)

cmdlist: src/test.h
	@echo "1"
	scripts/get_cmd_list
	scripts/link-in

clean:
	rm -rf build
	rm -rf cmdlist

distclean: clean
