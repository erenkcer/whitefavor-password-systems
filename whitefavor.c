#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <windows.h>
#include <conio.h>
#include <sys/stat.h> // mkdir
#include <sys/types.h>
#include <direct.h> // Windows mkdir

#define MAX_LINE 1024
#define MAX_USER 32
#define MAX_ATTEMPT 3
#define SALT "mySalt123"
#define IDLE_TIMEOUT 120 // seconds (2 minutes)

// Renkli yazi icin
typedef enum { ALLOW_EMPTY = 0, NO_EMPTY = 1 } EmptyPolicy;
typedef enum { SEARCH_SHOW_INDEX, SEARCH_NO_INDEX } SearchMode;

#define GREEN   10



// Fonksiyon prototipleri
void xorFile(const char *filename, const char *key, const char *outname);
void addRecord(const char *filename, const char *key);
void showRecords(const char *filename, const char *key);
void deleteRecord(const char *filename, const char *key);
void searchRecords(const char *filename, const char *key, SearchMode mode);
void updateRecord(const char *filename, const char *key);
void hideFileOrFolder(const char *name);
void getTimestamp(char *buf, size_t size);
void copyToClipboard(const char *text);
void getUserFile(const char *user, char *buf, size_t size);
void backupRecords(const char *filename);
void generateStrongPassword(char *buf, size_t len);
const char* checkPasswordStrength(const char *pw);
void getInput(const char *prompt, char *buf, size_t size, int no_empty);
void getUserPasswordFile(const char *user, char *buf, size_t size);

// Konsolu temizle
void clearScreen() { system("cls"); }

// Renkli yazi
void setColor(int color) { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color); }

// Dosya veya klasore git
void hideFileOrFolder(const char *name) { SetFileAttributes(name, FILE_ATTRIBUTE_HIDDEN); }

// Zaman damgası al
void getTimestamp(char *buf, size_t size) { time_t now = time(NULL); struct tm *t = localtime(&now); strftime(buf, size, "%Y-%m-%d %H:%M:%S", t); }

// Clipboard'a kopyala (Windows)
void copyToClipboard(const char *text) {
    const size_t len = strlen(text) + 1;
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
    memcpy(GlobalLock(hMem), text, len);
    GlobalUnlock(hMem);
    OpenClipboard(0);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();
}

// Kullanicinin dosya ismini olustur
void getUserFile(const char *user, char *buf, size_t size) { snprintf(buf, size, "data\\%s_records.txt", user); }

// Kullaniciya ozel sifre dosyasi adi olustur
void getUserPasswordFile(const char *user, char *buf, size_t size) {
    snprintf(buf, size, "data\\%s_password.txt", user);
}

// Kayıtlar dosyasının yedeğini alır
void backupRecords(const char *filename) {
    char backupName[128];
    snprintf(backupName, sizeof(backupName), "%s_backup.txt", filename);
    FILE *src = fopen(filename, "rb");
    if (!src) return;
    FILE *dst = fopen(backupName, "wb");
    if (!dst) { fclose(src); return; }
    char buf[512];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), src)) > 0) fwrite(buf, 1, n, dst);
    fclose(src); fclose(dst);
}

// Güçlü şifre üretici
void generateStrongPassword(char *buf, size_t len) {
    const char *lower = "abcdefghijklmnopqrstuvwxyz";
    const char *upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const char *digits = "0123456789";
    const char *special = "!@#$%^&*()_+-=<>?";
    const char *all = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+-=<>?";
    srand((unsigned int)time(NULL) ^ (unsigned int)GetCurrentProcessId());
    int i = 0;
    buf[i++] = lower[rand() % 26];
    buf[i++] = upper[rand() % 26];
    buf[i++] = digits[rand() % 10];
    buf[i++] = special[rand() % 18];
    for (; i < (int)len-1; i++) buf[i] = all[rand() % strlen(all)];
    buf[len-1] = '\0';
}

