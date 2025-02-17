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

```shell
./ls
```

```shell
./cp <source file> <destination file>
```

```shell
./timeout <max duration in seconds> <command> <command argument>
```

Para **eliminar** los ejecutables correr el comando:

```shell
make clean
```

Para **formatear** el código fuente:

```shell
make format
```

También se ofrece ejecución contenerizada con Docker, donde se pueden ejecutar los siguientes comandos:

Para crear el container ejecutar:

```shell
make docker-build
```

Para ejecutar una shell dentro de Docker, lista para compilar y ejecutar los challenges (como se describió anteriormente):

```shell
make docker-run
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
- `-`: si es un archivo

Los permisos tienen la forma estándar:

```
<read perm user><write perm user><execute perm user> <read perm group><write perm group><execute perm group> <read perm others><write perm others><execute perm others>
```

Donde si el permiso correspondiente no está presente el caracter mostrado es: `-`.

El `[link destination]` se muestra sólo en el caso de las entidades que son links.

### cp

Copia un archivo, denominado archivo fuente, en una ubicación con nombre especificado, archivo denominado cono destino.

### timeout

Realiza una ejecución de un segundo proceso, y espera una cantidad de tiempo prefijada. Si se excede ese tiempo y el proceso sigue en ejecución, lo termina enviándole SIGTERM. Si el proceso termina antes, el programa finaliza.

En la invocación `<command argument>` se refiere a un argumento que será pasado al proceso a monitorear `<command>`. Se ofrece como ejemplo un programa `infloop` que cicla de manera indefinida imprimiendo un mensaje por pantalla cada cierto tiempo. Para probar timeout, también de podría ejecutar algo como: `./timeout 3 ping google.com`.
