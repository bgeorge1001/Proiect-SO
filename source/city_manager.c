#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>

// --- 1. DEFINIȚII ȘI STRUCTURI ---
#define MAX_NAME_LEN 50
#define MAX_CAT_LEN 30
#define MAX_DESC_LEN 256

typedef struct {
    int report_id;
    char inspector_name[MAX_NAME_LEN];
    float latitude;
    float longitude;
    char category[MAX_CAT_LEN];
    int severity;
    time_t timestamp;
    char description[MAX_DESC_LEN];
} Report;

void init_district(const char *district_name) {
    char filepath[256];

  
    mkdir(district_name, 0750); 
    chmod(district_name, 0750); 

   
    snprintf(filepath, sizeof(filepath), "%s/reports.dat", district_name);
    int fd_reports = open(filepath, O_CREAT | O_WRONLY | O_APPEND, 0664);
    if (fd_reports != -1) {
        fchmod(fd_reports, 0664); 
        close(fd_reports);
    }

    
    snprintf(filepath, sizeof(filepath), "%s/district.cfg", district_name);
    int fd_cfg = open(filepath, O_CREAT | O_WRONLY, 0640);
    if (fd_cfg != -1) {
        fchmod(fd_cfg, 0640);
        close(fd_cfg);
    }

    
    snprintf(filepath, sizeof(filepath), "%s/logged_district", district_name);
    int fd_log = open(filepath, O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (fd_log != -1) {
        fchmod(fd_log, 0644);
        close(fd_log);
    }
    // ... [codul existent cu mkdir și open din init_district] ...

    // 5. Crearea linkului simbolic
    char symlink_name[256];
    snprintf(symlink_name, sizeof(symlink_name), "active_reports-%s", district_name);
    
    // Numele fișierului țintă (target)
    char target_name[256];
    snprintf(target_name, sizeof(target_name), "%s/reports.dat", district_name);

    // symlink() returnează 0 pe succes. Dacă dă eroare, verificăm să nu fie pentru că există deja.
    if (symlink(target_name, symlink_name) == -1) {
        // Ignorăm eroarea "File exists", afișăm doar dacă e o altă problemă
        if (errno != EEXIST) {
            perror("Eroare la crearea linkului simbolic");
        }
    }
}

void add_report(const char *district, const char *user) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/reports.dat", district);

    // 1. Deschidem fișierul pentru adăugare (O_APPEND)
    int fd = open(filepath, O_WRONLY | O_APPEND);
    if (fd == -1) {
        perror("Eroare: Nu am putut deschide reports.dat. Exista districtul?");
        return;
    }

    // 2. Aflăm ce ID trebuie să îi dăm noului raport 
    // Folosim lseek pentru a muta cursorul la finalul fișierului și a afla dimensiunea totală în byți
    off_t file_size = lseek(fd, 0, SEEK_END);
    
    // Împărțind dimensiunea totală la dimensiunea unei structuri, obținem numărul de rapoarte
    // Ex: Dacă fișierul are 0 byți, 0 / sizeof(Report) = 0 (primul ID va fi 0)
    int new_id = file_size / sizeof(Report); 

    // 3. Construim structura de date în memorie
    Report new_report;
    memset(&new_report, 0, sizeof(Report)); // Inițializăm tot blocul de memorie cu zero

    new_report.report_id = new_id;
    strncpy(new_report.inspector_name, user, MAX_NAME_LEN - 1); // Preluăm din --user 
    new_report.timestamp = time(NULL); // Generăm timestamp-ul curent automat [cite: 22]

    // Pentru Faza 1, ca să păstrăm lucrurile "cât mai simple posibil", 
    // vom hardcoda restul detaliilor. Mai târziu le poți cere de la tastatură cu scanf()
    new_report.latitude = 47.05; 
    new_report.longitude = 21.93;
    strncpy(new_report.category, "road", MAX_CAT_LEN - 1); // [cite: 20]
    new_report.severity = 2; // [cite: 21]
    strncpy(new_report.description, "Groapa periculoasa pe banda 2", MAX_DESC_LEN - 1); // [cite: 23]

    // 4. Scriem direct blocul de byți (structura) în fișier
    ssize_t bytes_written = write(fd, &new_report, sizeof(Report));
    if (bytes_written != sizeof(Report)) {
        perror("Eroare la scrierea datelor binare");
    } else {
        printf("Succes! Raportul #%d a fost adaugat de '%s' in %s.\n", new_id, user, district);
    }

    close(fd);
}

