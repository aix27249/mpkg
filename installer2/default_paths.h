#ifndef SETUP2_DEFAULT_PATHS_H__
#define SETUP2_DEFAULT_PATHS_H__

// Basic directories
#define SETUP_ROOT string("/usr/lib/setup/")
#define SETUP_BASIC_CONF SETUP_ROOT+string("basic_conf/")
#define SETUP_CONFIG_DIR string("/tmp/setup_config/")
#define SETUP_PKGSET_CACHE_DIR string("/tmp/setup_variants.cache/")
#define SETUP_PKGSET_DIR SETUP_BASIC_CONF+string("setup_variants/")
#define SETUP_INDEX_CACHE string("/tmp/setup_cache/")

// Files that contains available options (basic conf)
#define SETUP_LANGUAGES SETUP_BASIC_CONF+string("languages.conf")
#define PARTITION_EDITORS SETUP_BASIC_CONF+string("partition_editors.conf")

// Generated configuration files
#define SETUPCONFIG_LANGUAGE SETUP_CONFIG_DIR+string("language")
#define SETUPCONFIG_PARTITIONEDITOR SETUP_CONFIG_DIR+string("partition_editor")
#define SETUPCONFIG_SWAP SETUP_CONFIG_DIR+string("swap")
#define SETUPCONFIG_ROOT SETUP_CONFIG_DIR+string("root")
#define SETUPCONFIG_FORMATTING SETUP_CONFIG_DIR+string("formatting")
#define SETUPCONFIG_MOUNT SETUP_CONFIG_DIR+string("mountpoints")
#define SETUPCONFIG_PKGSOURCE SETUP_CONFIG_DIR+string("pkgsource")
#define SETUPCONFIG_REPOSITORYLIST SETUP_CONFIG_DIR+string("repositorylist")
#define SETUPCONFIG_TEMPMOUNTS SETUP_CONFIG_DIR+string("tempmounts")
#define SETUPCONFIG_PKGSET SETUP_CONFIG_DIR+string("pkgset")
#define SETUPCONFIG_CDROM SETUP_CONFIG_DIR+string("cdrom")
#define SETUPCONFIG_BOOTLOADER SETUP_CONFIG_DIR+string("bootloader")
#define SETUPCONFIG_BOOTLOADER_TARGET SETUP_CONFIG_DIR+string("bootloader_target")
#define SETUPCONFIG_FRAMEBUFFER SETUP_CONFIG_DIR+string("framebuffer")
#define SETUPCONFIG_ALTERNATIVES SETUP_CONFIG_DIR+string("alternatives")
#define SETUPCONFIG_NVIDIA SETUP_CONFIG_DIR+string("nvidia_driver")


#endif