// Şifre güçlülük kontrolü
const char* checkPasswordStrength(const char *pw) {
    int len = strlen(pw), hasLower=0, hasUpper=0, hasDigit=0, hasSpecial=0;
    for (int i=0; i<len; i++) {
        if ('a'<=pw[i] && pw[i]<='z') hasLower=1;
        else if ('A'<=pw[i] && pw[i]<='Z') hasUpper=1;
        else if ('0'<=pw[i] && pw[i]<='9') hasDigit=1;
        else hasSpecial=1;
    }
    if (len>=12 && hasLower && hasUpper && hasDigit && hasSpecial) return "Strong";
    if (len>=8 && ((hasLower+hasUpper+hasDigit+hasSpecial)>=3)) return "Medium";
    return "Weak";
}

// Kayıt ekle
void addRecord(const char *filename, const char *key) {
    char tur[100], mail[100], username[100], sifre[100], etiket[100], notlar[256], fav[8], kayit[MAX_LINE], zaman[64];
    getInput("Service (Instagram, Gmail, etc.): ", tur, sizeof(tur), 1);
    getInput("Email (if any): ", mail, sizeof(mail), 0);
    getInput("Username: ", username, sizeof(username), 1);
    setColor(GREEN); printf("Press '!' to automatically generate a strong password.\n"); setColor(GREEN);
    getInput("Password: ", sifre, sizeof(sifre), 1);
    if (strcmp(sifre, "!") == 0) {
        generateStrongPassword(sifre, 16);
        setColor(GREEN); printf("Generated password: %s\n", sifre); setColor(GREEN);
    }
    setColor(GREEN); printf("Password strength: ");
    const char *guc = checkPasswordStrength(sifre);
    if (strcmp(guc, "Strong")==0) setColor(GREEN);
    else if (strcmp(guc, "Medium")==0) setColor(GREEN);
    else setColor(GREEN);
    printf("%s\n", guc); setColor(GREEN);
    getInput("Tags (comma separated): ", etiket, sizeof(etiket), 0);
    getInput("Note (optional): ", notlar, sizeof(notlar), 0);
    getInput("Favorite? (yes/no): ", fav, sizeof(fav), 1);
    getTimestamp(zaman, sizeof(zaman));
    snprintf(kayit, sizeof(kayit), "Tur: %s | Mail: %s | Username: %s | Password: %s | Tags: %s | Note: %s | Favorite: %s | Date: %s\n", tur, mail, username, sifre, etiket, notlar, fav, zaman);
    xorFile(filename, key, "temp.txt");
    FILE *fp = fopen("temp.txt", "a");
    if (fp) { fputs(kayit, fp); fclose(fp); }
    xorFile("temp.txt", key, filename); remove("temp.txt");
    setColor(GREEN); printf("Record added and encrypted!\n"); setColor(GREEN);
    hideFileOrFolder(filename);
}

// Kayıtları göster (alfabetik sıralı, favoriler önce)
void showRecords(const char *filename, const char *key) {
    xorFile(filename, key, "temp.txt");
    FILE *fp = fopen("temp.txt", "r");
    if (!fp) { setColor(GREEN); printf("No record or wrong password.\n"); setColor(GREEN); return; }
    char *lines[1000]; int count = 0; char line[MAX_LINE];
    while (fgets(line, sizeof(line), fp) && count < 1000) { lines[count] = strdup(line); count++; }
    fclose(fp); remove("temp.txt");
    // Favoriler başa, alfabetik sırala
    for (int i = 0; i < count-1; i++)
        for (int j = i+1; j < count; j++) {
            int fav_i = strstr(lines[i], "Favorite: yes")!=NULL;
            int fav_j = strstr(lines[j], "Favorite: yes")!=NULL;
            if (fav_j > fav_i || (fav_i==fav_j && strcmp(lines[i], lines[j])>0)) {
                char *tmp = lines[i]; lines[i] = lines[j]; lines[j] = tmp;
            }
        }
    setColor(GREEN); printf("\n--- Records (Favorites first, Alphabetically) ---\n"); setColor(GREEN);
    for (int i = 0; i < count; i++) { printf("%d) %s", i+1, lines[i]); free(lines[i]); }
    setColor(GREEN); printf("-------------------------------------------\n"); setColor(GREEN);
}