// Funcție pentru a converti st_mode într-un string de tip "rwxr-xr--"
void mode_to_string(mode_t mode, char *str) {
    strcpy(str, "---------"); // Inițializăm cu liniuțe
    
    // Verificăm fiecare bit folosind operatorul "și" pe biți (&)
    if (mode & S_IRUSR) str[0] = 'r'; // Owner Read
    if (mode & S_IWUSR) str[1] = 'w'; // Owner Write
    if (mode & S_IXUSR) str[2] = 'x'; // Owner Execute
    if (mode & S_IRGRP) str[3] = 'r'; // Group Read
    if (mode & S_IWGRP) str[4] = 'w'; // Group Write
    if (mode & S_IXGRP) str[5] = 'x'; // Group Execute
    if (mode & S_IROTH) str[6] = 'r'; // Others Read
    if (mode & S_IWOTH) str[7] = 'w'; // Others Write
    if (mode & S_IXOTH) str[8] = 'x'; // Others Execute
}

void list_reports(const char *district) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/reports.dat", district);

    // 1. Apelăm stat() pentru a obține informațiile despre fișier
    struct stat file_stat;
    if (stat(filepath, &file_stat) == -1) {
        perror("Eroare: Nu am putut gasi reports.dat. Exista districtul?");
        return;
    }

    // 2. Afișăm detaliile fișierului cerute de proiect
    char perms[10];
    mode_to_string(file_stat.st_mode, perms);
    
    // ctime convertește timestamp-ul într-un text citibil
    char time_str[26];
    strncpy(time_str, ctime(&file_stat.st_mtime), sizeof(time_str));
    time_str[strcspn(time_str, "\n")] = 0; // Scoatem enter-ul de la final pus de ctime

    printf("--- INFO FISIER ---\n");
    printf("Fisier: %s\nPermisiuni: %s\nDimensiune: %ld bytes\nModificat: %s\n\n", 
           filepath, perms, file_stat.st_size, time_str);

    // 3. Deschidem fișierul doar pentru citire (O_RDONLY)
    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        perror("Eroare la deschiderea fisierului pentru citire");
        return;
    }

    // 4. Citim rapoartele rând pe rând într-o buclă
    Report r;
    printf("--- LISTA RAPOARTE ---\n");
    int count = 0;
    
    // read() returnează numărul de byți citiți. Dacă citește fix cât structura noastră, înseamnă că a găsit un raport valid.
    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        printf("[ID: %d] Categorie: %s | Severitate: %d | Inspector: %s\n",
               r.report_id, r.category, r.severity, r.inspector_name);
        count++;
    }

    if (count == 0) {
        printf("Nu exista rapoarte in acest district.\n");
    }

    close(fd);
}

