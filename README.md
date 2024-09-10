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
