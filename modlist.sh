#! /bin/bash
#Autores: Francisco Denis y Diego Laguna
#Intalar Ftrace
#Entramos en el directorio
cd Documentos/P1/modlist
#Limpiamos
make clean
#Hacemos make
make
#Metemos el modulo
sudo insmod modlist.ko
#Pruebas
echo add 10 > /proc/modlist
echo add 16 > /proc/modlist
echo add 17 > /proc/modlist
echo add 1 > /proc/modlist
echo add 19 > /proc/modlist
cat /proc/modlist
echo remove 1 > /proc/modlist
cat /proc/modlist
#echo cleanup > /proc/modlist
cat /proc/modlist
#Limpiamos
make clean
#Sacamos el modulo
sudo rmmod modlist
#Mostramos log del kernel
dmesg | tail

