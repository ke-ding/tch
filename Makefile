all: srv cli

.PHONY: srv cli
srv:
	make -C srvsrc
cli:
	make -C clisrc

tags:
	make -C srvsrc tags
	make -C clisrc tags

rebuild: clean all

.PHONY: clean
clean:
	make -C srvsrc clean
	make -C clisrc clean
