UI_APP_VERSION = 1.0
UI_APP_SITE = $(BR2_EXTERNAL)/package/ui_app
UI_APP_SITE_METHOD = local

$(eval $(cmake-package))