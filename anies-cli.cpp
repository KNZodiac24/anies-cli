/*
 *  anies-cli: Aplicación de consola multiplataforma para ver anime en español
 *  Copyright (C) 2024, 2026 KNZodiac24
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
 *
 *  Repo: https://github.com/KNZodiac24/anies-cli
 */

#include <iostream>
#include <iomanip>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <array>
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <ctre.hpp>
#include <botan/cipher_mode.h>
#include <botan/hex.h>
#include <boost/process.hpp>
#include <boost/asio.hpp>
#if defined(_WIN32)
#include <windows.h>
#endif
#include <ftxui/ftxui.hpp>

//////// Preliminares ////////

void mostrar_ayuda() {
    std::cout <<
R"(anies-cli [OPCIONES] [BÚSQUEDA]

    OPCIONES:
        Sin argumentos, -h      Mostrar este mensaje de ayuda con las opciones disponibles
        -v, --version           Mostrar la versión del programa
        -e <número>             Especificar el número de episodio a reproducir
        -u                      Verificar si el programa tiene una nueva versión disponible

    Ejemplos de uso:
        anies-cli one piece
        anies-cli naruto -e 10
        anies-cli -e 24 bleach
        anies-cli jujutsu -e 9 kaisen)" << '\n';
}

// De: https://stackoverflow.com/questions/46479513/trim-matching-characters-from-start-or-end-of-string-c
void trim(std::string& s, char c, bool reverse = false ) {
    if (reverse) s.erase( s.find_last_not_of( c ) + 1 );
    else s.erase( 0, s.find_first_not_of( c ) );
}

// De: https://gist.github.com/arthurafarias/56fec2cd49a32f374c02d1df2b6c350f
std::string encodeURIComponent(const std::string& decoded) {
    std::ostringstream oss;
    constexpr auto regex { ctre::match<"[A-Za-z0-9_\\.!~\\*'\\(\\)\\-]"> };

    for (const char &c: decoded) {
        if (regex(std::string{c})) {
            oss << c;
        } else {
            oss << "%"
                << std::uppercase
                << std::setw(2)
                << std::setfill('0')
                << std::hex
                << static_cast<unsigned char>(c);
        }
    }

    return oss.str();
} // No sé hasta qué punto es necesaria esta función en el lugar que se la usa, pero parece que siempre lo hace; así que por si acaso

bool verificar_argumento_numerico(const std::string& arg);

void verificar_version_disponible();

void validar_titulo_anime();

std::string consultar_sitio(const std::string& sitio, const std::list<std::string>& otros_headers = {}, bool verbose = false);

std::tuple<std::unordered_map<std::string, std::string>, bool, short> generar_lista_animes_encontrados(const std::string& respuesta_sitio);

void seleccionar_titulo(const std::unordered_map<std::string, std::string>& animes_encontrados, bool resultados_mas_del_limite, short numero_paginas_omitidas);

void seleccionar_episodio(const std::string& respuesta_titulo_seleccionado);

std::unordered_map<std::string, std::string> generar_servers_con_links(const std::string& respuesta_episodio_seleccionado);

std::vector<std::string> obtener_links_video(const std::unordered_map<std::string, std::string>& servers_con_links);

int exec_reproductor(const std::vector<std::string>& args);

void limpiar_consola();

void reproducir(const std::vector<std::string>& links_video_episodio);

enum class OpcionReproduccion {
    EPISODIO_ANTERIOR,
    SIGUIENTE_EPISODIO,
    SELECCIONAR_EPISODIO,
    REPETIR_EPISODIO,
    SALIR
};

OpcionReproduccion controlar_opciones_reproduccion();

//////// Main ////////

constexpr std::string VERSION_PROGRAMA { "1.0.7" };
const std::string SITIO_ANIME { "https://animeav1.com/" };
constinit std::string ANIME {};
constinit std::string TITULO_SELECCIONADO {};
constinit int TOTAL_EPISODIOS_ANIME {};
constinit int EPISODIO {};
// Para la reutilización de variables de la TUI
auto CONSOLA { ftxui::App::TerminalOutput() };
auto CONTAINER_CONSOLA { ftxui::Container::Vertical({}) };

