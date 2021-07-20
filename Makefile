LIB_DIR=-L/usr/local/systemc-2.3.2/lib-linux64
INC_DIR=-I/usr/local/systemc-2.3.2/include
LIB=-lsystemc
RPATH=-Wl,-rpath,/usr/local/systemc-2.3.2/lib-linux64

APP=buf

all:
	 g++ -g -Wall -pedantic -Wno-long-long -Wno-variadic-macros -Werror $(APP).cpp -o $(APP) $(INC_DIR) $(LIB_DIR) $(LIB) $(RPATH)

clean:
	 rm -rf $(APP)
