#include <iostream>
#include <string>
#include <cstdlib>
#include <pthread.h>

using namespace std;

// Tamanho da matriz codificadora
#define SIZE 4

// Matriz codificadora
// Atual = ENGEN COMPUTACAO
// Futuramente: criada a partir de palavra inserida
double cod[SIZE][SIZE] = {
    {69, 78, 71, 69},
    {78, 32, 67, 79},
    {77, 80, 85, 84},
    {65, 67, 65, 79}
};
// Matriz decodificadora = cod^-1
// Futuramente: calcular matriz inversa
double decod[SIZE][SIZE] = {
    {15103.0/136453.0, 14603.0/409359.0, -42418.0/409359.0, -9074.0/409359.0},
    {5023.0/136453.0, -6482.0/409359.0, -12680.0/409359.0, 6803.0/409359.0},
    {-10267.0/136453.0, -1225.0/136453.0, 17939.0/136453.0, -8882.0/136453.0},
    {-8239.0/136453.0, -3494.0/409359.0, 1375.0/409359.0, 28802.0/409359.0}
};
int step = 0; // Variavel para linha atual

// Informacoes do dado analisado
double* input[SIZE];
double* output[SIZE];
string info;
int columns = 0;

// Matrizes da multiplicação: A * B = C
struct thread_data {
  double** A, B, C;  
};

// Mutex e threads para analise da linha a ser calculada
pthread_mutex_t mutex;
pthread_t threads[SIZE];

void read () {
    // Ler dado
    cout << "Digite dado a ser codificado: ";
    getline(cin, info);

    // Determinar tamanho do dado e da sua matriz
    int length = info.size();
    columns = length / SIZE;
    int remainder = length % SIZE;
    if (remainder != 0) {
        columns++;
        for (int i = 0; i < remainder; i++)
            info[length + i] = char(0);
    }

    // Alocar espaço de memória para matrizes
    for (int i = 0; i < SIZE ; i++) {
        input[i] = new double[columns];
        output[i] = new double[columns];
    }

    // Gerar matriz correspondente ao dado (caracteres em ASCII)
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < columns; j++)
            input[i][j] = int(info[i * columns + j]);

    cout << "Matriz correspondente: " << endl;
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < columns; j++)
            cout << input[i][j] << " ";
        cout << endl;
    }
}

void* mult (void* arg) {
    /*struct thread_data *this_data;
    this_data = (struct thread_data *) arg;*/

    // Determinar linha calculada: região critica
    pthread_mutex_lock(&mutex);
    int core = step++;
    if (step == SIZE) step = 0;
    pthread_mutex_unlock(&mutex);

    // Calcular cada elemento da linha
    for (int i = 0; i < columns; i++)
        for (int j = 0; j < SIZE; j++)
            output[core][i] += cod[core][j] * input[j][i];
            //(this_data->C)[core][i] += (this_data->A)[core][j] * (this_data->B)[j][i];
}

int main () {
    // Exibir matriz cod e decod
    cout << "Matriz cod" << endl;
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++)
            cout << cod[i][j] << " ";
        cout << endl;
    }
    cout << endl
         << "Matriz decod" << endl;
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++)
            cout << decod[i][j] << " ";
        cout << endl;
    }

    // Inicializar o mutex
    if (pthread_mutex_init(&mutex, NULL) != 0) {
        cout << "Inicializacao do mutex falhou!" << endl;
        return 1;
    }

    // Leitura do dado a ser codificado
    read();

    // Criacao das threads para cada linha
    /*struct thread_data td;
    *td.A = *cod;
    td.B = **input;
    td.C = **output;*/

    for (int i = 0; i < SIZE; i++) 
        pthread_create(&threads[i], NULL, mult, NULL);
        //pthread_create(&threads[i], NULL, mult, (void *)&td);

    for (int i = 0; i < SIZE; i++)
        pthread_join(threads[i], NULL);

    cout << endl << "Matriz resultado:" << endl;
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < columns; j++)
            cout << output[i][j] << " ";
        cout << endl;
    }

    // Liberar espaço alocado para as matrizes
    for (int i = 0; i < SIZE; i++) {
        delete[] input[i];
        delete[] output[i];
    }

    // Destruir o mutex
    pthread_mutex_destroy(&mutex);

    return 0;
}