void remove_report(const char *district, const char *role, int target_id) {
    // 1. Verificăm rolul (Doar managerii au voie)
    if (strcmp(role, "manager") != 0) {
        printf("Eroare de securitate: Doar utilizatorii cu rol de 'manager' pot sterge rapoarte!\n");
        return;
    }

    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/reports.dat", district);

    // Deschidem fișierul pentru citire ȘI scriere (O_RDWR)
    int fd = open(filepath, O_RDWR);
    if (fd == -1) {
        perror("Eroare: Nu am putut deschide reports.dat");
        return;
    }

    // Aflăm câte rapoarte sunt în total
    off_t file_size = lseek(fd, 0, SEEK_END);
    int total_reports = file_size / sizeof(Report);

    if (target_id < 0 || target_id >= total_reports) {
        printf("Eroare: Raportul cu ID-ul %d nu exista.\n", target_id);
        close(fd);
        return;
    }

    // 2. Logica de shiftare (mutare a datelor)
    Report temp;
    // Începem de la raportul IMEDIAT URMĂTOR celui pe care vrem să îl ștergem
    for (int i = target_id + 1; i < total_reports; i++) {
        // Citim raportul i
        lseek(fd, i * sizeof(Report), SEEK_SET);
        read(fd, &temp, sizeof(Report));

        // Opțional: Putem să îi updatăm ID-ul ca să nu avem "goluri" în numărătoare
        temp.report_id = i - 1;

        // Ne întoarcem cu o poziție în spate (peste cel vechi/șters) și îl suprascriem
        lseek(fd, (i - 1) * sizeof(Report), SEEK_SET);
        write(fd, &temp, sizeof(Report));
    }

    // 3. Tăierea fișierului (ftruncate)
    // Acum că am mutat totul la stânga, ultimul bloc de memorie a rămas dublat.
    // Calculăm noua dimensiune dorită (cu un raport mai puțin) și tăiem fișierul.
    off_t new_size = (total_reports - 1) * sizeof(Report);
    if (ftruncate(fd, new_size) == -1) {
        perror("Eroare la trunchierea fisierului");
    } else {
        printf("Raportul cu ID-ul %d a fost sters cu succes.\n", target_id);
    }

    close(fd);
}

void view_report(const char *district, int target_id) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/reports.dat", district);

    // Deschidem fișierul doar pentru citire
    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        perror("Eroare: Nu am putut deschide reports.dat");
        return;
    }

    // Calculăm câte rapoarte există pentru a valida ID-ul
    off_t file_size = lseek(fd, 0, SEEK_END);
    int total_reports = file_size / sizeof(Report);

    if (target_id < 0 || target_id >= total_reports) {
        printf("Eroare: Raportul cu ID-ul %d nu exista in districtul %s.\n", target_id, district);
        close(fd);
        return;
    }

    // Magia lseek: sărim direct la offset-ul raportului dorit
    // SEEK_SET înseamnă că măsurăm distanța de la începutul absolut al fișierului
    lseek(fd, target_id * sizeof(Report), SEEK_SET);

    Report r;
    if (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        printf("\n=== DETALII RAPORT #%d ===\n", r.report_id);
        printf("Inspector: %s\n", r.inspector_name);
        printf("Categorie: %s\n", r.category);
        printf("Severitate: %d (1=minor, 2=moderat, 3=critic)\n", r.severity);
        printf("Coordonate GPS: Lat %.4f, Lon %.4f\n", r.latitude, r.longitude);
        
        // Transformăm timpul din format intern într-un șir de caractere citibil
        char time_str[26];
        strncpy(time_str, ctime(&r.timestamp), sizeof(time_str));
        time_str[strcspn(time_str, "\n")] = 0; // Eliminăm newline-ul (\n) inserat de ctime
        printf("Data si ora: %s\n", time_str);
        
        printf("Descriere: %s\n", r.description);
        printf("=========================\n\n");
    } else {
        perror("Eroare la citirea raportului");
    }

    close(fd);
}

void check_symlinks() {
    DIR *dir = opendir("."); // Deschidem directorul curent
    if (dir == NULL) {
        perror("Eroare la citirea directorului curent");
        return;
    }

    struct dirent *entry;
    struct stat link_stat;
    struct stat target_stat;

    printf("\n--- VERIFICARE LINKURI SIMBOLICE ---\n");

    // Citim fiecare element din director rând pe rând
    while ((entry = readdir(dir)) != NULL) {
        // Căutăm doar fișierele care încep cu "active_reports-"
        if (strncmp(entry->d_name, "active_reports-", 15) == 0) {
            
            // Folosim lstat() pentru a citi metadata LINKULUI, nu a fișierului țintă 
            if (lstat(entry->d_name, &link_stat) == 0) {
                
                // Verificăm dacă fișierul este cu adevărat un link simbolic
                if (S_ISLNK(link_stat.st_mode)) {
                    
                    // Acum folosim stat() normal pentru a încerca să ajungem la fișierul țintă
                    // Dacă stat() dă eroare (-1), înseamnă că fișierul țintă nu există!
                    if (stat(entry->d_name, &target_stat) == -1) {
                        printf("WARNING: Dangling link detectat -> '%s' (fisiertul tinta a fost sters sau mutat!)\n", entry->d_name); [cite: 91]
                        
                        // Opțional, conform cerințelor de curățare, îl putem șterge:
                        // unlink(entry->d_name); 
                        // printf("Linkul dangling a fost sters automat.\n");
                    } else {
                        printf("OK: Linkul '%s' este valid.\n", entry->d_name);
                    }
                }
            }
        }
    }
    printf("------------------------------------\n\n");
    closedir(dir);
}