int main(int argc, char *argv[]) {

    if (argc == 1) {
        std::cout << "Aplicación de consola multiplataforma para ver anime en español.\n\n";
        mostrar_ayuda();
        return EXIT_SUCCESS;
    }

    std::vector<std::string> argumentos { argv, argv+argc };
    argumentos.erase(std::remove(argumentos.begin(), argumentos.end(), argumentos[0]), argumentos.cend());

    for(std::size_t i {}; i < argumentos.size(); ++i){
        const std::string& op { argumentos[i] };
        if (op == "-h") {
            std::cout << "Aplicación de consola multiplataforma para ver anime en español.\n\n";
            mostrar_ayuda();
            return EXIT_SUCCESS;
        }
        else if (op == "-v" || op == "--version") {
            std::cout << VERSION_PROGRAMA << '\n';
            return EXIT_SUCCESS;
        }
        else if (op == "-e") {
            if (static_cast<int>(i+1) < argc && verificar_argumento_numerico(argumentos[i+1])) {
                EPISODIO = std::stoi(argumentos[i+1]);
                argumentos.erase(std::remove(argumentos.begin(), argumentos.end(), argumentos[i+1]), argumentos.cend());
                continue;
            } else {
                std::cerr << "Por favor, especifica el número de episodio correctamente.\n\n";
                mostrar_ayuda();
                return EXIT_FAILURE;
            }
        }
        else if (op == "-u") {
            verificar_version_disponible();
            return EXIT_SUCCESS;
        }
        else {
            if (op.contains("+")) {
                ANIME += std::regex_replace(op, std::regex{"\\+"}, "%2B") + '+';
            } else if (op.contains(' ')) {
                ANIME += std::regex_replace(op, std::regex{" "}, "+");
            } else if (op.contains('%')) {
                std::cerr << "Error: El título ingresado no es válido.\n";
                return EXIT_FAILURE;
            } else {
                ANIME += op + '+';
            }
        }
    }

    try {
        validar_titulo_anime();

        const std::string respuesta_sitio { consultar_sitio(SITIO_ANIME+"catalogo?search="+ANIME) };

        const std::tuple<std::unordered_map<std::string, std::string>, bool, short> animes_encontrados { generar_lista_animes_encontrados(respuesta_sitio) };

        seleccionar_titulo(std::get<0>(animes_encontrados), std::get<1>(animes_encontrados), std::get<2>(animes_encontrados));

        const std::string link_titulo_seleccionado { SITIO_ANIME+"media/"+std::get<0>(animes_encontrados).at(TITULO_SELECCIONADO) };

        const std::string respuesta_titulo_seleccionado { consultar_sitio(link_titulo_seleccionado) };

        auto opcion_reproduccion { OpcionReproduccion::SELECCIONAR_EPISODIO };

        while (opcion_reproduccion != OpcionReproduccion::SALIR) {
            if (opcion_reproduccion == OpcionReproduccion::SELECCIONAR_EPISODIO) seleccionar_episodio(respuesta_titulo_seleccionado);

            const std::string link_episodio_seleccionado { link_titulo_seleccionado+'/'+std::to_string(EPISODIO) };

            const std::string respuesta_episodio_seleccionado { consultar_sitio(link_episodio_seleccionado) };

            const std::unordered_map<std::string, std::string> servers_con_links { generar_servers_con_links(respuesta_episodio_seleccionado) };

            const std::vector<std::string> links_video_episodio { obtener_links_video(servers_con_links) };

            reproducir(links_video_episodio);

            opcion_reproduccion = controlar_opciones_reproduccion();
        }
    } catch(const std::exception& e) {
        std::cerr << e.what();
        return EXIT_FAILURE;
    }

    limpiar_consola();

    return EXIT_SUCCESS;
}

/////// Definiciones ///////

bool verificar_argumento_numerico(const std::string& arg) {
    try {
        constexpr auto match_valor_numerico { ctre::match<"[0-9]+"> };
        if (!match_valor_numerico(arg)) {
            std::cerr << "Error: El valor ingresado no es válido.\n";
            return false;
        }

        int num_temp { std::stoi(arg) };

        if (num_temp > 0) return true;

        std::cerr << "Error: El número ingresado es menor al mínimo permitido.\n";

        return false;

    } catch(std::invalid_argument const& ex) {
        std::cerr << "Error: El valor ingresado no es válido.\n";
    } catch(std::out_of_range const& ex) {
        std::cerr << "Error: El número ingresado es mayor al máximo permitido.\n";
    }

    return false;
}

void verificar_version_disponible() {
    constexpr auto busqueda_nombre_version { ctre::search<"\"name\": \"v[0-9]+\\.[0-9]+\\.[0-9]+\""> };

    const std::string res { consultar_sitio("https://api.github.com/repos/KNZodiac24/anies-cli/releases/latest") };

    std::string ultima_version { busqueda_nombre_version(res).to_string() };
    ultima_version = std::regex_replace(ultima_version, std::regex{"\"name\": \"v"}, "");
    ultima_version = std::regex_replace(ultima_version, std::regex{"\""}, "");

    if (ultima_version == VERSION_PROGRAMA) {
        std::cout << "El programa está actualizado \\(^_^)/\n";
    } else {
        std::cout << "Hay una nueva versión disponible (" << ultima_version << ").\n"
                  << "Obténla en https://github.com/KNZodiac24/anies-cli/releases/tag/v" << ultima_version << '\n';
    }
}

