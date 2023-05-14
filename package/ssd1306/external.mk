SSD1306_VERSION = 1.0
SSD1306_SITE = $(BR2_EXTERNAL)/package/ssd1306
SSD1306_SITE_METHOD = local
$(eval $(kernel-module))
$(eval $(generic-package))