/*
 *  anies-cli: Aplicación de consola para ver anime en español
 *  Copyright (C) 2024-2026 KNZodiac24
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <fstream>
#include <iostream>
#include <regex>
// #include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <ctre.hpp>

//////// Preliminares ////////

void mostrar_ayuda() {
    std::cout << R"(
    anies-cli [OPCIONES] [BÚSQUEDA]

    OPCIONES:
        Sin argumentos, -h      Mostrar este mensaje de ayuda con las opciones disponibles
        -v, --version           Mostrar la versión del programa
        --vlc                   Usar VLC como reproductor. Se utiliza MPV por defecto
        -e <número>             Especificar el número de episodio a reproducir

    Ejemplos de uso:
        anies-cli one piece -v
        anies-cli naruto
        anies-cli -e 24 bleach
        anies-cli jujutsu kaisen -e 9)" << '\n';
}

bool verificar_argumento_episodio(const std::string& arg_episodio);

std::string consultar_sitio(const std::string& sitio);

std::unordered_map<std::string, std::string> generar_lista_animes_encontrados(const std::string& respuesta_sitio);

//////// Main ////////

constexpr std::string_view VERSION_PROGRAMA { "0.2.0" };
const std::string SITIO_ANIME { "https://animeav1.com/" };
constinit std::string_view REPRODUCTOR { "mpv" };
constinit std::string ANIME {};
constinit int EPISODIO {};

int main(int argc, char *argv[]) {

    if (argc == 1) {
        mostrar_ayuda();
        exit(EXIT_SUCCESS);
    }

    std::vector<std::string> argumentos { argv, argv+argc };
    argumentos.erase(std::remove(argumentos.begin(), argumentos.end(), argumentos[0]), argumentos.cend());

    for(std::size_t i {}; i < argumentos.size(); ++i){
        const std::string& op { argumentos[i] };
        if (op == "-h") {
            mostrar_ayuda();
            exit(EXIT_SUCCESS);
        }
        else if (op == "-v" || op == "--version") {
            std::cout << VERSION_PROGRAMA << '\n';
            exit(EXIT_SUCCESS);
        }
        else if (op == "--vlc"){
            REPRODUCTOR = "vlc";
            continue;
        }
        else if (op == "-e") {
            if (static_cast<int>(i+1) < argc && verificar_argumento_episodio(argumentos[i+1])) { 
                EPISODIO = std::stoi(argumentos[i+1]);
                argumentos.erase(std::remove(argumentos.begin(), argumentos.end(), argumentos[i+1]), argumentos.cend());
                continue;
            } else {
                std::cout << "Por favor, especifica el número de episodio correctamente.";
                mostrar_ayuda();
                exit(EXIT_FAILURE);
            }
        }
        else {
            ANIME += op + '+';
        }
    }

    const std::string respuesta_sitio { consultar_sitio(SITIO_ANIME+"catalogo?search="+ANIME) };

    const std::unordered_map<std::string, std::string> animes_encontrados { generar_lista_animes_encontrados(respuesta_sitio) };

    return 0;
}

/////// Definiciones ///////

bool verificar_argumento_episodio(const std::string& arg_episodio) {
    try {
        int num_temp { std::stoi(arg_episodio) };
        
        if(num_temp > 0) return true;

        std::cout << "\nEl número de episodio es menor al mínimo permitido.\n";

        return false;

    } catch(std::invalid_argument const& ex) {
        std::cout << "\nEl argumento de episodio ingresado no es válido.\n";
    } catch(std::out_of_range const& ex) {
        std::cout << "\nEl número de episodio ingresado es mayor al máximo permitido.\n";
    }

    return false;
}

std::string consultar_sitio(const std::string& sitio) {
    /*std::ostringstream os {};
    curlpp::options::WriteStream ws{&os};

    try {
		curlpp::Easy req {};

		req.setOpt<curlpp::options::Url>(sitio);

        req.setOpt(ws);

		req.perform();
	} catch(const curlpp::RuntimeError& e) {
		std::cout << e.what() << '\n';
	} catch(const curlpp::LogicError& e) {
		std::cout << e.what() << '\n';
	}

    return os.str();*/

    std::string salida {};

    std::ifstream inf{ "text.txt" };

    if (!inf)
    {
        std::cerr << "Error al leer archivo\n";
        exit(EXIT_FAILURE);
    }

    std::string linea {};
    while (std::getline(inf, linea)) { 
        salida += linea + '\n';
    }

    return salida;
}

std::unordered_map<std::string, std::string> generar_lista_animes_encontrados(const std::string& respuesta_sitio) {

    std::unordered_map<std::string, std::string> animes_encontrados {};
 
    int numero_ultima_pagina { 1 };

    // Verificar paginación
    if (respuesta_sitio.contains("\">»</a>")) {
        auto busqueda_numero_paginas { ctre::search_all<"\">[0-9]+</a><!--]">(respuesta_sitio) };

        std::string ultimo_resultado {};

        for (const auto& r: busqueda_numero_paginas) {
            ultimo_resultado = r;
        }

        ultimo_resultado = std::regex_replace(ultimo_resultado, std::regex{"\">"}, "");
        ultimo_resultado = std::regex_replace(ultimo_resultado, std::regex{"</a><!--]"}, "");
        numero_ultima_pagina = std::stoi(ultimo_resultado);
    }

    auto extraer_titulo_y_link = [](const std::string& pagina_fuente) {
        std::unordered_map<std::string, std::string> titulos_con_links {};

        auto busqueda_resultados_pagina { ctre::search_all<"title:\".+?\",synopsis:\".+?\",categoryId:.+?,slug:\".+?\"">(pagina_fuente) };

        for (const auto& resultado: busqueda_resultados_pagina) {
            std::string r { resultado.begin(), resultado.end() };

            r = std::regex_replace(r, std::regex{"title:\""}, "");
            r = std::regex_replace(r, std::regex{"\",synopsis:\".+?,slug:\""}, "%");
            r = std::regex_replace(r, std::regex{"\""}, "");

            std::string titulo { r.substr(0, r.find('%')) };
            std::string link { r.substr(r.find('%')+1, r.size()-1) };

            // Este bloque es para solo añadir una verificacón de que lo ingresado por el usuario salga en la búsqueda,
            // ya que por alguna razón animeav1 suele mostrar resultados que no tienen que ver
            std::string temp_titulo { titulo };
            std::transform(
                temp_titulo.begin(),
                temp_titulo.end(),
                temp_titulo.begin(),
                [](unsigned char c){ return std::tolower(c); }
            );
            std::string temp_anime { std::regex_replace(ANIME, std::regex{"\\+"}, " ") }; 
            temp_anime = temp_anime.substr(0, temp_anime.size()-1);

            if (!temp_titulo.contains(temp_anime)) continue;

            titulos_con_links.insert({titulo, link});
        }

        return titulos_con_links;
    }; 

    auto temp { extraer_titulo_y_link(respuesta_sitio) };

/*
    for (int i { 1 }; i <= numero_ultima_pagina; ++i) {
        
    }
*/
    return animes_encontrados;
}