void validar_titulo_anime() {
    trim(ANIME, '+', true);

    if (ANIME.empty()) {
        std::cerr << "Error: No se ha especificado el título a buscar.\n\n";
        mostrar_ayuda();
        throw std::runtime_error("");
    }

    constexpr auto matcher_cadena_valida { ctre::match<"[a-zA-Z0-9|\\$=]+[a-zA-Z0-9\\.,:!'|\\$\\-\\?\\[\\]\\(\\)\"\\+\Q%2B\E=]*"> };

    if (!matcher_cadena_valida(ANIME)) {
        throw std::runtime_error("Error: El título ingresado no es válido.\n");
    }
}

std::string consultar_sitio(const std::string& sitio, const std::list<std::string>& otros_headers, bool verbose) {
    std::ostringstream os {};
    std::string output_verbose {};
    curlpp::options::WriteStream ws{&os};

    try {
        curlpp::Easy req {};

        std::list<std::string> headers {
            "accept: */*",
            "accept-language: es-419,es;q=0.8",
            "cache-control: no-cache",
            "pragma: no-cache",
            "priority: u=1, i",
            "sec-ch-ua: \"Not:A-Brand\";v=\"8\", \"Chromium\";v=\"156\", \"Brave\";v=\"156\"",
            "sec-ch-ua-mobile: ?0",
#if defined(__linux__)
            "sec-ch-ua-platform: \"Linux\"",
#elif defined(_WIN32)
            "sec-ch-ua-platform: \"Windows\"",
#endif
            "sec-gpc: 1",
#if defined(__linux__)
            "user-agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/156.0.0.0 Safari/537.36"
#elif defined(_WIN32)
            "user-agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/156.0.0.0 Safari/537.36"
#endif
        };
        if (otros_headers.size()) {
            for (const auto& h: otros_headers) {
                headers.emplace_back(h);
            }
        }

        req.setOpt<curlpp::options::Url>(sitio);
        req.setOpt<curlpp::options::HttpHeader>(headers);

        req.setOpt(ws);

        if (verbose) {
            req.setOpt<curlpp::options::Verbose>(true);
            req.setOpt<curlpp::options::DebugFunction>(
                [&output_verbose](curl_infotype type, char* data, size_t size) {
                    output_verbose.append(data, size);
                    return 0;
                }
            );
        }

        req.perform();
    } catch(const curlpp::RuntimeError& e) {
        throw std::runtime_error(std::string{e.what()}+'\n');
    } catch(const curlpp::LogicError& e) {
        throw std::runtime_error(std::string{e.what()}+'\n');
    }

    return verbose ? output_verbose : os.str();
}

std::tuple<std::unordered_map<std::string, std::string>, bool, short> generar_lista_animes_encontrados(const std::string& respuesta_sitio) {

    std::unordered_map<std::string, std::string> animes_encontrados {};
    bool resultados_mas_del_limite {};

    short numero_ultima_pagina { 1 };
    short numero_paginas_omitidas {};

    // Verificar paginación
    if (respuesta_sitio.contains("\">»</a>")) {
        constexpr auto busqueda_numero_paginas { ctre::search_all<"\">[0-9]+</a><!--]"> };

        std::string ultimo_resultado {};

        for (const auto& r: busqueda_numero_paginas(respuesta_sitio)) {
            ultimo_resultado = r;
        }

        ultimo_resultado = std::regex_replace(ultimo_resultado, std::regex{"\">"}, "");
        ultimo_resultado = std::regex_replace(ultimo_resultado, std::regex{"</a><!--]"}, "");
        numero_ultima_pagina = std::stoi(ultimo_resultado);

        if (numero_ultima_pagina > 15) {
            numero_paginas_omitidas = numero_ultima_pagina - 15;
            numero_ultima_pagina = 15;
        }
    }

    short numero_resultados_encontrados {};

    const auto extraer_titulo_y_link = [&resultados_mas_del_limite, &numero_resultados_encontrados](const std::string& pagina_fuente) {
        std::unordered_map<std::string, std::string> titulos_con_links {};

        constexpr auto busqueda_resultados_pagina { ctre::search_all<"title:\".+?\",synopsis:\".+?\",categoryId:.+?,slug:\".+?\""> };

        // Esto es útil más abajo, se hace aquí para no hacerlo en cada iteración
        std::string temp_anime { std::regex_replace(ANIME, std::regex{"\\+"}, " ") };
        temp_anime = std::regex_replace(temp_anime, std::regex{"%2B"}, "+");
        std::transform(
            temp_anime.begin(),
            temp_anime.end(),
            temp_anime.begin(),
            [](unsigned char c){ return std::tolower(c); }
        );

        for (const auto& resultado: busqueda_resultados_pagina(pagina_fuente)) {

            if (numero_resultados_encontrados == 50) {
                resultados_mas_del_limite = true;
                break;
            }

            std::string r { resultado.to_string() };

            r = std::regex_replace(r, std::regex{"title:\""}, "");
            r = std::regex_replace(r, std::regex{"\",synopsis:\".+?,slug:\""}, "%");
            r = std::regex_replace(r, std::regex{"\\\\\""}, "&quot");
            r = std::regex_replace(r, std::regex{"\""}, "");
            r = std::regex_replace(r, std::regex{"&quot"}, "\"");

            const std::string titulo { r.substr(0, r.find('%')) };
            const std::string link { r.substr(r.find('%')+1, r.size()-1) };

            // Este bloque es solo para añadir una verificación de que lo ingresado por el usuario salga en la búsqueda,
            // ya que por alguna razón animeav1 suele mostrar resultados que no tienen que ver (y a veces no muestra los que sí...)
            std::string temp_titulo { titulo };
            std::transform(
                temp_titulo.begin(),
                temp_titulo.end(),
                temp_titulo.begin(),
                [](unsigned char c){ return std::tolower(c); }
            );

            if (!temp_titulo.contains(temp_anime)) continue;

            titulos_con_links.insert({titulo, link});

            ++numero_resultados_encontrados;
        }

        return titulos_con_links;
    };

    for (int i { 1 }; i <= numero_ultima_pagina; ++i) {
        if (i == 1) animes_encontrados.merge(extraer_titulo_y_link(respuesta_sitio));
        else {
            if (resultados_mas_del_limite) {
                numero_paginas_omitidas = 0;
                break;
            }
            animes_encontrados.merge(
                extraer_titulo_y_link(consultar_sitio(SITIO_ANIME+"catalogo?search="+ANIME+"&page="+std::to_string(i)))
            );
        }
    }

    return std::tuple{animes_encontrados, resultados_mas_del_limite, numero_paginas_omitidas};
}

