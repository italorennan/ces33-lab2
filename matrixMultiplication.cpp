#include <iostream>
#include <string>
#include <cstdlib>
#include <cmath>
#include <pthread.h>

using namespace std;

// Tamanho da matriz codificadora
//#define SIZE 4

// Informacoes das matrizes de criptografia
int SIZE = 0; // Dimensões da matriz
int step = 0; // Variavel para linha atual
double** cod; // Matriz codificadora
double** decod; // Matriz decodificadora

// Informacoes do dado analisado
double** input;
double** output;
double** output2;
string info;
int columns = 0;

// Matrizes da multiplicação: A * B = C
struct thread_data {
  double** A;
  double** B;
  double** C;  
};

// Mutex para analise da linha a ser calculada
pthread_mutex_t mutex;

void defineCripto () {
    // Alocacao e definicao das matrizes de criptografia
    // Futuramente: feito em funcao separada, matrizes nao fixas
    SIZE = 4;
    cod = new double*[SIZE];
    decod = new double*[SIZE];
    for (int i = 0; i < SIZE; i++) {
        cod[i] = new double[SIZE];
        decod[i] = new double[SIZE];
    }

    // Usando tabelas auxiliares pq n sei fazer direto com os ponteiros
    // E ja suguei tentando descobrir fodase
    double auxCod[SIZE][SIZE] = {
        {69, 78, 71, 69},
        {78, 32, 67, 79},
        {77, 80, 85, 84},
        {65, 67, 65, 79}
    };

    double auxDecod[SIZE][SIZE] = {
        {15103.0/136453.0, 14603.0/409359.0, -42418.0/409359.0, -9074.0/409359.0},
        {5023.0/136453.0, -6482.0/409359.0, -12680.0/409359.0, 6803.0/409359.0},
        {-10267.0/136453.0, -1225.0/136453.0, 17939.0/136453.0, -8882.0/136453.0},
        {-8239.0/136453.0, -3494.0/409359.0, 1375.0/409359.0, 28802.0/409359.0}
    };

    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++) {
            cod[i][j] = auxCod[i][j];
            decod[i][j] = auxDecod[i][j];
        }
}

void read () {
    // Ler dado
    cout << "Digite dado a ser codificado: ";
    getline(cin, info);

    // Determinar tamanho do dado e da sua matriz
    int length = info.size();
    columns = length / SIZE;
    if (length % SIZE != 0) {
        columns++;
        // Preenchimento de espaços vazios usados na matriz
        int remainder = SIZE * columns - length;
        for (int i = 0; i < remainder; i++)
            info[length + i] = char(1);
    }

    // Alocar espaço de memória para matrizes
    input = new double*[SIZE];
    output = new double*[SIZE];
    output2 = new double*[SIZE];
    for (int i = 0; i < SIZE ; i++) {
        input[i] = new double[columns];
        output[i] = new double[columns];
        output2[i] = new double[columns];
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

// Funcao para escrever dado correspondente a uma matriz
void write (double** M) {
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < columns; j++) {
            int c = round(M[i][j]);
            if (c == 1) return;
            cout << char(c);
        }
}

// Funcao de multiplicação entre duas matrizes referenciadas no argumento
void* mult (void* arg) {
    struct thread_data *this_data;
    this_data = (struct thread_data *) arg;

    // Determinar linha calculada: região critica
    pthread_mutex_lock(&mutex);
    int core = step++;
    if (step == SIZE) step = 0;
    pthread_mutex_unlock(&mutex);

    // Calcular cada elemento da linha
    for (int i = 0; i < columns; i++)
        for (int j = 0; j < SIZE; j++)
            (this_data->C)[core][i] += (this_data->A)[core][j] * (this_data->B)[j][i];
}

int main () {
    defineCripto();

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

    // Criacao das threads para cada linha para codificar
    struct thread_data td;
    td.A = cod;
    td.B = input;
    td.C = output;

    pthread_t threads[SIZE];

    for (int i = 0; i < SIZE; i++) 
        pthread_create(&threads[i], NULL, mult, (void *)&td);

    for (int i = 0; i < SIZE; i++)
        pthread_join(threads[i], NULL);

    cout << endl << "Matriz codificada:" << endl;
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < columns; j++)
            cout << output[i][j] << " ";
        cout << endl;
    }

    // Criacao das threads para cada linha para decodificar
    td.A = decod;
    td.B = output;
    td.C = output2;

    for (int i = 0; i < SIZE; i++)
        pthread_create(&threads[i], NULL, mult, (void *)&td);

    for (int i = 0; i < SIZE; i++)
        pthread_join(threads[i], NULL);

    cout << endl << "Matriz decodificada:" << endl;
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < columns; j++)
            cout << output2[i][j] << " ";
        cout << endl;
    }

    cout << "Escrevendo o dado decodificado..." << endl;
    write(output2);

    // Liberar espaço alocado para as matrizes
    for (int i = 0; i < SIZE; i++) {
        delete[] cod[i];
        delete[] decod[i];
        delete[] input[i];
        delete[] output[i];
        delete[] output2[i];
    }

    delete[] cod;
    delete[] decod;
    delete[] input;
    delete[] output;
    delete[] output2;

    // Destruir o mutex
    pthread_mutex_destroy(&mutex);

    return 0;
}