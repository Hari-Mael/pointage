#define _POSIX_C_SOURCE 200809L
#ifdef _WIN32
#define _CRT_RAND_S
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#define MKDIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#define MKDIR(p) mkdir((p), 0700)
#endif

#define MAXF 18
#define FLEN 128
#define ID_LEN 17
#define NAME_LEN 128
#define LINE_LEN 4096
#define MAX_PEOPLE 20000
#define PBKDF2_ITER 150000
#define SALT_LEN 16
#define HASH_LEN 32
#define DATA_DIR "pointage_data"
#define USERS_FILE DATA_DIR "/users.dat"
#define MAX_LOGIN_FAILS 5

typedef struct {
    uint32_t h[8];
    uint64_t len;
    uint8_t buf[64];
    size_t n;
} Sha;

typedef struct {
    Sha inner;
    Sha outer;
} Hmac;

static const uint32_t K256[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

#define ROR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

static void sha_init(Sha *s) {
    static const uint32_t iv[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                                   0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    memcpy(s->h, iv, sizeof iv);
    s->len = 0;
    s->n = 0;
}

static void sha_block(Sha *s, const uint8_t *p) {
    uint32_t w[64];
    for (int i = 0; i < 16; i++)
        w[i] = ((uint32_t)p[i * 4] << 24) | ((uint32_t)p[i * 4 + 1] << 16) | ((uint32_t)p[i * 4 + 2] << 8) | p[i * 4 + 3];
    for (int i = 16; i < 64; i++) {
        uint32_t s0 = ROR(w[i - 15], 7) ^ ROR(w[i - 15], 18) ^ (w[i - 15] >> 3);
        uint32_t s1 = ROR(w[i - 2], 17) ^ ROR(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    uint32_t a = s->h[0], b = s->h[1], c = s->h[2], d = s->h[3];
    uint32_t e = s->h[4], f = s->h[5], g = s->h[6], h = s->h[7];
    for (int i = 0; i < 64; i++) {
        uint32_t t1 = h + (ROR(e, 6) ^ ROR(e, 11) ^ ROR(e, 25)) + ((e & f) ^ (~e & g)) + K256[i] + w[i];
        uint32_t t2 = (ROR(a, 2) ^ ROR(a, 13) ^ ROR(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    s->h[0] += a;
    s->h[1] += b;
    s->h[2] += c;
    s->h[3] += d;
    s->h[4] += e;
    s->h[5] += f;
    s->h[6] += g;
    s->h[7] += h;
}

static void sha_update(Sha *s, const uint8_t *data, size_t len) {
    s->len += len;
    while (len) {
        size_t take = 64 - s->n;
        if (take > len) take = len;
        memcpy(s->buf + s->n, data, take);
        s->n += take;
        data += take;
        len -= take;
        if (s->n == 64) {
            sha_block(s, s->buf);
            s->n = 0;
        }
    }
}

static void sha_final(Sha *s, uint8_t out[32]) {
    uint64_t bits = s->len * 8;
    uint8_t pad = 0x80;
    sha_update(s, &pad, 1);
    pad = 0;
    while (s->n != 56) sha_update(s, &pad, 1);
    uint8_t lenbuf[8];
    for (int i = 0; i < 8; i++) lenbuf[i] = (uint8_t)(bits >> (56 - 8 * i));
    sha_update(s, lenbuf, 8);
    for (int i = 0; i < 8; i++) {
        out[i * 4] = (uint8_t)(s->h[i] >> 24);
        out[i * 4 + 1] = (uint8_t)(s->h[i] >> 16);
        out[i * 4 + 2] = (uint8_t)(s->h[i] >> 8);
        out[i * 4 + 3] = (uint8_t)s->h[i];
    }
}

static void sha256(const uint8_t *data, size_t len, uint8_t out[32]) {
    Sha s;
    sha_init(&s);
    sha_update(&s, data, len);
    sha_final(&s, out);
}

static void zero_mem(void *p, size_t n) {
    volatile unsigned char *v = (volatile unsigned char *)p;
    while (n--) *v++ = 0;
}

static void hmac_init(Hmac *m, const uint8_t *key, size_t keylen) {
    uint8_t k[64] = {0}, pad[64];
    if (keylen > 64) {
        sha256(key, keylen, k);
    } else {
        memcpy(k, key, keylen);
    }
    for (int i = 0; i < 64; i++) pad[i] = k[i] ^ 0x36;
    sha_init(&m->inner);
    sha_update(&m->inner, pad, 64);
    for (int i = 0; i < 64; i++) pad[i] = k[i] ^ 0x5c;
    sha_init(&m->outer);
    sha_update(&m->outer, pad, 64);
    zero_mem(k, sizeof k);
    zero_mem(pad, sizeof pad);
}

static void hmac_compute(const Hmac *m, const uint8_t *msg, size_t len, uint8_t out[32]) {
    Sha in = m->inner, ou = m->outer;
    uint8_t tmp[32];
    sha_update(&in, msg, len);
    sha_final(&in, tmp);
    sha_update(&ou, tmp, 32);
    sha_final(&ou, out);
}

static void pbkdf2_sha256(const uint8_t *pw, size_t pwlen, const uint8_t *salt, size_t saltlen,
                          uint32_t iterations, uint8_t out[32]) {
    Hmac m;
    hmac_init(&m, pw, pwlen);
    uint8_t first[SALT_LEN + 4 + 64];
    uint8_t u[32], t[32];
    if (saltlen > SALT_LEN + 64) saltlen = SALT_LEN + 64;
    memcpy(first, salt, saltlen);
    first[saltlen] = 0;
    first[saltlen + 1] = 0;
    first[saltlen + 2] = 0;
    first[saltlen + 3] = 1;
    hmac_compute(&m, first, saltlen + 4, u);
    memcpy(t, u, 32);
    for (uint32_t i = 1; i < iterations; i++) {
        hmac_compute(&m, u, 32, u);
        for (int j = 0; j < 32; j++) t[j] ^= u[j];
    }
    memcpy(out, t, 32);
    zero_mem(&m, sizeof m);
    zero_mem(u, sizeof u);
    zero_mem(t, sizeof t);
}

static void to_hex(const uint8_t *in, size_t n, char *out) {
    static const char digits[] = "0123456789abcdef";
    for (size_t i = 0; i < n; i++) {
        out[i * 2] = digits[in[i] >> 4];
        out[i * 2 + 1] = digits[in[i] & 15];
    }
    out[n * 2] = 0;
}

static int hex_val(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

static int from_hex(const char *in, uint8_t *out, size_t n) {
    if (strlen(in) != n * 2) return 0;
    for (size_t i = 0; i < n; i++) {
        int hi = hex_val(in[i * 2]), lo = hex_val(in[i * 2 + 1]);
        if (hi < 0 || lo < 0) return 0;
        out[i] = (uint8_t)(hi * 16 + lo);
    }
    return 1;
}

static int ct_equal(const uint8_t *a, const uint8_t *b, size_t n) {
    uint8_t diff = 0;
    for (size_t i = 0; i < n; i++) diff |= (uint8_t)(a[i] ^ b[i]);
    return diff == 0;
}

static int random_bytes(uint8_t *out, size_t n) {
#ifdef _WIN32
    size_t i = 0;
    while (i < n) {
        unsigned v;
        if (rand_s(&v)) return 0;
        for (int k = 0; k < 4 && i < n; k++, i++) out[i] = (uint8_t)(v >> (8 * k));
    }
    return 1;
#else
    FILE *f = fopen("/dev/urandom", "rb");
    if (!f) return 0;
    size_t r = fread(out, 1, n, f);
    fclose(f);
    return r == n;
#endif
}

static int self_test(void) {
    uint8_t out[32];
    char hex[65];
    int ok = 1;
    sha256((const uint8_t *)"abc", 3, out);
    to_hex(out, 32, hex);
    ok &= strcmp(hex, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") == 0;
    pbkdf2_sha256((const uint8_t *)"password", 8, (const uint8_t *)"salt", 4, 1, out);
    to_hex(out, 32, hex);
    ok &= strcmp(hex, "120fb6cffcf8b32c43e7225256c4f837a86548c92ccc35480805987cb70be17b") == 0;
    pbkdf2_sha256((const uint8_t *)"password", 8, (const uint8_t *)"salt", 4, 2, out);
    to_hex(out, 32, hex);
    ok &= strcmp(hex, "ae4d0c95af6b46d32d0adff928f06dd02a303f8ef3c251dfd6e2d85a95474c43") == 0;
    puts(ok ? "Auto-test crypto : OK" : "Auto-test crypto : ECHEC");
    return ok ? 0 : 1;
}

static int use_color = 1;

static void sleep_ms(unsigned ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
#endif
}

#ifndef _WIN32
static struct termios saved_term;
static volatile sig_atomic_t term_saved = 0;
static void on_sigint(int sig) {
    (void)sig;
    if (term_saved) tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_term);
    _exit(130);
}
#endif

static void trim(char *s) {
    size_t i = 0;
    while (s[i] && isspace((unsigned char)s[i])) i++;
    if (i) memmove(s, s + i, strlen(s + i) + 1);
    size_t n = strlen(s);
    while (n && isspace((unsigned char)s[n - 1])) s[--n] = 0;
}

static int read_raw(char *buf, size_t size) {
    if (!fgets(buf, (int)size, stdin)) return 0;
    size_t len = strlen(buf);
    int truncated = 0;
    if (len && buf[len - 1] == '\n') {
        buf[--len] = 0;
    } else if (len == size - 1) {
        int c;
        truncated = 1;
        while ((c = getchar()) != '\n' && c != EOF) {
        }
    }
    if (len && buf[len - 1] == '\r') buf[--len] = 0;
    return truncated ? -1 : 1;
}

static void read_line(const char *prompt, char *buf, size_t size) {
    if (prompt) {
        fputs(prompt, stdout);
        fflush(stdout);
    }
    if (read_raw(buf, size) == 0) {
        fputc('\n', stdout);
        exit(0);
    }
    for (size_t i = 0; buf[i]; i++)
        if ((unsigned char)buf[i] < 32 || buf[i] == 127) buf[i] = ' ';
    trim(buf);
}

static int read_password(const char *prompt, char *buf, size_t size) {
    int r;
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    int tty = GetConsoleMode(h, &mode);
    if (tty) SetConsoleMode(h, mode & ~ENABLE_ECHO_INPUT);
#else
    struct termios nw;
    int tty = isatty(STDIN_FILENO) && tcgetattr(STDIN_FILENO, &saved_term) == 0;
    if (tty) {
        term_saved = 1;
        nw = saved_term;
        nw.c_lflag &= ~(tcflag_t)ECHO;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &nw);
    }
#endif
    fputs(prompt, stdout);
    fflush(stdout);
    r = read_raw(buf, size);
#ifdef _WIN32
    if (tty) SetConsoleMode(h, mode);
#else
    if (tty) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_term);
        term_saved = 0;
    }
#endif
    fputc('\n', stdout);
    if (r == 0) exit(0);
    return r;
}

static int is_leap(int y) { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }

static int dim(int y, int m) {
    static const int d[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return (m == 2 && is_leap(y)) ? 29 : d[m - 1];
}

static int weekday(int y, int m, int d) {
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3) y--;
    return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}

static int make_date(int y, int m, int d) { return y * 10000 + m * 100 + d; }

static void today_iso(char out[11]) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    snprintf(out, 11, "%04d-%02d-%02d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
}

static int parse_date(const char *in, char out[11]) {
    int y = 0, m = 0, d = 0, n = 0;
    if (sscanf(in, "%d-%d-%d%n", &y, &m, &d, &n) == 3 && in[n] == 0) {
    } else if (sscanf(in, "%d/%d/%d%n", &d, &m, &y, &n) == 3 && in[n] == 0) {
    } else {
        return 0;
    }
    if (y < 1900 || y > 2100 || m < 1 || m > 12 || d < 1 || d > dim(y, m)) return 0;
    snprintf(out, 11, "%04d-%02d-%02d", y % 10000, m % 100, d % 100);
    return 1;
}

static int valid_phone(const char *s) {
    size_t n = strlen(s);
    if (n < 6 || n > 24) return 0;
    for (size_t i = 0; i < n; i++)
        if (!isdigit((unsigned char)s[i]) && !strchr("+()-. ", s[i])) return 0;
    return 1;
}

static int valid_email(const char *s) {
    size_t n = strlen(s);
    if (n < 5 || n > 100) return 0;
    const char *at = strchr(s, '@');
    if (!at || at == s || strchr(at + 1, '@')) return 0;
    for (size_t i = 0; i < n; i++)
        if (isspace((unsigned char)s[i])) return 0;
    const char *dot = strrchr(at, '.');
    return dot && dot > at + 1 && dot[1] != 0;
}

static int valid_id(const char *s) {
    size_t n = strlen(s);
    if (n < 1 || n >= ID_LEN) return 0;
    for (size_t i = 0; i < n; i++)
        if (!isalnum((unsigned char)s[i]) && s[i] != '-' && s[i] != '_') return 0;
    return 1;
}

static int eq_ci(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++;
        b++;
    }
    return *a == *b;
}

static int contains_ci(const char *hay, const char *needle) {
    size_t n = strlen(needle);
    if (!n) return 1;
    for (; *hay; hay++) {
        size_t i = 0;
        while (i < n && hay[i] && tolower((unsigned char)hay[i]) == tolower((unsigned char)needle[i])) i++;
        if (i == n) return 1;
    }
    return 0;
}

static void copy_str(char *dst, size_t size, const char *src) {
    size_t n = strlen(src);
    if (n >= size) n = size - 1;
    memcpy(dst, src, n);
    dst[n] = 0;
}

typedef enum { T_TEXT, T_EMAIL, T_PHONE, T_DATE, T_CHOICE } FieldType;

typedef struct {
    const char *label;
    FieldType type;
    int required;
    const char *const *choices;
    const char *section;
} Field;

typedef struct {
    const char *singular;
    const char *plural;
    const char *slug;
    char prefix;
    const Field *fields;
    int nfields;
} Kind;

static const char *const CONTRACTS[] = {"CDI", "CDD", "Intérim", "Stage", "Freelance", NULL};
static const char *const SETUPS[] = {"Sur site", "Télétravail", "Hybride", NULL};
static const char *const STATUSES[] = {"Période d'essai", "Titulaire", "Suspendu", "Démissionnaire", NULL};

#define SEC_PERSONAL "Informations personnelles"
#define SEC_JOB "Détails d'emploi"
#define SEC_EMERGENCY "Contact d'urgence"

static const Field EMP_FIELDS[] = {
    {"Prénom", T_TEXT, 1, NULL, SEC_PERSONAL},
    {"Nom", T_TEXT, 1, NULL, SEC_PERSONAL},
    {"Poste", T_TEXT, 0, NULL, SEC_PERSONAL},
    {"Département", T_TEXT, 0, NULL, SEC_PERSONAL},
    {"Téléphone", T_PHONE, 0, NULL, SEC_PERSONAL},
    {"Email", T_EMAIL, 0, NULL, SEC_PERSONAL},
    {"Adresse", T_TEXT, 0, NULL, SEC_PERSONAL},
    {"Type de contrat", T_CHOICE, 0, CONTRACTS, SEC_PERSONAL},
    {"Mode de travail", T_CHOICE, 0, SETUPS, SEC_PERSONAL},
    {"Date d'embauche", T_DATE, 0, NULL, SEC_JOB},
    {"Statut", T_CHOICE, 0, STATUSES, SEC_JOB},
    {"Supérieur direct", T_TEXT, 0, NULL, SEC_JOB},
    {"Fin de période d'essai", T_DATE, 0, NULL, SEC_JOB},
    {"Date de titularisation", T_DATE, 0, NULL, SEC_JOB},
    {"Urgence — nom", T_TEXT, 0, NULL, SEC_EMERGENCY},
    {"Urgence — lien de parenté", T_TEXT, 0, NULL, SEC_EMERGENCY},
    {"Urgence — téléphone", T_PHONE, 0, NULL, SEC_EMERGENCY},
    {"Urgence — adresse", T_TEXT, 0, NULL, SEC_EMERGENCY}};

static const Field STU_FIELDS[] = {
    {"Nom", T_TEXT, 1, NULL, "Identité de l'élève"},
    {"Prénom", T_TEXT, 1, NULL, "Identité de l'élève"},
    {"Date de naissance", T_DATE, 0, NULL, "Identité de l'élève"},
    {"Lieu de naissance", T_TEXT, 0, NULL, "Identité de l'élève"},
    {"Adresse", T_TEXT, 0, NULL, "Identité de l'élève"},
    {"Nom du père", T_TEXT, 0, NULL, "Père"},
    {"Profession du père", T_TEXT, 0, NULL, "Père"},
    {"Nom de la mère", T_TEXT, 0, NULL, "Mère"},
    {"Profession de la mère", T_TEXT, 0, NULL, "Mère"},
    {"Contact des parents", T_PHONE, 0, NULL, "Contact et scolarité d'origine"},
    {"École d'origine", T_TEXT, 0, NULL, "Contact et scolarité d'origine"},
    {"Classe d'origine", T_TEXT, 0, NULL, "Contact et scolarité d'origine"}};

enum { E_HIRED = 9, E_PROBATION = 12, E_REGULARIZATION = 13, S_BIRTH = 2 };

static const Kind KINDS[2] = {
    {"employé", "Employés", "employes", 'E', EMP_FIELDS, 18},
    {"élève", "Élèves", "eleves", 'M', STU_FIELDS, 12}};

typedef struct {
    int date;
    char value;
} Mark;

typedef struct {
    char id[ID_LEN];
    char name[NAME_LEN];
    char f[MAXF][FLEN];
    Mark *marks;
    size_t nmarks, capmarks;
} Person;

typedef struct {
    Person *items;
    size_t n, cap;
} List;

typedef struct {
    char email[128];
    char name[64];
    List lists[2];
    unsigned rest;
    int month, year, kind;
    char filter[64];
} Session;

static const char *const MONTHS[12] = {"Janvier", "Février", "Mars", "Avril", "Mai", "Juin",
                                       "Juillet", "Août", "Septembre", "Octobre", "Novembre", "Décembre"};
static const char *const WD2[7] = {"Di", "Lu", "Ma", "Me", "Je", "Ve", "Sa"};
static const char *const WD3[7] = {"dim", "lun", "mar", "mer", "jeu", "ven", "sam"};

static Person *list_add(List *l) {
    if (l->n >= MAX_PEOPLE) return NULL;
    if (l->n == l->cap) {
        size_t cap = l->cap ? l->cap * 2 : 16;
        Person *items = realloc(l->items, cap * sizeof *items);
        if (!items) return NULL;
        l->items = items;
        l->cap = cap;
    }
    Person *p = &l->items[l->n++];
    memset(p, 0, sizeof *p);
    return p;
}

static void list_remove(List *l, size_t index) {
    free(l->items[index].marks);
    memmove(&l->items[index], &l->items[index + 1], (l->n - index - 1) * sizeof(Person));
    l->n--;
}

static void list_free(List *l) {
    for (size_t i = 0; i < l->n; i++) free(l->items[i].marks);
    free(l->items);
    l->items = NULL;
    l->n = l->cap = 0;
}

static Person *find_person(List *l, const char *id) {
    for (size_t i = 0; i < l->n; i++)
        if (eq_ci(l->items[i].id, id)) return &l->items[i];
    return NULL;
}

static char get_mark(const Person *p, int date) {
    for (size_t i = 0; i < p->nmarks; i++)
        if (p->marks[i].date == date) return p->marks[i].value;
    return 0;
}

static int set_mark(Person *p, int date, char value) {
    for (size_t i = 0; i < p->nmarks; i++) {
        if (p->marks[i].date != date) continue;
        if (value) {
            p->marks[i].value = value;
        } else {
            p->marks[i] = p->marks[--p->nmarks];
        }
        return 1;
    }
    if (!value) return 1;
    if (p->nmarks == p->capmarks) {
        size_t cap = p->capmarks ? p->capmarks * 2 : 32;
        Mark *marks = realloc(p->marks, cap * sizeof *marks);
        if (!marks) return 0;
        p->marks = marks;
        p->capmarks = cap;
    }
    p->marks[p->nmarks].date = date;
    p->marks[p->nmarks].value = value;
    p->nmarks++;
    return 1;
}

static int is_rest(const Session *s, int day) {
    return (s->rest >> weekday(s->year, s->month, day)) & 1u;
}

static void count_for(const Session *s, const Person *p, int *present, int *absent) {
    *present = *absent = 0;
    int n = dim(s->year, s->month);
    for (int d = 1; d <= n; d++) {
        if (is_rest(s, d)) continue;
        char v = get_mark(p, make_date(s->year, s->month, d));
        if (v == 'P') (*present)++;
        else if (v == 'A') (*absent)++;
    }
}

static void user_path(const char *email, char *out, size_t size) {
    uint8_t digest[32];
    char hex[65];
    sha256((const uint8_t *)email, strlen(email), digest);
    to_hex(digest, 16, hex);
    snprintf(out, size, DATA_DIR "/u_%s.dat", hex);
}

static int split_tabs(char *line, char **out, int max) {
    int n = 0;
    char *p = line;
    while (n < max) {
        out[n++] = p;
        char *tab = strchr(p, '\t');
        if (!tab) break;
        *tab = 0;
        p = tab + 1;
    }
    return n;
}

static void strip_eol(char *s) {
    size_t n = strlen(s);
    while (n && (s[n - 1] == '\n' || s[n - 1] == '\r')) s[--n] = 0;
}

static int save_session(const Session *s) {
    char path[256], tmp[262];
    user_path(s->email, path, sizeof path);
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    FILE *f = fopen(tmp, "w");
    if (!f) return 0;
    fprintf(f, "V1\nR\t%u\n", s->rest);
    for (int k = 0; k < 2; k++) {
        const List *l = &s->lists[k];
        for (size_t i = 0; i < l->n; i++) {
            const Person *p = &l->items[i];
            fprintf(f, "P\t%d\t%s\t%s", k, p->id, p->name);
            for (int j = 0; j < KINDS[k].nfields; j++) fprintf(f, "\t%s", p->f[j]);
            fputc('\n', f);
            for (size_t m = 0; m < p->nmarks; m++)
                fprintf(f, "M\t%d\t%s\t%d\t%c\n", k, p->id, p->marks[m].date, p->marks[m].value);
        }
    }
    int ok = !ferror(f);
    if (fclose(f) != 0) ok = 0;
    if (!ok) {
        remove(tmp);
        return 0;
    }
#ifdef _WIN32
    remove(path);
#endif
    return rename(tmp, path) == 0;
}

static void load_session(Session *s) {
    char path[256], line[LINE_LEN];
    user_path(s->email, path, sizeof path);
    FILE *f = fopen(path, "r");
    if (!f) return;
    while (fgets(line, sizeof line, f)) {
        if (!strchr(line, '\n') && !feof(f)) {
            int c;
            while ((c = fgetc(f)) != '\n' && c != EOF) {
            }
            continue;
        }
        strip_eol(line);
        char *tok[4 + MAXF + 1];
        int n = split_tabs(line, tok, 4 + MAXF);
        if (n >= 2 && strcmp(tok[0], "R") == 0) {
            s->rest = (unsigned)strtoul(tok[1], NULL, 10) & 0x7Fu;
        } else if (n >= 4 && strcmp(tok[0], "P") == 0) {
            int k = tok[1][0] - '0';
            if (k < 0 || k > 1 || !valid_id(tok[2]) || find_person(&s->lists[k], tok[2])) continue;
            Person *p = list_add(&s->lists[k]);
            if (!p) continue;
            copy_str(p->id, sizeof p->id, tok[2]);
            copy_str(p->name, sizeof p->name, tok[3]);
            for (int j = 0; j < KINDS[k].nfields && 4 + j < n; j++) copy_str(p->f[j], FLEN, tok[4 + j]);
        } else if (n >= 5 && strcmp(tok[0], "M") == 0) {
            int k = tok[1][0] - '0';
            if (k < 0 || k > 1) continue;
            Person *p = find_person(&s->lists[k], tok[2]);
            long date = strtol(tok[3], NULL, 10);
            char v = tok[4][0];
            if (p && date > 19000000L && date < 21010000L && (v == 'P' || v == 'A')) set_mark(p, (int)date, v);
        }
    }
    fclose(f);
}

static int find_user(const char *email, char *name, size_t name_size, uint8_t salt[SALT_LEN], uint8_t hash[HASH_LEN]) {
    char line[LINE_LEN];
    FILE *f = fopen(USERS_FILE, "r");
    if (!f) return 0;
    int found = 0;
    while (!found && fgets(line, sizeof line, f)) {
        strip_eol(line);
        char *tok[5];
        if (split_tabs(line, tok, 5) < 4) continue;
        if (strcmp(tok[0], email) != 0) continue;
        if (from_hex(tok[2], salt, SALT_LEN) && from_hex(tok[3], hash, HASH_LEN)) {
            copy_str(name, name_size, tok[1]);
            found = 1;
        }
    }
    fclose(f);
    return found;
}

static int add_user(const char *email, const char *name, const uint8_t salt[SALT_LEN], const uint8_t hash[HASH_LEN]) {
    char salt_hex[SALT_LEN * 2 + 1], hash_hex[HASH_LEN * 2 + 1];
    to_hex(salt, SALT_LEN, salt_hex);
    to_hex(hash, HASH_LEN, hash_hex);
    FILE *f = fopen(USERS_FILE, "a");
    if (!f) return 0;
    fprintf(f, "%s\t%s\t%s\t%s\n", email, name, salt_hex, hash_hex);
    return fclose(f) == 0;
}

static void lower_str(char *s) {
    for (; *s; s++) *s = (char)tolower((unsigned char)*s);
}

static int utf8_len(unsigned char c) {
    if (c < 0x80) return 1;
    if ((c >> 5) == 6) return 2;
    if ((c >> 4) == 14) return 3;
    if ((c >> 3) == 30) return 4;
    return 1;
}

static void pad_print(const char *s, int width) {
    int cols = 0;
    while (*s && cols < width) {
        int len = utf8_len((unsigned char)*s), i = 0;
        while (i < len && s[i]) putchar(s[i++]);
        s += i;
        cols++;
    }
    while (cols++ < width) putchar(' ');
}

static const char *col(const char *code) { return use_color ? code : ""; }

static void print_header(const Session *s) {
    printf("\n== Feuille de pointage — %s — %s %d", KINDS[s->kind].plural, MONTHS[s->month - 1], s->year);
    if (s->filter[0]) printf(" — filtre : « %s »", s->filter);
    puts(" ==");
}

static void show_sheet(const Session *s) {
    const List *l = &s->lists[s->kind];
    int n = dim(s->year, s->month);
    print_header(s);
    int shown = 0;
    for (size_t i = 0; i < l->n; i++) {
        const Person *p = &l->items[i];
        int match = !s->filter[0] || contains_ci(p->id, s->filter) || contains_ci(p->name, s->filter);
        for (int j = 0; !match && j < KINDS[s->kind].nfields; j++) match = contains_ci(p->f[j], s->filter);
        shown += match;
    }
    if (!shown) {
        puts(l->n ? "Aucun résultat." : "Aucune entrée. Utilisez le menu pour en ajouter.");
    }
    for (int start = 1; shown && start <= n; start += 16) {
        int end = start + 15 < n ? start + 15 : n;
        printf("\n%-8s%-22s", "ID", "Nom");
        for (int d = start; d <= end; d++) printf("%s ", WD2[weekday(s->year, s->month, d)]);
        printf("\n%-8s%-22s", "", "");
        for (int d = start; d <= end; d++) printf("%2d ", d);
        putchar('\n');
        for (size_t i = 0; i < l->n; i++) {
            const Person *p = &l->items[i];
            int match = !s->filter[0] || contains_ci(p->id, s->filter) || contains_ci(p->name, s->filter);
            for (int j = 0; !match && j < KINDS[s->kind].nfields; j++) match = contains_ci(p->f[j], s->filter);
            if (!match) continue;
            pad_print(p->id, 8);
            pad_print(p->name, 22);
            for (int d = start; d <= end; d++) {
                if (is_rest(s, d)) {
                    printf("%s-- %s", col("\x1b[90m"), col("\x1b[0m"));
                    continue;
                }
                char v = get_mark(p, make_date(s->year, s->month, d));
                if (v == 'P') printf("%s P %s", col("\x1b[32;1m"), col("\x1b[0m"));
                else if (v == 'A') printf("%s A %s", col("\x1b[31;1m"), col("\x1b[0m"));
                else printf(" . ");
            }
            if (end == n) {
                int pr, ab;
                count_for(s, p, &pr, &ab);
                printf(" P:%-2d A:%-2d", pr, ab);
            }
            putchar('\n');
        }
    }
    int total_p = 0, total_a = 0;
    for (size_t i = 0; i < l->n; i++) {
        int pr, ab;
        count_for(s, &l->items[i], &pr, &ab);
        total_p += pr;
        total_a += ab;
    }
    printf("\nPrésences : %d   Absences : %d   %s : %zu\n", total_p, total_a, KINDS[s->kind].plural, l->n);
}

static int validate_field(const Field *fd, const char *in, char out[FLEN]) {
    switch (fd->type) {
    case T_TEXT:
        copy_str(out, FLEN, in);
        return 1;
    case T_EMAIL:
        if (!valid_email(in)) {
            puts("Email invalide.");
            return 0;
        }
        copy_str(out, FLEN, in);
        return 1;
    case T_PHONE:
        if (!valid_phone(in)) {
            puts("Numéro invalide (6 à 24 caractères : chiffres, + ( ) - . espace).");
            return 0;
        }
        copy_str(out, FLEN, in);
        return 1;
    case T_DATE:
        if (!parse_date(in, out)) {
            puts("Date invalide (format JJ/MM/AAAA ou AAAA-MM-JJ).");
            return 0;
        }
        return 1;
    case T_CHOICE: {
        int count = 0;
        while (fd->choices[count]) count++;
        char *end;
        long idx = strtol(in, &end, 10);
        if (*end == 0 && idx >= 1 && idx <= count) {
            copy_str(out, FLEN, fd->choices[idx - 1]);
            return 1;
        }
        for (int i = 0; i < count; i++) {
            if (eq_ci(in, fd->choices[i])) {
                copy_str(out, FLEN, fd->choices[i]);
                return 1;
            }
        }
        puts("Choix invalide.");
        return 0;
    }
    }
    return 0;
}

static int prompt_field(const Field *fd, char *value) {
    char buf[FLEN], prompt[256], canon[FLEN];
    for (;;) {
        int n = snprintf(prompt, sizeof prompt, "  %s%s", fd->label, fd->required ? " *" : "");
        if (fd->type == T_CHOICE) {
            n += snprintf(prompt + n, sizeof prompt - (size_t)n, " (");
            for (int i = 0; fd->choices[i]; i++)
                n += snprintf(prompt + n, sizeof prompt - (size_t)n, "%s%d=%s", i ? ", " : "", i + 1, fd->choices[i]);
            n += snprintf(prompt + n, sizeof prompt - (size_t)n, ")");
        }
        if (value[0]) n += snprintf(prompt + n, sizeof prompt - (size_t)n, " [%s]", value);
        snprintf(prompt + n, sizeof prompt - (size_t)n, " : ");
        read_line(prompt, buf, sizeof buf);
        if (strcmp(buf, "/q") == 0) return 0;
        if (!buf[0]) {
            if (value[0]) return 1;
            if (fd->required) {
                puts("Champ obligatoire.");
                continue;
            }
            return 1;
        }
        if (validate_field(fd, buf, canon)) {
            copy_str(value, FLEN, canon);
            return 1;
        }
    }
}

static int cross_check(int kind, char f[][FLEN], const char **msg) {
    if (kind == 0) {
        if (f[E_HIRED][0] && f[E_PROBATION][0] && strcmp(f[E_PROBATION], f[E_HIRED]) < 0) {
            *msg = "La fin d'essai précède la date d'embauche.";
            return E_PROBATION;
        }
        if (f[E_PROBATION][0] && f[E_REGULARIZATION][0] && strcmp(f[E_REGULARIZATION], f[E_PROBATION]) < 0) {
            *msg = "La titularisation précède la fin d'essai.";
            return E_REGULARIZATION;
        }
    } else {
        char today[11];
        today_iso(today);
        if (f[S_BIRTH][0] && strcmp(f[S_BIRTH], today) > 0) {
            *msg = "La date de naissance est dans le futur.";
            return S_BIRTH;
        }
    }
    return -1;
}

static void next_id(const List *l, char prefix, char out[ID_LEN]) {
    long max = 0;
    for (size_t i = 0; i < l->n; i++) {
        const char *id = l->items[i].id;
        if (toupper((unsigned char)id[0]) != prefix || !id[1]) continue;
        char *end;
        long v = strtol(id + 1, &end, 10);
        if (*end == 0 && v > max) max = v;
    }
    snprintf(out, ID_LEN, "%c%03ld", prefix, max + 1);
}

static void action_add(Session *s, int kind) {
    const Kind *k = &KINDS[kind];
    List *l = &s->lists[kind];
    char id[FLEN] = "", f[MAXF][FLEN];
    memset(f, 0, sizeof f);
    printf("\n== Fiche %s (saisir /q pour annuler, Entrée pour passer un champ facultatif) ==\n", k->singular);
    char suggestion[ID_LEN];
    next_id(l, k->prefix, suggestion);
    copy_str(id, sizeof id, suggestion);
    Field id_field = {kind == 0 ? "ID employé" : "Numéro matricule", T_TEXT, 1, NULL, NULL};
    for (;;) {
        if (!prompt_field(&id_field, id)) return;
        if (!valid_id(id)) {
            puts("ID invalide (1 à 16 caractères : lettres, chiffres, - et _).");
            id[0] = 0;
            continue;
        }
        if (find_person(l, id)) {
            puts("Cet identifiant existe déjà.");
            id[0] = 0;
            continue;
        }
        break;
    }
    const char *section = "";
    for (int i = 0; i < k->nfields; i++) {
        if (strcmp(section, k->fields[i].section) != 0) {
            section = k->fields[i].section;
            printf("\n-- %s --\n", section);
        }
        if (!prompt_field(&k->fields[i], f[i])) {
            puts("Annulé.");
            return;
        }
    }
    const char *msg;
    int bad;
    while ((bad = cross_check(kind, f, &msg)) >= 0) {
        printf("%s\n", msg);
        f[bad][0] = 0;
        if (!prompt_field(&k->fields[bad], f[bad])) {
            puts("Annulé.");
            return;
        }
    }
    Person *p = list_add(l);
    if (!p) {
        puts("Capacité maximale atteinte ou mémoire insuffisante.");
        return;
    }
    copy_str(p->id, sizeof p->id, id);
    snprintf(p->name, sizeof p->name, "%.60s %.60s", f[0], f[1]);
    memcpy(p->f, f, sizeof f);
    s->kind = kind;
    s->filter[0] = 0;
    puts(save_session(s) ? "Enregistré." : "Ajouté en mémoire, mais ÉCHEC de la sauvegarde sur disque.");
}

static int parse_days(const char *in, int n, int *from, int *to) {
    if (strcmp(in, "*") == 0) {
        *from = 1;
        *to = n;
        return 1;
    }
    int a, b, len = 0;
    if (sscanf(in, "%d-%d%n", &a, &b, &len) == 2 && in[len] == 0) {
    } else if (sscanf(in, "%d%n", &a, &len) == 1 && in[len] == 0) {
        b = a;
    } else {
        return 0;
    }
    if (a < 1 || b > n || a > b) return 0;
    *from = a;
    *to = b;
    return 1;
}

static void action_mark(Session *s) {
    List *l = &s->lists[s->kind];
    char id[64], days[32], status[8];
    int n = dim(s->year, s->month), from, to;
    read_line("ID (ou * pour toute la liste) : ", id, sizeof id);
    Person *single = NULL;
    if (strcmp(id, "*") != 0) {
        single = find_person(l, id);
        if (!single) {
            puts("ID introuvable.");
            return;
        }
    }
    read_line("Jour (5), plage (1-10) ou * : ", days, sizeof days);
    if (!parse_days(days, n, &from, &to)) {
        printf("Jours invalides (1 à %d).\n", n);
        return;
    }
    read_line("Statut P=présent, A=absent, -=effacer : ", status, sizeof status);
    char v;
    if (eq_ci(status, "P")) v = 'P';
    else if (eq_ci(status, "A")) v = 'A';
    else if (strcmp(status, "-") == 0) v = 0;
    else {
        puts("Statut invalide.");
        return;
    }
    int changed = 0;
    for (size_t i = 0; i < l->n; i++) {
        Person *p = &l->items[i];
        if (single && p != single) continue;
        for (int d = from; d <= to; d++) {
            if (is_rest(s, d)) continue;
            if (!set_mark(p, make_date(s->year, s->month, d), v)) {
                puts("Mémoire insuffisante.");
                return;
            }
            changed++;
        }
    }
    printf("%d case(s) modifiée(s) (jours de repos ignorés).\n", changed);
    if (!save_session(s)) puts("ÉCHEC de la sauvegarde sur disque.");
}

static void action_details(Session *s) {
    List *l = &s->lists[s->kind];
    const Kind *k = &KINDS[s->kind];
    char id[64];
    read_line("ID : ", id, sizeof id);
    Person *p = find_person(l, id);
    if (!p) {
        puts("ID introuvable.");
        return;
    }
    int pr, ab;
    count_for(s, p, &pr, &ab);
    int n = dim(s->year, s->month), rest = 0;
    for (int d = 1; d <= n; d++) rest += is_rest(s, d);
    printf("\n== %s (%s) — %s %d ==\n", p->name, p->id, MONTHS[s->month - 1], s->year);
    printf("  Présences : %d\n  Absences : %d\n  Non pointés : %d\n  Jours de repos : %d\n  Taux de présence : %d %%\n", pr, ab,
           n - rest - pr - ab, rest, pr + ab ? pr * 100 / (pr + ab) : 0);
    const char *section = "";
    for (int i = 0; i < k->nfields; i++) {
        if (!p->f[i][0]) continue;
        if (strcmp(section, k->fields[i].section) != 0) {
            section = k->fields[i].section;
            printf("\n-- %s --\n", section);
        }
        fputs("  ", stdout);
        pad_print(k->fields[i].label, 28);
        printf(" %s\n", p->f[i]);
    }
}

static void action_delete(Session *s) {
    List *l = &s->lists[s->kind];
    char id[64], answer[8];
    read_line("ID à supprimer : ", id, sizeof id);
    Person *p = find_person(l, id);
    if (!p) {
        puts("ID introuvable.");
        return;
    }
    printf("Supprimer %s (%s) et tout son historique ? (o/N) ", p->name, p->id);
    read_line(NULL, answer, sizeof answer);
    if (!eq_ci(answer, "o") && !eq_ci(answer, "oui")) {
        puts("Annulé.");
        return;
    }
    list_remove(l, (size_t)(p - l->items));
    puts(save_session(s) ? "Supprimé." : "Supprimé en mémoire, mais ÉCHEC de la sauvegarde.");
}

static void action_rest_days(Session *s) {
    char buf[96];
    printf("Jours de repos actuels :");
    int any = 0;
    for (int i = 0; i < 7; i++)
        if ((s->rest >> i) & 1u) {
            printf(" %s", WD3[i]);
            any = 1;
        }
    puts(any ? "" : " aucun");
    read_line("Nouveaux jours (ex : sam dim), 'aucun', ou Entrée pour garder : ", buf, sizeof buf);
    if (!buf[0]) return;
    unsigned mask = 0;
    if (!eq_ci(buf, "aucun")) {
        for (char *tok = strtok(buf, " ,;"); tok; tok = strtok(NULL, " ,;")) {
            int found = 0;
            if (strlen(tok) >= 3) {
                char head[4] = {(char)tolower((unsigned char)tok[0]), (char)tolower((unsigned char)tok[1]),
                                (char)tolower((unsigned char)tok[2]), 0};
                for (int i = 0; i < 7; i++) {
                    if (strcmp(head, WD3[i]) == 0) {
                        mask |= 1u << i;
                        found = 1;
                    }
                }
            }
            if (!found) {
                printf("Jour inconnu : %s\n", tok);
                return;
            }
        }
    }
    s->rest = mask;
    puts(save_session(s) ? "Enregistré." : "ÉCHEC de la sauvegarde.");
}

static void action_period(Session *s) {
    char buf[32];
    int m, y, len = 0;
    read_line("Période (MM/AAAA) ou mois seul (1-12) : ", buf, sizeof buf);
    if (sscanf(buf, "%d/%d%n", &m, &y, &len) == 2 && buf[len] == 0) {
    } else if (sscanf(buf, "%d%n", &m, &len) == 1 && buf[len] == 0) {
        y = s->year;
    } else {
        puts("Format invalide.");
        return;
    }
    if (m < 1 || m > 12 || y < 2000 || y > 2100) {
        puts("Période hors limites (mois 1-12, année 2000-2100).");
        return;
    }
    s->month = m;
    s->year = y;
}

static void html_escape(FILE *f, const char *s) {
    for (; *s; s++) {
        switch (*s) {
        case '&': fputs("&amp;", f); break;
        case '<': fputs("&lt;", f); break;
        case '>': fputs("&gt;", f); break;
        case '"': fputs("&quot;", f); break;
        case '\'': fputs("&#39;", f); break;
        default: fputc(*s, f);
        }
    }
}

static void action_export(Session *s) {
    List *l = &s->lists[s->kind];
    char ids[512];
    read_line("IDs à exporter séparés par des espaces (ou * pour tous) : ", ids, sizeof ids);
    if (!ids[0]) {
        puts("Aucune sélection.");
        return;
    }
    char *selected_ids[256];
    int nsel = 0, all = strcmp(ids, "*") == 0;
    if (!all) {
        for (char *tok = strtok(ids, " ,;"); tok && nsel < 256; tok = strtok(NULL, " ,;")) {
            if (!find_person(l, tok)) {
                printf("ID introuvable : %s\n", tok);
                return;
            }
            selected_ids[nsel++] = tok;
        }
    }
    char path[96];
    snprintf(path, sizeof path, "pointage_%s_%04d-%02d.html", KINDS[s->kind].slug, s->year, s->month);
    FILE *f = fopen(path, "w");
    if (!f) {
        puts("Impossible de créer le fichier d'export.");
        return;
    }
    int n = dim(s->year, s->month);
    fprintf(f, "<!doctype html><html lang=\"fr\"><head><meta charset=\"utf-8\"><title>Pointage</title><style>"
               "body{font-family:sans-serif;color:#111}table{border-collapse:collapse;font-size:10px;width:100%%}"
               "th,td{border:1px solid #bbb;padding:3px 2px;text-align:center}th{background:#0b3b5c;color:#fff;-webkit-print-color-adjust:exact;print-color-adjust:exact}"
               "td.n{text-align:left;white-space:nowrap}td.p{background:#dff5e6;color:#1b8a4b;font-weight:700}"
               "td.a{background:#fde2e4;color:#d12f3a;font-weight:700}td.r{background:#e6eaf0}"
               "td,th{-webkit-print-color-adjust:exact;print-color-adjust:exact}@page{size:landscape;margin:10mm}"
               "</style></head><body><h1>Feuille de pointage — %s — %s %d</h1><p>",
            KINDS[s->kind].plural, MONTHS[s->month - 1], s->year);
    html_escape(f, s->name);
    fputs("</p><table><thead><tr><th>ID</th><th>Nom</th>", f);
    for (int d = 1; d <= n; d++) fprintf(f, "<th>%s<br>%d</th>", WD2[weekday(s->year, s->month, d)], d);
    fputs("<th>P</th><th>A</th></tr></thead><tbody>", f);
    int rows = 0;
    for (size_t i = 0; i < l->n; i++) {
        Person *p = &l->items[i];
        int take = all;
        for (int j = 0; !take && j < nsel; j++) take = eq_ci(selected_ids[j], p->id);
        if (!take) continue;
        rows++;
        fputs("<tr><td>", f);
        html_escape(f, p->id);
        fputs("</td><td class=\"n\">", f);
        html_escape(f, p->name);
        fputs("</td>", f);
        for (int d = 1; d <= n; d++) {
            if (is_rest(s, d)) {
                fputs("<td class=\"r\"></td>", f);
                continue;
            }
            char v = get_mark(p, make_date(s->year, s->month, d));
            if (v) fprintf(f, "<td class=\"%c\">%c</td>", v == 'P' ? 'p' : 'a', v);
            else fputs("<td></td>", f);
        }
        int pr, ab;
        count_for(s, p, &pr, &ab);
        fprintf(f, "<td>%d</td><td>%d</td></tr>", pr, ab);
    }
    fputs("</tbody></table></body></html>\n", f);
    if (fclose(f) != 0) {
        puts("Erreur d'écriture de l'export.");
        return;
    }
    printf("%d ligne(s) exportée(s) dans %s — ouvrez-le dans un navigateur puis « Imprimer > Enregistrer en PDF ».\n", rows, path);
}

static void action_search(Session *s) {
    read_line("Rechercher (ID, nom, poste, classe… ; Entrée pour effacer) : ", s->filter, sizeof s->filter);
    show_sheet(s);
}

static void session_free(Session *s) {
    list_free(&s->lists[0]);
    list_free(&s->lists[1]);
    zero_mem(s, sizeof *s);
}

static void run_session(Session *s) {
    char choice[16];
    for (;;) {
        printf("\n[%s] %s %d — %s\n", s->name, MONTHS[s->month - 1], s->year, KINDS[s->kind].plural);
        puts(" 1 Afficher la feuille     2 Pointer              3 Ajouter un employé   4 Ajouter un élève\n"
             " 5 Fiche détaillée        6 Supprimer            7 Rechercher           8 Jours de repos\n"
             " 9 Mois / année           T Basculer Employés/Élèves   E Export HTML (PDF)   0 Déconnexion");
        read_line("> ", choice, sizeof choice);
        if (strlen(choice) != 1) {
            puts("Choix invalide.");
            continue;
        }
        switch (toupper((unsigned char)choice[0])) {
        case '1': show_sheet(s); break;
        case '2': show_sheet(s); action_mark(s); show_sheet(s); break;
        case '3': action_add(s, 0); break;
        case '4': action_add(s, 1); break;
        case '5': action_details(s); break;
        case '6': action_delete(s); break;
        case '7': action_search(s); break;
        case '8': action_rest_days(s); break;
        case '9': action_period(s); break;
        case 'T': s->kind = !s->kind; s->filter[0] = 0; break;
        case 'E': action_export(s); break;
        case '0': return;
        default: puts("Choix invalide.");
        }
    }
}

static void start_session(const char *email, const char *name) {
    Session *s = calloc(1, sizeof *s);
    if (!s) {
        puts("Mémoire insuffisante.");
        return;
    }
    copy_str(s->email, sizeof s->email, email);
    copy_str(s->name, sizeof s->name, name);
    s->rest = 1u;
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    s->month = t->tm_mon + 1;
    s->year = t->tm_year + 1900;
    load_session(s);
    run_session(s);
    session_free(s);
    free(s);
}

static void do_register(void) {
    char name[64], email[128], pw[128], pw2[128];
    uint8_t salt[SALT_LEN], hash[HASH_LEN], tmp_salt[SALT_LEN], tmp_hash[HASH_LEN];
    char tmp_name[64];
    read_line("Nom complet : ", name, sizeof name);
    if (!name[0]) {
        puts("Nom requis.");
        return;
    }
    read_line("Email : ", email, sizeof email);
    lower_str(email);
    if (!valid_email(email)) {
        puts("Email invalide.");
        return;
    }
    int r = read_password("Mot de passe (8 caractères min.) : ", pw, sizeof pw);
    if (r < 0 || strlen(pw) < 8) {
        puts("Mot de passe trop court (8 min.) ou trop long (127 max.).");
        zero_mem(pw, sizeof pw);
        return;
    }
    r = read_password("Confirmer le mot de passe : ", pw2, sizeof pw2);
    if (r < 0 || strcmp(pw, pw2) != 0) {
        puts("Les mots de passe ne correspondent pas.");
        zero_mem(pw, sizeof pw);
        zero_mem(pw2, sizeof pw2);
        return;
    }
    zero_mem(pw2, sizeof pw2);
    if (find_user(email, tmp_name, sizeof tmp_name, tmp_salt, tmp_hash)) {
        puts("Cet email est déjà utilisé.");
        zero_mem(pw, sizeof pw);
        return;
    }
    if (!random_bytes(salt, sizeof salt)) {
        puts("Générateur aléatoire indisponible.");
        zero_mem(pw, sizeof pw);
        return;
    }
    pbkdf2_sha256((const uint8_t *)pw, strlen(pw), salt, sizeof salt, PBKDF2_ITER, hash);
    zero_mem(pw, sizeof pw);
    if (!add_user(email, name, salt, hash)) {
        puts("Impossible d'enregistrer le compte.");
        return;
    }
    puts("Compte créé.");
    start_session(email, name);
}

static void do_login(void) {
    char email[128], pw[128], name[64];
    uint8_t salt[SALT_LEN], hash[HASH_LEN], computed[HASH_LEN];
    static const uint8_t dummy_salt[SALT_LEN] = {0};
    for (int attempt = 1; attempt <= MAX_LOGIN_FAILS; attempt++) {
        read_line("Email : ", email, sizeof email);
        lower_str(email);
        int r = read_password("Mot de passe : ", pw, sizeof pw);
        int known = r > 0 && find_user(email, name, sizeof name, salt, hash);
        pbkdf2_sha256((const uint8_t *)pw, strlen(pw), known ? salt : dummy_salt, SALT_LEN, PBKDF2_ITER, computed);
        zero_mem(pw, sizeof pw);
        if (known && ct_equal(computed, hash, HASH_LEN)) {
            start_session(email, name);
            return;
        }
        puts("Email ou mot de passe incorrect.");
        sleep_ms(500u * (unsigned)attempt);
    }
    puts("Trop de tentatives.");
}

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "--selftest") == 0) return self_test();
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (!GetConsoleMode(out, &mode) || !SetConsoleMode(out, mode | 0x0004)) use_color = 0;
#else
    umask(077);
    signal(SIGINT, on_sigint);
    if (!isatty(STDOUT_FILENO)) use_color = 0;
#endif
    if (getenv("NO_COLOR")) use_color = 0;
    if (MKDIR(DATA_DIR) != 0 && errno != EEXIST) {
        fprintf(stderr, "Impossible de créer le dossier %s : %s\n", DATA_DIR, strerror(errno));
        return 1;
    }
    char choice[16];
    for (;;) {
        puts("\n=== Logiciel de pointage ===\n 1 Connexion\n 2 Créer un compte\n 0 Quitter");
        read_line("> ", choice, sizeof choice);
        if (strcmp(choice, "1") == 0) do_login();
        else if (strcmp(choice, "2") == 0) do_register();
        else if (strcmp(choice, "0") == 0) break;
        else puts("Choix invalide.");
    }
    return 0;
}