void seleccionar_titulo(const std::unordered_map<std::string, std::string>& animes_encontrados, bool resultados_mas_del_limite, short numero_paginas_omitidas) {

    const auto mejorar_presentacion_titulo = [](){
        std::string r { std::regex_replace(ANIME, std::regex{"\\+"}, " ") };
        return std::regex_replace(r, std::regex{"%2B"}, "+");
    };

    if (animes_encontrados.empty()) {
        throw std::runtime_error("No se encontraron resultados para \""+mejorar_presentacion_titulo()+"\".\n");
    }

    limpiar_consola();

    int indice_seleccion_titulo {};
    std::vector<std::string> titulos_anime {};
    for (const auto& a: animes_encontrados) {
        titulos_anime.emplace_back(a.first);
    }

    const auto texto_en_tui = [](const std::string& t){
        return ftxui::Renderer([t]{ return ftxui::text(t); });
    };

    CONSOLA.TrackMouse(false);

    ftxui::MenuOption opcion_menu {};
    opcion_menu.on_enter = CONSOLA.ExitLoopClosure();

    CONTAINER_CONSOLA->Add(texto_en_tui("Resultados para \""+mejorar_presentacion_titulo()+"\":\n"));

    if (resultados_mas_del_limite && !numero_paginas_omitidas) {
        CONTAINER_CONSOLA->Add(texto_en_tui("Se presentan los primeros 50 resultados. Realiza una búsqueda más precisa si no encuentras lo que buscas.\n"));
    } else if (!resultados_mas_del_limite && numero_paginas_omitidas) {
        if (numero_paginas_omitidas == 1) {
            CONTAINER_CONSOLA->Add(texto_en_tui("Se omitió 1 página de la búsqueda. Realiza una búsqueda más precisa si no encuentras lo que buscas.\n"));
        } else {
            CONTAINER_CONSOLA->Add(texto_en_tui("Se omitieron "+std::to_string(numero_paginas_omitidas)+" páginas de la búsqueda. Realiza una búsqueda más precisa si no encuentras lo que buscas.\n"));
        }
    }

    const auto menu { ftxui::Menu(&titulos_anime, &indice_seleccion_titulo, opcion_menu) };
    const auto menu_renderer { ftxui::Renderer(menu, [&]{ return menu->Render() | ftxui::vscroll_indicator | ftxui::frame; }) };
    CONTAINER_CONSOLA->Add(menu_renderer);

    CONTAINER_CONSOLA->SetActiveChild(CONTAINER_CONSOLA->ChildAt(CONTAINER_CONSOLA->ChildCount()-1));

    bool seleccion_cancelada {};
    const auto evento_salir_menu { ftxui::CatchEvent(CONTAINER_CONSOLA, [&seleccion_cancelada](const ftxui::Event& evento) {
        if (evento == ftxui::Event::Character('\033')) {
            CONSOLA.Exit();
            seleccion_cancelada = true;
            return true;
        }
        return false;
    }) };

    CONSOLA.Loop(evento_salir_menu);

    CONTAINER_CONSOLA->DetachAllChildren();

    if (seleccion_cancelada) {
        limpiar_consola();
        throw std::runtime_error("\nSelección cancelada. Saliendo del programa.\n");
    }

    TITULO_SELECCIONADO = titulos_anime.at(static_cast<std::size_t>(indice_seleccion_titulo));
}

