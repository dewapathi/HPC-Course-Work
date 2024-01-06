#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

int bufferSize = 256;

struct MatrixSize {
    int x;
    int y;
};

struct Matrix {

    struct MatrixSize size;
    double* values;
    
};

struct Matrix** Matrices;

struct threadArguments {
    int currentThreadIndex;
    int iterateCount;
    struct Matrix* matrixInfo;
    double* allMtxAVals;
    double* allMtxBVals;
};

int emptyCharArray (char * chars, int size) {
    return chars[0] == '\n';
}

int FileMatrixCount(char* fileName) {
    FILE * fPtr;
    fPtr = fopen(fileName, "r");
    if (fPtr == NULL) {
        printf("Unable to load file '%s'\n", fileName);
        return 0;
    }

    int matrixCount = 0;
    char readBuffer[bufferSize];

    for (int i = 0; fgets(readBuffer, bufferSize, fPtr) != NULL; i++) {
        if (emptyCharArray(readBuffer, bufferSize) > 0) {
            matrixCount++;
        }
    }
   
    matrixCount++;

    fclose(fPtr);

    return matrixCount;
}

struct MatrixSize* charArrayIsMatrixSize (char * chars, int size) {
    struct MatrixSize* mtxSize = (struct MatrixSize*) malloc( sizeof(struct MatrixSize) );  
    int splitCount = 0;

    char temp[bufferSize];
    strcpy(temp, chars);

    char* split = strtok(temp, ",");
    while (split != NULL) {
        double interpreted = atof(split);
        if ( interpreted > 0 ) 
        {
            if (splitCount == 0) {
                (*mtxSize).x = interpreted;
            } else if (splitCount == 1) {
                (*mtxSize).y = interpreted;
            } else {
                return NULL;
            }
            splitCount++;
        }

        split = strtok(NULL, ",");
    }
    return mtxSize;
}

double* parseLineOfDoubles(char* lineChars, struct MatrixSize mtxSize) {
    double* lineVals = malloc (sizeof(double) * mtxSize.y);
    int lineYCount = 0;

    const char splitter[2] = ",";
    char* split = strtok(lineChars, splitter);
    while (split) {
        double interpreted = atof(split);
        if ( interpreted > 0 ) {
            lineVals[lineYCount] = interpreted;
            lineYCount += 1;
        }

        split = strtok(NULL, splitter);
    }

    return lineVals;
}

struct Matrix** loadFromFile (char * fileName) {
    if (fileName) {
      
      int matrixCount = FileMatrixCount(fileName);
      
      FILE * fPtr;
      fPtr = fopen(fileName, "r");
      if (fPtr == NULL) {
          printf("cannot load file '%s'\n", fileName);
          return NULL;
      }
      struct Matrix** matrices = malloc(sizeof(struct Matrix) * matrixCount);

      struct MatrixSize* currentMatrixSize = (struct MatrixSize*) malloc(sizeof(struct MatrixSize));
      (*currentMatrixSize).x = (*currentMatrixSize).y = 0;

      double* currentMatrixValues = NULL;
      int currentMatrixCount = 0;
      int currentMatrixCurrentLineCount = 0;

      char readBuffer[bufferSize];
      while ( fgets(readBuffer, bufferSize, fPtr) != NULL ) {
          struct MatrixSize* mtxSize = charArrayIsMatrixSize(readBuffer, bufferSize);
          if ( mtxSize != NULL && (*currentMatrixSize).x == 0 && (*currentMatrixSize).y == 0 ) {
              currentMatrixSize = mtxSize;
          } else if ( emptyCharArray(readBuffer, bufferSize) > 0 ) {
                            
              struct Matrix* currentMatrix = malloc( sizeof(struct Matrix) );
              (*currentMatrix).values = currentMatrixValues;
              (*currentMatrix).size = *currentMatrixSize;

              matrices[currentMatrixCount] = currentMatrix;
              
              currentMatrixCount++;

              currentMatrixCurrentLineCount = 0;
              (*currentMatrixSize).x = (*currentMatrixSize).y = 0;
              currentMatrixValues = NULL;
          } else {
              if (currentMatrixValues == NULL) {
                  currentMatrixValues = malloc( sizeof(double) * (*currentMatrixSize).y);
              }
              
              double* values = parseLineOfDoubles(readBuffer, *currentMatrixSize);
             
              for (int i = 0; i < (*currentMatrixSize).y; i++) {
                  int index = (currentMatrixCurrentLineCount * (*currentMatrixSize).y) + i;
                  currentMatrixValues[index] = values[i];
              }
              currentMatrixCurrentLineCount++;
          }
      }
      
      struct Matrix* currentMatrix = malloc( sizeof(struct Matrix) );
      (*currentMatrix).values = currentMatrixValues;
      (*currentMatrix).size = *currentMatrixSize;

      matrices[currentMatrixCount] = currentMatrix;

      currentMatrixCount++;

      fclose(fPtr);

      return matrices;
    } else {
        printf("Cannot find file '%s'\n", fileName);
        return NULL;
    }
}

int isMultiply(struct Matrix a, struct Matrix b);

int isMultiply(struct Matrix a, struct Matrix b) {
    if (a.size.y == b.size.x) {
        return 1;
    }
    return 0;
}

void* calculate(void* param);

