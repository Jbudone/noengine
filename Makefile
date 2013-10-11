
SOURCE_COMMON = lib_logger.o k_memory.o lib_containers.o lib_shdmgr.o lib_resmgr.o  k_terrain.o k_camera.o k_mesh.o k_entity.o k_world.o lib_math.o k_ui.o soil.a
SOURCE_SERVER = server.o k_net.server.o $(SOURCE_COMMON)
SOURCE_CLIENT = client.o k_net.client.o $(SOURCE_COMMON)
NEW_WARNINGS = -Wc++11-compat -Wmaybe-uninitialized 
NOWARNINGS = -Waddress   -Warray-bounds  -Wchar-subscripts  -Wenum-compare -Wcomment  -Wformat   -Wnonnull  -Wparentheses  -Wreturn-type  -Wsequence-point  -Wstrict-aliasing  -Wstrict-overflow=1  -Wswitch  -Wtrigraphs  -Wuninitialized  -Wunknown-pragmas  -Wunused-label     -Wvolatile-register-var
LIBS = -L/usr/local/lib/ -L/usr/local/lib64/ -lGLEW -lGLU -lGL -lglut -lcurses `freetype-config --libs`
INCLUDES = -Isrc/ -Isrc/extern -I/usr/include/
OPTIONS = -g -O0 -std=c++0x $(INCLUDES) $(NOWARNINGS)

COMPILE    = ~/.vim/bin/cc_args.py g++
COMPILEOBJ = g++ -shared -c

all: server client

server: $(SOURCE_SERVER)
	$(COMPILE) $(SOURCE_SERVER) -DSERVER -o build/server $(LIBS) $(OPTIONS)

client: $(SOURCE_CLIENT)
	$(COMPILE) $(SOURCE_CLIENT) -DCLIENT -o build/client $(LIBS) $(OPTIONS)

server.o: src/server.cpp
	$(COMPILEOBJ) $? $(LIBS) $(OPTIONS)

client.o: src/client.cpp
	$(COMPILEOBJ) $? $(LIBS) $(OPTIONS)

k_%.o   : src/kernel/k_%.cpp
	$(COMPILEOBJ) $? $(LIBS) $(OPTIONS)

k_memory.o : src/kernel/memory/k_memory.cpp
	$(COMPILEOBJ) $? $(LIBS) $(OPTIONS)

lib_%.o : src/libutils/lib_%.cpp
	$(COMPILEOBJ) $? $(LIBS) $(OPTIONS)

soil.a: src/extern/soil/*.c
	gcc -shared -c src/extern/soil/*.c
	ar rvs soil.a *.o

clean:
	rm -rf *.o
	rm -rf *.a