void seleccionar_episodio(const std::string& respuesta_titulo_seleccionado) {

    if (!TOTAL_EPISODIOS_ANIME) {
        constexpr auto busqueda_numero_episodios { ctre::search<"episodesCount:[0-9]+,s"> };

        std::string r { busqueda_numero_episodios(respuesta_titulo_seleccionado).to_string() };
        r = std::regex_replace(r, std::regex{"episodesCount:"}, "");
        r = std::regex_replace(r, std::regex{",s"}, "");

        TOTAL_EPISODIOS_ANIME = std::stoi(r);
    }

    if (!EPISODIO) {
        if (TOTAL_EPISODIOS_ANIME == 1) {
            EPISODIO = 1;
            return;
        }

        limpiar_consola();

        std::string temp_episodio {};
        ftxui::InputOption opcion_input {};
        opcion_input.multiline = false;
        opcion_input.on_enter = CONSOLA.ExitLoopClosure();
        opcion_input.transform = [](ftxui::InputState estado) {
            estado.element |= ftxui::size(ftxui::WIDTH, ftxui::Constraint::EQUAL, 5);
            estado.element |= ftxui::border;
            return estado.element;
        };

        ftxui::Component input_episodio { ftxui::Input(&temp_episodio, opcion_input) };
        input_episodio |= ftxui::CatchEvent([&](const ftxui::Event& evento) {
            return evento.is_character() && !std::isdigit(evento.character()[0]);
        });
        input_episodio |= ftxui::CatchEvent([&](const ftxui::Event& evento) {
            return evento.is_character() && temp_episodio.size() > 3;
        });
        const auto input_renderer { ftxui::Renderer(input_episodio, [&]{
            return ftxui::vbox({
                ftxui::text('\n'+TITULO_SELECCIONADO+" tiene "+std::to_string(TOTAL_EPISODIOS_ANIME)+" episodios disponibles."),
                ftxui::hbox(ftxui::vcenter(ftxui::text("Ingresa el episodio que deseas ver: ")), ftxui::notflex(input_episodio->Render()))
            });
        }) };

        CONTAINER_CONSOLA->Add(input_renderer);

        bool input_cancelado {};
        const auto evento_salir_input { ftxui::CatchEvent(CONTAINER_CONSOLA, [&input_cancelado](const ftxui::Event& evento) {
            if (evento == ftxui::Event::Character('\033')) {
                CONSOLA.Exit();
                input_cancelado = true;
                return true;
            }
            return false;
        }) };

        CONSOLA.Loop(evento_salir_input);

        CONTAINER_CONSOLA->DetachAllChildren();

        if (input_cancelado) {
            limpiar_consola();
            throw std::runtime_error("\nIngreso de episodio cancelado. Saliendo del programa.\n");
        }

        if (verificar_argumento_numerico(temp_episodio)) EPISODIO = std::stoi(temp_episodio);
        else throw std::runtime_error("");
    }

    if (EPISODIO > TOTAL_EPISODIOS_ANIME) {
        throw std::runtime_error("Error: El número de episodio ingresado es mayor al total de disponibles ("
                                 +std::to_string(TOTAL_EPISODIOS_ANIME)+") en "+TITULO_SELECCIONADO+'\n');
    }
}

std::unordered_map<std::string, std::string> generar_servers_con_links(const std::string& respuesta_episodio_seleccionado) {

    constexpr auto busqueda_servers_con_links { ctre::search<"embeds:\\{.*SUB:\\[\\{server:\".+\",url:\".+\"\\}\\]\\},"> };

    std::string r { busqueda_servers_con_links(respuesta_episodio_seleccionado).to_string() };

    r = std::regex_replace(r, std::regex{"embeds:.+SUB:\\[\\{"}, "");
    r = std::regex_replace(r, std::regex{"\\}\\]\\},"}, "");

    constexpr auto extraccion_servers_y_links { ctre::search_all<"server:\".+?\",url:\".+?\""> };

    std::unordered_map<std::string, std::string> servers_con_links {};

    for (const auto& e: extraccion_servers_y_links(r)) {
        std::string s { e.to_string() };

        s = std::regex_replace(s, std::regex{"server:\""}, "");
        s = std::regex_replace(s, std::regex{"\",url:\""}, "%");
        s = std::regex_replace(s, std::regex{"\""}, "");

        const std::string server { s.substr(0, s.find('%')) };
        const std::string link { s.substr(s.find('%')+1, s.size()-1) };

        servers_con_links.insert({server, link});
    }

    return servers_con_links;
}

