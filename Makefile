
.PHONY: module utility

all: module utility

module: bin bin/vcmod.ko bin/vcctrl

bin: 
	mkdir bin

bin/vcmod.ko:
	cd src; $(MAKE) ; cd ..; cp src/vcmod.ko bin/

bin/vcctrl:
	$(MAKE) -C vcam_ctrl; cp vcam_ctrl/vcctrl bin/

clean:
	rm -r bin; cd src; $(MAKE) clean; cd ..; $(MAKE) clean -C vcam_ctrl;
