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

// #include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <ctre.hpp>
#include <botan/cipher_mode.h>
#include <botan/hex.h>
#include <boost/process.hpp>
#include <boost/asio.hpp>

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
        anies-cli one piece --vlc
        anies-cli naruto
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

std::unordered_map<std::string, std::string> generar_lista_animes_encontrados(const std::string& respuesta_sitio);

std::string seleccionar_titulo(const std::unordered_map<std::string, std::string>& animes_encontrados);

void seleccionar_episodio(const std::string& titulo_seleccionado, const std::string& respuesta_titulo_seleccionado);

std::unordered_map<std::string, std::string> generar_servers_con_links(const std::string& respuesta_episodio_seleccionado);

std::vector<std::string> obtener_links_video(const std::unordered_map<std::string, std::string>& servers_con_links);

void exec_anies_cli(const std::vector<std::string>& args);

void reproducir(const std::vector<std::string>& links_video_episodio);

//////// Main ////////

constexpr std::string_view VERSION_PROGRAMA { "0.7.2" };
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
    // const std::string respuesta_sitio { consultar_sitio("text_consulta.txt") };

    const std::unordered_map<std::string, std::string> animes_encontrados { generar_lista_animes_encontrados(respuesta_sitio) };

    const std::string titulo_seleccionado { seleccionar_titulo(animes_encontrados) };

    const std::string link_titulo_seleccionado { SITIO_ANIME+"media/"+animes_encontrados.at(titulo_seleccionado) };

    const std::string respuesta_titulo_seleccionado { consultar_sitio(link_titulo_seleccionado) };
    // const std::string respuesta_titulo_seleccionado { consultar_sitio("text_anime.txt") };

    seleccionar_episodio(titulo_seleccionado, respuesta_titulo_seleccionado);

    const std::string link_episodio_seleccionado { link_titulo_seleccionado+'/'+std::to_string(EPISODIO) };

    const std::string respuesta_episodio_seleccionado { consultar_sitio(link_episodio_seleccionado) };
    // const std::string respuesta_episodio_seleccionado { consultar_sitio("text_episodio.txt") };

    const std::unordered_map<std::string, std::string> servers_con_links { generar_servers_con_links(respuesta_episodio_seleccionado) };

    const std::vector<std::string> links_video_episodio { obtener_links_video(servers_con_links) };

    reproducir(links_video_episodio);

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

/*    std::string salida {};

    std::ifstream inf{ sitio };

    if (!inf)
    {
        std::cerr << "Error al leer archivo\n";
        exit(EXIT_FAILURE);
    }

    std::string linea {};
    while (std::getline(inf, linea)) { 
        salida += linea + '\n';
    }

    return salida;*/
}

std::unordered_map<std::string, std::string> generar_lista_animes_encontrados(const std::string& respuesta_sitio) {

    std::unordered_map<std::string, std::string> animes_encontrados {};
 
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

    const auto extraer_titulo_y_link = [](const std::string& pagina_fuente) {
        std::unordered_map<std::string, std::string> titulos_con_links {};

        const auto busqueda_resultados_pagina { ctre::search_all<"title:\".+?\",synopsis:\".+?\",categoryId:.+?,slug:\".+?\"">(pagina_fuente) };

        for (const auto& resultado: busqueda_resultados_pagina) {
            std::string r { resultado.to_string() };

            r = std::regex_replace(r, std::regex{"title:\""}, "");
            r = std::regex_replace(r, std::regex{"\",synopsis:\".+?,slug:\""}, "%");
            r = std::regex_replace(r, std::regex{"\""}, "");

            std::string titulo { r.substr(0, r.find('%')) };
            std::string link { r.substr(r.find('%')+1, r.size()-1) };

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
        }

        return titulos_con_links;
    }; 

    for (int i { 1 }; i <= numero_ultima_pagina; ++i) {
        if (i == 1) animes_encontrados.merge(extraer_titulo_y_link(respuesta_sitio));
        else {
            animes_encontrados.merge(
                extraer_titulo_y_link(consultar_sitio(SITIO_ANIME+"catalogo?search="+ANIME+"&page="+std::to_string(i)))
            );
        }
    }

    return animes_encontrados;
}

std::string seleccionar_titulo(const std::unordered_map<std::string, std::string>& animes_encontrados) {

    const auto mejorar_presentacion_titulo = [](){
        std::string r { std::regex_replace(ANIME, std::regex{"\\+"}, " ") };
        return std::regex_replace(r, std::regex{"%2B"}, "+");
    };

    std::cout << "Resultados para \"" << mejorar_presentacion_titulo() << "\":\n\n";

    int indice_resultado {};
    for (const auto& a: animes_encontrados) {
        ++indice_resultado;
        std::cout << indice_resultado << ". " << a.first << '\n';
    }

    int indice_seleccion_titulo {};
    std::string temp_seleccion_titulo {};
    std::cout << "\nSelecciona la opción que deseas ver: "; std::cin >> temp_seleccion_titulo; std::cout << '\n';

    if (verificar_argumento_numerico(temp_seleccion_titulo)) indice_seleccion_titulo = std::stoi(temp_seleccion_titulo); 
    else exit(EXIT_FAILURE);

    if (indice_seleccion_titulo > indice_resultado) {
        std::cerr << "Error: La selección está fuera de los resultados encontrados.\n";
        exit(EXIT_FAILURE);
    }

    std::string titulo_seleccionado {};
    int indice_helper { 1 };

    for (const auto& a: animes_encontrados) {
        if (indice_helper == indice_seleccion_titulo) {
            titulo_seleccionado = a.first;
            break;
        }else {
            ++indice_helper;
        }
    }

    return titulo_seleccionado;
}

