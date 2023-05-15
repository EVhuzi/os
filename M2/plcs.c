#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "thread.h"
#include "thread-sync.h"

#define MAXN 10000
#define FALSE 0
#define TRUE  1
#define END		2

int T, N, M;
char A[MAXN + 1], B[MAXN + 1];
int dp[MAXN][MAXN];
int result;
int State = 0; 
int EndFlag = FALSE;
int nRound = 0;
spinlock_t lock = SPIN_INIT();

#define DP(x, y) (((x) >= 0 && (y) >= 0) ? dp[x][y] : 0)
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MAX3(x, y, z) MAX(MAX(x, y), z)


void Tworker2(int id) {
  int i, j;
  int BaseOff = 0;
  int MaxVal = ((M > N) ? M : N);
  i = j = 0;
  for (int k = 0; k < (M+N-1); k++) {
    int n = j, m = 0;
    if (j > MaxVal - 1)
      BaseOff = j - BaseOff + 1;
    
    n = (j >= N) ? (N - 1) : j;
    m = k - n;

    for (; n >= BaseOff; n--){
      int skip_a = DP(n - 1, m);
      int skip_b = DP(n, m - 1);
      int take_both = DP(n - 1, m - 1) + (A[n] == B[m]);
      dp[i][j] = MAX3(skip_a, skip_b, take_both);
      m++;
    }
  }
}

void Tprint()	{
	for (int i = 0; i < M; i++){
		for (int j = 0; j < N; j++)	{
			printf("%d ", dp[i][j]);
		}
		printf("\n");
	}
}

void Thandle(int id)  {
  int m, n, nLoop, nLoopCnt;
  int Sta, BackRound = nRound;
	int m2;

	while (nRound < (M + N - 1))	{
	  BackRound = nRound;	
		spin_lock(&lock);
		Sta	= State;
		State++;
		spin_unlock(&lock);

		if (nRound < N)  {
			n = nRound - (nRound + 1) / T * Sta;
			m = nRound - n;
		}
		else  {
			m = (nRound - M + 1) + (2 * M - nRound) / T * Sta; 
			n = nRound - m;
		}

		if (m >= M || n >= N)	{
			spin_lock(&lock);
			EndFlag++;
			spin_unlock(&lock);
			return;
		}
		printf("%d, %d, %d\n", m, n, Sta);

		nLoop = 0;
		nLoopCnt = nRound / T;
		m2 = m + ((M - m) / T) * (Sta + 1);

		while (nLoop <= nLoopCnt) {
			int skip_a = DP(n - 1, m);
			int skip_b = DP(n, m - 1);
			int take_both = DP(n - 1, m - 1) + (A[n] == B[m]);
			dp[m][n] = MAX3(skip_a, skip_b, take_both);
			printf("(%d, %d) = %d %d\n", m, n, dp[m][n], Sta);
		
			if (m >= m2 || m >= (M - 1) || n == 0) {
				break;
			}

			m++;
			n--;
			nLoop++;
		}
		
		spin_lock(&lock);
		EndFlag++;
		spin_unlock(&lock);
		while ((BackRound + 1) != nRound);
	}
}

void Tprocess(int id) {
  for (int i = 0; i < T; i++) {
    create(Thandle);  
  }

	for (nRound  = 0; nRound < (M + N - 1); nRound++)
  {
		while (EndFlag != END);
		EndFlag = FALSE;
		
		State = 0;
  }
	join();
	
	result = dp[N-1][M-1];
	Tprint();
}
void Tworker(int id) {
  if (id != 1) {
    // This is a serial implementation
    // Only one worker needs to be activated
    return;
  }

  for (int i = 0; i < N; i++) {
    for (int j = 0; j < M; j++) {
      // Always try to make DP code more readable
      int skip_a = DP(i - 1, j);
      int skip_b = DP(i, j - 1);
      int take_both = DP(i - 1, j - 1) + (A[i] == B[j]);
      dp[i][j] = MAX3(skip_a, skip_b, take_both);
    }
  }

  result = dp[N - 1][M - 1];
}

int main(int argc, char *argv[]) {
  // No need to change
  assert(scanf("%s%s", A, B) == 2);
  N = strlen(A);
  M = strlen(B);
  T = !argv[1] ? 1 : atoi(argv[1]);

  // Add preprocessing code here
#if 0
  for (int i = 0; i < T; i++) {
    create(Tworker);
  }
  join();  // Wait for all workers
#endif
	printf("M = %d N = %d T = %d\n", M, N, T);
  Tprocess(T);
  printf("%d\n", result);
}
