/*
 *  anies-cli: Aplicación de consola para ver anime en español
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
#include <conio.h>
#endif

//////// Preliminares ////////

void mostrar_ayuda() {
    std::cout << R"(
anies-cli [OPCIONES] [BÚSQUEDA]

    OPCIONES:
        Sin argumentos, -h      Mostrar este mensaje de ayuda con las opciones disponibles
        -v, --version           Mostrar la versión del programa
        -e <número>             Especificar el número de episodio a reproducir

    Ejemplos de uso:
        anies-cli one piece
        anies-cli naruto -e 10
        anies-cli -e 24 bleach
        anies-cli jujutsu kaisen -e 9)" << '\n';
}

// De: https://stackoverflow.com/questions/46479513/trim-matching-characters-from-start-or-end-of-string-c
void trim(std::string& s, char c, bool reverse = false ) {
    if (reverse) s.erase( s.find_last_not_of( c ) + 1 );
    else s.erase( 0, s.find_first_not_of( c ) );
}

bool verificar_argumento_numerico(const std::string& arg);

void validar_titulo_anime();

std::string consultar_sitio(const std::string& sitio, const std::list<std::string>& otros_headers = {});

std::pair<std::unordered_map<std::string, std::string>, bool> generar_lista_animes_encontrados(const std::string& respuesta_sitio);

void seleccionar_titulo(const std::unordered_map<std::string, std::string>& animes_encontrados, bool resultados_mas_del_limite);

void seleccionar_episodio(const std::string& titulo_seleccionado, const std::string& respuesta_titulo_seleccionado);

std::unordered_map<std::string, std::string> generar_servers_con_links(const std::string& respuesta_episodio_seleccionado);

std::vector<std::string> obtener_links_video(const std::unordered_map<std::string, std::string>& servers_con_links);

int exec_anies_cli(const std::vector<std::string>& args);

void limpiar_consola();

void reproducir(const std::vector<std::string>& links_video_episodio);

char controlar_opciones_reproduccion();

//////// Main ////////

constexpr std::string VERSION_PROGRAMA { "0.10.4" };
const std::string SITIO_ANIME { "https://animeav1.com/" };
constinit std::string ANIME {};
constinit std::string TITULO_SELECCIONADO {};
constinit int TOTAL_EPISODIOS_ANIME {};
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
        else if (op == "-e") {
            if (static_cast<int>(i+1) < argc && verificar_argumento_numerico(argumentos[i+1])) {
                EPISODIO = std::stoi(argumentos[i+1]);
                argumentos.erase(std::remove(argumentos.begin(), argumentos.end(), argumentos[i+1]), argumentos.cend());
                continue;
            } else {
                std::cerr << "Por favor, especifica el número de episodio correctamente.\n";
                mostrar_ayuda();
                exit(EXIT_FAILURE);
            }
        }
        else {
            if (op.contains("+")) {
                ANIME += std::regex_replace(op, std::regex{"\\+"}, "%2B") + '+';
            } else if (op.contains(' ')) {
                ANIME += std::regex_replace(op, std::regex{" "}, "+");
            } else {
                ANIME += op + '+';
            }
        }
    }

    validar_titulo_anime();

    const std::string respuesta_sitio { consultar_sitio(SITIO_ANIME+"catalogo?search="+ANIME) };

    const std::pair<std::unordered_map<std::string, std::string>, bool> animes_encontrados { generar_lista_animes_encontrados(respuesta_sitio) };

    seleccionar_titulo(animes_encontrados.first, animes_encontrados.second);

    const std::string link_titulo_seleccionado { SITIO_ANIME+"media/"+animes_encontrados.first.at(TITULO_SELECCIONADO) };

    const std::string respuesta_titulo_seleccionado { consultar_sitio(link_titulo_seleccionado) };

    seleccionar_episodio(TITULO_SELECCIONADO, respuesta_titulo_seleccionado);

    char opcion_reproduccion {};

    while (opcion_reproduccion != 'z') {
        const std::string link_episodio_seleccionado { link_titulo_seleccionado+'/'+std::to_string(EPISODIO) };

        const std::string respuesta_episodio_seleccionado { consultar_sitio(link_episodio_seleccionado) };

        const std::unordered_map<std::string, std::string> servers_con_links { generar_servers_con_links(respuesta_episodio_seleccionado) };

        const std::vector<std::string> links_video_episodio { obtener_links_video(servers_con_links) };

        reproducir(links_video_episodio);

        opcion_reproduccion = controlar_opciones_reproduccion();
    }

    return 0;
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

void validar_titulo_anime() {
    trim(ANIME, '+', true);

    constexpr auto matcher_espacios { ctre::match<"^[\\h\\s\v\\n\t]+$"> };

    if (ANIME.empty() || ANIME == "" || matcher_espacios(ANIME)) {
        std::cerr << "Error: No se ha especificado el título a buscar.\n";
        mostrar_ayuda();
        exit(EXIT_FAILURE);
    }

    constexpr auto matcher_caracteres_solos { ctre::match<"^(\\.)+$|^(,)+$|^(!)+$|^(:)+$|^(-)+$|^(`)+$|^(\\[)+$|^(\\])+$|^(\\()+$|^(\\))+$|^(\")+$|^(')+$|^(%2B)+$|^(\\\\)+$|^(/)+$|^(\\e)+$|^(\\n)+$"> };

    if (matcher_caracteres_solos(ANIME) || ANIME.contains("\\\\") || ANIME.contains("/") ||
        ANIME.contains("\\e") || ANIME.contains("\\s") || ANIME.contains("\t") ||
        ANIME.contains("\n") || ANIME.contains("\\")) {
        std::cerr << "Error: El título ingresado no es válido.\n";
        exit(EXIT_FAILURE);
    }
}

std::string consultar_sitio(const std::string& sitio, const std::list<std::string>& otros_headers) {
    std::ostringstream os {};
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
            "sec-ch-ua-platform: \"Linux\"",
            "sec-gpc: 1",
            "user-agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/156.0.0.0 Safari/537.36"
        };
        if (otros_headers.size()) {
            for (const auto& h: otros_headers) {
                headers.emplace_back(h);
            }
        }

        req.setOpt<curlpp::options::Url>(sitio);
        req.setOpt<curlpp::options::HttpHeader>(headers);

        req.setOpt(ws);

        req.perform();
    } catch(const curlpp::RuntimeError& e) {
        std::cout << e.what() << '\n';
    } catch(const curlpp::LogicError& e) {
        std::cout << e.what() << '\n';
    }

    return os.str();
}

std::pair<std::unordered_map<std::string, std::string>, bool> generar_lista_animes_encontrados(const std::string& respuesta_sitio) {

    std::unordered_map<std::string, std::string> animes_encontrados {};
    bool resultados_mas_del_limite {};

    int numero_ultima_pagina { 1 };

    // Verificar paginación
    if (respuesta_sitio.contains("\">»</a>")) {
        const auto busqueda_numero_paginas { ctre::search_all<"\">[0-9]+</a><!--]">(respuesta_sitio) };

        std::string ultimo_resultado {};

        for (const auto& r: busqueda_numero_paginas) {
            ultimo_resultado = r;
        }

        ultimo_resultado = std::regex_replace(ultimo_resultado, std::regex{"\">"}, "");
        ultimo_resultado = std::regex_replace(ultimo_resultado, std::regex{"</a><!--]"}, "");
        numero_ultima_pagina = std::stoi(ultimo_resultado);
    }

    short numero_resultados_encontrados {};

    const auto extraer_titulo_y_link = [&resultados_mas_del_limite, &numero_resultados_encontrados](const std::string& pagina_fuente) {
        std::unordered_map<std::string, std::string> titulos_con_links {};

        const auto busqueda_resultados_pagina { ctre::search_all<"title:\".+?\",synopsis:\".+?\",categoryId:.+?,slug:\".+?\"">(pagina_fuente) };

        for (const auto& resultado: busqueda_resultados_pagina) {

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
            // ya que por alguna razón animeav1 suele mostrar resultados que no tienen que ver
            std::string temp_titulo { titulo };
            std::transform(
                temp_titulo.begin(),
                temp_titulo.end(),
                temp_titulo.begin(),
                [](unsigned char c){ return std::tolower(c); }
            );
            std::string temp_anime { std::regex_replace(ANIME, std::regex{"\\+"}, " ") };
            temp_anime = std::regex_replace(temp_anime, std::regex{"%2B"}, "+");

            if (!temp_titulo.contains(temp_anime)) continue;

            titulos_con_links.insert({titulo, link});

            ++numero_resultados_encontrados;
        }

        return titulos_con_links;
    };

    for (int i { 1 }; i <= numero_ultima_pagina; ++i) {
        if (i == 1) animes_encontrados.merge(extraer_titulo_y_link(respuesta_sitio));
        else {
            if (resultados_mas_del_limite) break;
            animes_encontrados.merge(
                extraer_titulo_y_link(consultar_sitio(SITIO_ANIME+"catalogo?search="+ANIME+"&page="+std::to_string(i)))
            );
        }
    }

    return std::pair{animes_encontrados, resultados_mas_del_limite};
}

void seleccionar_titulo(const std::unordered_map<std::string, std::string>& animes_encontrados, bool resultados_mas_del_limite) {

    const auto mejorar_presentacion_titulo = [](){
        std::string r { std::regex_replace(ANIME, std::regex{"\\+"}, " ") };
        return std::regex_replace(r, std::regex{"%2B"}, "+");
    };

    if (animes_encontrados.empty()) {
        std::cout << "No se encontraron resultados para \"" << mejorar_presentacion_titulo() << "\".\n";
        exit(EXIT_FAILURE);
    }

    std::cout << "Resultados para \"" << mejorar_presentacion_titulo() << "\":\n\n";

    int indice_resultado {};
    for (const auto& a: animes_encontrados) {
        ++indice_resultado;
        std::cout << indice_resultado << ". " << a.first << '\n';
    }

    if (resultados_mas_del_limite) std::cout << "\nSe presentan los primeros 50 resultados. Realiza una búsqueda más precisa si no encuentras lo que buscas.";

    int indice_seleccion_titulo {};
    std::string temp_seleccion_titulo {};
    std::cout << "\nSelecciona la opción que deseas ver: "; std::cin >> temp_seleccion_titulo; std::cout << '\n';

    if (verificar_argumento_numerico(temp_seleccion_titulo)) indice_seleccion_titulo = std::stoi(temp_seleccion_titulo);
    else exit(EXIT_FAILURE);

    if (indice_seleccion_titulo > indice_resultado) {
        std::cerr << "Error: La selección está fuera de los resultados encontrados.\n";
        exit(EXIT_FAILURE);
    }

    int indice_helper { 1 };

    for (const auto& a: animes_encontrados) {
        if (indice_helper == indice_seleccion_titulo) {
            TITULO_SELECCIONADO = a.first;
            break;
        }else {
            ++indice_helper;
        }
    }
}

void seleccionar_episodio(const std::string& titulo_seleccionado, const std::string& respuesta_titulo_seleccionado) {

    constexpr auto busqueda_numero_episodios { ctre::search<"episodesCount:[0-9]+,s"> };

    std::string r { busqueda_numero_episodios(respuesta_titulo_seleccionado).to_string() };
    r = std::regex_replace(r, std::regex{"episodesCount:"}, "");
    r = std::regex_replace(r, std::regex{",s"}, "");

    int numero_total_episodios { std::stoi(r) };

    TOTAL_EPISODIOS_ANIME = numero_total_episodios;

    if (!EPISODIO) {
        if (numero_total_episodios == 1) {
            EPISODIO = 1;
            return;
        }

        std::string temp_episodio {};
        std::cout << titulo_seleccionado << " tiene " << numero_total_episodios << " episodios disponibles.\n";
        std::cout << "Ingresa el episodio que deseas ver: "; std::cin >> temp_episodio; std::cout << '\n';

        if (verificar_argumento_numerico(temp_episodio)) EPISODIO = std::stoi(temp_episodio);
        else exit(EXIT_FAILURE);
    }

    if (EPISODIO > numero_total_episodios) {
        std::cerr << "Error: El número de episodio ingresado es mayor al total de disponibles en "
                  << titulo_seleccionado << " (" << numero_total_episodios << ")\n";
        exit(EXIT_FAILURE);
    }
}

std::unordered_map<std::string, std::string> generar_servers_con_links(const std::string& respuesta_episodio_seleccionado) {

    constexpr auto busqueda_servers_con_links { ctre::search<"embeds:\\{.*SUB:\\[\\{server:\".+\",url:\".+\"\\}\\]\\},"> };

    std::string r { busqueda_servers_con_links(respuesta_episodio_seleccionado).to_string() };

    r = std::regex_replace(r, std::regex{"embeds:.+SUB:\\[\\{"}, "");
    r = std::regex_replace(r, std::regex{"\\}\\]\\},"}, "");

    const auto extraccion_servers_y_links { ctre::search_all<"server:\".+?\",url:\".+?\"">(r) };

    std::unordered_map<std::string, std::string> servers_con_links {};

    for (const auto& e: extraccion_servers_y_links) {
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
        std::cout << "El episodio existe pero no se encontraron servidores fuente disponibles.\n";
        exit(EXIT_FAILURE);
    }

    std::vector<std::string> links_video {};

    const std::array<std::string, 2> orden_busqueda { "HLS", "UPNShare" };

    // Se va guardando en el mismo orden preferido de las opciones de reproducción de la página
    for (const auto& server: orden_busqueda) {
        auto it { servers_con_links.find(server) };

        if (it != servers_con_links.cend()) {
            if (it->first == "HLS") {
                const std::string id { std::regex_replace(it->second, std::regex{".+/play/"}, "") };
                links_video.emplace_back("https://player.zilla-networks.com/m3u8/"+id); // Increíble lo que tuve que monear, investigar y entender solo para obtener este link...
                continue;
            }

            if (it->first == "UPNShare") {
                const std::string link { it->second };
                const std::string id { link.substr(link.find('#')+1, link.size()-1) };
                const std::string hex { consultar_sitio("https://animeav1.uns.bio/api/v1/video?id="+id, {"referer: https://animeav1.uns.bio/", "sec-fetch-dest: empty", "sec-fetch-mode: cors", "sec-fetch-site: same-origin"}) };

                constexpr auto match_hex { ctre::match<"^[0-9a-f]+$"> };
                if (!match_hex(hex)) continue;

                const std::vector<unsigned char> cifrado { Botan::hex_decode(hex) };

                const auto aes { Botan::Cipher_Mode::create_or_throw("AES-128/CBC/PKCS7", Botan::Cipher_Dir::Decryption) };
                aes->set_key(Botan::hex_decode("6b69656d7469656e6d75613931316361"));
                aes->start(Botan::hex_decode("313233343536373839306f6975797472"));

                Botan::secure_vector<unsigned char> buffer { cifrado.begin(), cifrado.end() };

                aes->finish(buffer);

                const std::vector<unsigned char> descifrado { buffer.begin(), buffer.end() };

                const std::string resultado { descifrado.begin(), descifrado.end() };

                constexpr auto busqueda_1 { ctre::search<"Tiktok\\\\\".+?disabled.+?domain.+?params"> };

                std::string p1 { busqueda_1(resultado).to_string() };
                if (p1.empty()) continue;
                p1 = std::regex_replace(p1, std::regex{".+domain\\\\\":\\\\\""}, "");
                p1 = std::regex_replace(p1, std::regex{"\\\\\",\\\\\"params"}, "");

                constexpr auto busqueda_2 { ctre::search<"hls\\\\/.+?m3u8"> };

                std::string p2 { busqueda_2(resultado).to_string() };
                if (p2.empty()) continue;
                p2 = std::regex_replace(p2, std::regex{"hls\\\\/"}, "");
                p2 = std::regex_replace(p2, std::regex{"\\\\"}, "");

                constexpr auto busqueda_3 { ctre::search<"v\\\\\":\\\\\".+?\\\\\"\\}\\}"> };

                std::string p3 { busqueda_3(resultado).to_string() };
                if (p3.empty()) continue;
                p3 = std::regex_replace(p3, std::regex{"v\\\\\":\\\\\""}, "");
                p3 = std::regex_replace(p3, std::regex{"\\\\\"\\}\\}"}, "");

                links_video.emplace_back("https://animeav1.uns.bio/hlsmod/"+p1+'/'+p2+"?v="+p3); // Bueno, obtener este estuvo más interesante...
                continue;
            } // Creo que de momento solo esos 2 bastan
        }
    }

    return links_video;
}

int exec_anies_cli(const std::vector<std::string>& args) {
    namespace bp = boost::process::v2;
    namespace asio = boost::asio;

    asio::io_context ctx {};

    bp::process proc {ctx, "/usr/bin/mpv", args};

    proc.wait();

    return proc.exit_code();
}

void limpiar_consola() {
#if defined(__linux__)
    std::cout<< "\033[2J\033[1;1H" << std::flush;
#elif defined(_WIN32)
    clrscr();
#endif
}

void reproducir(const std::vector<std::string>& links_video_episodio) {

    if (links_video_episodio.empty()) {
        std::cout << "Los servidores fuente existen pero no se encontraron videos válidos.\n";
        exit(EXIT_FAILURE);
    }

    limpiar_consola();

    if (TOTAL_EPISODIOS_ANIME == 1) std::cout << "\n=== Reproduciendo " << TITULO_SELECCIONADO << " ===\n";
    else std::cout << "\n=== Reproduciendo episodio " << EPISODIO << " de " << TITULO_SELECCIONADO << " ===\n";

    int ultimo_exit_code { 1 };

    for (const auto& link: links_video_episodio) {
        if (link.contains("player.zilla")) { // HLS
            using namespace std::string_literals;

            int exit_code {
                exec_anies_cli({
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
                    "sec-ch-ua-platform: \"Linux\","+
                    "sec-fetch-dest: video,"+
                    "sec-fetch-mode: cors,"+
                    "sec-fetch-site: same-origin,"+
                    "sec-gpc: 1,"+
                    "user-agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/156.0.0.0 Safari/537.36"
                })
            };

            ultimo_exit_code = exit_code;

            if (exit_code) continue;
            else break;
        }

        if (link.contains("uns.bio")) { // UPNShare
            int exit_code { exec_anies_cli({link, "--window-maximized", "--no-terminal"}) };

            ultimo_exit_code = exit_code;

            if (exit_code) continue;
            else break;
        }
    }

    if (ultimo_exit_code) {
        std::cout << "\nHubo un problema al reproducir el video del episodio.\n";
        exit(EXIT_FAILURE);
    }
}

char controlar_opciones_reproduccion() {

    constexpr auto match_opciones_general { ctre::match<"^[asrz]$"> };
    constexpr auto match_opciones_min_episodio { ctre::match<"^[srz]$"> };
    constexpr auto match_opciones_max_episodio { ctre::match<"^[arz]$"> };

    char opcion { 'n' };

    while (opcion == 'n') {
        std::cout << '\n';
        if (EPISODIO > 1) std::cout << "(a) Reproducir episodio anterior\n";
        if (EPISODIO < TOTAL_EPISODIOS_ANIME) std::cout << "(s) Reproducir siguiente episodio\n";
        std::cout << "(r) Repetir episodio\n";
        std::cout << "(z) Salir\n";

        std::cout << "Selecciona una opción: "; std::cin >> opcion;

        if (EPISODIO == 1 && EPISODIO == TOTAL_EPISODIOS_ANIME) {
            if (!match_opciones_min_episodio(std::string{opcion})) {
                limpiar_consola();
                std::cout << "\n=== Reproducción actual: " << TITULO_SELECCIONADO << " ===\n";
                std::cout << "\nPor favor ingresa una opción válida.\n";
                opcion = 'n';
            }
        } else if (EPISODIO == 1 && EPISODIO < TOTAL_EPISODIOS_ANIME) {
            if (!match_opciones_min_episodio(std::string{opcion})) {
                limpiar_consola();
                std::cout << "\n=== Reproducción actual: Episodio " << EPISODIO << " de "<< TITULO_SELECCIONADO << " ===\n";
                std::cout << "\nPor favor ingresa una opción válida.\n";
                opcion = 'n';
            }
        } else if (EPISODIO > 1 && EPISODIO < TOTAL_EPISODIOS_ANIME) {
            if (!match_opciones_general(std::string{opcion})) {
                limpiar_consola();
                std::cout << "\n=== Reproducción actual: Episodio " << EPISODIO << " de "<< TITULO_SELECCIONADO << " ===\n";
                std::cout << "\nPor favor ingresa una opción válida.\n";
                opcion = 'n';
            }
        } else if (EPISODIO == TOTAL_EPISODIOS_ANIME) {
            if (!match_opciones_max_episodio(std::string{opcion})) {
                limpiar_consola();
                std::cout << "\n=== Reproducción actual: Episodio " << EPISODIO << " de "<< TITULO_SELECCIONADO << " ===\n";
                std::cout << "\nPor favor ingresa una opción válida.\n";
                opcion = 'n';
            }
        }
    }

    switch (opcion) {
        case 'a': EPISODIO -= 1; break;
        case 's': EPISODIO += 1; break;
    }

    return opcion;
}