int main(int argc, char *argv[]) {
    // Variabile pentru a stoca datele primite
    char *role = NULL;
    char *user = NULL;
    char *command = NULL;
    char *district = NULL;
    char *report_id_str = NULL; // Pentru remove sau view [cite: 58, 59]
    char *filter_cond = NULL;   // Pentru filter [cite: 64]

    // Trecem prin toate argumentele, incepand de la 1 
    // (deoarece argv[0] este mereu numele programului, adica "./city_manager")
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--role") == 0 && i + 1 < argc) {
            role = argv[i + 1];
            i++; // Sarim peste valoare ca sa nu o citim ca pe o comanda
        } 
        else if (strcmp(argv[i], "--user") == 0 && i + 1 < argc) {
            user = argv[i + 1];
            i++;
        } 
        else if (strcmp(argv[i], "--add") == 0 && i + 1 < argc) {
            command = "add";
            district = argv[i + 1];
            i++;
        }
        else if (strcmp(argv[i], "--list") == 0 && i + 1 < argc) {
            command = "list";
            district = argv[i + 1];
            i++;
        }
        else if (strcmp(argv[i], "--remove_report") == 0 && i + 2 < argc) {
            command = "remove_report";
            district = argv[i + 1];
            report_id_str = argv[i + 2];
            i += 2; // Aici sarim 2 pasi, deoarece avem district si ID
        }
        else if (strcmp(argv[i], "--view") == 0 && i + 2 < argc) {
            command = "view";
            district = argv[i + 1];
            report_id_str = argv[i + 2];
            i += 2;
        }
        // Poti adauga restul comenzilor (view, update_threshold, filter) folosind acelasi sablon
    }

    // --- VALIDAREA DATELOR OBTINUTE ---
    
    // 1. Verificam daca avem elementele obligatorii
    if (role == NULL || user == NULL || command == NULL) {
        printf("Eroare: Lipsesc argumente obligatorii (--role, --user sau comanda)!\n");
        printf("Utilizare: ./city_manager --role <rol> --user <nume> --<comanda> <district>\n");
        return 1; // Returnam 1 pentru a semnala o eroare la iesire
    }

    // 2. Validam rolul (doar inspector si manager sunt permise) 
    if (strcmp(role, "inspector") != 0 && strcmp(role, "manager") != 0) {
        printf("Eroare: Rol invalid! Folositi 'inspector' sau 'manager'.\n");
        return 1;
    }

    // --- EXECUTAREA COMENZILOR ---

    printf("Pornim executia pentru utilizatorul '%s', cu rolul de '%s'.\n", user, role);
    printf("Comanda ceruta: %s pe districtul: %s\n\n", command, district);

    if (strcmp(command, "add") == 0) {
        // Înainte de a adăuga un raport, ne asigurăm că districtul există (are fișierele create)
        init_district(district); 
        add_report(district,user); 
    } 
    else if (strcmp(command, "list") == 0) {
        list_reports(district);
    }
    else if (strcmp(command, "remove_report") == 0) {
        if (report_id_str == NULL) {
            printf("Eroare: Comanda remove_report necesita un ID!\n");
        } else {
            // Convertim textul argumentului (ex: "0") într-un număr întreg (int)
            int target_id = atoi(report_id_str); 
            remove_report(district, role, target_id);
        }
    }
    else if (strcmp(command, "view") == 0) {
        if (report_id_str == NULL) {
            printf("Eroare: Comanda view necesita un ID de raport!\n");
        } else {
            int target_id = atoi(report_id_str);
            view_report(district, target_id);
        }
    }
    return 0;
}