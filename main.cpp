#include <iostream> //input output
#include <fstream>  //file inputoutput
#include <vector>   
#include <array>    // para std::array de tamaño fijo
#include <cstdint>  // para uint64_t, uint32_t, int16_t
#include <map>        // para std::map
#include <sstream>    // para std::istringstream
#include <cmath>  // para std::round
#include <filesystem>
#include "H5Cpp.h"
#include <regex>
#include <algorithm>


#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define RESET   "\033[0m"


// starting recording 1727799934808099
// primer reg 1727799934808115
const size_t HEADER_BYTES = 16 * 1024; // 16 KB, 16384 B
const size_t SAMPLES_PER_REG = 512;
const size_t REG_BYTES = 8 + 4 + 4 + 4 + 512 * 2;   // 1044 (Timestamps + Chanell number + SampleFrec + NValidSamples + 512 samples)

#pragma pack(push, 1)
struct NCS_REG {
    uint64_t timestamp;             // 8 bytes
    uint32_t channel;               // 4 bytes
    uint32_t samplingFreq;          // 4 bytes
    uint32_t nValidSamples;         // 4 bytes
    std::array<int16_t, SAMPLES_PER_REG> samples; // 512 * 2 bytes
};
#pragma pack(pop)

#pragma pack(push, 1)
struct NEV_RECORD {
int16_t stx;
int16_t pkt_id;
int16_t pkt_data_size;
uint64_t TimeStamp;
int16_t event_id;
int16_t ttl;
int16_t crc;
int16_t dummy1;
int16_t dummy2;
int32_t Extra[8];
char EventString[128];
};
#pragma pack(pop)

