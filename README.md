# Tarea 2

## Requerimientos
- glibc (para `argp`)
- Instalar [`libmicrohttpd`](https://www.gnu.org/software/libmicrohttpd/)

Por ejemplo en arch linux:
```
sudo pacman -S libmicrohttpd
```

## Instrucciones de uso
Para compilar el servidor corra el comando `make` estando en la carpeta "root" del proyecto. Luego, para ejecutar el servidor corra de solo corra `./server.exe`.

## Compilación para LSP (clangd)
Si utiliza el LSP `clangd` puede compilar el `compile_commands.json` utilizando [bear](https://github.com/rizsotto/Bear):

```
make clean; bear -- make
```
