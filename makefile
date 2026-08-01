INC_DIR   	= include
TARGET_NAME = vector

COMMIT =?

PREFIX 	?= /usr/local
INCLDIR	= $(PREFIX)/include

.PHONY: install uninstall commit push

install:
	install -d $(INCLDIR)/$(TARGET_NAME)
	install -m 644 $(INC_DIR)/*.h $(INCLDIR)/$(TARGET_NAME)/

uninstall:
	rm -rf $(INCLDIR)/$(TARGET_NAME)

commit:
	git add .
	git commit -m "$(COMMIT)"

push: commit
	git push origin main
