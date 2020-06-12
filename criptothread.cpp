#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <sstream>
#include <pthread.h>
#include <chrono>

using namespace std;

// Informacoes das matrizes de criptografia
int SIZE = 0;     // Dimensões da matriz
int step = 0;     // Variavel para linha atual
string operation; // Tipo de operacao executada
string key;       // Chave de criptografia
double** cod;     // Matriz codificadora
double** decod;   // Matriz decodificadora

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
struct ThreadData{
   double** A;
   double** B;
   double** C;
   int core;
};

// Mutexes
pthread_mutex_t line;  // Linha a ser calculada
pthread_mutex_t inputMutex, outputMutex, 
   opMutex1, opMutex2; // Protecao das operacoes

void createMatrix(double*** matrix, int matrixSize){
   *matrix =  new double*[matrixSize];
   for(int i = 0; i < matrixSize; i++){
      (*matrix)[i] = new double[matrixSize];
   }
}

void cofactorLineCol(double** matrix, int matrixSize, int line, int col, double** cofactorMatrix){
   int cofactorI, cofactorJ;
   cofactorI = 0;
   for(int i = 0; i < matrixSize; i++){
      cofactorJ = 0;
      if(i == line)
         continue;
      for(int j = 0; j < matrixSize; j++){
         if(j == col){
            continue;
         }
         cofactorMatrix[cofactorI][cofactorJ] = matrix[i][j];
         cofactorJ++;
      }
      cofactorI++;
   }
}

double determinant(double** matrix, int matrixSize){

   double matrixDeterminant = 0;
   double** cofactorJ;
   createMatrix(&cofactorJ,matrixSize-1);

   if(matrixSize == 1){
      return matrix[0][0];
   }
   else if(matrixSize == 2){
      return matrix[0][0]*matrix[1][1] - matrix[1][0]*matrix[0][1];
   }
   else{
      for(int j = 0; j < matrixSize; j++){
         cofactorLineCol(matrix, matrixSize, 0, j, cofactorJ);
         matrixDeterminant = matrixDeterminant + pow(-1,j)*matrix[0][j]*determinant(cofactorJ, matrixSize-1);
      }
   }
   return matrixDeterminant;
}

void calculateCofactorMatrix(double** matrix, int matrixSize, double** cofactorMatrix){
   double** cofactorIJ;
   createMatrix(&cofactorIJ, matrixSize);
   for(int i = 0; i < matrixSize; i++){
      for(int j = 0; j < matrixSize; j++){
         cofactorLineCol(matrix, matrixSize,i, j, cofactorIJ);
         cofactorMatrix[i][j] = pow(-1,i+j)*determinant(cofactorIJ,matrixSize-1);
      }
   }
}

void transposeMatrix(double** matrix, int matrixSize, double** transposedMatrix){
   for(int i = 0; i < matrixSize; i++){
      for(int j = 0; j < matrixSize; j++)
         transposedMatrix[j][i] = matrix[i][j];
   }
}

void calculateAdjointMatrix(double** matrix, int matrixSize, double** adjointMatrix){
   double** cofactorMatrix;
   createMatrix(&cofactorMatrix,matrixSize);
   calculateCofactorMatrix(matrix, matrixSize, cofactorMatrix);
   transposeMatrix(cofactorMatrix, matrixSize, adjointMatrix);
}

unsigned int calculateInverseMatrix(double** matrix, int matrixSize, double** inverseMatrix){
   double matrixDeterminant = determinant(matrix,matrixSize);
   double** adjointMatrix;
   if(matrixDeterminant == 0){
      cout << "Erro: Nao existe matriz inversa" << endl;
      return 1;
   }
   createMatrix(&adjointMatrix, matrixSize);
   calculateAdjointMatrix(matrix, matrixSize, adjointMatrix);
   for(int i = 0; i < matrixSize; i++){
      for(int j = 0; j < matrixSize; j++){
         inverseMatrix[i][j] = adjointMatrix[i][j]/matrixDeterminant;
      }
   }
   return 0;
}

void createCodeMatrix(string key, double** cod){
   for (int i = 0; i < SIZE; i++)
      for (int j = 0; j < SIZE; j++)
         cod[i][j] = double(key[i*SIZE + j]);
}

void createDecodeMatrix(double** cod, double** decod){
   if(calculateInverseMatrix(cod, SIZE, decod))
      cout << "Nao foi possivel criar a matriz de decodificacao." << endl;
}

int setupCodeDecodeMatrix(){

   // Alocacao e definicao das matrizes de criptografia
   SIZE = sqrt(key.size());
   createMatrix(&cod,SIZE);
   createMatrix(&decod,SIZE);
   
   // Geracao da matriz codificadora
   createCodeMatrix(key, cod);

   // Geracao da matriz decodificadora, caso necessario
   if (operation == "d")
      createDecodeMatrix(cod, decod);

   return 0;
}

