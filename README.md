# luCifer

Este proyecto es para la Tarea 2 del curso Principios de Sistemas Operativos en el segundo semestre del 2023.

## Requerimientos
- glibc (para [`argp`](https://www.gnu.org/software/libc/manual/html_node/Argp.html))
- Instalar [`libmicrohttpd`](https://www.gnu.org/software/libmicrohttpd/)
- Instalar [`freeimage`](https://freeimage.sourceforge.io/)

Por ejemplo en arch linux:
```
sudo pacman -S libmicrohttpd freeimage
```

## Instrucciones de uso

### Servidor
Para compilar el servidor corra el comando `make` estando en la carpeta "root" del proyecto. Luego, para ejecutar el servidor corra `./server.exe`.

### Cliente
Para enviar imágenes al servidor puede utilizar el cliente `/client/client.sh`.

Para que el cliente funcione debe setear la dirección del servidor a través de la variable de entorno `ADDRESS_URI`.
Por default la dirección del servidor, si se ejecuta localmente, será `localhost:1717`. Entonces puede ejecutar el cliente con:

```
ADDRESS_URI=localhost:1717 client/client.sh
```

#### Cliente en contenedor
El cliente también se puede usar dentro de un contenedor Docker.
Si el servidor se está ejecutando localmente, ingrese al folder `client` y ejecute:

```
docker run --add-host host.docker.internal:host-gateway \
  --env ADDRESS_URI=http://host.docker.internal:1717 \
  --rm -v $HOME:/home/ -it $(docker buildx build -q .)
```

Asegúrese de escoger el puerto correcto en la variable `ADDRESS_URI`.
Este comando monta su $HOME al directorio /home para que pueda buscar entre sus archivos guardados.

Si el servidor se está ejecutando en otro URI puede simplificar un poco el commando:
```
docker run --env ADDRESS_URI=http://server-address.com \
  --rm -v $HOME:/home/ -it $(docker buildx build -q .)
```

Asegúrese de poner la dirección correcta en la variable `ADDRESS_URI`.

## Compilación para LSP (clangd)
Si utiliza el language server `clangd` puede compilar el `compile_commands.json` utilizando [bear](https://github.com/rizsotto/Bear):

```
make clean; bear -- make
```
