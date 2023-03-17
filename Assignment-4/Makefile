all: browser server
browser: MyBrowser.c
	gcc -o browser MyBrowser.c
server: MyHTTP.c
	gcc -o server MyHTTP.c
clean:
	rm -f browser server