void seleccionar_episodio(const std::string& titulo_seleccionado, const std::string& respuesta_titulo_seleccionado) {

    constexpr auto busqueda_numero_episodios { ctre::search<"episodesCount:[0-9]+,s"> };

    std::string r { busqueda_numero_episodios(respuesta_titulo_seleccionado).to_string() };
    r = std::regex_replace(r, std::regex{"episodesCount:"}, "");
    r = std::regex_replace(r, std::regex{",s"}, "");

    int numero_total_episodios { std::stoi(r) };

    if (!EPISODIO) {
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

        std::string server { s.substr(0, s.find('%')) };
        std::string link { s.substr(s.find('%')+1, s.size()-1) };

        servers_con_links.insert({server, link});
    }

    return servers_con_links;
}

std::vector<std::string> obtener_links_video(const std::unordered_map<std::string, std::string>& servers_con_links){

    std::vector<std::string> links_video {};

    // for (const auto& s: servers_con_links) {
        if (servers_con_links.contains("HLS")) {
            const std::string id { std::regex_replace(servers_con_links.at("HLS"), std::regex{".+/play/"}, "") };
            links_video.emplace_back("https://player.zilla-networks.com/m3u8/"+id); // Increíble lo que tuve que monear, investigar y entender solo para obtener este link...
            // continue;
        }

        if (servers_con_links.contains("UPNShare")) {
            const std::string link { servers_con_links.at("UPNShare") };
            const std::string id { link.substr(link.find('#')+1, link.size()-1) };
            const std::string hex { consultar_sitio("https://animeav1.uns.bio/api/v1/video?id="+id, {"referer: https://animeav1.uns.bio/", "sec-fetch-dest: empty", "sec-fetch-mode: cors", "sec-fetch-site: same-origin"}) };

            constexpr auto match_hex { ctre::match<"^[0-9a-f]+$"> };
            if (!match_hex(hex)) goto xd;

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
            p1 = std::regex_replace(p1, std::regex{".+domain\\\\\":\\\\\""}, "");
            p1 = std::regex_replace(p1, std::regex{"\\\\\",\\\\\"params"}, "");

            constexpr auto busqueda_2 { ctre::search<"hls\\\\/.+?m3u8"> };

            std::string p2 { busqueda_2(resultado).to_string() };
            p2 = std::regex_replace(p2, std::regex{"hls\\\\/"}, "");
            p2 = std::regex_replace(p2, std::regex{"\\\\"}, "");

            constexpr auto busqueda_3 { ctre::search<"v\\\\\":\\\\\".+?\\\\\"\\}\\}"> };

            std::string p3 { busqueda_3(resultado).to_string() }; 
            p3 = std::regex_replace(p3, std::regex{"v\\\\\":\\\\\""}, "");
            p3 = std::regex_replace(p3, std::regex{"\\\\\"\\}\\}"}, "");

            links_video.emplace_back("https://animeav1.uns.bio/hlsmod/"+p1+'/'+p2+"?v="+p3); // Bueno, obtener este estuvo más interesante...
            // continue;
        } // Creo que de momento solo esos 2 bastan
    // }

    xd:
    return links_video;
}

void exec_anies_cli(const std::vector<std::string>& args) {
    namespace bp = boost::process;
    namespace asio = boost::asio;

    asio::io_context ctx {};

    bp::process proc {ctx, "/usr/bin/mpv", args};

    if (proc.wait() == 0) {
        std::cout << "Acabó bien\n";
        exit(EXIT_SUCCESS);
    } else {
        std::cout << "Acabó mal\n";
    }
}

void reproducir(const std::vector<std::string>& links_video_episodio) {

    std::cout << links_video_episodio[0]<<'\n';
    std::vector<std::string> args { links_video_episodio[0], "--http-header-fields='accept: */*','accept-language: es-419,es;q=0.8','cache-control: no-cache','pragma: no-cache','priority: i','range: bytes=0-','referer: "+links_video_episodio[0]+"','sec-ch-ua: \"Not:A-Brand\";v=\"8\", \"Chromium\";v=\"156\", \"Brave\";v=\"156\"','sec-ch-ua-mobile: ?0','sec-ch-ua-platform: \"Linux\"','sec-fetch-dest: video','sec-fetch-mode: cors','sec-fetch-site: same-origin','sec-gpc: 1','user-agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/156.0.0.0 Safari/537.36'"};

    exec_anies_cli(args);
}
