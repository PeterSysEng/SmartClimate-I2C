/*
 *  Data logger program.
 *
 *
 *
 * */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>

#define SERIAL_PORT "/dev/ttyACM0"
#define CSV_FILE    "log_data.csv"

int configure_serial_port(int fd) {
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) return -1;

    // Baud rate beállítása 115200-ra
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);

    // 8N1 beállítások (8 adatbit, nincs paritás, 1 stoppbit)
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag |= CREAD | CLOCAL; // Vétel engedélyezése

    // Nyers mód (Raw mode) - karakterenkénti feldolgozáshoz
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY | IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    tty.c_oflag &= ~OPOST;

    // Időzítések: Várjon, amíg legalább 1 karakter érkezik
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 5;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) return -1;
    return 0;
}

int main() {
    // 1. Soros port megnyitása olvasásra (O_RDONLY)
    int serial_fd = open(SERIAL_PORT, O_RDONLY | O_NOCTTY);
    if (serial_fd < 0) {
        perror("Hiba a soros port megnyitasakor");
        return EXIT_FAILURE;
    }

    // Port konfigurálása
    if (configure_serial_port(serial_fd) < 0) {
        perror("Hiba a port beallitasanal");
        close(serial_fd);
        return EXIT_FAILURE;
    }

    // 2. CSV fájl megnyitása hozzáfűzésre (Append)
    FILE *csv_file = fopen(CSV_FILE, "a");
    if (!csv_file) {
        perror("Hiba a CSV fajl megnyitasakor");
        close(serial_fd);
        return EXIT_FAILURE;
    }

    // Ha a fájl üres, írjunk rá egy fejlécet
    fseek(csv_file, 0, SEEK_END);
    if (ftell(csv_file) == 0) {
        fprintf(csv_file, "Timestamp,Raw Data\n");
    }

    printf("Data Logger elindult. Port: %s, Mentes: %s\n", SERIAL_PORT, CSV_FILE);
    printf("Nyomj Ctrl+C-t a kilepeshez.\n");

    char read_buf[256];
    char line_buf[512];
    int line_index = 0;

    // 3. Végtelen ciklus a beérkező adatok olvasására
    while (1) {
        int n = read(serial_fd, read_buf, 1); // Karakterenként olvasunk
        if (n > 0) {
            char c = read_buf[0];
            
            // Ha nem soremelés, gyűjtjük a karaktereket
            if (c != '\n' && c != '\r' && line_index < sizeof(line_buf) - 1) {
                line_buf[line_index++] = c;
            } 
            // Ha megvan egy teljes sor (soremelés jött és van benne adat)
            else if ((c == '\n' || c == '\r') && line_index > 0) {
                line_buf[line_index] = '\0'; // Lezárjuk a szöveget

                // Időbélyeg lekérése (Timestamp)
                time_t now = time(NULL);
                struct tm *t = localtime(&now);
                char time_str[32];
                strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);

                // Kiírás a terminálra és mentés a CSV fájlba
                printf("[%s] %s\n", time_str, line_buf);
                fprintf(csv_file, "%s,%s\n", time_str, line_buf);
                fflush(csv_file); // Azonnali lemezre írás

                line_index = 0; // Puffer ürítése a következő sornak
            }
        }
    }

    // Bezárás (ide a Ctrl+C miatt sosem jut el, de szép szokás)
    fclose(csv_file);
    close(serial_fd);
    return EXIT_SUCCESS;
}
