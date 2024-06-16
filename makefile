CC = gcc
CFLAGS	=	-Wall $(COPTIONS)

OBJS1	=	server_main.o server_func.o server_net.o
OBJS2	=	client_main.o client_func.o client_win.o client_net.o
TARGET1	=	server
TARGET2	=	client
# 僕の環境では-lSDL2_gfxが使えなったので-lSDL_gfxになってます
LDFLAGS  = -lm -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL_gfx -lSDL2_mixer -lGL -lGLU -lglut -L/usr/lib
# LDFLAGS  = -lm -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_gfx -lSDL2_mixer -lGL -lGLU -lglut -ljoyconlib -lhidapi-hidraw -L/usr/lib

debug:	COPTIONS = -g3
debug:	$(TARGET1) $(TARGET2)

release: CPPFLAGS = -DNDEBUG
release: COPTIONS = -O3
release: $(TARGET1) $(TARGET2)

$(TARGET1):	$(OBJS1)
	$(CC) -o $@ $^ $(LDFLAGS)
$(TARGET2):	$(OBJS2)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY:	clean
clean:
		@rm -f *.o $(TARGET1) $(TARGET2) core *~

server_main.o: server_main.c server.h server.h common.h
server_func.o: server_func.c server.h server.h common.h
server_net.o: server_net.c server.h server.h common.h
client_main.o: client_main.c client.h common.h
client_func.o: client_func.c client.h common.h
client_net.o: client_net.c client.h common.h
client_win.o: client_win.c client.h common.h