std::vector<std::string> obtener_links_video(const std::unordered_map<std::string, std::string>& servers_con_links){

    if (servers_con_links.empty()) {
        throw std::runtime_error("El episodio existe pero no se encontraron servidores fuente disponibles.\n");
    }

    std::vector<std::string> links_video {};

    const std::array<std::string, 2> orden_busqueda { "HLS", "UPNShare" };

    // Se va guardando en el mismo orden preferido de las opciones de reproducción de la página
    for (const auto& server: orden_busqueda) {
        const auto& it { servers_con_links.find(server) };

        if (it != servers_con_links.cend()) {
            if (it->first == "HLS") {
                const std::string id { std::regex_replace(it->second, std::regex{".+/play/"}, "") };
                links_video.emplace_back("https://player.zilla-networks.com/m3u8/"+id); // Increíble lo que tuve que monear, investigar y entender solo para obtener este link...
                continue;
            }

            if (it->first == "UPNShare") {
                const std::string link { it->second };
                const std::string id { link.substr(link.find('#')+1, link.size()-1) };

                const std::string res_link_2 { consultar_sitio(
                    "https://animeav1.uns.bio/api/public/resolve?file1="+encodeURIComponent(id),
                    {"referer: https://animeav1.uns.bio/", "sec-fetch-dest: empty", "sec-fetch-mode: cors", "sec-fetch-site: same-origin"}
                )};
                constexpr auto busqueda_link_2 { ctre::search<"\"url\":\".+?\",\"host1"> };
                const auto r_link_2 { busqueda_link_2(res_link_2) };
                if (!r_link_2) continue;

                std::string link_2 { r_link_2.to_string() };
                if (link_2.empty()) continue;
                link_2 = std::regex_replace(link_2, std::regex{"\"url\":\""}, "");
                link_2 = std::regex_replace(link_2, std::regex{"\",\"host1"}, "");

                const std::string id_2 { std::regex_replace(link_2, std::regex{".+\\.com/"}, "") };

                Botan::secure_vector<unsigned char> buffer_id_2 { id_2.begin(), id_2.end() };

                const auto aes_e { Botan::Cipher_Mode::create_or_throw("AES-128/CBC/PKCS7", Botan::Cipher_Dir::Encryption) };
                aes_e->set_key(Botan::hex_decode("1d54a1d53c1d56e1d6811d53c1d6831d"));
                aes_e->start(Botan::hex_decode("1f5101f5dd1f5111f6aa1f6e126a1747"));
                aes_e->finish(buffer_id_2);

                const std::vector<unsigned char> cifrado_id_2 { buffer_id_2.begin(), buffer_id_2.end() };
                std::string id_3 { Botan::hex_encode(cifrado_id_2) };
                std::transform(
                    id_3.begin(),
                    id_3.end(),
                    id_3.begin(),
                    [](unsigned char c){ return std::tolower(c); }
                );

                const std::string res_hex { consultar_sitio(
                    "https://turboembed.com/api/stream/tracking?c="+id_3,
                    {"referer: https://animeav1.uns.bio/", "sec-fetch-dest: empty", "sec-fetch-mode: cors", "sec-fetch-site: same-origin"},
                    true
                )};
                constexpr auto busqueda_hex { ctre::search<"x\\-tracking: [0-9a-f]+"> };
                const auto b_hex { busqueda_hex(res_hex) };
                if (!b_hex) continue;

                std::string hex { b_hex.to_string() };
                if (hex.empty()) continue;
                hex = std::regex_replace(hex, std::regex{"x\\-tracking: "}, "");
                constexpr auto match_hex { ctre::match<"^[0-9a-f]+$"> };
                if (!match_hex(hex)) continue;

                const std::vector<unsigned char> cifrado_hex { Botan::hex_decode(hex) };
                Botan::secure_vector<unsigned char> buffer_hex { cifrado_hex.begin(), cifrado_hex.end() };

                const auto aes_d { Botan::Cipher_Mode::create_or_throw("AES-128/CBC/PKCS7", Botan::Cipher_Dir::Decryption) };
                aes_d->set_key(Botan::hex_decode("1d54a1d53c1d56e1d6811d53c1d6831d"));
                aes_d->start(Botan::hex_decode("1f5101f5dd1f5111f6aa1f6e126a1747"));
                aes_d->finish(buffer_hex);

                const std::vector<unsigned char> descifrado_hex { buffer_hex.begin(), buffer_hex.end() };
                const std::string resultado { descifrado_hex.begin(), descifrado_hex.end() };

                constexpr auto busqueda_link_final { ctre::search<"\"tiktok\":\".+?\",\"hls\""> };
                const auto b_link_final { busqueda_link_final(resultado) };
                if (!b_link_final) continue;

                std::string link_final { b_link_final.to_string() };
                if (link_final.empty()) continue;
                link_final = std::regex_replace(link_final, std::regex{"\"tiktok\":\""}, "");
                link_final = std::regex_replace(link_final, std::regex{"\",\"hls\""}, "");

                links_video.emplace_back(link_final); // Bueno, obtener este estuvo más interesante... (v2.0)

                continue;
            } // Creo que de momento solo esos 2 bastan
        }
    }

    return links_video;
}

