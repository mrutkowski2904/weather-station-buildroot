DISPLAY_APP_VERSION = 1.0
DISPLAY_APP_SITE = $(BR2_EXTERNAL)/package/display_app
DISPLAY_APP_SITE_METHOD = local

$(eval $(cmake-package))