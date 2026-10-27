"""Public profile identifiers and backward-compatible personal aliases.

Identifiers describe a tested MCU/display/touch combination AND its supplied
pin mapping. They are not universal drivers for arbitrary boards with those
chips. Numeric IDs remain stable so existing C/C++ hardware branches are kept.
"""
import argparse

PROFILES = {
    'esp32_ili9341_xpt2046': (1, 'ESP32 Dev Module', 'esp32:esp32:esp32'),
    'esp32_ili9341_cap_touch': (2, 'ESP32 Dev Module', 'esp32:esp32:esp32'),
    'esp32s3_axs15231b_qspi': (3, 'ESP32S3 Dev Module',
        'esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,CDCOnBoot=cdc,PartitionScheme=app3M_fat9M_16MB'),
    'esp32_st7789_parallel_cst820': (4, 'ESP32 Dev Module', 'esp32:esp32:esp32'),
}
ALIASES = {
    'amarela2.8': 'esp32_ili9341_xpt2046',
    'pink': 'esp32_ili9341_cap_touch',
    'black': 'esp32s3_axs15231b_qspi',
    'preta': 'esp32s3_axs15231b_qspi',
    'amarela2.2': 'esp32_st7789_parallel_cst820',
}


def canonical_name(name):
    """Normalize a public identifier or alias; reject unknown profiles."""
    name = name.strip().lower()
    name = ALIASES.get(name, name)
    if name not in PROFILES:
        raise argparse.ArgumentTypeError('Unknown profile. Choose: ' + ', '.join(PROFILES))
    return name


def ide_settings(board):
    """Return user-facing Tools menu settings shared by CLI and GUI selectors."""
    board=canonical_name(board)
    board_id,ide_name,_=PROFILES[board]
    settings=[('Board',ide_name)]
    if board_id==3:
        settings.extend([
            ('Flash Size','16MB (128Mb)'),
            ('PSRAM','OPI PSRAM'),
            ('USB CDC On Boot','Enabled'),
            ('Partition Scheme','16M Flash (3MB APP/9.9MB FATFS)'),
        ])
    else:
        settings.extend([
            ('Flash Size','4MB'),
            ('PSRAM','Disabled'),
            ('Partition Scheme','Default 4MB with spiffs'),
        ])
    return settings
