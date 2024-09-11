# Lab Challenges

## Compilación y ejecutables

Para **compilar** correr el comando:

```shell
make
```

Una vez compilado, se pueden **ejecutar** los siguientes programas:

```shell
./ps
```

```shell
./find [-i] <phrase>
```

Para **eliminar** los ejecutables correr el comando:

```shell
make clean
```

Para **formatear** el código fuente:

```shell
make format
```

## Más información sobre los ejecutables

### ps (process status)

Muestra información básica de los procesos que están corriendo en el sistema. Equivale al comando:

```shell
ps -eo pid,comm
```

La implementación muestra el pid y comando (i.e. argv) de cada proceso.

### find

Invocado como `./find xyz`, el programa buscará y mostrará por pantalla todos los archivos del directorio actual (y subdirectorios) cuyo nombre contenga (o sea igual a) xyz. Si se invoca como `./find -i xyz`, se realizará la misma búsqueda, pero sin distinguir entre mayúsculas y minúsculas.

### ls

Información del output:

```
<filetype> <permissions> <owner id> <owner name>  <filename> [link destination]
```

Donde filetype toma los valores:
- `d`: si es un directorio
- `l`: si es un link
- `-`:  si es un archivo

Los permisos tienen la forma estándar:
```
<read perm user><write perm user><execute perm user> <read perm group><write perm group><execute perm group> <read perm others><write perm others><execute perm others>
```
Donde si el permiso correspondiente no está presente el caracter mostrado es: `-`.

El `[link destination]` se muestra sólo en el caso de las entidades que son links.
