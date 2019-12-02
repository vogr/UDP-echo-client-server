MAKEFLAGS += --warn-undefined-variables --no-builtin-rules --no-print-directory

NAMES := client server
ARCHIVE := UDP_OGIER.zip

.PHONY: all
all: $(NAMES)

.PHONY: server
server:
	(cd src_server && $(MAKE))
	cp src_server/server .

.PHONY: client
server:
client:
	(cd src_client && $(MAKE))
	cp src_client/client .

.PHONY: archive
archive: clean
	zip -r $(ARCHIVE) . -x \*.git\*

.PHONY: clean
clean:
	rm -f $(ARCHIVE) $(NAMES)
	(cd src_server && $(MAKE) clean)
	(cd src_client && $(MAKE) clean)