int exec_reproductor(const std::vector<std::string>& args) {
    namespace bp = boost::process::v2;
    namespace asio = boost::asio;

    asio::io_context ctx {};
#if defined(__linux__)
    bp::process proc {ctx, "/usr/bin/mpv", args};
#elif defined(_WIN32)
    bp::process proc {ctx, "C:\\Program Files\\MPV Player\\mpv.exe", args};
#endif
    proc.wait();

    return proc.exit_code();
}

// De: https://stackoverflow.com/questions/6486289/how-to-clear-the-console-in-c
void limpiar_consola() {
#if defined(__linux__)
    std::cout<< "\033[2J\033[1;1H" << std::flush;
#elif defined(_WIN32)
    HANDLE consola { GetStdHandle(STD_OUTPUT_HANDLE) };
    CONSOLE_SCREEN_BUFFER_INFO pantalla {};
    COORD coord {0, 0};
    DWORD count {};

    GetConsoleScreenBufferInfo(consola, &pantalla);
    FillConsoleOutputCharacter(consola, ' ', static_cast<DWORD>(pantalla.dwSize.X) * static_cast<DWORD>(pantalla.dwSize.Y), coord, &count);
    SetConsoleCursorPosition(consola, coord);
#endif
}

void reproducir(const std::vector<std::string>& links_video_episodio) {

    if (links_video_episodio.empty()) {
        throw std::runtime_error("Los servidores fuente existen pero no se encontraron videos válidos.\n");
    }

    const auto texto_en_tui = [](const std::string& t){
        return ftxui::Renderer([t]{ return ftxui::text(t); });
    };

    limpiar_consola();

    if (TOTAL_EPISODIOS_ANIME == 1) {
        CONTAINER_CONSOLA->Add(texto_en_tui("\n❭❭❭❭❭ Reproduciendo "+TITULO_SELECCIONADO+" ❬❬❬❬❬\n"));
    } else {
        CONTAINER_CONSOLA->Add(texto_en_tui("\n❭❭❭❭❭ Reproduciendo episodio "+std::to_string(EPISODIO)+" de "+TITULO_SELECCIONADO+" ❬❬❬❬❬\n"));
    }

    {
        ftxui::Loop loop { &CONSOLA, CONTAINER_CONSOLA };
        loop.RunOnce();
    }

    using namespace std::string_literals;

    int ultimo_exit_code { 1 };

    for (const auto& link: links_video_episodio) {
        if (link.contains("player.zilla")) { // HLS
            int exit_code {
                exec_reproductor({
                    link,
                    "--window-maximized",
                    "--no-terminal",
                    "--http-header-fields=accept: */*,"s+
                    "accept-language: es-419,es;q=0.8,"+
                    "cache-control: no-cache,"+
                    "pragma: no-cache,priority: i,"+
                    "range: bytes=0-,"+
                    "referer: "+link+","+
                    "sec-ch-ua: \"Not:A-Brand\";v=\"8\", \"Chromium\";v=\"156\", \"Brave\";v=\"156\","+
                    "sec-ch-ua-mobile: ?0,"+
#if defined(__linux__)
                    "sec-ch-ua-platform: \"Linux\","+
#elif defined(_WIN32)
                    "sec-ch-ua-platform: \"Windows\","+
#endif
                    "sec-fetch-dest: video,"+
                    "sec-fetch-mode: cors,"+
                    "sec-fetch-site: same-origin,"+
                    "sec-gpc: 1,"+
#if defined(__linux__)
                    "user-agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/156.0.0.0 Safari/537.36"
#elif defined(_WIN32)
                    "user-agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/156.0.0.0 Safari/537.36"
#endif
                })
            };

            ultimo_exit_code = exit_code;

            if (exit_code) continue;
            else break;
        }

        if (link.contains("turboembed.com")) { // UPNShare
            int exit_code { exec_reproductor({link, "--window-maximized", "--no-terminal"}) };

            ultimo_exit_code = exit_code;

            if (exit_code) continue;
            else break;
        }
    }

    if (ultimo_exit_code) {
        throw std::runtime_error("\nOcurrió un problema al reproducir el video del episodio.\n"s
                                 +"Posiblemente hubo un conflicto con el reproductor o el video no está disponible.\n");
    }
}

