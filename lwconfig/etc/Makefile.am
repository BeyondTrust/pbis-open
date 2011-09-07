lwconfig_DATA = lwconfig.xml

if PLATFORM_MAC
LWCONFIG_FILES=lwconfig.header.xml lwconfig.body.xml lwconfig.mac.xml lwconfig.footer.xml
else
LWCONFIG_FILES=lwconfig.header.xml lwconfig.body.xml lwconfig.footer.xml
endif

lwconfig.xml: $(LWCONFIG_FILES)
	cat $^ > lwconfig.xml
