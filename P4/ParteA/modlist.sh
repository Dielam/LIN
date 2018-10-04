#! /bin/bash
#Autores: Francisco Denis y Diego Laguna

#Script para cargar el modulo e introducir elementos en la lista

#Pruebas
for i in `seq 1 100`
do
echo add i > /proc/modlist
sleep 1
done

