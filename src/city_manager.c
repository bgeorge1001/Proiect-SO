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
    char symlink_name[256];
    snprintf(symlink_name, sizeof(symlink_name), "active_reports-%s", district_name);
    
    char target_name[256];
    snprintf(target_name, sizeof(target_name), "%s/reports.dat", district_name);

    if (symlink(target_name, symlink_name) == -1) {
        if (errno != EEXIST) {
            perror("Eroare la crearea linkului simbolic");
        }
    }
}

void add_report(const char *district, const char *user) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/reports.dat", district);
    int fd = open(filepath, O_WRONLY | O_APPEND);
    if (fd == -1) {
        perror("Eroare: Nu am putut deschide reports.dat. Exista districtul?");
        return;
    }

    off_t file_size = lseek(fd, 0, SEEK_END);
    int new_id = file_size / sizeof(Report); 
    Report new_report;
    memset(&new_report, 0, sizeof(Report)); 
    new_report.report_id = new_id;
    strncpy(new_report.inspector_name, user, MAX_NAME_LEN - 1); 
    new_report.timestamp = time(NULL); 
    new_report.latitude = 47.05; 
    new_report.longitude = 21.93;
    strncpy(new_report.category, "road", MAX_CAT_LEN - 1); 
    new_report.severity = 2;
    strncpy(new_report.description, "Groapa periculoasa pe banda 2", MAX_DESC_LEN - 1); 

    ssize_t bytes_written = write(fd, &new_report, sizeof(Report));
    if (bytes_written != sizeof(Report)) {
        perror("Eroare la scrierea datelor binare");
    } else {
        printf("Succes! Raportul #%d a fost adaugat de '%s' in %s.\n", new_id, user, district);
    }

    close(fd);
}

