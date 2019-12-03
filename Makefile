MAKEFLAGS += --warn-undefined-variables --no-builtin-rules --no-print-directory

NAMES := client server
ARCHIVE := UDP_OGIER.zip

.PHONY: all
all: $(NAMES)

.PHONY: server
server:
	$(MAKE) -C src_server
	cp src_server/server .

.PHONY: client
server:
client:
	$(MAKE) -C src_client
	cp src_client/client .

.PHONY: archive
archive: clean
	zip -r $(ARCHIVE) . -x \*.git\*

.PHONY: clean
clean:
	rm -f $(ARCHIVE) $(NAMES)
	$(MAKE) -C src_server clean
	$(MAKE) -C src_client clean
