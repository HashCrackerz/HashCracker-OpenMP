#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <omp.h>
#include <openssl/sha.h>
#include "ESTENSIONE/omp_estensione.h"
#include "UTILS/utils.h"     
#include "UTILS/costanti.h"  

// Variabile globale per fermare la ricerca
volatile int stop_search = 0;

double cpuSecond() {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return ((double)ts.tv_sec + (double)ts.tv_nsec * 1.e-9);
}

void idxToString(unsigned long long idx, char* str, int len, char* charSet, int charSetLen) {
    str[len] = '\0';
    for (int i = len - 1; i >= 0; i--) {
        str[i] = charSet[idx % charSetLen];
        idx /= charSetLen;
    }
}

// Helper per Hash check
int check_hash(const char* candidate, int len, unsigned char* target_hash) {
    unsigned char current_hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)candidate, len, current_hash);
    return (memcmp(current_hash, target_hash, SHA256_DIGEST_LENGTH) == 0);
}

// ---------------------------------------------------------
// ATTACCO A DIZIONARIO 
// ---------------------------------------------------------
int attackDictionaryOMP(char* flat_dict, int num_words, int salt_len, char* charSet, unsigned char* target_hash, char* result_buffer) {
    int charSetLen = strlen(charSet);
    unsigned long long totalSalts = pow((double)charSetLen, (double)salt_len);
    
    stop_search = 0;
    int found = 0;

    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < num_words; i++) {
        if (stop_search) continue;

        char* myWord = &flat_dict[i * DICT_WORD_LEN]; 
        int wordLen = strlen(myWord); 
        if (wordLen == 0) continue;

        char salt[MAX_SALT_LENGTH + 1]; 
        char combined[DICT_WORD_LEN + MAX_SALT_LENGTH + 1];

        // Loop sui possibili salt
        for (unsigned long long sIdx = 0; sIdx < totalSalts; sIdx++) {
            if (stop_search) break;

            idxToString(sIdx, salt, salt_len, charSet, charSetLen);

            // SALT + PASSWORD
            int k = 0;
            memcpy(combined, salt, salt_len);
            k += salt_len;
            memcpy(combined + k, myWord, wordLen);
            k += wordLen;
            
            if (check_hash(combined, k, target_hash)) {
                #pragma omp critical
                {
                    if (!stop_search) {
                        // Restituisco SOLO la parola del dizionario
                        strcpy(result_buffer, myWord);
                        stop_search = 1;
                        found = 1;
                    }
                }
                break;
            }

            //  PASSWORD + SALT 
            k = 0;
            memcpy(combined, myWord, wordLen);
            k += wordLen;
            memcpy(combined + k, salt, salt_len);
            k += salt_len;

            if (check_hash(combined, k, target_hash)) {
                #pragma omp critical
                {
                    if (!stop_search) {
                        // Restituisco SOLO la parola del dizionario anche qui
                        strcpy(result_buffer, myWord);
                        stop_search = 1;
                        found = 1;
                    }
                }
                break;
            }
        }
    }
    return found;
}

// ---------------------------------------------------------
// BRUTE FORCE SALTED 
// ---------------------------------------------------------
void bruteForceSaltOMP(int len, unsigned char* target_hash, char* charSet, char* result)
{
    int charSetLen = strlen(charSet);
    stop_search = 0;

    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < charSetLen; i++) {
        if (stop_search) continue;

        char* buf = (char*)malloc(sizeof(char) * (len + 1));
        int* indices = (int*)malloc(sizeof(int) * len);
        
        if(!buf || !indices) { 
            if(buf) free(buf); 
            if(indices) free(indices); 
            continue; 
        }

        buf[len] = '\0';
        indices[0] = i; 
        for (int k = 1; k < len; k++) indices[k] = 0;

        while (!stop_search) {
            for (int k = 0; k < len; k++) buf[k] = charSet[indices[k]];

            if (check_hash(buf, len, target_hash)) {
                #pragma omp critical
                {
                    strcpy(result, buf);
                    stop_search = 1;
                }
                break;
            }

            int k = len - 1;
            while (k > 0) {
                indices[k]++;
                if (indices[k] < charSetLen) break;
                else { indices[k] = 0; k--; }
            }
            if (k == 0) break;
        }
        free(indices);
        free(buf);
    }
}

void testEstensioneOpenMP(unsigned char* target_hash, int min_test_len, int max_test_len, char* charSet, char* salt_str, bool use_dict, char* dict_path) {
    
    char found_full_string[MAX_CANDIDATE] = {0};
    bool found = false;
    double startT = 0.0;

    if (use_dict) {
        printf("\nAttacco a Dizionario + Generazione Salt (OpenMP)...\n");
        int numWords = 0;
        char* flat_dict = load_flattened_dictionary(dict_path, &numWords);

        if (flat_dict && numWords > 0) {
            printf("Dizionario caricato: %d parole.\n", numWords);
            
            int saltLen = strlen(salt_str); 
            startT = cpuSecond();
            
            if (attackDictionaryOMP(flat_dict, numWords, saltLen, charSet, target_hash, found_full_string)) {
                
                printf("Parola candidata trovata (OpenMP): %s\n", found_full_string);

                if (testLogin(found_full_string, strlen(found_full_string), target_hash, salt_str)) {
                    printf("\n************************************************\n");
                    printf("*** PASSWORD TROVATA E VERIFICATA: %s ***\n", found_full_string);
                    printf("************************************************\n");
                    found = true;
                } else {
                    printf("Attenzione: Collisione trovata (probabilmente con un salt diverso), ma la verifica testLogin ha fallito.\n");
                    printf("Continuo la ricerca...\n");
                }

            } else {
                printf("Non trovata nel dizionario.\n");
            }
            free(flat_dict);
        } else {
            printf("Errore caricamento dizionario (o file vuoto).\n");
        }
    }

    if (!found) {
        printf("\nBrute Force OpenMP (Salted)...\n");
        
        memset(found_full_string, 0, MAX_CANDIDATE * sizeof(char)); 

        for (int len = min_test_len; len <= max_test_len; len++) {
            if (found) break;
            printf("Tentativo lunghezza totale (pass+salt) %d... ", len);
            fflush(stdout);

            memset(found_full_string, 0, MAX_CANDIDATE * sizeof(char));

            bruteForceSaltOMP(len, target_hash, charSet, found_full_string);

            if (strlen(found_full_string) > 0) {
                printf("TROVATA!\n");
                printf("\n************************************************\n");
                printf("*** PASSWORD TROVATA (Brute Force): %s ***\n", found_full_string);
                printf("************************************************\n");
                found = true;
            } else {
                printf("No.\n");
            }
        }
    }

    double totalTime = cpuSecond() - startT;

    if (!found) {
        printf("\nNessuna password trovata nel range specificato.\n");
    }

    printf("Tempo Totale: %.4f s\n", totalTime);
}