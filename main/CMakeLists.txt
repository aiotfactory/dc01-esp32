idf_component_register(
SRC_DIRS "." "driver" "driver/lora" "driver/ble" "driver/mlx90640"
INCLUDE_DIRS "." "driver" "driver/lora" "driver/ble" "driver/mlx90640"
)

spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)

#for lib compile x
add_prebuilt_library(prebuilt "prebuilt/libcomm_core.a" PRIV_REQUIRES esp_http_client app_update esp_partition esp_app_format bootloader bootloader_support main)
target_link_libraries(${COMPONENT_LIB} PRIVATE prebuilt)