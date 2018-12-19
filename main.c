#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>
#define FREE -1
#define YES 1
#define NO 0
#define MAX_TRY 100000000

/** VARIAVEIS GLOBAIS *************************************************************************************************/

typedef struct {                        // estrutura de um sapo ou ra
    int frog_id;
    int position;
    pthread_t thread;
} frog;

int n_stones = 0;                       // numero de pedras
int n_frog_f = 0;                       // numero de ras
int n_frog_m = 0;                       // numero de sapos
frog *frogs;                            // vetor de frogs
int *stones;                            // vetor de pedras
int ready_frogs = 0;                    // numero de animais prontos
int fail_jump = 0;                      // contador de pulos fracassados
int max_fail_jump = 0;                  // contador de pulos fracassados maximo
int continue_jumping = YES;             // condicional para continuar o programa
int cleared_puzzle = NO;                // condicional que indica se o quebra-cabeca foi solucionado
int n_try = 0;                          // numero de tentativas ate finalizar o quebra-cabeca
uint64_t s_time = 0;                    // instante do inicio do programa
uint64_t f_time = 0;                    // instante de termino do programa
pthread_mutex_t mutex;
pthread_cond_t condition;

/** LAKE STATE ********************************************************************************************************/

// imprime do estado atual do lago
void print_lake() {
    int i;
    for (i = 0; i < n_stones; i++) {
        if (stones[i] == FREE)
            printf("(   ) ");
        else if (stones[i] < n_frog_f)
            printf("{ %d } ", stones[i]);
        else
            printf("[ %d ] ", stones[i]);
    }
    printf("\n");
}


// verifica se o quebra-cabeca terminou
void verify_cleared() {
    int i;
    for (i = 0; i < n_frog_m; i++)
        if (stones[i] == FREE || stones[i] < n_frog_f) return;

    for (i = n_stones - 1; i >= n_stones - n_frog_f; i--)
        if (stones[i] == FREE || stones[i] > n_frog_f) return;

    cleared_puzzle = YES;
}


// verifica se o programa entrou em DEADLOCK
void verify_deadlock() {
    int i;
    for (i = 0; i < n_stones; i++) {
        if (stones[i] == FREE) {
            int one_left = i - 1;
            int two_left = i - 2;
            int one_right = i + 1;
            int two_right = i + 2;

            if (one_left >= 0)
                if (stones[one_left] != FREE && stones[one_left] < n_frog_f) return;
            if (two_left >= 0)
                if (stones[two_left] != FREE && stones[two_left] < n_frog_f) return;
            if (one_right < n_stones)
                if (stones[one_right] != FREE && stones[one_right] > n_frog_f) return;
            if (two_right < n_stones)
                if (stones[two_right] != FREE && stones[two_right] > n_frog_f) return;
        }
    }
    printf("Parado por DEADLOCK\n");
    printf("Pulos fracassados: %d\n", fail_jump);
    continue_jumping = NO;
    verify_cleared();
}


// reinicia o contador de pulos fracassados e guarda a maior contagem
void reset_jump_count() {
    if (fail_jump > max_fail_jump)
        max_fail_jump = fail_jump;
    fail_jump = 0;
}

/** THREAD DOS ANIMAIS ************************************************************************************************/

// funcao responsavel para os sapos pularem para a esquerda
void *jump_to_left(void *frog_m) {
    int position, one_left, two_left;
    frog *f = (frog *) frog_m;

    pthread_mutex_lock(&mutex);
    ready_frogs++;
    pthread_cond_wait(&condition, &mutex);
    pthread_mutex_unlock(&mutex);

    while (continue_jumping) {
        pthread_mutex_lock(&mutex);

        position = f->position;
        one_left = position - 1;
        two_left = position - 2;
        if (one_left >= 0 && stones[one_left] == FREE) {
            stones[one_left] = f->frog_id;
            stones[position] = FREE;
            f->position = one_left;
            print_lake();
            verify_deadlock();
            reset_jump_count();
        } else if (two_left >= 0 && stones[two_left] == FREE) {
            stones[two_left] = f->frog_id;
            stones[position] = FREE;
            f->position = two_left;
            print_lake();
            verify_deadlock();
            reset_jump_count();
        } else {
            fail_jump++;
        }

        pthread_mutex_unlock(&mutex);
    }
    pthread_exit(NULL);
}


