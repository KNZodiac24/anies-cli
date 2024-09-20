import re
import requests
import sys
import io
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')


SITIO_ANIME = "https://animefenix.tv/"

anime = "goul"

anime_busqueda = anime.replace(" ", "+")

sitio_busqueda = requests.get(SITIO_ANIME+"animes?q="+anime_busqueda).text

patron = r"title=\".*\""

lista_resultado_busqueda = re.findall(patron, str(sitio_busqueda))
lista_sin_repetidos = list(set(lista_resultado_busqueda))

if lista_sin_repetidos.__len__() > 0:

    for resultado in lista_sin_repetidos:
        resultado = resultado.replace('title="', "")
        resultado = resultado.replace('"', "")
        print(resultado)

else:
    print("No se han encontrado resultados")
