PROGRAM=quarqd

BINARIES=build-linux-i386/$(PROGRAM) build-iphone-arm/$(PROGRAM) build-osx-ppc-i386/$(PROGRAM) build-linux-x86_64/$(PROGRAM)  #build-win32/$(PROGRAM).exe 

all: $(BINARIES)

build-osx-ppc-i386/$(PROGRAM):
	cd build-osx-ppc-i386 && $(MAKE)

build-linux-i386/$(PROGRAM):
	cd build-linux-i386 && $(MAKE)

build-linux-x86_64/$(PROGRAM):
	cd build-linux-x86_64 && $(MAKE)

build-win32/$(PROGRAM).exe:
	cd build-win32 && $(MAKE)

build-iphone-arm/$(PROGRAM):
	cd build-iphone-arm && $(MAKE)

clean:	
	cd build-linux-i386 && $(MAKE) clean
	cd build-linux-x86_64 && $(MAKE) clean
	cd build-iphone-arm && $(MAKE) clean
	cd build-osx-ppc-i386 && $(MAKE) clean

publish: all
	tar -c $(BINARIES) README.txt | gzip | ssh quarqus@quarq.us tar -C public_html/opensource/$(PROGRAM) -xzv
