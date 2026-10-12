#include <stdint.h>
#include <stdio.h>

int main() {
    uint8_t reg[1] = {0x12};  // 初始值为 0x12 (0b00010010)

    // 语句 1
    reg[0] = (reg[0] & 0x3f) << 1;
    printf("Statement 1 result: 0x%02x (%d)\n", reg[0], reg[0]);

    // 重置 reg[0] 以便进行第二次测试
    reg[0] = 0x12;

    // 语句 2
    reg[0] = reg[0] & 0x7e;
    printf("Statement 2 result: 0x%02x (%d)\n", reg[0], reg[0]);

    return 0;
}