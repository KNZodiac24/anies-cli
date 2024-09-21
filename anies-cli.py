import re
import requests
import sys
import io

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')

def mostrar_ayuda():
    msg = """
    anies-cli [OPCIONES] [BÚSQUEDA]

    OPCIONES:
        Sin argumentos, -h      Mostrar este mensaje de ayuda con las opciones disponibles y finalizar
        -v, --vlc               Usar VLC como reproductor
        -V, --version           Mostrar la versión del script
        -e <número>             Especificar el número de episodio a reproducir
        -d                      Descargar un episodio 

    Ejemplos de uso:
        anies-cli one piece -v
        anies-cli naruto
        anies-cli -e 24 bleach
        anies-cli -d jujutsu kaisen -e 9
    """
    print(msg)


if sys.argv.__len__() == 1:
    mostrar_ayuda()
    exit(0)



VERSION_SCRIPT = "0.0.1"
REPRODUCTOR = "mpv"
EPISODIO = None
DESCARGAR = False
ANIME = ""
SITIO_ANIME = "https://animefenix.tv/"

if sys.argv.__len__() > 1:
    for op in sys.argv:
        if op == "-h":
            mostrar_ayuda()
            exit(0)

        elif op in ["-V", "--version"]:
            print(VERSION_SCRIPT)
            exit(0)

        elif op in ["-v", "--vlc"]:
            REPRODUCTOR = "vlc"
            continue

        elif op == "-d":
            DESCARGAR = True
            continue
        
        elif op == "-e":
            if sys.argv.index(op)+1 < sys.argv.__len__() and sys.argv[sys.argv.index(op)+1].isdecimal(): 
                EPISODIO = sys.argv[sys.argv.index(op)+1]
                sys.argv.remove(sys.argv[sys.argv.index(op)+1])
                continue
            else:
                print("\nPor favor, especificar el número de episodio.")
                mostrar_ayuda()
                exit(0)

        elif op != sys.argv[0]:
            ANIME += op+"+"


sitio_busqueda = requests.get(SITIO_ANIME+"animes?q="+ANIME).text

patron_busqueda = r"href=\".*\"\stitle=\".*\"\>"
lista_resultado_busqueda = re.findall(patron_busqueda, str(sitio_busqueda))
lista_sin_repetidos = list(set(lista_resultado_busqueda))
lista_sin_repetidos.sort()

LISTA_ANIMES = {}
SELECCION_ANIME = None

if lista_sin_repetidos.__len__() > 0:

    for resultado in lista_sin_repetidos:
        corregido = resultado.replace('href="', "")
        corregido = corregido.replace('title="', "")
        corregido = corregido.replace('"', "")
        if "&quot;" in corregido:
            corregido = corregido.replace("&quot;", '"')
        corregido = corregido.replace(">", "")
        temp = corregido.split(" ", 1)
        LISTA_ANIMES[temp[1]] = temp[0]

    for i, (clave, valor) in enumerate(LISTA_ANIMES.items(), start = 1):
        print(str(i)+". "+ clave) 
    
    seleccion_temp = input("Selecciona un anime de la lista: ")

    if not seleccion_temp.isdecimal():
        print("Por favor, ingresa una opción válida.")
        exit(0)
    else:
        if int(seleccion_temp) > LISTA_ANIMES.__len__():
            print("La selección no está dentro del rango admitido.")
            exit(0)
        else:
            SELECCION_ANIME = int(seleccion_temp)
    
else:
    print("No se han encontrado resultados")
    exit(0)


sitio_anime_seleccionado = requests.get(list(LISTA_ANIMES.values())[SELECCION_ANIME-1]).text
patron_busqueda_episodios = r"is-rounded\s\"\shref=\".*[0-9]+\"\>"
temp = re.search(patron_busqueda_episodios, str(sitio_anime_seleccionado))
ultimo_episodio = temp.group()

ultimo_episodio = re.search(r"\-[0-9]+\"\>", ultimo_episodio).group()
ultimo_episodio = ultimo_episodio.removeprefix("-")
num_episodios_anime = ultimo_episodio.removesuffix('">')


# TODO: Verificar casos (Ep indicado & Descargar), (Ep NO indicado & Descargar), (Ep indicado & NO Descargar), (Ep NO indicado & NO Descargar)

if EPISODIO is not None and DESCARGAR is False:
    print("ya se ha seleccionado un episodio: ",EPISODIO) 
else:

    SELECCION_EPISODIO= input(f"\n{list(LISTA_ANIMES.keys())[SELECCION_ANIME-1]} tiene {num_episodios_anime} episodio(s). Selecciona el número de episodio a reproducir: ")

    print(SELECCION_EPISODIO)