int checkKeySize(){
   int keySize = key.size();
   int rootKey = (int) sqrt(keySize);
   
   if(rootKey*rootKey == keySize)
      return 1;
   
   return 0;
}

void deleteMatrix(double** matrix,int matrixSize){
   for(int i = 0; i < matrixSize; i++){
      delete[] matrix[i];
   }
   delete[] matrix;
}

// Funcao de multiplicacao de uma linha 
// entre duas matrizes
void* mult(void* arg){
   struct ThreadData *thisData;
   thisData = (struct ThreadData *)arg;

   // Determinar linha calculada: regiao critica
   // pthread_mutex_lock(&line);
   // int core = step++;
   // if (step == SIZE)
   //    step = 0;
   // pthread_mutex_unlock(&line);

   // Calcular cada elemento da linha
   for (int i = 0; i < inputColumns; i++)
      for (int j = 0; j < SIZE; j++) 
         (thisData->C)[thisData->core][i] += 
            (thisData->A)[thisData->core][j] * 
            (thisData->B)[j][i];
}

// Funcao de leitura das entradas para codificar
void *readCode(void *arg){
   finishRead = false;

   while (!finishRead)
   {
      pthread_mutex_lock(&inputMutex);

      // Detectar proximo dado
      getline(cin, info);
      if (info == "$")
         finishRead = true; // Finalizar leitura

      // Ler dado para input
      else
      {
         // Determinar largura da matriz input
         int length = info.size();
         inputColumns = length / SIZE;
         if (length % SIZE != 0)
         {
            inputColumns++;
            int remainder = SIZE * inputColumns 
               - length;
            for (int i = 0; i < remainder; i++)
               info[length + i] = char(1);
         }

         // Alocar espaco de memoria para matriz input
         input = new double* [SIZE];
         for (int i = 0; i < SIZE; i++)
            input[i] = new double[inputColumns];

         // Gerar matriz input correspondente
         for (int i = 0; i < SIZE; i++)
            for (int j = 0; j < inputColumns; j++)
               input[i][j] = 
                  int(info[i * inputColumns + j]);
      }

      pthread_mutex_unlock(&opMutex1);
   }
}

// Funcao de codificacao das entradas
void* opCode(void* arg){
   finishOp = false;

   while (!finishOp)
   {
      pthread_mutex_lock(&opMutex1);
      pthread_mutex_lock(&opMutex2);

      if (finishRead)
         finishOp = true; // Finalizar operacoes

      // Codificar dado para output
      else
      {
         // Alocar espaco de memoria para output
         outputColumns = inputColumns;
         output = new double*[SIZE];
         for (int i = 0; i < SIZE; i++)
            output[i] = new double[outputColumns];

         // Setando os valores iniciais de output em 0
         for (int i = 0; i < SIZE; i++)
            for (int j = 0; j < outputColumns; j++)
               output[i][j] = 0;

         // Criacao das threads para cada linha
         struct ThreadData td;
         td.A = cod;
         td.B = input;
         td.C = output;

         pthread_t opThreads[SIZE];

         for (int i = 0; i < SIZE; i++){
            td.core = i;
            pthread_create(&opThreads[i], NULL, 
               mult, (void *)&td);
         }

         // Esperar threads terminarem execucao
         for (int i = 0; i < SIZE; i++)
            pthread_join(opThreads[i], NULL);

         // Liberar espaco de memoria da matriz input
         deleteMatrix(input, SIZE);
      }

      pthread_mutex_unlock(&inputMutex);
      pthread_mutex_unlock(&outputMutex);
   }
}

// Funcao de escrita das saidas codificadas
void* writeCode(void* arg){
   finishWrite = false;

   // Escrita inicial no arquivo de saidass
   cout << key << endl
        << "d" << endl;

   while (!finishWrite)
   {
      pthread_mutex_lock(&outputMutex);

      if (finishOp)
         finishWrite = true; // Finalizar escrita

      // Escrever dado codificado
      else
      {
         // Simbolo indicativo de novo dado codificado
         cout << "#" << endl;

         // Escrever matriz de codificacao do dado
         for (int i = 0; i < SIZE; i++)
         {
            for (int j = 0; j < outputColumns; j++)
               cout << output[i][j] << " ";
            cout << endl;
         }

         // Liberar espaco de memoria da matriz output
         deleteMatrix(output, SIZE);
      }

      pthread_mutex_unlock(&opMutex2);
   }
}