// Kayıt arama (anahtar kelime veya etiket)
void searchRecords(const char *filename, const char *key, SearchMode mode) {
    xorFile(filename, key, "temp.txt");
    FILE *fp = fopen("temp.txt", "r");
    if (!fp) { setColor(GREEN); printf("No record or wrong password.\n"); setColor(GREEN); return; }
    char query[100], line[MAX_LINE];
    getInput("Enter the word/tag to search: ", query, sizeof(query), 1);
    int i = 1, found = 0;
    setColor(GREEN); printf("\n--- Results ---\n"); setColor(GREEN);
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, query)) { printf("%d) %s", i, line); found = 1; }
        i++;
    }
    if (!found) printf("Record not found.\n");
    fclose(fp); remove("temp.txt");
    setColor(GREEN); printf("----------------\n"); setColor(GREEN);
}

// Kayıt sil
void deleteRecord(const char *filename, const char *key) {
    xorFile(filename, key, "temp.txt");
    FILE *fp = fopen("temp.txt", "r");
    FILE *fp2 = fopen("temp2.txt", "w");
    if (!fp || !fp2) { setColor(GREEN); printf("No record or file not opened.\n"); setColor(GREEN); if (fp) fclose(fp); if (fp2) fclose(fp2); remove("temp.txt"); return; }
    char line[MAX_LINE]; int i = 1, del, found = 0;
    showRecords(filename, key);
    printf("Enter the record number to delete: "); scanf("%d%*c", &del);
    setColor(GREEN); printf("Are you sure? (yes/no): "); setColor(GREEN);
    char onay[10]; fgets(onay, sizeof(onay), stdin); onay[strcspn(onay, "\n")] = 0;
    if (strcmp(onay, "yes") != 0) { printf("Deletion canceled.\n"); fclose(fp); fclose(fp2); remove("temp.txt"); remove("temp2.txt"); return; }
    backupRecords("records.txt");
    while (fgets(line, sizeof(line), fp)) { if (i != del) fputs(line, fp2); else found = 1; i++; }
    fclose(fp); fclose(fp2);
    if (!found) { setColor(GREEN); printf("Record not found.\n"); setColor(GREEN); remove("temp2.txt"); remove("temp.txt"); return; }
    xorFile("temp2.txt", key, filename); remove("temp.txt"); remove("temp2.txt");
    setColor(GREEN); printf("Record deleted! (backup taken)\n"); setColor(GREEN);
    hideFileOrFolder(filename);
}

// Kayıt güncelle
void updateRecord(const char *filename, const char *key) {
    xorFile(filename, key, "temp.txt");
    FILE *fp = fopen("temp.txt", "r");
    FILE *fp2 = fopen("temp2.txt", "w");
    if (!fp || !fp2) { setColor(GREEN); printf("File not opened.\n"); setColor(GREEN); if (fp) fclose(fp); if (fp2) fclose(fp2); remove("temp.txt"); return; }
    int i = 1, upd, found = 0;
    char line[MAX_LINE];
    showRecords(filename, key);
    printf("Enter the record number to update: "); scanf("%d%*c", &upd);
    while (fgets(line, sizeof(line), fp)) {
        if (i == upd) {
            char tur[100], mail[100], username[100], sifre[100], etiket[100], notlar[256], fav[8], kayit[MAX_LINE], zaman[64];
            printf("Enter new information:\n");
            getInput("Tur: ", tur, sizeof(tur), 1);
            getInput("Mail: ", mail, sizeof(mail), 0);
            getInput("Username: ", username, sizeof(username), 1);
            setColor(GREEN); printf("Press '!' to automatically generate a strong password.\n"); setColor(GREEN);
            getInput("Password: ", sifre, sizeof(sifre), 1);
            if (strcmp(sifre, "!") == 0) { generateStrongPassword(sifre, 16); setColor(GREEN); printf("Generated password: %s\n", sifre); setColor(GREEN); }
            setColor(GREEN); printf("Password strength: ");
            const char *guc = checkPasswordStrength(sifre);
            if (strcmp(guc, "Strong")==0) setColor(GREEN);
            else if (strcmp(guc, "Medium")==0) setColor(GREEN);
            else setColor(GREEN);
            printf("%s\n", guc); setColor(GREEN);
            getInput("Tags: ", etiket, sizeof(etiket), 0);
            getInput("Note: ", notlar, sizeof(notlar), 0);
            getInput("Favorite? (yes/no): ", fav, sizeof(fav), 1);
            getTimestamp(zaman, sizeof(zaman));
            snprintf(kayit, sizeof(kayit), "Tur: %s | Mail: %s | Username: %s | Password: %s | Tags: %s | Note: %s | Favorite: %s | Date: %s\n", tur, mail, username, sifre, etiket, notlar, fav, zaman);
            fputs(kayit, fp2); found = 1;
        } else { fputs(line, fp2); }
        i++;
    }
    fclose(fp); fclose(fp2);
    if (!found) { setColor(GREEN); printf("Record not found.\n"); setColor(GREEN); remove("temp2.txt"); remove("temp.txt"); return; }
    xorFile("temp2.txt", key, filename); remove("temp.txt"); remove("temp2.txt");
    setColor(GREEN); printf("Record updated!\n"); setColor(GREEN);
    hideFileOrFolder(filename);
}

