#include <windows.h>
#include <iostream>
#include <fstream>
#include <json/json.h>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <conio.h>


// Функция для чтения данных из COM-порта
std::string ReadFromSerialPort(HANDLE hSerial) {
    DWORD bytesRead;
    const int bufferSize = 256;
    char buffer[bufferSize];
    if (!ReadFile(hSerial, buffer, bufferSize - 1, &bytesRead, NULL)) {
        std::cerr << "Error reading from serial port" << std::endl;
        return "";
    }
    else {
        buffer[bytesRead] = '\0';
        return std::string(buffer);
    }
}


int main() {
    // Открываем COM порт
    HANDLE hSerial = CreateFile(L"\\\\.\\COM1", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hSerial == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            std::cerr << "Serial port doesn't exist!" << std::endl;
        }
        else {
            std::cerr << "Error opening serial port!" << std::endl;
        }
        return 1;
    }

    // Настраиваем COM порт
    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "Error getting serial port state" << std::endl;
        return 1;
    }
    dcbSerialParams.BaudRate = CBR_2400;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    if (!SetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "Error setting serial port state" << std::endl;
        return 1;
    }

    char keyPressed;

    std::ofstream file("sensor_data.json", std::ios::app);
    Json::StreamWriterBuilder builder;
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    file << "[";
    file << std::endl;

    // Чтение данных из порта и вывод на экран
    while (true) {

        std::string input = ReadFromSerialPort(hSerial);
        if (!input.empty()) {
            // Проверяем, соответствует ли сообщение формату и обрабатываем
            size_t start = input.find('$');
            size_t end = input.find("\r\n");
            if (start != std::string::npos && end != std::string::npos) {
                std::string rawData = input.substr(start + 1, end - start - 1);
                size_t commaPos = rawData.find(',');
                std::string ws = rawData.substr(0, commaPos);
                std::string wd = rawData.substr(commaPos + 1);

                Json::Value jsonData;

                auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                std::tm now_tm;
                localtime_s(&now_tm, &now);
                std::stringstream ss;
                ss << std::put_time(&now_tm, "%F %T");

                jsonData["time"] = ss.str();
                jsonData["sensor_name"] = "WMT700";
                jsonData["wind_speed"] = std::stof(ws);
                jsonData["wind_direction"] = std::stof(wd);

                writer->write(jsonData, &file);
                if (_kbhit()) {
                    char keyPressed = _getch();
                    std::cout << "Stopping the program..." << std::endl;
                    break;
                }
                file << ",";
                file << std::endl;
            }
        }
        
    }

    file << std::endl;
    file << "]";
    file.close();

    // Закрываем COM порт
    CloseHandle(hSerial);

    return 0;
}