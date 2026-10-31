<div align="center"><h1>anies-cli</h1></div>

<div align="center"><h4>Aplicación de consola multiplataforma para ver anime en español.</h4>
Proyecto inspirado por la excelente herramienta <a href="https://github.com/pystardust/ani-cli">ani-cli</a>.</div>

## Uso

```
anies-cli [OPCIONES] [BÚSQUEDA]

    OPCIONES:
        Sin argumentos, -h      Mostrar este mensaje de ayuda con las opciones disponibles
        -v, --version           Mostrar la versión del programa
        -e <número>             Especificar el número de episodio a reproducir

    Ejemplos de uso:
        anies-cli one piece
        anies-cli naruto -e 10
        anies-cli -e 24 bleach
        anies-cli jujutsu kaisen -e 9
```
## Demostración

[Demostración anies-cli](https://github.com/user-attachments/assets/2bf4a309-dfd2-4411-bcc9-af17bbaced64)

#### Características

- La herramienta hace scraping de Animeav1 para obtener la información. De momento se buscan los videos solo de dos servidores de los varios que ofrece la página; para versiones futuras se irán añadiendo más para asegurar lo mejor posible la disponibilidad de reproducción.
- Se puede especificar un episodio a reproducir en la misma línea de comandos.
- La búsqueda de un título está limitada a un máximo de 50 resultados.
- Al finalizar la reproducción de un episodio, se presenta un menú con opciones para controlar la reproducción (según aplique): reproducir el episodio siguiente o anterior, repetir episodio, y salir del programa.
- El programa está disponible para sistemas Linux y Windows de 64 bits; para macOS quizás en versiones futuras.

## Instalación

Descargar el ejecutable desde las [releases](https://github.com/KNZodiac24/anies-cli/releases) o compilar manualmente el proyecto; y añadir el ejecutable al PATH.

En Linux hay que descomprimir el archivo de las releases con `tar -xzvf anies-cli_linux_x86_64.tar.gz`. 

## Dependencias

- [curl](https://curl.se) (solo Linux): Puede que ya esté instalado, pero igual es mejor comprobarlo con `curl --version`; si no, se lo puede instalar mediante el gestor de paquetes que utilices.
- [mpv](https://mpv.io): Este es el reproductor de video que se utiliza. Por simplicidad y para generalizar, se busca el ejecutable de mpv en rutas definidas:
    - En Linux el ejecutable debe estar ubicado dentro de `/usr/bin/`. Varios gestores de paquetes lo tienen disponible y lo instalan en esa ruta; pero en caso de instalarlo de otra forma, hay que asegurarse de que esté ubicado ahí.
    - En Windows debe estar dentro de `C:\Program Files\MPV Player\`. Aquí sí hay más variedad para la instalación, pero recomiendo utilizar las builds de shinchiro (que también lo [recomienda](https://mpv.io/installation) mpv). Lo bueno es que se puede instalar fácilmente ejecutando `winget install shinchiro.mpv`, y por defecto lo ubica en la ruta mencionada.

## Desarrollo y compilación

### Requisitos (Linux/Windows)

- CMake
- [Ninja](https://ninja-build.org/)
- Compilador de C++
> [!NOTE] 
> El compilador que se utiliza por defecto al ejecutar el build es `g++`. Para el caso de Windows, se puede utilizar el ejecutable de `g++` que viene incluido en la suite de herramientas y paquetes `MinGW-W64-builds` de [Mingw-w64](https://www.mingw-w64.org/downloads/). Descarga la versión x86_64 de win32 ucrt y añade la carpeta `bin` al PATH.
- Make
> [!NOTE] 
> Para Windows se puede utilizar el ejecutable `mingw32-make` que también viene incluido en la suite de herramientas de Mingw-w64.
- Vcpkg [instalado y configurado](https://learn.microsoft.com/vcpkg/get_started/get-started) (seguir las instrucciones solo hasta el primer literal del paso 2); con la variable de entorno `VCPKG_ROOT` definida.

#### Sobre las dependencias de desarrollo

En el vcpkg.json se definen las dependencias que se necesitan para realizar el desarrollo:

- **ctre**: Para la búsqueda y verificación de texto mediante expresiones regulares.
- **curlpp**: Wrapper de curl para simplificar su uso.
- **botan**: Para la desencriptación de respuestas a ciertas solicitudes de algunos servers.
- **boost-process**: Para la ejecución controlada del reproductor.
- **curl** (solo Windows): Para el acceso a las funcionalidades de libcurl, las cuales utiliza curlpp para funcionar. En Linux se tiene acceso a libcurl al instalar curl.

### Proceso 

1. Clonar el repo y ejecutar en la carpeta raíz:
    - Linux:
        ```
        make setup
        ```
    - Windows:
        ```
        mingw32-make setup
        ```
    Con esto ya se puede realizar el desarrollo.

2. Para compilar el proyecto se ejecuta en la carpeta raíz:
    - Linux:
        ```
        make build
        ```
    - Windows:
        ```
        mingw32-make build
        ```
    Por defecto, el ejecutable resultante se ubicará en la carpeta `build`.
