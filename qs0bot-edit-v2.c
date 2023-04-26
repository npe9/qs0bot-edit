#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>

#define MAX_IN_LEN 1000
#define MAX_RESP_LEN 100
#define MAX_QUEST 10

typedef struct {
    char quest[MAX_IN_LEN];
    char resp[MAX_RESP_LEN];
} QSOItem;

char their_call[7];
int debug = 0;

QSOItem tab[MAX_QUEST] = {
        {"GM", "GM OM"},
        {"GA", "GA OM"},
        {"GE", "GE OM"},
        {"TNX FER", "R U DR OM"},
        {"UR RST 599", "RST 599 FB OM"},
        {"QTH", "MY QTH IS FRESNO, CA OM"},
        {"RIG", "MY RIG IS AN IC 7300 OM"},
        {"PWR", "MY PWR IS 50W OM"},
        {"ANT", "MY ANT IS A DIPOLE UP 45 FT OM"},
        {"WX", "WX HERE IS RAINY AND TEMP IS 50F OM"}
};

int min(int a, int b);
int edit_dist(const char* s1, const char* s2, int max_dist);
char* qso_conv(const char* input, int max_dist);

char* qso_conv(const char* input, int max_dist) {
    static char *resp = NULL;
    if (strncmp(input, "de ", 3) == 0) {
        strncpy(their_call, input + 3, 6);
        if (debug) {
            fprintf(stderr, "Call sign '%s' received. Beginning QSO.\n", their_call);
        }
        return NULL;
    }
    for (int d = 0; d <= max_dist; d++) {
        for (int i = 0; i < MAX_QUEST; i++) {
            int dist = edit_dist(tab[i].quest, input, d);
            if (dist >= 0) {
                resp = (char*) malloc(MAX_RESP_LEN);
                if (resp == NULL) {
                    fprintf(stderr, "Error allocating memory for response.\n");
                    exit(EXIT_FAILURE);
                }
                strncpy(resp, tab[i].resp, MAX_RESP_LEN);
                if (debug) {
                    fprintf(stderr, "Matched '%s' with distance %d, responding with '%s'\n",
                            tab[i].quest, dist, tab[i].resp);
                }
                return resp;
            }
        }
    }
    if (debug) {
        fprintf(stderr, "No match found. Ignoring.\n");
    }
    return NULL;
}




int main(int argc, char* argv[]) {
    int max_dist = 2;
    char* input_file = NULL;
    int opt;
    while ((opt = getopt(argc, argv, "dm:")) != -1) {
        switch (opt) {
            case 'd':
                debug = 1;
                break;
            case 'm':
                max_dist = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s [-d] [-m max_distance] [file]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    if (optind < argc) {
        input_file = argv[optind];
    }
    if (input_file == NULL) {
        input_file = "stdin";
    }
    FILE* fp;
    if (strcmp(input_file, "stdin") == 0) {
        fp = stdin;
    } else {
        fp = fopen(input_file, "r");
        if (fp == NULL) {
            fprintf(stderr, "Error opening file %s\n", input_file);
            exit(EXIT_FAILURE);
        }
    }
    if (debug) {
        fprintf(stderr, "Debugging enabled.\n");
        fprintf(stderr, "Max edit distance: %d.\n", max_dist);
        fprintf(stderr, "Input source: %s.\n", input_file);
    }

    char input[MAX_IN_LEN];
    while (fgets(input, MAX_IN_LEN, fp)) {
        int len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }
        char *resp = qso_conv(input, max_dist);
        if (resp != NULL) {
            printf("%s\n", resp);
            free(resp);
            if (strcmp(input, their_call) == 0) {
                if (debug) {
                    fprintf(stderr, "QSO ended.\n");
                }
                break;
            }
        }
    }
    if (fp != stdin) {
        fclose(fp);
    }
    return 0;
}

int min(int a, int b) {
    return (a < b) ? a : b;
}

int edit_dist(const char *s1, const char *s2, int max_dist) {
    if (debug) {
        fprintf(stderr, "Calculating edit distance between '%s' and '%s' with max_dist %d.\n", s1, s2, max_dist);
    }
    int m = strlen(s1), n = strlen(s2);
    if (abs(m - n) > max_dist) {
        if (debug) {
            fprintf(stderr, "Strings '%s' and '%s' are too far apart to match.\n", s1, s2);
        }
        return -1;
    }
    int dp[2][n+1];
    memset(dp, 0, sizeof(dp));
    for (int j = 1; j <= n; j++) {
        dp[0][j] = j;
    }
    for (int i = 1; i <= m; i++) {
        dp[i%2][0] = i;
        for (int j = 1; j <= n; j++) {
            dp[i%2][j] = (s1[i-1] == s2[j-1]) ? dp[(i-1)%2][j-1] :
                         1 + min(dp[(i-1)%2][j], min(dp[i%2][j-1], dp[(i-1)%2][j-1]));
        }
    }
    int dist = dp[m%2][n];
    if (dist <= max_dist) {
        if (debug) {
            fprintf(stderr, "Strings '%s' and '%s' match with edit distance %d.\n", s1, s2, dist);
        }
        return dist;
    } else {
        if (debug) {
            fprintf(stderr, "Strings '%s' and '%s' are too far apart to match.\n", s1, s2);
        }
        return -1;
    }
}