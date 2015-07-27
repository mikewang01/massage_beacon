bin_path="../bin/upgrade"
binfile1=user1.1024.new.bin
binfile2=user2.1024.new.bin
boot=new
ftp_path=~/cling_ftp/Files
spi_speed=40
spi_mode=QIO
spi_size=1024

echo $bin_path
echo "generating user1"
echo " "
rm -f $bin_path/$binfile1 
app=1
touch user/user_main.c
rm -f $bin_path/$binfile1
rm -f $bin_path/user1.bin
make COMPILE=gcc BOOT=$boot APP=$app SPI_SPEED=$spi_speed SPI_MODE=$spi_mode SPI_SIZE=$spi_size
cp $bin_path/$binfile1 $bin_path/user1.bin

echo "generating user2"
echo " "

app=2
touch user/user_main.c
rm -f $bin_path/$binfile2
rm -f $bin_path/user2.bin
make COMPILE=gcc BOOT=$boot APP=$app SPI_SPEED=$spi_speed SPI_MODE=$spi_mode SPI_SIZE=$spi_size
cp $bin_path/$binfile2  $bin_path/user2.bin

echo "copy files to ftp"
sudo umount /home/esp8266/cling_ftp
sudo curlftpfs -o rw,allow_other ftp://192.168.1.208:7021  /home/esp8266/cling_ftp/
cp $bin_path/user2.bin  $ftp_path/
cp $bin_path/user1.bin  $ftp_path/
echo "copy finished"