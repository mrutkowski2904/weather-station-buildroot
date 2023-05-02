LPS25HB_VERSION = 1.0
LPS25HB_SITE = $(BR2_EXTERNAL)/package/lps25hb
LPS25HB_SITE_METHOD = local
$(eval $(kernel-module))
$(eval $(generic-package))