list(APPEND SOURCES
        proxy_common.c
	# proxy_common_unittest.cpp
        proxy_http.c
        proxy_http_connector.c
        proxy_http_rewriter.c
        proxy_setup.cpp
        ProxyUtils.cpp
	# ProxyUtils_unittest.cpp
	)

add_library(proxy ${SOURCES})


