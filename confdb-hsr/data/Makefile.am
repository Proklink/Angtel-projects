
yang_schemas = \
	angtel-hsr.yin \
	$(NULL)

confdbdir = $(datadir)/confdb
confdb_DATA = $(yang_schemas)


$(yang_schemas) : %.yin : $(srcdir)/%.yang
	pyang $< -f yin -o $@ -p $(srcdir)

CLEANFILES = $(yang_schemas)

