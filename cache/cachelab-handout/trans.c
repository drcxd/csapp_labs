/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>
#include <string.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

void td32(int* src, int* dst, int N) {
  for (int i = 0; i < 8; ++i) {
    int a0 = *(src + i * N + 0);
    int a1 = *(src + i * N + 1);
    int a2 = *(src + i * N + 2);
    int a3 = *(src + i * N + 3);
    int a4 = *(src + i * N + 4);
    int a5 = *(src + i * N + 5);
    int a6 = *(src + i * N + 6);
    int a7 = *(src + i * N + 7);
    *(dst + i * N + 0) = a0;
    *(dst + i * N + 1) = a1;
    *(dst + i * N + 2) = a2;
    *(dst + i * N + 3) = a3;
    *(dst + i * N + 4) = a4;
    *(dst + i * N + 5) = a5;
    *(dst + i * N + 6) = a6;
    *(dst + i * N + 7) = a7;
  }
  for (int i = 0; i < 8; ++i) {
    for (int j = 0; j < i; ++j) {
      int x = *(dst + i * N + j);
      *(dst + i * N + j) = *(dst + j * N + i);
      *(dst + j * N + i) = x;
    }
  }
}

void tnd32(int* src, int *dst, int N, int n) {
  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < n; ++j) {
      *(dst + j * N + i) = *(src + i * N + j);
    }
  }
}

void transsq32(int* A, int* B, int n, int N) {
  for (int i = 0; i < N / n; ++i) {
    for (int j = 0; j < N / n; ++j) {
      if (i == j) {
        td32(A + i * n * N + j * n,
             B + i * n * N + j * n, N);
      } else {
        tnd32(A + i * n * N + j * n,
              B + j * n * N + i * n, N, n);
      }
    }
  }
}

void transsq64(int* A, int* B) {

}

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
  if (M == 32 && N == 32) {
    transsq32((int*)A, (int*)B, 8, 32);
  } else if (M == 64 && N == 64) {
    transsq64((int*)A, (int*)B);
  }
}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

/*
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);

}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}
