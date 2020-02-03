int block1, block2, block3;

void test_func(int a)
{
    if (a  != 0)
    {
        asm("nop");
        block1++;
    }
    else
    {
        asm("nop");
        block2++;
    }
    block3++;
}
