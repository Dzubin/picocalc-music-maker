# SD Card

This driver allows file systems to talk to the SD Card at the block-level. Functions provided configure the Pico to talk to the SD card as described in the PicoCalc schematic, initialize the SD card itself, and read/write blocks to the SD Card.

This driver abstracts away differences between SDSC and SDHC cards.


## sd_card_init

`sd_error_t sd_card_init(void)`

Initialise the SD card itself. The SD card must be initialised before blocks can reads and writes can occur. Returns SD_OK if successful, an error code if not.


## sd_card_present

`bool sd_card_present(void)`

Returns true is a SD card is inserted into the PicoCalc.


## sd_init

`void sd_init(void)`

Initialise the Pico to communicate with the SD card.


## sd_is_sdhc

`bool sd_is_sdhc(void)`

After the SD card is initialised, returns true if the inserted card is SDSC (Standard Capacity) or SDHC (High Capacity). This is for informational purposes.

SDSC cards have different block addressing than SDHC cards. The block-level functions use block numbers to address a block and adjust to whether an SDSC or an SDHC card was inserted into the PicoCalc.

## sd_read_block

`sd_error_t sd_read_block(uint32_t block, uint8_t *buffer)`

Reads a block from the SD card. Returns SD_OK if successful, an error code if not.

### Parameters

- block – The block number to read
- buffer – The buffer where to store the data from the block (must be `SD_BLOCK_SIZE` in size)

## sd_write_block

`sd_error_t sd_write_block(uint32_t block, const uint8_t *buffer)`

Write a block to the SD card. Returns SD_OK if successful, an error code if not.

### Parameters

- block – The block number to read
- buffer – The buffer of data to write to the block (must be `SD_BLOCK_SIZE` in size)


## sd_read_blocks

`sd_error_t sd_read_blocks(uint32_t start_block, uint32_t num_blocks, uint8_t *buffer)`

Reads a continuous series of blocks from the SD card. 

### Parameters

- start_block – The first block number of the series to read
- num_blocks – The number of blocks to read
- buffer - The buffer to store the data from the SD card (must be at least `num_blocks * SD_BLOCK_SIZE` in size)


## sd_write_blocks

`sd_error_t sd_write_blocks(uint32_t start_block, uint32_t num_blocks, const uint8_t *buffer)`

Writes a continuous series of blocks to the SD card. 

### Parameters

- start_block – The first block number of the series to read
- num_blocks – The number of blocks to read
- buffer - The buffer of data to write to the SD card (must be at least `num_blocks * SD_BLOCK_SIZE` in size)


## sd_error_string

`const char *sd_error_string(sd_error_t error)`

Returns an English translation of the numeric error code.