// Kayıtları dışa aktar
void exportRecords(const char *filename, const char *key) {
    xorFile(filename, key, "temp.txt");
    FILE *fp = fopen("temp.txt", "r");
    if (!fp) { setColor(GREEN); printf("No record or wrong password.\n"); setColor(GREEN); return; }
    char outname[128];
    getInput("Enter the file name to export: ", outname, sizeof(outname), 1);
    FILE *out = fopen(outname, "w");
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), fp)) fputs(line, out);
    fclose(fp); fclose(out);
    remove("temp.txt");
    setColor(GREEN); printf("Export completed!\n"); setColor(GREEN);
}

// Kayıtları içe aktar
void importRecords(const char *filename, const char *key) {
    char inname[128];
    getInput("Enter the file name to import: ", inname, sizeof(inname), 1);
    FILE *in = fopen(inname, "r");
    if (!in) { setColor(GREEN); printf("File not opened.\n"); setColor(GREEN); return; }
    xorFile(filename, key, "temp.txt");
    FILE *fp = fopen("temp.txt", "a");
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), in)) fputs(line, fp);
    fclose(fp); fclose(in);
    xorFile("temp.txt", key, filename);
    remove("temp.txt");
    setColor(GREEN); printf("Import completed!\n"); setColor(GREEN);
    hideFileOrFolder(filename);
}

// Clipboard menüsü
void clipboardMenu(const char *filename, const char *key) {
    xorFile(filename, key, "temp.txt");
    FILE *fp = fopen("temp.txt", "r");
    if (!fp) { setColor(GREEN); printf("No record or wrong password.\n"); setColor(GREEN); return; }
    char *lines[1000]; int count = 0; char line[MAX_LINE];
    while (fgets(line, sizeof(line), fp) && count < 1000) { lines[count] = strdup(line); count++; }
    fclose(fp); remove("temp.txt");
    showRecords(filename, key);
    printf("Enter the record number to copy to clipboard: ");
    int idx; scanf("%d%*c", &idx);
    if (idx < 1 || idx > count) { setColor(GREEN); printf("Invalid number!\n"); setColor(GREEN); for (int i=0;i<count;i++) free(lines[i]); return; }
    char *start = strstr(lines[idx-1], "Username: ");
    char *mid = strstr(lines[idx-1], "Password: ");
    if (!start || !mid) { setColor(GREEN); printf("Record format error!\n"); setColor(GREEN); for (int i=0;i<count;i++) free(lines[i]); return; }
    char username[100], sifre[100];
    sscanf(start, "Username: %99[^|]| Password: %99[^|]", username, sifre);
    setColor(GREEN); printf("1) Username\n2) Password\nChoice: "); setColor(GREEN);
    int sec; scanf("%d%*c", &sec);
    if (sec==1) { copyToClipboard(username); setColor(GREEN); printf("Username copied to clipboard!\n"); setColor(GREEN); }
    else if (sec==2) { copyToClipboard(sifre); setColor(GREEN); printf("Password copied to clipboard!\n"); setColor(GREEN); }
    else { setColor(GREEN); printf("Invalid choice!\n"); setColor(GREEN); }
    for (int i=0;i<count;i++) free(lines[i]);
}

