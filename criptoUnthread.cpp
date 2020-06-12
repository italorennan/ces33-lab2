#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <sstream>
#include <pthread.h>

using namespace std;

// Informacoes das matrizes de criptografia
int SIZE = 0; // Dimensões da matriz
int step = 0; // Variavel para linha atual
string mode;
string operation;
string key;     // Chave de criptografia
double** cod;   // Matriz codificadora
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
struct ThreadData{
   double** A;
   double** B;
   double** C;
};

// Mutexes
pthread_mutex_t line;                                        // Linha a ser calculada
pthread_mutex_t inputMutex, outputMutex, opMutex1, opMutex2; // Protecao das operacoes

void printMatrix(double** matrix, int lineNumber, int columnNumber){
   for(int i = 0; i < lineNumber; i++){
      for(int j = 0; j < columnNumber; j++)
         cout << matrix[i][j] << " ";
      cout << endl;
   }
}

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

double multiplyLineColumn(double** A, int line, double** B, int col, int size){
   double lineColumnProduct = 0;
   for(int k = 0; k < size; k++){
      lineColumnProduct+= A[line][k]*B[k][col];
   }
   return lineColumnProduct;
}

void matrixMultiplication(double** A, double** B, double** M, int aSize, int bSize, int equalSize){
   for(int i = 0; i < aSize; i++){
      for(int j = 0; j < bSize; j++){
         M[i][j] = multiplyLineColumn(A, i, B, j, equalSize);
      }
   }
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

// Funcao centralizadora no caso de decodificar entradas
void deleteMatrix(double** matrix,int matrixSize){
   for(int i = 0; i < matrixSize; i++){
      delete[] matrix[i];
   }
   delete[] matrix;
}

void unthreadedEncode(){
   cout << key << endl;
   cout << "d" << endl;
   while (true)
   {
      // Detectar proximo dado
      cout << "#" << endl;
      getline(cin, info);
      cout << info << endl;
      if (info == "$")
         return;

      // Determinar largura da matriz input
      int length = info.size();
      inputColumns = length / SIZE;
      if (length % SIZE != 0)
      {
         inputColumns++;
         int remainder = SIZE * inputColumns - length;
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
            input[i][j] = int(info[i * inputColumns + j]);

      double** encodedMatrix = new double*[SIZE];
      for(int i = 0; i < SIZE; i++)
         encodedMatrix[i] = new double[inputColumns];

      matrixMultiplication(cod,input,encodedMatrix, SIZE, inputColumns, SIZE);
      printMatrix(encodedMatrix,SIZE, inputColumns);
   }

}

void unthreadedDecode(){
   cout << key << endl;
   cout << "c" << endl;

   while (true)
   {
      getline(cin, info);
      if (info != "#"){
         cout << "$";
         return;
      }
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
      int j = 0;
      for (int n = 0; n < SIZE; n++){
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
      double** decodedMatrix = new double*[SIZE];
      for(int i = 0; i < SIZE; i++){
         decodedMatrix[i] = new double[inputColumns];
      }
      matrixMultiplication(decod,input,decodedMatrix, SIZE, inputColumns, SIZE);
      for(int i = 0; i < SIZE; i++){
         for(int j = 0; j < inputColumns; j++){
            int ascii = round(decodedMatrix[i][j]);
            if(ascii != 1) cout << char(ascii);
         }
      }
      cout << endl;
   }
}

void executeUnthreadedOperation(string operation){
   if(operation == "c")
      unthreadedEncode();
   else if(operation == "d")
      unthreadedDecode();
   else
      cout << "Operacao nao definida" << endl;
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
   
   // Executando a operacao (Threaded)
   executeUnthreadedOperation(operation);

   // Liberar espaco de memoria das matrizes
   deleteMatrix(cod, SIZE);
   deleteMatrix(decod, SIZE);

   return 0;
};