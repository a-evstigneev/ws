web_server : web_server.o servfunc.o initconf.o keyval.o readstr.o servconn.o servweb.o servcgi.o
	gcc -D_GNU_SOURCE web_server.o servfunc.o initconf.o keyval.o readstr.o servconn.o servweb.o servcgi.o -o web_server
web_server.o : web_server.c servfunc.h initconf.h keyval.h servconn.h
	gcc -c web_server.c -o web_server.o
servfunc.o : servfunc.c servfunc.h 
	gcc -c servfunc.c -o servfunc.o
initconf.o : initconf.c servfunc.h initconf.h keyval.h
	gcc -c initconf.c -o initconf.o
keyval.o : keyval.c servfunc.h keyval.h
	gcc -c keyval.c -o keyval.o
readstr.o : readstr.c readstr.h
	gcc -c readstr.c -o readstr.o
servconn.o : servconn.c servconn.h keyval.h initconf.h servfunc.h servweb.h initconf.h readstr.h servcgi.h
	gcc -c servconn.c -o servconn.o
servweb.o : servweb.c servweb.h keyval.h readstr.h
	gcc -c servweb.c -o servweb.o
servcgi.o : servcgi.c servcgi.h servweb.h keyval.h servfunc.h initconf.h readstr.h
	gcc -c servcgi.c -o servcgi.o
clean : 
	rm web_server *.o

