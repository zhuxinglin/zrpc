SUB_DIR:=libnet \
	libzk \
	libmysql_cli \
	zsvc \
	libshmconfig_server \
	libshmconfig_agent \
	libshm_config \
	libxmlconfig_server \
	libxml_config \

SUB_DIR_CLEAN = $(SUB_DIR:%=%_clean)

all:$(SUB_DIR)

$(SUB_DIR):
	@+make -C $@

$(SUB_DIR_CLEAN):
	@+make clean -C $(@:%_clean=%)

clean:$(SUB_DIR_CLEAN)


.PHONY: all clean $(SUB_DIR) $(SUB_DIR_CLEAN)