void printWelcomeBanner() {
    std::cout << BLUE << R"(
    __٨ﮩ__٨ﮩ__٨ﮩ__٨ﮩ__٨ﮩ____٨ﮩ____٨ﮩ٨ﮩ٨ﮩ__٨ﮩ٨ﮩ__٨ﮩ٨ﮩ___٨ﮩ__٨ﮩ_____٨ﮩ٨ﮩ___٨ﮩ___٨ﮩ___٨ﮩ______٨ﮩ______٨ﮩ____٨ﮩ


    ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣠⠤⠤⠤⠤⣄⠀⠀⠀⠀⠀⠀⢀⣀⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
    ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣶⡊⠉⠉⣉⣱⡷⠶⢢⣠⢴⣶⡝⠒⠉⢉⣭⡽⠟⢉⣀⡀⠹⢭⠒⢤⣀⣀⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
    ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⡠⠔⢚⣩⡽⠿⠊⢉⣉⡂⣀⣩⠭⢴⠟⠋⠉⠉⠉⠛⠳⢦⣬⣤⡴⠞⠛⠁⠛⠳⣾⣧⠀⠟⠀⠉⠲⢄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
    ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⡠⢚⠁⠀⠰⠋⢡⠄⠀⠞⣫⢟⡥⠒⠉⠹⣿⡀⠀⠀⢦⡀⠀⠀⠀⠈⠻⡧⡀⠀⠀⠀⠀⠈⠻⣗⡶⠶⠶⢤⡀⠱⣄⡀⠀⠀⠀⠀⠀⠀⠀⠀
    ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢮⣤⡾⠀⠀⣠⡴⠋⠀⡠⣚⠥⠒⢛⡲⠄⠀⠈⢻⡆⠀⠀⠻⣦⣀⠀⠀⠀⣿⠻⣦⣀⣴⠶⠂⠀⠘⣷⡄⠀⠀⢀⣴⡿⠈⠢⡀⠀⠀⠀⠀⠀⠀
    ⠀⠀⠀⠀⠀⠀⠀⡠⠖⣉⣁⡀⠀⢀⣾⠋⠴⢿⣽⠋⠀⠞⢉⣉⣽⣳⣄⣀⠀⠋⠀⠀⠀⠈⠙⣷⡄⠀⠁⠀⠙⢤⣯⡀⠀⠀⠀⣼⡇⠀⡾⠋⠁⠀⠳⣄⠘⢆⠀⠀⠀⠀⠀
    ⠀⠀⠀⠀⠀⡰⠋⠰⠛⢻⡞⢉⣠⣼⡇⢀⣴⠟⠛⠒⣴⠟⠋⠉⠀⠀⢀⣀⣀⡀⠀⠀⠀⠀⠀⣸⡇⠀⠀⠳⣄⠀⠉⢿⣄⠀⢰⣿⣧⡀⠀⣴⠶⠶⣦⡼⢧⠈⢣⠀⠀⠀⠀
    ⠀⠀⠀⢀⡞⣡⣶⡄⠀⡟⣳⠿⠋⠙⡍⡽⠁⣀⣤⣤⣿⡄⠀⠀⠀⠀⡿⡉⣀⣀⣤⣤⣤⣴⠾⠥⠽⣦⣄⠀⠉⠻⢶⡼⢻⠀⠈⠇⠘⡷⡄⠘⠂⠀⢀⡍⠻⣷⣄⡇⠀⠀⠀
    ⠀⠀⠀⠘⣺⠏⢸⢃⣼⠟⢁⡤⠀⣠⢟⡷⠟⢋⣉⣤⡿⠇⠀⠀⠀⢰⣣⠞⠋⠉⠉⠁⡀⠀⠀⠀⠀⠀⠙⢷⣄⠀⠀⢹⣾⠀⠀⠀⠀⢸⡇⠀⣀⡀⣾⠀⠀⠈⢻⡁⢦⠀⠀
    ⠀⠀⣠⢚⣵⣄⠈⣼⡇⠀⢸⠧⢞⡵⠋⠠⠚⠉⠉⠀⠀⢀⡇⠀⣰⣟⣁⣀⠀⠀⠀⠀⠉⠒⠶⣤⣤⣀⠀⠀⠙⠀⠀⢸⡇⢰⣟⠛⢶⡋⣇⠀⠉⠻⡟⡄⠀⢀⢀⣿⠀⢧⠀
    ⠀⣰⠃⢸⠁⣿⠀⠸⣧⠀⢸⢣⠋⠀⣠⣤⠶⢶⢒⣤⣔⣻⠣⢼⠟⠁⠀⠙⢷⡄⠀⠀⢀⠀⠀⠀⠈⠓⢟⢦⠀⠀⠀⢸⡇⠈⠻⣦⡀⠈⠻⣷⣄⠀⠘⣿⠀⠸⣿⣇⣀⢸⠀
    ⠀⡇⠀⠀⣼⠇⠀⢀⣿⠀⣇⣇⣴⠟⠋⢠⣾⠟⠉⠀⠀⠈⠳⣼⠀⠀⠀⠀⠀⠳⠀⠀⠈⢳⣄⠀⠀⠀⢸⣼⠀⠀⠀⠈⡟⢆⠀⠈⢻⡀⠀⠈⢻⣆⠀⣻⠃⠀⠀⢹⡟⠻⡀
    ⠀⢧⡆⣼⠏⠀⣾⠟⠁⢰⠃⡵⠃⢀⣴⡿⠁⠀⡀⠀⠀⠀⠀⠹⣧⡀⠰⣦⡀⠀⠀⠀⠀⠀⣻⢦⣀⣠⡾⣇⠀⠀⢀⣰⠟⠙⢷⣄⠀⠀⠀⠀⠀⣿⠀⠉⢠⠄⠀⣼⡇⠀⢧            Neuralynx HDF5 Converter
    ⢀⠞⢡⡟⠀⠀⣿⠀⢀⡏⡼⠁⣴⠟⠁⠀⠀⠀⣿⠀⣀⣀⢀⣴⠘⣷⡀⠈⢻⣦⣀⠀⢀⣾⠟⠉⠀⠀⠉⠻⣷⣄⠀⠀⠀⠀⠀⠙⢷⡄⠀⠀⠀⠉⠀⣠⡟⠀⣼⣟⠀⠀⢸                 
    ⢸⠀⠘⣧⠀⡴⠛⠳⢸⢰⠁⢰⠏⠀⠀⠀⢀⣼⡯⠟⠋⠙⠻⣷⡀⠘⠀⠀⠀⠈⠉⠻⣿⠁⠀⠀⢰⡟⠉⠀⠈⢻⣦⠀⠀⠀⣄⠀⠀⡗⠀⢸⡇⣠⣾⠟⢀⣾⠋⠹⣷⢀⡇
    ⠈⢆⠀⠹⢷⣤⣀⣠⠎⡇⠀⠸⠀⠀⢀⣴⠟⠉⠀⠀⠀⢄⠀⠹⣧⡀⠀⠀⣀⡀⠀⠀⣿⠀⠀⠀⠘⣿⡄⠀⠀⠀⢹⣦⡀⠀⢿⣄⠀⢀⣠⡿⠽⣯⡁⠀⠸⠃⠀⠀⡏⠉⠀
    ⠀⢠⢷⣄⠀⠈⣉⣉⢢⢳⡀⠀⠀⠀⣾⡏⠀⠀⠠⣀⡤⢿⠀⠀⠙⠷⠶⠛⠉⠈⠀⣰⠟⠀⠀⠀⠀⠘⣷⡀⠀⠠⠛⠉⠉⠀⢈⣯⠗⠛⠁⠀⠀⠈⠃⠀⢀⣴⠇⢠⠇⠀⠀
    ⠀⢸⡀⠻⣧⠈⠉⠹⣏⢀⣑⠤⣀⣀⠼⠳⣄⠀⠀⠀⠙⠺⠖⣦⣤⠤⣀⡀⠀⠀⠘⠁⠀⠀⠀⠀⠀⠀⢸⢧⡀⠀⠀⢀⣀⢴⣿⣅⡀⠠⠶⢿⢦⣀⣠⣴⠟⠃⡠⠋⠀⠀⠀
    ⠀⠀⠳⡀⠘⠃⠀⡤⠸⣼⠀⠉⠛⠋⠉⠉⠙⠻⣦⣄⠀⠀⠀⠀⠈⠉⠙⠻⣦⠀⠀⠀⠀⡀⠀⠀⠀⢀⣾⠖⠚⠛⠛⠛⠋⠁⠀⠙⣷⠀⠀⣸⡴⠛⠉⢁⡤⠊⠁⠀⠀⠀⠀
    ⠀⠀⠀⠘⢦⡀⠸⣧⠀⢻⢇⠀⠳⡤⣤⠆⠀⠀⠈⢻⡇⠀⠀⠀⢰⡄⠀⠀⣿⡇⢀⡾⠛⠛⠻⡝⣲⠟⠋⠀⢀⡄⠀⠀⠀⣀⡄⠀⠋⢀⡴⣻⡄⣤⡶⡍⠀⠀⠀⠀⠀⠀⠀
    ⠀⠀⠀⠀⠀⠈⠙⠁⠉⠉⠈⠣⡀⠹⣇⠀⠀⠀⠀⠘⠀⠀⠀⢀⡾⢳⡶⠾⠋⠀⠈⠃⠀⠀⣠⠟⢄⣀⣠⡴⠋⠀⠀⠀⣼⢻⣤⣴⠶⠟⠋⣡⡷⣏⢿⡧⠀⠀⠀⠀⠀⠀⠀
    ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠓⢫⣲⣤⣀⣀⣀⣀⣤⣶⣻⠋⠛⠷⣦⣤⣤⣄⡤⢤⣺⠕⠋⠉⠉⠁⠀⠀⣀⣤⣾⠏⢩⠀⠀⢀⣤⣾⠛⣧⢻⣼⠀⠀⠀⠀⠀⠀⠀⠀
    ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠓⠮⢭⣉⣉⡩⠥⠚⠈⢇⠀⢠⡄⠀⠉⠉⠙⣿⠀⢠⠶⠖⢫⣩⠟⠛⠛⠉⠀⣠⣿⣦⠶⠿⣭⣸⣇⡿⠞⠁⠀⠀⠀⠀⠀⠀⠀⠀
    ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠳⣌⡿⣄⠀⠒⠚⠋⠀⠀⠀⣠⡾⠃⠀⢀⣀⠴⠚⠉⠣⢍⣛⣶⡶⠝⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
    ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⠒⠂⠀⠒⠒⠉⠀⠉⠉⠉⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀


    __٨ﮩ__٨ﮩ__٨ﮩ__٨ﮩ__٨ﮩ____٨ﮩ____٨ﮩ٨ﮩ٨ﮩ__٨ﮩ٨ﮩ___٨ﮩ__٨ﮩ_____٨ﮩ٨ﮩ___٨ﮩ___٨ﮩ___٨ﮩ______٨ﮩ______٨ﮩ___٨ﮩ٨ﮩ___٨ﮩ
)" << std::endl;
}

