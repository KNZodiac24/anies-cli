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

#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>

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

std::string realizar_consulta_sitio();

std::vector<std::string_view> generar_lista_animes_encontrados(const std::string& respuesta_sitio);

//////// Main ////////

constexpr std::string_view VERSION_PROGRAMA { "0.1.0" };
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

    const std::string respuesta_sitio { realizar_consulta_sitio() };

    const std::vector<std::string_view> animes_encontrados { generar_lista_animes_encontrados(respuesta_sitio) };

    return 0;
}

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

std::string realizar_consulta_sitio() {
    std::ostringstream os {};
    curlpp::options::WriteStream ws{&os};

    try {
		curlpp::Easy req {};

		req.setOpt<curlpp::options::Url>(SITIO_ANIME+"catalogo?search="+ANIME);

        req.setOpt(ws);

		req.perform();
	} catch(const curlpp::RuntimeError& e) {
		std::cout << e.what() << '\n';
	} catch(const curlpp::LogicError& e) {
		std::cout << e.what() << '\n';
	}

    return os.str();
}

std::vector<std::string_view> generar_lista_animes_encontrados(const std::string& respuesta_sitio) {
    
    std::vector<std::string_view> animes_encontrados {};

    // Primero determinar si hay paginación para encontrar todos los resultados posibles:
    // Buscar el patrón ">»</a>
    // Si no se encuentra entonces proseguir normalmente
    // Si se encuentra entonces buscar el patrón ">[0-9]{1,}</a><!--] y extraer el número del último resultado
    //
    // En caso de haber encontrado paginación, ir rotando en cada página haciendo el proceso de abajo, tomando
    // en cuenta el número extraido anteriormente como límite.
    //
    // Proceso: Leer el json disponible en la página para listar los resultados. En cada resultado verificar si 
    // se encuentra una coincidencia exacta (ignorecase) con el nombre ingresado por el usuario, ya que
    // por alguna razón animeav1 suele lanzar resultados que no tienen que ver. Ir acumulando los resultados en 
    // el array final.
    //
    // PS: si me animo les ordeno alfabéticamente

    return animes_encontrados;
}

