# Get all .c files in the current directory automatically
set(sources 
    "wifi_sta.c" 
    )
    
file(GLOB SOURCES *.c)

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS "include"    # Public headers (other components can see these)
    REQUIRES esp_wifi esp_netif
)