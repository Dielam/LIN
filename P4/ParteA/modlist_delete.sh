#! /bin/bash
#Autores: Francisco Denis y Diego Laguna

#Script para borrar y mostrar elementos de la lista y sacar el modulo

for i in `seq 1 100`
do 
echo remove i > /proc/modlist
cat /proc/modlist
sleep 1
done 

#Limpiamos
#make clean
#Sacamos el modulo
#sudo rmmod modlist
#Mostramos log del kernel
#dmesg | tail
