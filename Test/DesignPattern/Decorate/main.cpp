#include <iostream>
#include <filestreamreader.hpp>
#include <decryptstreamreader.hpp>
#include <imagedecodestreamreader.hpp>

int main() {
    FileStreamReader f;
    DecryptStreamReader ds{&f};
    ImageDecodeStreamReader ids{&ds};
    ids.open("../../Test/DesignPattern/Decorate/testfile");
    while (true) {
        uint8_t buffer[1024]{};

        if (ids.read(buffer,std::size(buffer)) <= 0) {
            break;
        }
        std::cout << buffer << std::endl;
    }
    ids.close();
    return 0;
}

