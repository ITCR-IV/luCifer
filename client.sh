#!/bin/bash

if [ -z "$ADDRESS_URI" ]; then
	echo "\$ADDRESS_URI no está seteado, por favor defina esta variable de entorno con la dirección del servidor"
	exit 1
fi

echo "ADDRESS: $ADDRESS_URI"

echo "Bienvenido al cliente del ImageServer."
echo ""
echo "Puede utilizar alguno de los siguientes comandos:"
echo "  exit              Salir de la aplicación"
echo "  e [IMAGEN]        Decirle al servidor que realice ecualización de"
echo "  c [IMAGEN]        Decirle al servidor que clasifique la imagen basado"
echo "                    en el color más predominante"
echo ""

while true; do
	read -rp "> " COMMAND

	if [ -n "$COMMAND" ]; then
		BASE_CMD=$(echo "$COMMAND" | awk '{print $1;}')
		ARG=$(echo "$COMMAND" | awk '{print $2;}')
		ARG="${ARG/#\~/$HOME}" # Expand ~/ into $HOME/

		case $BASE_CMD in
			e) 
				URL_PATH="/equalize"
				;;
			c)
				URL_PATH="/classify/rgb"
				;;
			exit)
				exit 0
				;;
			*)
				echo "Comando '$BASE_CMD' no reconocido, por favor intente de nuevo"
		esac

		if [ -r "$ARG" ] && [ -f "$ARG" ]; then
			FILE=$(realpath "$ARG")
			RESPONSE=$(curl "${ADDRESS_URI}${URL_PATH}" -F "file=@$FILE")
			echo ""
			echo "ImageServer: $RESPONSE"
			
		elif [ ! -e "$ARG" ]; then
			echo "No existe el archivo '$ARG'"
		elif [ ! -f "$ARG" ]; then
			echo "'$ARG' no es un archivo regular"
		elif [ ! -r "$ARG" ]; then
			echo "No se tiene permiso de leer '$ARG'"
		fi

	fi
done
