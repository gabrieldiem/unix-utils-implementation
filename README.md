# Lab Challenges

## Compilación y ejecutables

Para **compilar** correr el comando:

``` shell
make
```

Una vez compilado, se pueden **ejecutar** los siguientes programas:

``` shell
./ps
```

Para **eliminar** los ejecutables correr el comando:

``` shell
make clean
```

Para **formatear** el código fuente:

``` shell
make format
```

## Más información sobre los ejecutables

### ps (process status)

Muestra información básica de los procesos que están corriendo en el sistema. Equivale al comando: `ps -eo pid,comm`. La implementación muestra el pid y comando (i.e. argv) de cada proceso.