OpcionReproduccion controlar_opciones_reproduccion() {

    limpiar_consola();

    int indice_seleccion_opcion {};
    std::vector<std::string> opciones_reproduccion {};

    if (EPISODIO > 1) opciones_reproduccion.emplace_back("Episodio anterior");
    if (EPISODIO < TOTAL_EPISODIOS_ANIME) opciones_reproduccion.emplace_back("Siguiente episodio");
    if (TOTAL_EPISODIOS_ANIME > 1) opciones_reproduccion.emplace_back("Seleccionar episodio");
    opciones_reproduccion.emplace_back("Repetir episodio");
    opciones_reproduccion.emplace_back("Salir");

    ftxui::MenuOption opcion_menu {};
    opcion_menu.on_enter = CONSOLA.ExitLoopClosure();

    const auto menu { ftxui::Menu(&opciones_reproduccion, &indice_seleccion_opcion, opcion_menu) };
    const auto menu_renderer { ftxui::Renderer(menu, [&]{ return menu->Render() | ftxui::vscroll_indicator | ftxui::frame; }) };
    CONTAINER_CONSOLA->Add(menu_renderer);

    CONTAINER_CONSOLA->SetActiveChild(CONTAINER_CONSOLA->ChildAt(CONTAINER_CONSOLA->ChildCount()-1));

    bool seleccion_cancelada {};
    const auto evento_salir_menu { ftxui::CatchEvent(CONTAINER_CONSOLA, [&seleccion_cancelada](const ftxui::Event& evento) {
        if (evento == ftxui::Event::Character('\033')) {
            CONSOLA.Exit();
            seleccion_cancelada = true;
            return true;
        }
        return false;
    }) };

    CONSOLA.Loop(evento_salir_menu);

    CONTAINER_CONSOLA->DetachAllChildren();

    if (seleccion_cancelada) {
        limpiar_consola();
        throw std::runtime_error("\nSelección cancelada. Saliendo del programa.\n");
    }

    OpcionReproduccion opcion_reproduccion {};

    if (EPISODIO == 1 && EPISODIO == TOTAL_EPISODIOS_ANIME) {
        switch (indice_seleccion_opcion) {
            case 0: opcion_reproduccion = OpcionReproduccion::REPETIR_EPISODIO; break;
            case 1: opcion_reproduccion = OpcionReproduccion::SALIR; break;
        }
    } else if (EPISODIO == 1 && EPISODIO < TOTAL_EPISODIOS_ANIME) {
        switch (indice_seleccion_opcion) {
            case 0: opcion_reproduccion = OpcionReproduccion::SIGUIENTE_EPISODIO; break;
            case 1: opcion_reproduccion = OpcionReproduccion::SELECCIONAR_EPISODIO; break;
            case 2: opcion_reproduccion = OpcionReproduccion::REPETIR_EPISODIO; break;
            case 3: opcion_reproduccion = OpcionReproduccion::SALIR; break;
        }
    } else if (EPISODIO > 1 && EPISODIO < TOTAL_EPISODIOS_ANIME) {
        switch (indice_seleccion_opcion) {
            case 0: opcion_reproduccion = OpcionReproduccion::EPISODIO_ANTERIOR; break;
            case 1: opcion_reproduccion = OpcionReproduccion::SIGUIENTE_EPISODIO; break;
            case 2: opcion_reproduccion = OpcionReproduccion::SELECCIONAR_EPISODIO; break;
            case 3: opcion_reproduccion = OpcionReproduccion::REPETIR_EPISODIO; break;
            case 4: opcion_reproduccion = OpcionReproduccion::SALIR; break;
        }
    } else if (EPISODIO == TOTAL_EPISODIOS_ANIME) {
        switch (indice_seleccion_opcion) {
            case 0: opcion_reproduccion = OpcionReproduccion::EPISODIO_ANTERIOR; break;
            case 1: opcion_reproduccion = OpcionReproduccion::SELECCIONAR_EPISODIO; break;
            case 2: opcion_reproduccion = OpcionReproduccion::REPETIR_EPISODIO; break;
            case 3: opcion_reproduccion = OpcionReproduccion::SALIR; break;
        }
    }

    switch (opcion_reproduccion) {
        case OpcionReproduccion::EPISODIO_ANTERIOR: EPISODIO -= 1; break;
        case OpcionReproduccion::SIGUIENTE_EPISODIO: EPISODIO += 1; break;
        case OpcionReproduccion::SELECCIONAR_EPISODIO: EPISODIO = 0; break;
        default: break;
    }

    return opcion_reproduccion;
}
