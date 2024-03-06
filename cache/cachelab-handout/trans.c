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

void t32d(int* src, int* dst, int N) {
  for (int i = 0; i < 8; ++i) {
    int a0 = *(src + 0 * N + i);
    int a1 = *(src + 1 * N + i);
    int a2 = *(src + 2 * N + i);
    int a3 = *(src + 3 * N + i);
    int a4 = *(src + 4 * N + i);
    int a5 = *(src + 5 * N + i);
    int a6 = *(src + 6 * N + i);
    int a7 = *(src + 7 * N + i);
    *(dst + i * N + 0) = a0;
    *(dst + i * N + 1) = a1;
    *(dst + i * N + 2) = a2;
    *(dst + i * N + 3) = a3;
    *(dst + i * N + 4) = a4;
    *(dst + i * N + 5) = a5;
    *(dst + i * N + 6) = a6;
    *(dst + i * N + 7) = a7;
  }
}

void t32nd(int* src, int *dst, int N) {
  for (int i = 0; i < 8; ++i) {
    for (int j = 0; j < 8; ++j) {
      *(dst + i * N + j) = *(src + j * N + i);
    }
  }
}

void trans32(int* A, int* B, int N) {
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      if (i == j) {
        t32d(A + i * 8 * N + j * 8,
             B + i * 8 * N + j * 8, N);
      } else {
        t32nd(A + i * 8 * N + j * 8,
              B + j * 8 * N + i * 8, N);
      }
    }
  }
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
    trans32((int*)A, (int*)B, N);
  } else if (M == 64 && N == 64) {
    trans32((int*)A, (int*)B, N);
    trans32((int*)A + 32 * 64 + 32, (int*)B + 32 * 64 + 32, N);
    trans32((int*)A + 32, (int*)B + 32 * 64, N);
    trans32((int*)A + 32 * 64, (int*)B + 32, N);
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