// funcao responsavel para as ras pularem para direita
void *jump_to_right(void *frog_f) {
    int position, one_right, two_right;
    frog *f = (frog *) frog_f;

    pthread_mutex_lock(&mutex);
    ready_frogs++;
    pthread_cond_wait(&condition, &mutex);
    pthread_mutex_unlock(&mutex);

    while (continue_jumping) {
        pthread_mutex_lock(&mutex);

        position = f->position;
        one_right = position + 1;
        two_right = position + 2;
        if (one_right < n_stones && stones[one_right] == FREE) {
            stones[one_right] = f->frog_id;
            stones[position] = FREE;
            f->position = one_right;
            print_lake();
            verify_deadlock();
            reset_jump_count();
        } else if (two_right < n_stones && stones[two_right] == FREE) {
            stones[two_right] = f->frog_id;
            stones[position] = FREE;
            f->position = two_right;
            print_lake();
            verify_deadlock();
            reset_jump_count();
        } else {
            fail_jump++;
        }

        pthread_mutex_unlock(&mutex);
    }
    pthread_exit(NULL);
}

/** FUNCOES RESPONSAVEIS PELO ANDAMENTO DO PROGRAMA *******************************************************************/

uint64_t getTime(void) {
    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    return ((uint64_t) (time.tv_sec) * 1000000000 + (uint64_t) (time.tv_nsec));
}


// recebe os parametros digitados pelo usuario
void get_info() {
    int consistent = 0;
    do {
        printf("Por favor, entre com\n");
        printf("Numero de Pedras:\n");
        scanf("%d", &n_stones);
        printf("Numero de Ras:\n");
        scanf("%d", &n_frog_f);
        printf("Numero de Sapos:\n");
        scanf("%d", &n_frog_m);
        if (n_stones >= n_frog_f + n_frog_m + 1)
            consistent = 1;
        else
            printf("A quantidade de pedras deve ser maior do que total de animais!\n");
    } while (!consistent);
    s_time = getTime();
}


// inicia as variaveis
void initialize() {
    ready_frogs = 0;
    fail_jump = 0;
    continue_jumping = YES;
}


// cria o lago com as pedras e os animais
void create_lake() {
    int i, j;
    stones = (int *) malloc(n_stones * sizeof(int));
    frogs = (frog *) malloc((n_frog_f + n_frog_m) * sizeof(frog));

    for (i = j = 0; i < n_stones; i++) {
        if (i < n_frog_f) {                     // cria as ras na esquerda
            stones[i] = i;
            frogs[j].frog_id = i;
            frogs[j].position = i;
            pthread_create(&frogs[j].thread, NULL, jump_to_right, &frogs[j]);
            j++;
        } else if (i < n_stones - n_frog_m) {
            stones[i] = FREE;                   // pedras vazias no centro
        } else {                                // cria os sapos na direita
            stones[i] = i;
            frogs[j].frog_id = i;
            frogs[j].position = i;
            pthread_create(&frogs[j].thread, NULL, jump_to_left, &frogs[j]);
            j++;
        }
    }
    print_lake();
}


// inicia o processo de pulo dos animais quando todos estiverem prontos
void start() {
    int ready = NO;

    do {
        if (pthread_mutex_lock(&mutex) == 0) {
            if (ready_frogs == n_frog_f + n_frog_m)
                ready = YES;
            pthread_cond_broadcast(&condition);
            pthread_mutex_unlock(&mutex);
        }
    } while (!ready);
}


// deixa o programa trabalhando
void get_working() {
    while (continue_jumping) {
        if (fail_jump > MAX_TRY) {
            printf("Parado por ultrapassar o numero de tentativas!\n");
            continue_jumping = NO;
        }
    }
}


// desaloca todos os alocamentos da memoria
void finish() {
    int i;
    for (i = 0; i < n_frog_f + n_frog_m; i++)
        pthread_join(frogs[i].thread, NULL);
    free(stones);
    free(frogs);
}

// imprime resultado
void print_results() {
    f_time = getTime();
    printf("QUEBRA-CABECA FINALIZADO!\n");
    printf("Numero de tentativas: %d\n", n_try);
    printf("Maximo de pulos fracassados: %d\n", max_fail_jump);
    printf("Tempo decorrido: %ld nanossegundos\n", f_time - s_time);
}

/** FUNCAO PRINCIPAL***************************************************************************************************/

int main() {

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&condition, NULL);

    get_info();

    do {
        n_try++;
        initialize();
        create_lake();
        start();
        get_working();
        finish();
    } while (!cleared_puzzle);

    print_results();
    pthread_exit(NULL);
}