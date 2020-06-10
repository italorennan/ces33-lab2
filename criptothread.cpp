#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <pthread.h>

using namespace std;

// Informacoes das matrizes de criptografia
int SIZE = 0; // Dimensões da matriz
int step = 0; // Variavel para linha atual
string key; // Chave de criptografia
double** cod; // Matriz codificadora
double** decod; // Matriz decodificadora

// Informacoes do dado analisado
double** input;
double** output;
string info;
int inputColumns = 0;
int outputColumns = 0;
bool finishRead = false;
bool finishOp = false;
bool finishWrite = false;

// Matrizes da multiplicação: A * B = C
struct thread_data {
  double** A;
  double** B;
  double** C;  
};

// Mutexes
pthread_mutex_t line; // Linha a ser calculada
pthread_mutex_t inputMutex, outputMutex, opMutex1, opMutex2; // Protecao das operacoes

// Definicao das matrizes de criptografia
int defineCripto () {
    // Leitura da chave
    getline(cin, key);
    int keySize = key.size();
    int rootKey = sqrt(keySize);
    if (rootKey * rootKey != keySize) return -1; // Erro para chave de tamanho invalido
    
    // Alocacao e definicao das matrizes de criptografia
    SIZE = rootKey;
    cod = new double*[SIZE];
    decod = new double*[SIZE];
    for (int i = 0; i < SIZE; i++) {
        cod[i] = new double[SIZE];
        decod[i] = new double[SIZE];
    }

    // Geracao da matriz codificadora
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            cod[i][j] = int(key[i * SIZE + j]);

    // Futuramente: definir decod = cod^-1
    // Por enquanto, chave fixa

    // Usando tabelas auxiliares pq n sei fazer direto com os ponteiros
    // E ja suguei tentando descobrir fodase
    double auxDecod[SIZE][SIZE] = {
        {15103.0/136453.0, 14603.0/409359.0, -42418.0/409359.0, -9074.0/409359.0},
        {5023.0/136453.0, -6482.0/409359.0, -12680.0/409359.0, 6803.0/409359.0},
        {-10267.0/136453.0, -1225.0/136453.0, 17939.0/136453.0, -8882.0/136453.0},
        {-8239.0/136453.0, -3494.0/409359.0, 1375.0/409359.0, 28802.0/409359.0}
    };

    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            decod[i][j] = auxDecod[i][j];

    return 0;
}

// Funcao de multiplicacao de uma linha entre duas matrizes
void* mult (void* arg) {
    struct thread_data *this_data;
    this_data = (struct thread_data *) arg;

    // Determinar linha calculada: regiao critica
    pthread_mutex_lock(&line);
    int core = step++;
    if (step == SIZE) step = 0;
    pthread_mutex_unlock(&line);

    // Calcular cada elemento da linha
    for (int i = 0; i < inputColumns; i++)
        for (int j = 0; j < SIZE; j++)
            (this_data->C)[core][i] += (this_data->A)[core][j] * (this_data->B)[j][i];
}

// Funcao de leitura das entradas para codificar
void* readCode (void* arg) {
    finishRead = false;

    while (!finishRead) { 
        pthread_mutex_lock(&inputMutex);

        // Detectar proximo dado
        getline(cin, info);
        if (info == "$")
            finishRead = true; // Finalizar leitura

        // Ler dado para input
        else {
            // Determinar largura da matriz input
            int length = info.size();
            inputColumns = length / SIZE;
            if (length % SIZE != 0) {
                inputColumns++;
                int remainder = SIZE * inputColumns - length;
                for (int i = 0; i < remainder; i++)
                    info[length + i] = char(1);
            }

            // Alocar espaco de memoria para matriz input
            input = new double*[SIZE];
            for (int i = 0; i < SIZE; i++)
                input[i] = new double[inputColumns];

            // Gerar matriz input correspondente
            for (int i = 0; i < SIZE; i++)
                for (int j = 0; j < inputColumns; j++)
                    input[i][j] = int(info[i * inputColumns + j]);
        }

        pthread_mutex_unlock(&opMutex1);
    }
}

// Funcao de codificacao das entradas
void* opCode (void* arg) {
    finishOp = false;

    while (!finishOp) {
        pthread_mutex_lock(&opMutex1);
        pthread_mutex_lock(&opMutex2);
        
        if (finishRead)
            finishOp = true; // Finalizar operacoes

        // Codificar dado para output
        else {
            // Alocar espaco de memoria para matriz output
            outputColumns = inputColumns;
            output = new double*[SIZE];
            for (int i = 0; i < SIZE; i++)
                output[i] = new double[outputColumns];

            // Criacao das threads para cada linha do resultado
            struct thread_data td;
            td.A = cod;
            td.B = input;
            td.C = output;
            
            pthread_t opThreads[SIZE];

            for (int i = 0; i < SIZE; i++)
                pthread_create(&opThreads[i], NULL, mult, (void *)&td);

            // Esperar threads terminarem execucao
            for (int i = 0; i < SIZE; i++)
                pthread_join(opThreads[i], NULL);

            // Liberar espaco de memoria da matriz input
            for (int i = 0; i < SIZE; i++)
                delete[] input[i];
            delete[] input;
        }
        
        pthread_mutex_unlock(&inputMutex);
        pthread_mutex_unlock(&outputMutex);
    }
}