// login fonksiyonunu guncelliyorum
int login(char *user, char *key) {
    int attempts = 0;
    char filename[128], sifre[100], dosyadaki[100], sifrefile[128];
    while (1) {
        getInput("Username: ", user, MAX_USER, 1);
        getUserFile(user, filename, sizeof(filename));
        getUserPasswordFile(user, sifrefile, sizeof(sifrefile));
        FILE *fp = fopen(sifrefile, "r");
        if (!fp) {
            getInput("Enter a new password: ", sifre, sizeof(sifre), 1);
            mkdir("data");
            fp = fopen(sifrefile, "w");
            fprintf(fp, "%s\n", sifre);
            fclose(fp);
            hideFileOrFolder("data");
            hideFileOrFolder(sifrefile);
            setColor(GREEN); printf("User and password created. Please restart the program.\n"); setColor(GREEN);
            return 0;
        } else {
            fgets(dosyadaki, sizeof(dosyadaki), fp);
            fclose(fp);
            dosyadaki[strcspn(dosyadaki, "\n")] = 0;
            getInput("Enter your password: ", sifre, sizeof(sifre), 1);
            if (strcmp(sifre, dosyadaki) == 0) {
                strcpy(key, sifre);
                return 1;
            } else {
                setColor(GREEN); printf("Wrong password!\n"); setColor(GREEN);
                attempts++;
                if (attempts >= MAX_ATTEMPT) {
                    setColor(GREEN); printf("Too many wrong attempts! Program locked.\n"); setColor(GREEN);
                    exit(1);
                }
            }
        }
    }
}

// Otomatik çıkış için zaman kontrolü
time_t lastActionTime;
void resetIdleTimer() { lastActionTime = time(NULL); }
void checkIdleTimeout() { if (difftime(time(NULL), lastActionTime) > IDLE_TIMEOUT) { setColor(GREEN); printf("\nToo long time without operation, automatic exit!\n"); setColor(GREEN); exit(0); } }

// XOR + salt şifreleme
void xorFile(const char *filename, const char *key, const char *outname) {
    FILE *in = fopen(filename, "rb");
    FILE *out = fopen(outname, "wb");
    if (!in || !out) {
        if (in) fclose(in);
        if (out) fclose(out);
        return;
    }
    int c, i = 0, keylen = strlen(key), saltlen = strlen(SALT);
    while ((c = fgetc(in)) != EOF) {
        fputc(c ^ key[i % keylen] ^ SALT[i % saltlen], out);
        i++;
    }
    fclose(in);
    fclose(out);
}

// Kullanıcıdan güvenli giriş al
void getInput(const char *prompt, char *buf, size_t size, int no_empty) {
    while (1) {
        setColor(GREEN);
        printf("%s", prompt);
        setColor(GREEN);
        if (fgets(buf, size, stdin)) {
            buf[strcspn(buf, "\n")] = 0;
            if (no_empty && strlen(buf) == 0) {
                setColor(GREEN);
                printf("Do not leave empty!\n");
                setColor(GREEN);
                continue;
            }
            break;
        }
    }
}

int main() {
    char user[MAX_USER], key[100], filename[128];
    clearScreen();
    setColor(GREEN);
    printf("=== WHITEFAVOR PASSWORD SYSTEMS ===\n");
    setColor(GREEN);
    if (!login(user, key)) return 0;
    getUserFile(user, filename, sizeof(filename));
    hideFileOrFolder("data");
    hideFileOrFolder(filename);
    int secim;
    resetIdleTimer();
    while (1) {
        setColor(GREEN);
        printf("\n1) Add Record\n2) Show Records\n3) Delete Record\n4) Search Record\n5) Update Record\n6) Export Records\n7) Import Records\n8) Copy to Clipboard\n9) Exit\nYour choice: ");
        setColor(GREEN);
        if (scanf("%d%*c", &secim) != 1) { while (getchar() != '\n'); continue; }
        clearScreen();
        checkIdleTimeout();
        resetIdleTimer();
        if (secim == 1) addRecord(filename, key);
        else if (secim == 2) showRecords(filename, key);
        else if (secim == 3) deleteRecord(filename, key);
        else if (secim == 4) searchRecords(filename, key, SEARCH_SHOW_INDEX);
        else if (secim == 5) updateRecord(filename, key);
        else if (secim == 6) exportRecords(filename, key);
        else if (secim == 7) importRecords(filename, key);
        else if (secim == 8) clipboardMenu(filename, key);
        else if (secim == 9) { setColor(GREEN); printf("Exiting...\n"); setColor(GREEN); break; }
        else { setColor(GREEN); printf("Invalid choice!\n"); setColor(GREEN); }
    }
    return 0;
}