// Funcao centralizadora no caso de codificar entradas
void encode(){
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
void* readDecode(void* arg){
   finishRead = false;

   while (!finishRead)
   {
      pthread_mutex_lock(&inputMutex);

      // Detectar proximo dado
      getline(cin, info);
      if (info != "#")
         finishRead = true; // Finalizar leitura

      // Ler dado para input
      else {
         // Determinar largura da matriz input
         getline(cin, info);
         inputColumns = 0;
         int i = 0;
         while (i < info.size()) {
            char c = info[i];  
            while (c != ' ') {
                i++;
                c = info[i];
            }
            while (c == ' ') {
                i++;
                c = info[i];
            }
            inputColumns++;
         }
         
         // Alocar espaco de memoria para matriz input
         input = new double* [SIZE];
         for (i = 0; i < SIZE; i++)
            input[i] = new double[inputColumns];

         // Gerar matriz input correspondente
         string infoAux;
         int j;
         for (int n = 0; n < SIZE; n++) {
            if (n != 0) getline(cin, info);
            j = 0;
            for (i = 0; i < inputColumns; i++) {
                infoAux = "";
                int k = 0;
                char c = info[j];
                while (c != ' ') {
                    infoAux += c;
                    k++;
                    j++;
                    c = info[j];
                }
                while (c == ' ') {
                    j++;
                    c = info[j];
                }
                istringstream(infoAux) >> input[n][i];
            }
         }
      }

      pthread_mutex_unlock(&opMutex1);
   }
}

// Funcao de decodificacao das entradas
void* opDecode(void* arg){
    finishOp = false;

    while (! finishOp) {
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

            // Setando os valores inicias da matriz output em 0
            for (int i = 0; i < SIZE; i++)
               for (int j = 0; j < outputColumns; j++)
                  output[i][j] = 0;

            // Criacao das threads para cada linha do resultado
            struct ThreadData td;
            td.A = decod;
            td.B = input;
            td.C = output;

            pthread_t opThreads[SIZE];

            for (int i = 0; i < SIZE; i++){
               td.core = i;
               pthread_create(&opThreads[i], NULL, mult, (void *)&td);
            }
            // Esperar threads terminarem execucao
            for (int i = 0; i < SIZE; i++)
                pthread_join(opThreads[i], NULL);

            // Liberar espaco de memoria da matriz input
            deleteMatrix(input, SIZE);
        }

        pthread_mutex_unlock(&inputMutex);
        pthread_mutex_unlock(&outputMutex);
    }
}

// Funcao de escrita das saidas decodificadas
void* writeDecode(void* arg){
    finishWrite = false;

    // Escrita inicial no arquivo de saidas
    cout << key << endl
         << "c" << endl;

    while (!finishWrite) {
        pthread_mutex_lock(&outputMutex);

        if (finishOp)
            finishWrite = true; // Finalizar escrita

        // Escrever dado decodificado
        else {
            // Escrever mensagem decodificada
            for (int i = 0; i < SIZE; i++)
                for (int j = 0; j < outputColumns; j++) {
                    int ascii = round(output[i][j]);
                    if (ascii != 1) cout << char(ascii);
                }
            
            cout << endl;

            // Liberar espaco de memoria da matriz output
            deleteMatrix(output, SIZE);
        }

        pthread_mutex_unlock(&opMutex2);
    }

    cout << "$";
}

// Funcao centralizadora no caso de decodificar entradas
void decode(){
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

void executeThreadedOperation(string operation){
   if(operation == "c")
      encode();
   else if(operation == "d")
      decode();
   else
      cout << "Operacao nao especificada" << endl;
}

int setupMutex(){
   if(
       pthread_mutex_init(&line, NULL) != 0 ||
       pthread_mutex_init(&inputMutex, NULL) != 0 ||
       pthread_mutex_init(&outputMutex, NULL) != 0 ||
       pthread_mutex_init(&opMutex1, NULL) != 0 ||
       pthread_mutex_init(&opMutex2, NULL) != 0) {
      cout << "Inicializacao dos mutexes falhou!" 
         << endl;
      return 1;
   }

   return 0;
}

void destroyMutex(){
   pthread_mutex_destroy(&line);
   pthread_mutex_destroy(&inputMutex);
   pthread_mutex_destroy(&outputMutex);
   pthread_mutex_destroy(&opMutex1);
   pthread_mutex_destroy(&opMutex2);
}

int main()
{
   getline(cin, key);
   getline(cin, operation);

   // Checando se a key é valida
   if (!checkKeySize()){
      cout << "Chave de criptografia invalida." << endl;
      return 1;
   }
   
   // Criando as matrizes necessárias
   setupCodeDecodeMatrix();
   
   // Inicializacao dos mutexes
   if (setupMutex()) return 2;
   
   auto init = chrono::steady_clock::now();
   // Executando a operacao (Threaded)
   executeThreadedOperation(operation);
   auto end = chrono::steady_clock::now();
   auto elapsed = chrono::duration_cast<chrono::microseconds>(end-init).count();

   // Liberar espaco de memoria das matrizes
   deleteMatrix(cod, SIZE);
   deleteMatrix(decod, SIZE);

   cout << "Tempo gasto paralelamente: " << elapsed << " us";
   // Destruir os mutexes
   destroyMutex();

   return 0;
};