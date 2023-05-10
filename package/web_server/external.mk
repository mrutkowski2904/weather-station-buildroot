WEB_SERVER_VERSION = 1.0
WEB_SERVER_SITE = $(BR2_EXTERNAL)/package/web_server
WEB_SERVER_SITE_METHOD = local

$(eval $(cmake-package))