void* calculate (void* param) {
  
    struct threadArguments* args = (struct threadArguments*)param;
    
    struct MatrixSize matrixSizeOutput = (*args).matrixInfo->size;
    int index = (*args).currentThreadIndex;

    int column = index %  matrixSizeOutput.x;
    int row = (index - column) / matrixSizeOutput.y;
    
    double result = 0;
    for (int i = 0; i < (*args).iterateCount; i++) {
        int mtxAThisIndex = (row * (matrixSizeOutput.x - 1)) + i;
        int mtxBThisIndex = (i * matrixSizeOutput.y) + column;
        double finalA = (*args).allMtxAVals[ mtxAThisIndex ];
        double finalB = (*args).allMtxBVals[ mtxBThisIndex ];
        result += finalA * finalB;
    }

    (*args).matrixInfo->values[(*args).currentThreadIndex] = result;

    return NULL;
}

void printMatrix(double* matrix, int countX, int countY);

void printMatrix(double* matrix, int countX, int countY) {
    for (int i = 0; i < countX; i++) {
        for(int j = 0; j < countY; j++) {
            printf("%.2f", *(matrix + (i * countY) + j));

            if ( j < countY - 1 ) {
                printf("    ");
            }
        }
        printf("\n");
    }
}

int saveToFile (char* inputFile, struct Matrix** matrices, int matricesCount) {

    FILE* writeFile = fopen(inputFile, "w");
    if (writeFile == NULL)
    {
        printf("Unable to open file '%s'.\n", inputFile);
        return 0;
    }
    
    fprintf(writeFile, "matrices\n\n");

    for( int i = 0; i < matricesCount; i++) {
        struct Matrix matrix = *(matrices[i]);

        fprintf(writeFile,"Matrix[%d] X Matrix[%d]\n----------------------\n\n", i, i + 1);

        for (int j = 0; j < matrix.size.x; j++) {
            int rowIndex = (j * matrix.size.y);
            for( int k = 0; k < matrix.size.y; k++ ) {
                fprintf(writeFile, "%.2f", matrix.values[rowIndex + k]);
                if (k < matrix.size.y - 1) {
                    fprintf(writeFile, "   ");
                }
            }
            fprintf(writeFile, "\n");
        }
        fprintf(writeFile, "\n");
    }
    fclose(writeFile);
    return 1;
}

int main () {
    char* inputFile = "./matrix.txt";
    char* outputFile = "./matrixresults2223748.txt";
    int matrixCount = FileMatrixCount(inputFile);
    Matrices = loadFromFile(inputFile);
    printf("%d matrices found\n\n", matrixCount);
    struct Matrix** allFinalMatrices = malloc(sizeof(struct Matrix) * matrixCount);
    int allMatricesCount = 0;

    for (int i = 0; i < matrixCount; i += 2) {
        struct Matrix matrixA = *(Matrices[i]);
        struct Matrix matrixB = *(Matrices[i + 1]);
        struct Matrix* matrixResult = malloc(sizeof(struct Matrix));
        (*matrixResult).size.x = matrixA.size.x;
        (*matrixResult).size.y = matrixB.size.y;
        (*matrixResult).values = malloc(sizeof(double) * (*matrixResult).size.x * (*matrixResult).size.y);
        int canMultiply = isMultiply(matrixA, matrixB);
        if (canMultiply == 1) 
        {
            int threadCount = (*matrixResult).size.x * (*matrixResult).size.y;
            
            pthread_t* threadIds = malloc( sizeof(pthread_t) * threadCount);
            
            for (int i = 0; i < threadCount; i++) {
                struct threadArguments *args = malloc(sizeof(struct threadArguments));
                if (args != NULL) 
                {
                    (*args).currentThreadIndex = i;
                    (*args).iterateCount = matrixA.size.y;
                    (*args).matrixInfo = matrixResult;
                    (*args).allMtxAVals = matrixA.values;
                    (*args).allMtxBVals = matrixB.values;

                    pthread_create( &threadIds[i], NULL, calculate, args);
                }
            }
         
            printf("\n------------------\n");
            printf("%dX%d X %dX%d\n",  matrixA.size.x, matrixA.size.y, matrixB.size.x, matrixB.size.y);     

            for (int i = 0; i < threadCount; i++) {
                pthread_join( threadIds[i], NULL );
            }
    
            printf("Matrix[%d] X Matrix[%d]\n", i, i + 1);
            printf("-----------------------\n");
            printMatrix((*matrixResult).values, (*matrixResult).size.x, (*matrixResult).size.y);
            printf("\n%d threads created\n", threadCount);

            allFinalMatrices[allMatricesCount] = matrixResult;
            allMatricesCount++;
        }
        else 
        {
            printf("\n----------------------\n");
            printf("Cannot multiply Matrices[%d] and Matrices[%d] because of %dx%d and %dx%d matrices are not same\n", i, i + 1, matrixA.size.x, matrixA.size.y, matrixB.size.x, matrixB.size.y);
        }
    }

    if (saveToFile(outputFile, allFinalMatrices, allMatricesCount)) {
        printf("saved '%d' matrices into file '%s'\n", allMatricesCount, outputFile);
    } else {
        printf("Can't save file '%s'\n", outputFile);
    }
    return 0;
}
