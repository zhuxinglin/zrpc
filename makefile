SUB_DIR:=libconet \
	libzk \
	libmysql_cli \
	corpc \
	libshmconfig_server \
	libshmconfig_agent \
	libshm_config \
	libxmlconfig_server \
	libxml_config \

SUB_DIR_CLEAN = $(SUB_DIR%=%_clean)

.PHONY: subdirs $(SUB_DIR)

subdirs:$(SUB_DIR)

$(SUB_DIR):
	@+make -C $@

all:${SUB_DIR}

$(SUB_DIR_CLEAN):
	@+make -C $@ clean;

clean:$(SUB_DIR_CLEAN)