// Funcao de escrita das saidas codificadas
void* writeCode (void* arg) {
    finishWrite = false;

    // Escrita inicial no arquivo de saida
    cout << key << endl
         << "d" << endl;

    while (!finishWrite) {
        pthread_mutex_lock(&outputMutex);

        if (finishOp)
            finishWrite = true; // Finalizar escrita

        // Escrever dado codificado
        else {
            // Simbolo indicativo de novo dado codificado
            cout << "#" << endl;

            // Escrever matriz de codificacao do dado
            for (int i = 0; i < SIZE; i++) {
                for (int j = 0; j < outputColumns; j++)
                    cout << output[i][j] << " ";
                cout << endl;
            }

            // Liberar espaco de memoria da matriz output
            for (int i = 0; i < SIZE; i++)
                delete[] output[i];
            delete[] output;
        }

        pthread_mutex_unlock(&opMutex2);
    }
}

// Funcao centralizadora no caso de codificar entradas
void encode () {
    // Setar mutexes inicialmente
    pthread_mutex_lock(&opMutex1);
    pthread_mutex_lock(&outputMutex);

    // Criar threads principais
    pthread_t mainThreads[3];
    pthread_create(&mainThreads[0], NULL, readCode, NULL);
    pthread_create(&mainThreads[1], NULL, opCode, NULL);
    pthread_create(&mainThreads[2], NULL, writeCode, NULL);

    // Aguardar finalizacao das threads
    for (int i = 0; i < 3; i++)
        pthread_join(mainThreads[i], NULL);
}

// Funcao de leitura das entradas para decodificar
void* readDecode (void* arg) {
    finishRead = false;

    while (! finishRead) {
        pthread_mutex_lock(&inputMutex);

        // Detectar proximo dado
        getline(cin, info);
        if (info != "#")
            finishRead = true; // Finalizar leitura

        // Ler dado para input
        else {
            // Determinar largura da matriz input
            getline(cin, info);
        }

        pthread_mutex_unlock(&opMutex1);
    }
}

// Funcao de decodificacao das entradas
void* opDecode (void* arg) {

}

// Funcao de escrita das saidas decodificadas
void* writeDecode (void* arg) {

}

// Funcao centralizadora no caso de decodificar entradas
void decode () {
    // Setar mutexes inicialmente
    pthread_mutex_lock(&opMutex1);
    pthread_mutex_lock(&outputMutex);

    // Criar threads principais
    pthread_t mainThreads[3];
    pthread_create(&mainThreads[0], NULL, readDecode, NULL);
    pthread_create(&mainThreads[1], NULL, opDecode, NULL);
    pthread_create(&mainThreads[2], NULL, writeDecode, NULL);

    // Aguardar finalizacao das threads
    for (int i = 0; i < 3; i++)
        pthread_join(mainThreads[i], NULL);

}

int main () {
    // Base da criptografia
    if(defineCripto() == -1) {
        cout << "Chave de criptografia invalida." << endl;
        return 1;
    } // Fim da execucao caso chave seja invalida.

    // Inicializar os mutexes
    if (pthread_mutex_init(&line, NULL) != 0
        || pthread_mutex_init(&inputMutex, NULL) != 0
        || pthread_mutex_init(&outputMutex, NULL) != 0
        || pthread_mutex_init(&opMutex1, NULL) != 0
        || pthread_mutex_init(&opMutex2, NULL) != 0) {
        cout << "Inicializacao dos mutexes falhou!" << endl;
        return 2;
    }

    // Leitura do tipo de operação
    string operation;
    getline(cin, operation);
    if (operation == "c") encode();
    else if (operation == "d") decode();
    else {
        cout << "Operacao nao especificada." << endl;
        return 3;
    }

    // Liberar espaco de memoria das matrizes
    for (int i = 0; i < SIZE; i++) {
        delete[] cod[i];
        delete[] decod[i];
    }
    delete[] cod;
    delete[] decod;

    // Destruir os mutexes
    pthread_mutex_destroy(&line);
    pthread_mutex_destroy(&inputMutex);
    pthread_mutex_destroy(&outputMutex);
    pthread_mutex_destroy(&opMutex1);
    pthread_mutex_destroy(&opMutex2);

    return 0;
}