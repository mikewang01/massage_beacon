#!/bin/bash
make COMPILE=gcc BOOT=$boot APP=$app SPI_SPEED=$spi_speed SPI_MODE=$spi_mode SPI_SIZE=$spi_size
cp $bin_path/$binfile1 $bin_path/user1.bin