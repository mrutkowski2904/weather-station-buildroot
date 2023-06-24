MCP23008_VERSION = 1.0
MCP23008_SITE = $(BR2_EXTERNAL)/package/mcp23008
MCP23008_SITE_METHOD = local
$(eval $(kernel-module))
$(eval $(generic-package))