void printProgress(size_t done, size_t total) {
    int barWidth = 50;
    float progress = float(done) / total;
    std::cout << "[";
    int pos = barWidth * progress;
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << int(progress * 100.0) << " %\r";
    std::cout.flush();
}

// ▓▓▓▓▓▓▓▓ ▓▓▓▓       MAIN       ▓▓▓▓ ▓▓▓▓▓▓▓▓ 

int main() {
    printWelcomeBanner();
    //  PEDIR CARPETA CON CSCN
    std::string dir_path;
    std::string nev_path;
    std::vector<std::string> csc_files;

    std::cout << BLUE << "Where are your .ncs files?\n";
    std::cout << "(Press "<< GREEN << "ENTER" << BLUE << " to use the current folder)" << RESET;
    std::getline(std::cin, dir_path);
    std::cout << "✔ \n\n";
    std::cout << BLUE << "Where is the .nev file?\n";
    std::cout << "(Press "<< GREEN << "ENTER" << BLUE << " to use the current folder)" << RESET;
    std::getline(std::cin, dir_path);
    std::cout << "✔ \n\n";

    { // BUSCAR ARCHIVOS EN CARPETA
        if (dir_path.empty()) { // Enter vacio. carpeta actual
            dir_path = std::filesystem::current_path().string();
        }
        if (nev_path.empty()) {
            nev_path = std::filesystem::current_path().string();
        }
        nev_path = std::filesystem::path(nev_path) / "Events.nev";

        std::cout << BLUE << "Using path: " << GREEN <<  dir_path << BLUE <<"\n";   


        
        
        for (auto& entry : std::filesystem::directory_iterator(dir_path)) {
            if (entry.path().filename().string().rfind("CSC", 0) == 0 &&
                entry.path().extension() == ".ncs") {
                    csc_files.push_back(entry.path().string());
            }
        }

        if (csc_files.empty()) { // NO SE ENCONTRARON ARCHIVOS CSCN
        std::cerr << "\nFound "<<GREEN<<"0"<<BLUE<<" channels.\n ...quiting";
        return 1;}
        else { 
            std::sort(csc_files.begin(), csc_files.end(),
                [](const std::string &a, const std::string &b){
                    std::regex re("CSC(\\d+)\\.ncs");
                    std::smatch ma, mb;

                    std::string name_a = std::filesystem::path(a).filename().string();
                    std::string name_b = std::filesystem::path(b).filename().string();
                    
                    std::regex_match(name_a, ma, re);
                    std::regex_match(name_b, mb, re);

                    return std::stoi(ma[1]) < std::stoi(mb[1])
                    ;
                });
                
                
        // Verificar existencia
        if (std::filesystem::exists(nev_path)) {
            std::cout << "Found "<< GREEN << "Events.nev" << BLUE << " and ";
        } else {
            std::cerr << ".nev NOT found: " << nev_path << "\n";
            return 1; // o manejar el error como quieras
        }
                
        std::cout << GREEN << csc_files.size() << BLUE << " channels. \n"; 
        
                
        }
    }
    
    //   LEER HEADER DEL PRIMERO
    
    std::ifstream file(csc_files[0], std::ios::binary);
    std::vector<char> header_buf(HEADER_BYTES);
    file.read(header_buf.data(), HEADER_BYTES);

    std::string header_str(header_buf.begin(), header_buf.end());
    std::istringstream header_stream(header_str);
    std::string line;
    std::map<std::string, std::string> header_vars;
    
    //  GUARDA HEADER
    while (std::getline(header_stream, line)) { // guarda en header_vars[key]
        if (line.empty() || line[0] != '-') continue;  // ignorar líneas vacías o que no empiecen con '-'
        auto pos = line.find(' ');
        if (pos != std::string::npos) {
            std::string key = line.substr(1, pos-1);  // quitar el '-'
            std::string value = line.substr(pos+1);
            header_vars[key] = value;
        }
    }
    
    double SF = std::stod(header_vars["SamplingFrequency"]);
    double adBitVolts   = std::stod(header_vars["ADBitVolts"]);
    //size_t numChannels = std::stoi(header_vars["NumADChannels"]);
    int adMax       = std::stoi(header_vars["ADMaxValue"]);
    double lowCut   = std::stod(header_vars["DspLowCutFrequency"]);
    double highCut  = std::stod(header_vars["DspHighCutFrequency"]);
    
    size_t total_samples = 0;

    //  ▓▓▓▓▓▓▓▓  CONTAR REGISTROS

    NCS_REG reg{};        
    uint64_t last_ts = 0;
    bool first = true;
    size_t cnt_lt16k = 0;
    size_t cnt_eq16k = 0;
    size_t cnt_gt16k = 0;
    size_t cnt_invalid_samples = 0;
    uint64_t gap_total_us = 0;
    uint64_t overlap_total_us = 0;
    
    size_t registro_num = 0;
    
    while(file.read(reinterpret_cast<char*>(&reg), REG_BYTES)) {
        registro_num++;
        
        if(!first) {
            double dt_us = static_cast<double>(reg.timestamp - last_ts); // microsegundos según Neuralynx
            
            if(dt_us < 16000.0) {
                cnt_lt16k++;
                overlap_total_us += static_cast<uint64_t>(16000.0 - dt_us);
            }
            else if(dt_us == 16000.0) {
                cnt_eq16k++;
            }
            else { // dt_us > 16000
                cnt_gt16k++;
                gap_total_us += static_cast<uint64_t>(dt_us - 16000.0);
            }
        }
        else {
            first = false;
            
        }
        
        if(reg.nValidSamples != SAMPLES_PER_REG) cnt_invalid_samples++;
        total_samples += reg.nValidSamples;
        last_ts = reg.timestamp;
    }

    int show_res = 0;
    std::cout << "\nShow metadata?\n" << GREEN << " 0" << BLUE<<"-> NO | " << GREEN << "1"<< BLUE <<" -> YES\n" << RESET;
    std::cin >> show_res;
    if (show_res)
    {//  ▓▓▓▓▓▓▓▓  SHOW VARIABLES ▓▓▓▓▓▓▓▓
    std::cout << "\n" << BLUE << "==================================" << BLUE << "\n";
    std::cout << GREEN << "            HEADER" << BLUE << "\n";
    std::cout << BLUE << "==================================" << BLUE << "\n";

    std::cout << "Sampling Frequency  : " << GREEN << SF << " Hz" << BLUE << "\n";
    std::cout << "AD Bit-to-Volt      : " << GREEN << adBitVolts << " V/bit" << BLUE << "\n";
    std::cout << "Number of channels  : " << GREEN << csc_files.size() << BLUE << "\n";
    std::cout << "Max AD Value        : " << GREEN << adMax << BLUE << "\n";
    std::cout << "Filter Low Cut      : " << GREEN << lowCut << " Hz" << BLUE << "\n";
    std::cout << "Filter High Cut     : " << GREEN << highCut << " Hz" << BLUE << "\n";

    std::cout << "\n" << BLUE << "==================================" << BLUE << "\n";
    std::cout << GREEN << "           RECORDS" << BLUE << "\n";
    std::cout << BLUE << "==================================" << BLUE << "\n";

    std::cout << "Total records       : " << GREEN << cnt_lt16k + cnt_eq16k + cnt_gt16k << BLUE << "\n";
    std::cout << "Records dt < 16k µs : " << GREEN << cnt_lt16k << BLUE << "\n";
    std::cout << "Records dt = 16k µs : " << GREEN << cnt_eq16k << BLUE << "\n";
    std::cout << "Records dt > 16k µs : " << GREEN << cnt_gt16k << BLUE << "\n";
    std::cout << "Invalid sample count : " << GREEN << cnt_invalid_samples << BLUE << "\n";
    std::cout << "Total samples       : " << GREEN << total_samples << BLUE << "\n";
    std::cout << "Total gap (µs)      : " << GREEN << gap_total_us << BLUE << "\n";
    std::cout << "Total overlap (µs)  : " << GREEN << overlap_total_us << BLUE << "\n";
    std::cout << BLUE << "==================================" << "\n";
    }
    
    
    //   DOWNSAMPLEO
    
    double target_freq = 4000.0;  // Frecuencia objetivo
    double downsample_ratio = SF / target_freq;  // 32000/4000 = 8
    int step = static_cast<int>(std::round(downsample_ratio));    
    double dt_downsampled = 1 / target_freq;
    size_t total_downsamples = total_samples / step; 
    int save = 0;
    std::cout << BLUE <<"\nCreate .h5 file with downsampled data? ( " << GREEN << "4000" << BLUE << " samples/s )\n" << GREEN << " 0" << BLUE<<"-> NO | " << GREEN << "1"<< BLUE <<" -> YES\n" << RESET;
    std::cin >> save;
    if (save) {  //  H5   
        std::string filename;
        std::cout << BLUE << "Name the h5 file (must en with .h5): " << RESET ;
        std::cin >> filename;
        H5::H5File h5file(filename, H5F_ACC_TRUNC);
        // ▓▓▓▓▓▓▓▓ H5 - Vector tiempo
        std::cout << BLUE << "\n Generating time vector, at dataset \"time\" \n"; 
        size_t CHUNK_SIZEt=1024*1024;
        std::vector<double> CHUNKt(CHUNK_SIZEt);

        hsize_t dims_time[2] = {1, total_downsamples};
        hsize_t dims_chunkt[2] = {1, CHUNK_SIZEt};
        
        H5::DataSpace dataspace_time( 2, dims_time );
        H5::FloatType datatype_time( H5::PredType::NATIVE_DOUBLE ); 

        H5::DSetCreatPropList tlist;
        tlist.setChunk(2, dims_chunkt);  // define chunk
        tlist.setDeflate(6);            // compresión gzip nivel 6

        H5::DataSet dataset_time = h5file.createDataSet(
            "time", datatype_time, dataspace_time, tlist
        );
        
        hsize_t offset[2] = {0, 0};
        hsize_t count[2] = {1, CHUNK_SIZEt};
        H5::DataSpace filespace_t = dataset_time.getSpace();
        
        size_t indices_t = 0;
        float progress = 0.0;
        while(indices_t < total_downsamples) { // VECTOR TIME  
            
            size_t CHUNK_ACTUAL = CHUNK_SIZEt; // ajustar ultimo bloque
            if(indices_t + CHUNK_SIZEt > total_downsamples)
                CHUNK_ACTUAL = total_downsamples - indices_t;

            for(size_t i = 0; i < CHUNK_ACTUAL; i++)
                CHUNKt[i] = static_cast<double>(indices_t + i) * dt_downsampled;

            count[1] = CHUNK_ACTUAL;
            
            filespace_t.selectHyperslab(H5S_SELECT_SET, count, offset);

            hsize_t mem_dimst[2] = {1, CHUNK_ACTUAL};
            H5::DataSpace memspacet(2, mem_dimst);

            dataset_time.write(CHUNKt.data(), H5::PredType::NATIVE_DOUBLE, memspacet, filespace_t);
            progress = 100.0 * indices_t / total_downsamples;
            std::cout << "\r" << BLUE << "⏳" << int(progress) << "% " << std::flush;
            
            offset[1] += CHUNK_ACTUAL;
            indices_t += CHUNK_ACTUAL;
            
        }
         // ▓▓▓▓▓▓▓▓ H5 - Data y headers por archivo 
            
        size_t CHUNK_SIZEv=1024*1024*4;
        std::vector<float> CHUNKv(CHUNK_SIZEv);

        hsize_t dims_v[2] = {csc_files.size(), total_downsamples};
        hsize_t dims_chunkv[2] = {1, CHUNK_SIZEv}; 

        H5::DataSpace dataspace_volt( 2, dims_v );
        H5::FloatType datatype_volt( H5::PredType::NATIVE_FLOAT ); 
        
        H5::DSetCreatPropList vlist;
        vlist.setChunk(2, dims_chunkv);  // define chunk
        vlist.setDeflate(6);            // compresión gzip nivel 6

        H5::DataSet dataset_volt = h5file.createDataSet(
            "volt", datatype_volt, dataspace_volt, vlist
        );
        
        offset[0] = offset[1] = 0;
        count[0] = 1;
        count[1] = CHUNK_SIZEv; 
        H5::DataSpace filespace_v = dataset_volt.getSpace();
        
        progress = 0.0;
        size_t canal=0;
        
        float canal_progress;
        size_t sample_global;
        size_t chunk_idx;           // posición dentro del chunk
        size_t total_written;  

        for(const auto& fpath : csc_files) {
            
            sample_global=0;
            chunk_idx = 0;           // posición dentro del chunk
            total_written = 0;       // posición total a lo largo del canal
            

            std::ifstream file(fpath, std::ios::binary);


            file.seekg(HEADER_BYTES, std::ios::beg);
            // por canales
            while(file.read(reinterpret_cast<char*>(&reg), REG_BYTES)) {
                // por registros
                
                for(size_t i = 0; i < reg.nValidSamples; ++i) {
                    // por sample 
                    if(sample_global % step == 0) { // si el sample es 1 de cada 8 
                        CHUNKv[chunk_idx] = static_cast<float>(reg.samples[i]) * adBitVolts; // escribo en el chunk

                        chunk_idx++;    // indice dentro del chucnk
                        total_written++; //indice total

                        if (chunk_idx == CHUNK_SIZEv || total_written == total_downsamples) { // si es el ultimo del chunk o termina

                                    // escribir CHUNKv al HDF5
                                    count[0] = 1;               // 1 canal
                                    count[1] = chunk_idx;       // cantidad real de samples
                                    offset[0] = canal;          // fila del canal
                                    offset[1] = total_written - chunk_idx; // posición temporal del chunk

                                    filespace_v = dataset_volt.getSpace();
                                    filespace_v.selectHyperslab(H5S_SELECT_SET, count, offset);

                                    hsize_t mem_dimsv[2] = {1, chunk_idx};
                                    H5::DataSpace memspacev(2, mem_dimsv);

                                    dataset_volt.write(CHUNKv.data(), H5::PredType::NATIVE_FLOAT, memspacev, filespace_v);

                                    canal_progress = 100.0f * total_written / total_downsamples;
                                    std::cout << BLUE << "\rWRITING CHANNEL " <<GREEN << canal+1 << BLUE << ": ⏳" 
                                            << int(canal_progress) << "% " << std::flush;

                                    // resetear chunk_idx
                                    chunk_idx = 0;
                                } // if (chunk_idx == CHUNK_SIZEv || total_written == total_downsamples) 

                    } // if(sample_global % step == 0) {


                    sample_global++;
                
                
                } // for i < validsamples

            } // while cada registro
        
        canal++;
        } // cada archivo




        
        //eventos
        
    /*

    std::ifstream nevfile(nev_path, std::ios::binary);
    if (!nevfile) {
        std::cerr << "\nNo se puede abrir Events.nev \n Se intentó abrir: " << nev_path << "\n";
        return 1;
        }
        

    nevfile.seekg(HEADER_BYTES, std::ios::beg);
    int nev_count = 0;
    // Leer todos los registros
    std::vector<NEV_RECORD> records;
    NEV_RECORD nev_reg;
    
    while (nevfile.read(reinterpret_cast<char*>(&nev_reg), sizeof(NEV_RECORD))) {
        records.push_back(nev_reg);
        nev_count++;
        std::cout << "Registro " << nev_count 
        << " TS: " << nev_reg.TimeStamp
        << " evt_id: " << nev_reg.event_id
        << " TTL: " << nev_reg.ttl
        << " str: " << std::string(nev_reg.EventString, nev_reg.EventString + 20)
        << "\n";
        }
        std::cout << "Leídos " << records.size() << " registros NEV\n";
        
        */
       
       struct NevRow {
        uint64_t ts;
        char str[128];
    };

    std::ifstream nevfile(nev_path, std::ios::binary);
    if (!nevfile) {
        std::cerr << BLUE << "Failed opening Events.nev: "<< GREEN << nev_path << BLUE << "\n";
        return 1;
    }
    
    nevfile.seekg(HEADER_BYTES, std::ios::beg);
    
    std::vector<NEV_RECORD> records;
    
    NEV_RECORD nev_reg;
    while (nevfile.read(reinterpret_cast<char*>(&nev_reg), sizeof(NEV_RECORD))) {
        records.push_back(nev_reg);
    }
    nevfile.close();
    double t0 =  records[0].TimeStamp;
    double dt_downsampled = 1e6 / target_freq;

    // Preparar datos para HDF5
    std::vector<NevRow> data(records.size());
    for (size_t i = 0; i < records.size(); i++) {
        uint64_t ts_rel = records[i].TimeStamp - t0; // μs relativo al inicio
        size_t sample_idx = static_cast<size_t>(std::round(ts_rel / dt_downsampled));
        data[i].ts = sample_idx * dt_downsampled; 
        std::fill(std::begin(data[i].str), std::end(data[i].str), 0);
        std::copy(records[i].EventString,
            records[i].EventString + 128,
            data[i].str);
        }
 
    // Definir tipo compuesto
    H5::CompType mtype(sizeof(NevRow));
    mtype.insertMember("TimeStamp", HOFFSET(NevRow, ts), H5::PredType::NATIVE_UINT64);
    mtype.insertMember("EventString", HOFFSET(NevRow, str),
    H5::StrType(H5::PredType::C_S1, 128));

    hsize_t dims[1] = { records.size() };
    H5::DataSpace dataspace(1, dims);
    
    H5::DataSet datasetnev = h5file.createDataSet("events", mtype, dataspace);
    datasetnev.write(data.data(), mtype);
    
    std::cout << BLUE <<"\nSaved " << GREEN  << records.size() << BLUE << " events.\n";
    
}// termina el guardado del h5
    
    
    
    



    std::cout << BLUE << "\nFINISHED " << RESET << " ✔\n";
    return 0; // FIN MAINs
}