// functie pentru a converti st_mode intr-un string de tip "rwxr-xr--"
void mode_to_string(mode_t mode, char *str) {
    strcpy(str, "---------"); 
    
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

    struct stat file_stat;
    if (stat(filepath, &file_stat) == -1) {
        perror("Eroare: Nu am putut gasi reports.dat. Exista districtul?");
        return;
    }
    char perms[10];
    mode_to_string(file_stat.st_mode, perms);
    char time_str[26];
    strncpy(time_str, ctime(&file_stat.st_mtime), sizeof(time_str));
    time_str[strcspn(time_str, "\n")] = 0; // Scoatem enter-ul de la final pus de ctime

    printf("--- INFO FISIER ---\n");
    printf("Fisier: %s\nPermisiuni: %s\nDimensiune: %ld bytes\nModificat: %s\n\n", 
           filepath, perms, file_stat.st_size, time_str);
    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        perror("Eroare la deschiderea fisierului pentru citire");
        return;
    }
    Report r;
    printf("--- LISTA RAPOARTE ---\n");
    int count = 0;
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
    if (strcmp(role, "manager") != 0) {
        printf("Eroare de securitate: Doar utilizatorii cu rol de 'manager' pot sterge rapoarte!\n");
        return;
    }

    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/reports.dat", district);

    int fd = open(filepath, O_RDWR);
    if (fd == -1) {
        perror("Eroare: Nu am putut deschide reports.dat");
        return;
    }

    off_t file_size = lseek(fd, 0, SEEK_END);
    int total_reports = file_size / sizeof(Report);

    if (target_id < 0 || target_id >= total_reports) {
        printf("Eroare: Raportul cu ID-ul %d nu exista.\n", target_id);
        close(fd);
        return;
    }

    Report temp;
    for (int i = target_id + 1; i < total_reports; i++) {
        lseek(fd, i * sizeof(Report), SEEK_SET);
        read(fd, &temp, sizeof(Report));
        temp.report_id = i - 1;
        lseek(fd, (i - 1) * sizeof(Report), SEEK_SET);
        write(fd, &temp, sizeof(Report));
    }

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
    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        perror("Eroare: Nu am putut deschide reports.dat");
        return;
    }

    off_t file_size = lseek(fd, 0, SEEK_END);
    int total_reports = file_size / sizeof(Report);

    if (target_id < 0 || target_id >= total_reports) {
        printf("Eroare: Raportul cu ID-ul %d nu exista in districtul %s.\n", target_id, district);
        close(fd);
        return;
    }
    lseek(fd, target_id * sizeof(Report), SEEK_SET);

    Report r;
    if (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        printf("\n=== DETALII RAPORT #%d ===\n", r.report_id);
        printf("Inspector: %s\n", r.inspector_name);
        printf("Categorie: %s\n", r.category);
        printf("Severitate: %d (1=minor, 2=moderat, 3=critic)\n", r.severity);
        printf("Coordonate GPS: Lat %.4f, Lon %.4f\n", r.latitude, r.longitude);
        char time_str[26];
        strncpy(time_str, ctime(&r.timestamp), sizeof(time_str));
        time_str[strcspn(time_str, "\n")] = 0; 
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

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "active_reports-", 15) == 0) {     
            if (lstat(entry->d_name, &link_stat) == 0) {     
                if (S_ISLNK(link_stat.st_mode)) {       
                    if (stat(entry->d_name, &target_stat) == -1) {
                        printf("WARNING: Dangling link detectat -> '%s' (fisiertul tinta a fost sters sau mutat!)\n", entry->d_name);       
                        // putem sterge, folosind comanda : 
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

void log_action(const char *district, const char *role, const char *user, const char *action) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/logged_district", district);

    struct stat file_stat;
    if (stat(filepath, &file_stat) == -1) {
        return; 
    }

    int can_write = 0;
    
    if (strcmp(role, "manager") == 0) {
        // Managerii sunt "Owner". Verificăm bitul de Write pentru Owner (S_IWUSR)
        if (file_stat.st_mode & S_IWUSR) {
            can_write = 1;
        }
    } else if (strcmp(role, "inspector") == 0) {
        // Inspectorii sunt "Group". Verificăm bitul de Write pentru Group (S_IWGRP)
        if (file_stat.st_mode & S_IWGRP) {
            can_write = 1;
        }
    }

    if (!can_write) {
        printf("Restrictie sistem: Acces refuzat! Rolul '%s' nu are permisiunea de scriere in %s.\n", role, filepath);
        return; 
    }

    int fd = open(filepath, O_WRONLY | O_APPEND);
    if (fd == -1) {
        perror("Eroare la deschiderea jurnalului pentru scriere");
        return;
    }

    time_t now = time(NULL);
    char time_str[26];
    strncpy(time_str, ctime(&now), sizeof(time_str));
    time_str[strcspn(time_str, "\n")] = 0;
    char log_entry[512];
    snprintf(log_entry, sizeof(log_entry), "[%s] Rol: %s, User: %s, Actiune: %s\n", 
             time_str, role, user, action);

    write(fd, log_entry, strlen(log_entry));
    close(fd);
}

void update_threshold(const char *district, const char *role, const char *value_str) {
    if (strcmp(role, "manager") != 0) {
        printf("Eroare de securitate: Doar utilizatorii cu rol de 'manager' pot actualiza pragul!\n");
        return;
    }

    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/district.cfg", district);

    struct stat file_stat;
    if (stat(filepath, &file_stat) == -1) {
        perror("Eroare: Nu am putut gasi district.cfg");
        return;
    }

    mode_t perms = file_stat.st_mode & 0777;
    
    if (perms != 0640) {
        printf("Diagnostic de Securitate: Permisiunile fisierului %s au fost modificate!\n", filepath);
        printf("Asteptat: 640 | Gasit: %o. Operatiunea este refuzata.\n", perms);
        return;
    }

    int fd = open(filepath, O_WRONLY | O_TRUNC);
    if (fd == -1) {
        perror("Eroare la deschiderea fisierului district.cfg");
        return;
    }

    char buffer[128];
    int len = snprintf(buffer, sizeof(buffer), "SEVERITY_THRESHOLD=%s\n", value_str);
    write(fd, buffer, len);
    
    printf("Succes: Pragul de severitate pentru %s a fost actualizat la %s.\n", district, value_str);
    
    close(fd);
}

int main(int argc, char *argv[]) {
    // Variabile pentru a stoca datele primite
    char *role = NULL;
    char *user = NULL;
    char *command = NULL;
    char *district = NULL;
    char *report_id_str = NULL;
    char *target_val_str = NULL; 
    char *filter_cond = NULL;  

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
        else if (strcmp(argv[i], "--update_threshold") == 0 && i + 2 < argc) {
            command = "update_threshold";
            district = argv[i + 1];
            // Folosim o variabilă nouă sau refolosim una existentă.
            // Să zicem că ai definit "char *target_val_str = NULL;" la începutul lui main:
            target_val_str = argv[i + 2]; 
            i += 2;
        }
        // Poti adauga restul comenzilor (view, update_threshold, filter) folosind acelasi sablon
    }

    
    if (role == NULL || user == NULL || command == NULL) {
        printf("Eroare: Lipsesc argumente obligatorii (--role, --user sau comanda)!\n");
        printf("Utilizare: ./city_manager --role <rol> --user <nume> --<comanda> <district>\n");
        return 1; // Returnam 1 pentru a semnala o eroare la iesire
    }

    if (strcmp(role, "inspector") != 0 && strcmp(role, "manager") != 0) {
        printf("Eroare: Rol invalid! Folositi 'inspector' sau 'manager'.\n");
        return 1;
    }
    printf("Pornim executia pentru utilizatorul '%s', cu rolul de '%s'.\n", user, role);
    printf("Comanda ceruta: %s pe districtul: %s\n\n", command, district);

    if (strcmp(command, "add") == 0) {
        init_district(district); 
        add_report(district,user); 
        log_action(district, role, user, "A adaugat un raport nou.");
    } 
    else if (strcmp(command, "list") == 0) {
        list_reports(district);
    }
    else if (strcmp(command, "remove_report") == 0) {
        if (report_id_str == NULL) {
            printf("Eroare: Comanda remove_report necesita un ID!\n");
        } else {
            int target_id = atoi(report_id_str);
            remove_report(district, role, target_id);
            
            char action_msg[256];
            snprintf(action_msg, sizeof(action_msg), "A sters raportul cu ID-ul %d.", target_id);
            log_action(district, role, user, action_msg);
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
    else if (strcmp(command, "update_threshold") == 0) {
        if (target_val_str == NULL) {
            printf("Eroare: Lipseste valoarea pragului!\n");
        } else {
            update_threshold(district, role, target_val_str);
            
            char action_msg[256];
            snprintf(action_msg, sizeof(action_msg), "A actualizat pragul la %s.", target_val_str);
            log_action(district, role, user, action_msg);
        }
    }
    return 0;
}